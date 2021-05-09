/*
 *	fec.c -- Driver for Fast Ethernet Controller of ColdFire 5272.
 *
 *	This code is based on Dan Malek's FECC driver for the MPC860T.
 *	Modified by gerg@snapgear.com to support M5272 ethernet controller.
 *
 *	Copyright (c) 1997 Dan Malek (dmalek@jlc.net)
 *	Copyright (c) 2000-2001, Lineo (www.lineo.com)
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/malloc.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>

#include <asm/io.h>
#include <asm/bitops.h>

#include "fec.h"


/*
 * For older kernel versions we need a few functions that where
 * introduced in newer kernels (since this is a back port).
 */
static __inline__ int test_and_set_bit(int nr, void *addr)
{
	unsigned long flags;
	int val;

	save_flags(flags); cli();
	val = (*((int *) addr) & (0x1 << nr)) != 0;
	*((int *) addr) |= (0x1 << nr);
	restore_flags(flags);
	return(val);
}

#define	__clear_user(a,l)	memset(a, 0, l)
#define	__pa(x)			((unsigned long) (x))
#define	__va(x)			((void *) (x))


/*
 * Defined the fixed address of the FEC hardware.
 */
#ifdef CONFIG_COLDFIRE
#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#define	FEC_ADDRESS	(MCF_MBAR + 0x840)
#endif /* CONFIG_COLDFIRE */

static volatile fec_t	*fec_hwp = (volatile fec_t *) FEC_ADDRESS;
static int		fec_hwfound = 0;

/*
 *	Define a default MAC address for SnapGear hardware.
 */
#if defined(CONFIG_NETtel) || defined(CONFIG_GILBARCONAP) || defined(CONFIG_HW_FEITH)
unsigned char   fec_defethaddr[] = { 0x00, 0xd0, 0xcf, 0x00, 0x00, 0x01 };
#endif /* CONFIG_NETtel || CONFIG_GILBARCONAP*/


/* Forward declarations of some structures to support different PHYs
*/

typedef struct {
	uint mii_data;
	void (*funct)(struct device *dev, uint mii_reg);
} phy_cmd_t;

typedef struct {
	uint id;
	char *name;
	const phy_cmd_t *config;
	const phy_cmd_t *startup;
	const phy_cmd_t *ack_int;
	const phy_cmd_t *shutdown;
} phy_info_t;


/* Register definitions for the PHY.
*/

#define MII_REG_CR          0  /* Control Register                         */
#define MII_REG_SR          1  /* Status Register                          */
#define MII_REG_PHYIR1      2  /* PHY Identification Register 1            */
#define MII_REG_PHYIR2      3  /* PHY Identification Register 2            */
#define MII_REG_ANAR        4  /* A-N Advertisement Register               */ 
#define MII_REG_ANLPAR      5  /* A-N Link Partner Ability Register        */
#define MII_REG_ANER        6  /* A-N Expansion Register                   */
#define MII_REG_ANNPTR      7  /* A-N Next Page Transmit Register          */
#define MII_REG_ANLPRNPR    8  /* A-N Link Partner Received Next Page Reg. */

/* values for phy_status */

#define PHY_CONF_ANE	0x0001  /* 1 auto-negotiation enabled */
#define PHY_CONF_LOOP	0x0002  /* 1 loopback mode enabled */
#define PHY_CONF_SPMASK	0x00f0  /* mask for speed */
#define PHY_CONF_10HDX	0x0010  /* 10 Mbit half duplex supported */
#define PHY_CONF_10FDX	0x0020  /* 10 Mbit full duplex supported */
#define PHY_CONF_100HDX	0x0040  /* 100 Mbit half duplex supported */
#define PHY_CONF_100FDX	0x0080  /* 100 Mbit full duplex supported */ 

#define PHY_STAT_LINK	0x0100  /* 1 up - 0 down */
#define PHY_STAT_FAULT	0x0200  /* 1 remote fault */
#define PHY_STAT_ANC	0x0400  /* 1 auto-negotiation complete	*/
#define PHY_STAT_SPMASK	0xf000  /* mask for speed */
#define PHY_STAT_10HDX	0x1000  /* 10 Mbit half duplex selected	*/
#define PHY_STAT_10FDX	0x2000  /* 10 Mbit full duplex selected	*/ 
#define PHY_STAT_100HDX	0x4000  /* 100 Mbit half duplex selected */
#define PHY_STAT_100FDX	0x8000  /* 100 Mbit full duplex selected */ 


/* The number of Tx and Rx buffers.  These are allocated from the page
 * pool.  The code may assume these are power of two, so it it best
 * to keep them that size.
 * We don't need to allocate pages for the transmitter.  We just use
 * the skbuffer directly.
 */
#if 1
#define FEC_ENET_RX_PAGES	4
#define FEC_ENET_RX_FRSIZE	2048
#define FEC_ENET_RX_FRPPG	(PAGE_SIZE / FEC_ENET_RX_FRSIZE)
#define RX_RING_SIZE		(FEC_ENET_RX_FRPPG * FEC_ENET_RX_PAGES)
#define TX_RING_SIZE		8	/* Must be power of two */
#define TX_RING_MOD_MASK	7	/*   for this to work */
#else
#define FEC_ENET_RX_PAGES	16
#define FEC_ENET_RX_FRSIZE	2048
#define FEC_ENET_RX_FRPPG	(PAGE_SIZE / FEC_ENET_RX_FRSIZE)
#define RX_RING_SIZE		(FEC_ENET_RX_FRPPG * FEC_ENET_RX_PAGES)
#define TX_RING_SIZE		16	/* Must be power of two */
#define TX_RING_MOD_MASK	15	/*   for this to work */
#endif

/* Interrupt events/masks.
*/
#define FEC_ENET_HBERR	((uint)0x80000000)	/* Heartbeat error */
#define FEC_ENET_BABR	((uint)0x40000000)	/* Babbling receiver */
#define FEC_ENET_BABT	((uint)0x20000000)	/* Babbling transmitter */
#define FEC_ENET_GRA	((uint)0x10000000)	/* Graceful stop complete */
#define FEC_ENET_TXF	((uint)0x08000000)	/* Full frame transmitted */
#define FEC_ENET_TXB	((uint)0x04000000)	/* A buffer was transmitted */
#define FEC_ENET_RXF	((uint)0x02000000)	/* Full frame received */
#define FEC_ENET_RXB	((uint)0x01000000)	/* A buffer was received */
#define FEC_ENET_MII	((uint)0x00800000)	/* MII interrupt */
#define FEC_ENET_EBERR	((uint)0x00400000)	/* SDMA bus error */

/* The FEC stores dest/src/type, data, and checksum for receive packets.
 */
#define PKT_MAXBUF_SIZE		1518
#define PKT_MINBUF_SIZE		64
#define PKT_MAXBLR_SIZE		1520

/* The FEC buffer descriptors track the ring buffers.  The rx_bd_base and
 * tx_bd_base always point to the base of the buffer descriptors.  The
 * cur_rx and cur_tx point to the currently available buffer.
 * The dirty_tx tracks the current buffer that is being sent by the
 * controller.  The cur_tx and dirty_tx are equal under both completely
 * empty and completely full conditions.  The empty/ready indicator in
 * the buffer descriptor determines the actual condition.
 */
struct fec_enet_private {
	/* The saved address of a sent-in-place packet/buffer, for skfree(). */
	struct	sk_buff* tx_skbuff[TX_RING_SIZE];
	ushort	skb_cur;
	ushort	skb_dirty;

	/* CPM dual port RAM relative addresses.
	*/
	cbd_t	*rx_bd_base;		/* Address of Rx and Tx buffers. */
	cbd_t	*tx_bd_base;
	cbd_t	*cur_rx, *cur_tx;	/* The next free ring entry */
	cbd_t	*dirty_tx;		/* The ring entries to be free()ed. */
	struct	enet_statistics stats;
	char	tx_full;
	unsigned long lock;

	uint	phy_id;		/* PHY info */
	uint	phy_addr;
	uint	phy_status;
	phy_info_t *phy_infp;
};

static int fec_enet_open(struct device *dev);
static int fec_enet_start_xmit(struct sk_buff *skb, struct device *dev);
static int fec_enet_rx(struct device *dev);
static void fec_enet_mii(struct device *dev);
static	void fec_enet_interrupt(int irq, void * dev_id, struct pt_regs * regs);
static int fec_enet_close(struct device *dev);
static struct enet_statistics *fec_enet_get_stats(struct device *dev);
static void set_multicast_list(struct device *dev);

#ifdef CONFIG_M5272
static	ushort	my_enet_addr[] = { 0x00d0, 0xcf00, 0x0072 };
#else
static	ushort	my_enet_addr[] = { 0x0800, 0x3e26, 0x1559 };
#endif

/* MII processing.  We keep this as simple as possible.  Requests are
 * placed on the list (if there is room).  When the request is finished
 * by the MII, an optional function may be called.
 */
typedef struct mii_list {
	uint	mii_regval;
	void	(*mii_func)(struct device *dev, uint val);
	struct	mii_list *mii_next;
} mii_list_t;

#define		NMII	10
mii_list_t	mii_cmds[NMII];
mii_list_t	*mii_free;
mii_list_t	*mii_head;
mii_list_t	*mii_tail;

static int	mii_queue(struct device *dev, int request, void (*func)(struct device *dev, uint));

/* Make MII read/write commands for the FEC.
*/
#define mk_mii_read(REG)	(0x60020000 | ((REG & 0x1f) << 18))
#define mk_mii_write(REG, VAL)	(0x50020000 | ((REG & 0x1f) << 18) | \
						(VAL & 0xffff))

static int
fec_enet_open(struct device *dev)
{

	/* I should reset the ring buffers here, but I don't yet know
	 * a simple way to do that.
	 */

	dev->tbusy = 0;
	dev->interrupt = 0;
	dev->start = 1;

	return 0;					/* Always succeed */
}

static int
fec_enet_start_xmit(struct sk_buff *skb, struct device *dev)
{
	struct fec_enet_private *fep = (struct fec_enet_private *)dev->priv;
	volatile cbd_t	*bdp;
	unsigned long flags;

	/* Transmitter timeout, serious problems. */
	if (dev->tbusy) {
		int tickssofar = jiffies - dev->trans_start;
		if (tickssofar < 20)
			return 1;
		printk("%s: transmit timed out.\n", dev->name);
		fep->stats.tx_errors++;
#ifndef final_version
		{
			int	i;
			cbd_t	*bdp;
			printk(" Ring data dump: cur_tx %x%s cur_rx %x.\n",
				   (int) fep->cur_tx, fep->tx_full ? " (full)" : "",
				   (int) fep->cur_rx);
			bdp = fep->tx_bd_base;
			for (i = 0 ; i < TX_RING_SIZE; i++)
				printk("%04x %04x %08x\n",
					(int) bdp->cbd_sc,
					(int) bdp->cbd_datlen,
					(int) bdp->cbd_bufaddr);
			bdp = fep->rx_bd_base;
			for (i = 0 ; i < RX_RING_SIZE; i++)
				printk("%04x %04x %08x\n",
					bdp->cbd_sc,
					bdp->cbd_datlen,
					(int) bdp->cbd_bufaddr);
		}
#endif

		dev->tbusy=0;
		dev->trans_start = jiffies;

		return 0;
	}

	/* Block a timer-based transmit from overlapping.  This could better be
	   done with atomic_swap(1, dev->tbusy), but set_bit() works as well. */
	if (test_and_set_bit(0, (void*)&dev->tbusy) != 0) {
		printk("%s: Transmitter access conflict.\n", dev->name);
		return 1;
	}

	if (test_and_set_bit(0, (void*)&fep->lock) != 0) {
		printk("%s: tx queue lock!.\n", dev->name);
		/* don't clear dev->tbusy flag. */
		return 1;
	}

	/* Fill in a Tx ring entry */
	bdp = fep->cur_tx;

#ifndef final_version
	if (bdp->cbd_sc & BD_ENET_TX_READY) {
		/* Ooops.  All transmit buffers are full.  Bail out.
		 * This should not happen, since dev->tbusy should be set.
		 */
		printk("%s: tx queue full!.\n", dev->name);
		fep->lock = 0;
		return 1;
	}
#endif

	/* Clear all of the status flags.
	 */
	bdp->cbd_sc &= ~BD_ENET_TX_STATS;

	/* Set buffer length and buffer pointer.
	*/
	bdp->cbd_bufaddr = __pa(skb->data);
	bdp->cbd_datlen = skb->len;

	/* Save skb pointer.
	*/
	fep->tx_skbuff[fep->skb_cur] = skb;

	fep->skb_cur = (fep->skb_cur+1) & TX_RING_MOD_MASK;
	
	/* Push the data cache so the CPM does not get stale memory
	 * data.
	 */
	/*flush_dcache_range(skb->data, skb->data + skb->len);*/

	/* Send it on its way.  Tell CPM its ready, interrupt when done,
	 * its the last BD of the frame, and to put the CRC on the end.
	 */
	save_flags(flags);
	cli();

	bdp->cbd_sc |= (BD_ENET_TX_READY | BD_ENET_TX_INTR | BD_ENET_TX_LAST | BD_ENET_TX_TC);

	dev->trans_start = jiffies;
	fec_hwp->fec_x_des_active = 0x01000000;

	/* If this was the last BD in the ring, start at the beginning again.
	*/
	if (bdp->cbd_sc & BD_ENET_TX_WRAP)
		bdp = fep->tx_bd_base;
	else
		bdp++;

	fep->lock = 0;
	if (bdp == fep->dirty_tx)
		fep->tx_full = 1;
	else
		dev->tbusy=0;
	restore_flags(flags);

	fep->cur_tx = (cbd_t *)bdp;

	return 0;
}

/* The interrupt handler.
 * This is called from the MPC core interrupt.
 */
static	void
fec_enet_interrupt(int irq, void * dev_id, struct pt_regs * regs)
{
	struct	device *dev = dev_id;
	struct	fec_enet_private *fep;
	volatile cbd_t	*bdp;
	volatile fec_t	*ep;
	uint	int_events;
	int c=0;

	fep = (struct fec_enet_private *)dev->priv;
	ep = fec_hwp;
	if (dev->interrupt)
		printk("%s: Re-entering the interrupt handler.\n", dev->name);
	dev->interrupt = 1;

	/* Get the interrupt events that caused us to be here.
	*/
	while ((int_events = ep->fec_ievent) != 0) {
	ep->fec_ievent = int_events;
	if ((int_events &
		(FEC_ENET_HBERR | FEC_ENET_BABR |
			FEC_ENET_BABT | FEC_ENET_EBERR)) != 0)
				printk("FEC ERROR %x\n", int_events);

	/* Handle receive event in its own function.
	*/
	if (int_events & (FEC_ENET_RXF | FEC_ENET_RXB))
		fec_enet_rx(dev_id);

	/* Transmit OK, or non-fatal error.  Update the buffer descriptors.
	 * FEC handles all errors, we just discover them as part of the
	 * transmit process.
	 */
	if (int_events & (FEC_ENET_TXF | FEC_ENET_TXB)) {
	    bdp = fep->dirty_tx;
	    while ((bdp->cbd_sc&BD_ENET_TX_READY)==0) {
#if 1
		if (bdp==fep->cur_tx)
		    break;
#endif
		if (++c>1) {/*we go here when an it has been lost*/};


		if (bdp->cbd_sc & BD_ENET_TX_HB)	/* No heartbeat */
		    fep->stats.tx_heartbeat_errors++;
		if (bdp->cbd_sc & BD_ENET_TX_LC)	/* Late collision */
		    fep->stats.tx_window_errors++;
		if (bdp->cbd_sc & BD_ENET_TX_RL)	/* Retrans limit */
		    fep->stats.tx_aborted_errors++;
		if (bdp->cbd_sc & BD_ENET_TX_UN)	/* Underrun */
		    fep->stats.tx_fifo_errors++;
		if (bdp->cbd_sc & BD_ENET_TX_CSL)	/* Carrier lost */
		    fep->stats.tx_carrier_errors++;

		if (bdp->cbd_sc & (BD_ENET_TX_HB | BD_ENET_TX_LC | 
		    BD_ENET_TX_RL | BD_ENET_TX_UN | BD_ENET_TX_CSL))
			fep->stats.tx_errors++;
	    
		fep->stats.tx_packets++;
		
#ifndef final_version
		if (bdp->cbd_sc & BD_ENET_TX_READY)
		    printk("HEY! Enet xmit interrupt and TX_READY.\n");
#endif
		/* Deferred means some collisions occurred during transmit,
		 * but we eventually sent the packet OK.
		 */
		if (bdp->cbd_sc & BD_ENET_TX_DEF)
		    fep->stats.collisions++;
	    
		/* Free the sk buffer associated with this last transmit.
		 */
		dev_kfree_skb(fep->tx_skbuff[fep->skb_dirty], FREE_WRITE);
		fep->skb_dirty = (fep->skb_dirty + 1) & TX_RING_MOD_MASK;

		/* Update pointer to next buffer descriptor to be transmitted.
		 */
		if (bdp->cbd_sc & BD_ENET_TX_WRAP)
		    bdp = fep->tx_bd_base;
		else
		    bdp++;
	    
		/* Since we have freed up a buffer, the ring is no longer
		 * full.
		 */
		if (fep->tx_full && dev->tbusy) {
		    fep->tx_full = 0;
		    dev->tbusy = 0;
		    mark_bh(NET_BH);
		}

		fep->dirty_tx = (cbd_t *)bdp;
#if 0
		if (bdp==fep->cur_tx)
		    break;
#endif
	    }/*while (bdp->cbd_sc&BD_ENET_TX_READY)==0*/
	 } /* if tx events */

	if (int_events & FEC_ENET_MII)
		fec_enet_mii(dev_id);
	
	} /* while any events */

	dev->interrupt = 0;

	return;
}

/* During a receive, the cur_rx points to the current incoming buffer.
 * When we update through the ring, if the next incoming buffer has
 * not been given to the system, we just set the empty indicator,
 * effectively tossing the packet.
 */
static int
fec_enet_rx(struct device *dev)
{
	struct	fec_enet_private *fep;
	volatile cbd_t *bdp;
	struct	sk_buff	*skb;
	ushort	pkt_len;
	volatile fec_t	*ep;

	fep = (struct fec_enet_private *)dev->priv;
	ep = fec_hwp;

	/* First, grab all of the stats for the incoming packet.
	 * These get messed up if we get called due to a busy condition.
	 */
	bdp = fep->cur_rx;

for (;;) {
	if (bdp->cbd_sc & BD_ENET_RX_EMPTY)
		break;
		
#ifndef final_version
	/* Since we have allocated space to hold a complete frame,
	 * the last indicator should be set.
	 */
	if ((bdp->cbd_sc & BD_ENET_RX_LAST) == 0)
		printk("FEC ENET: rcv is not +last\n");
#endif

	/* Frame too long or too short.
	*/
	if (bdp->cbd_sc & (BD_ENET_RX_LG | BD_ENET_RX_SH))
		fep->stats.rx_length_errors++;
	if (bdp->cbd_sc & BD_ENET_RX_NO)	/* Frame alignment */
		fep->stats.rx_frame_errors++;
	if (bdp->cbd_sc & BD_ENET_RX_CR)	/* CRC Error */
		fep->stats.rx_crc_errors++;
	if (bdp->cbd_sc & BD_ENET_RX_OV)	/* FIFO overrun */
		fep->stats.rx_crc_errors++;

	/* Report late collisions as a frame error.
	 * On this error, the BD is closed, but we don't know what we
	 * have in the buffer.  So, just drop this frame on the floor.
	 */
	if (bdp->cbd_sc & BD_ENET_RX_CL) {
		fep->stats.rx_frame_errors++;
	}
	else {

		/* Process the incoming frame.
		*/
		fep->stats.rx_packets++;
		pkt_len = bdp->cbd_datlen - 4; // remove 32bit ethernet checksum

		/* This does 16 byte alignment, exactly what we need.
		*/
		skb = dev_alloc_skb(pkt_len);

		if (skb == NULL) {
			printk("%s: Memory squeeze, dropping packet.\n", dev->name);
			fep->stats.rx_dropped++;
		}
		else {
			skb->dev = dev;
			skb_put(skb,pkt_len);	/* Make room */
			eth_copy_and_sum(skb,
				(unsigned char *)__va(bdp->cbd_bufaddr),
				pkt_len, 0);
			skb->protocol=eth_type_trans(skb,dev);
			netif_rx(skb);
		}
	}

	/* Clear the status flags for this buffer.
	*/
	bdp->cbd_sc &= ~BD_ENET_RX_STATS;

	/* Mark the buffer empty.
	*/
	bdp->cbd_sc |= BD_ENET_RX_EMPTY;

	/* Update BD pointer to next entry.
	*/
	if (bdp->cbd_sc & BD_ENET_RX_WRAP)
		bdp = fep->rx_bd_base;
	else
		bdp++;
	
#if 1
	/* Doing this here will keep the FEC running while we process
	 * incoming frames.  On a heavily loaded network, we should be
	 * able to keep up at the expense of system resources.
	 */
	ep->fec_r_des_active = 0x01000000;
#endif
   }
	fep->cur_rx = (cbd_t *)bdp;

#if 0
	/* Doing this here will allow us to process all frames in the
	 * ring before the FEC is allowed to put more there.  On a heavily
	 * loaded network, some frames may be lost.  Unfortunately, this
	 * increases the interrupt overhead since we can potentially work
	 * our way back to the interrupt return only to come right back
	 * here.
	 */
	ep->fec_r_des_active = 0x01000000;
#endif

	return 0;
}

static void mii_do_cmd(struct device *dev, const phy_cmd_t *c)
{
	if (c) {
		int k;
		for(k = 0; (c+k)->mii_data != 0; k++)
			mii_queue(dev, (c+k)->mii_data, (c+k)->funct);
	}
}

static void mii_parse_sr(struct device *dev, uint mii_reg)
{
	struct fec_enet_private *fep = dev->priv;
	volatile uint *s = &(fep->phy_status);

	*s &= ~(PHY_STAT_LINK | PHY_STAT_FAULT | PHY_STAT_ANC);

	if (mii_reg & 0x0004)
		*s |= PHY_STAT_LINK;
	if (mii_reg & 0x0010)
		*s |= PHY_STAT_FAULT;
	if (mii_reg & 0x0020)
		*s |= PHY_STAT_ANC;
}

static void mii_parse_cr(struct device *dev, uint mii_reg)
{
	struct fec_enet_private *fep = dev->priv;
	volatile uint *s = &(fep->phy_status);

	*s &= ~(PHY_CONF_ANE | PHY_CONF_LOOP);

	if (mii_reg & 0x1000)
		*s |= PHY_CONF_ANE;
	if (mii_reg & 0x4000)
		*s |= PHY_CONF_LOOP;
}

static void mii_parse_anar(struct device *dev, uint mii_reg)
{
	struct fec_enet_private *fep = dev->priv;
	volatile uint *s = &(fep->phy_status);

	*s &= ~(PHY_CONF_SPMASK);

	if (mii_reg & 0x0020)
		*s |= PHY_CONF_10HDX;
	if (mii_reg & 0x0040)
		*s |= PHY_CONF_10FDX;
	if (mii_reg & 0x0080)
		*s |= PHY_CONF_100HDX;
	if (mii_reg & 0x00100)
		*s |= PHY_CONF_100FDX;
}

static void mii_display_status(struct device *dev)
{
	struct fec_enet_private *fep = dev->priv;
	uint s = fep->phy_status;

	printk("fec: link %s", (s & PHY_STAT_LINK) ? "up" : "down");
	switch (s & PHY_STAT_SPMASK) {
	case PHY_STAT_100FDX: printk(", 100 Mbps Full Duplex"); break;
	case PHY_STAT_100HDX: printk(", 100 Mbps Half Duplex"); break;
	case PHY_STAT_10FDX: printk(", 10 Mbps Full Duplex"); break;
	case PHY_STAT_10HDX: printk(", 10 Mbps Half Duplex"); break;
	default: printk("Unknonw speed/duplex"); break;
	}
	if (s & PHY_STAT_ANC)
		printk(", auto complete");
	if (s & PHY_STAT_FAULT)
		printk(", remote fault");
	printk("\n");
}

/* ------------------------------------------------------------------------- */
/*
 *	The Level one (now INTEL) LXT971.
 */

/* Register definitions for the 971 */

#define MII_LXT971_PCR       16  /* Port Control Register     */
#define MII_LXT971_SR2       17  /* Status Register 2         */
#define MII_LXT971_IER       18  /* Interrupt Enable Register */
#define MII_LXT971_ISR       19  /* Interrupt Status Register */
#define MII_LXT971_LCR       20  /* LED Control Register      */
#define MII_LXT971_TCR       30  /* Transmit Control Register */

static void mii_parse_lxt971_sr2(struct device *dev, uint mii_reg)
{
	struct fec_enet_private *fep = dev->priv;
	volatile uint *s = &(fep->phy_status);

	*s &= ~(PHY_STAT_SPMASK | PHY_STAT_LINK | PHY_STAT_ANC);

	if (mii_reg & 0x0400)
		*s |= PHY_STAT_LINK;
	if (mii_reg & 0x0080)
		*s |= PHY_STAT_ANC;
	if (mii_reg & 0x4000) {
		if (mii_reg & 0x0200)
			*s |= PHY_STAT_100FDX;
		else
			*s |= PHY_STAT_100HDX;
	} else {
		if (mii_reg & 0x0200)
			*s |= PHY_STAT_10FDX;
		else
			*s |= PHY_STAT_10HDX;
	}
	if (mii_reg & 0x0008)
		*s |= PHY_STAT_FAULT;

	mii_display_status(dev);
}

static phy_info_t phy_info_lxt971 = {
	0x0001378e,
	"LXT971",
	
	(const phy_cmd_t []) {  /* config */  
		{ mk_mii_read(MII_REG_CR), mii_parse_cr },
		{ mk_mii_read(MII_REG_ANAR), mii_parse_anar },
		{ mk_mii_read(MII_REG_SR), mii_parse_sr }, 
		{ mk_mii_read(MII_LXT971_SR2), mii_parse_lxt971_sr2 },
		{ 0, NULL, }
	},
	(const phy_cmd_t []) {  /* startup - enable interrupts */
		{ mk_mii_write(MII_LXT971_IER, 0x00f2), NULL },
		{ mk_mii_write(MII_REG_CR, 0x1200), NULL }, /* autonegotiate */
		{ mk_mii_write(MII_LXT971_LCR, 0xd422), NULL }, /* LED config */
		{ mk_mii_read(MII_REG_SR), mii_parse_sr }, 
		{ 0, NULL, }
	},
	(const phy_cmd_t []) { /* ack_int */
		{ mk_mii_read(MII_REG_SR), mii_parse_sr },
		{ mk_mii_read(MII_LXT971_SR2), mii_parse_lxt971_sr2 },
		{ mk_mii_read(MII_LXT971_ISR), NULL },
		{ 0, NULL, }
	},
	(const phy_cmd_t []) {  /* shutdown - disable interrupts */
		{ mk_mii_write(MII_LXT971_IER, 0x0000), NULL },
		{ 0, NULL, }
	},
};

/* ------------------------------------------------------------------------- */
/*
 *	AMD AM79C874 phy.
 */

/* Register definitions for the 874 */

#define MII_AM79C874_MFR       16  /* Miscellaneous Feature Register */
#define MII_AM79C874_ICSR      17  /* Interrupt/Status Register      */
#define MII_AM79C874_DR        18  /* Diagnostic Register            */
#define MII_AM79C874_PMLR      19  /* Power and Loopback Register    */
#define MII_AM79C874_MCR       21  /* ModeControl Register           */
#define MII_AM79C874_DC        23  /* Disconnect Counter             */
#define MII_AM79C874_REC       24  /* Recieve Error Counter          */

static void mii_parse_am79c874_dr(struct device *dev, uint mii_reg)
{
	struct fec_enet_private *fep = dev->priv;
	volatile uint *s = &(fep->phy_status);

	*s &= ~(PHY_STAT_SPMASK | PHY_STAT_ANC);

	if (mii_reg & 0x0080)
		*s |= PHY_STAT_ANC;
	if (mii_reg & 0x0400)
		*s |= ((mii_reg & 0x0800) ? PHY_STAT_100FDX : PHY_STAT_100HDX);
	else
		*s |= ((mii_reg & 0x0800) ? PHY_STAT_10FDX : PHY_STAT_10HDX);

	mii_display_status(dev);
}

static phy_info_t phy_info_am79c874 = {
	0x00022561,
	"AM79C874",
	
	(const phy_cmd_t []) {  /* config */  
		{ mk_mii_read(MII_REG_CR), mii_parse_cr },
		{ mk_mii_read(MII_REG_ANAR), mii_parse_anar },
		{ mk_mii_read(MII_REG_SR), mii_parse_sr }, 
		{ mk_mii_read(MII_AM79C874_DR), mii_parse_am79c874_dr },
		{ 0, NULL, }
	},
	(const phy_cmd_t []) {  /* startup - enable interrupts */
		{ mk_mii_write(MII_AM79C874_ICSR, 0xff00), NULL },
		{ mk_mii_write(MII_REG_CR, 0x1200), NULL }, /* autonegotiate */
		{ mk_mii_read(MII_REG_SR), mii_parse_sr }, 
		{ 0, NULL, }
	},
	(const phy_cmd_t []) { /* ack_int */
		{ mk_mii_read(MII_REG_SR), mii_parse_sr },
		{ mk_mii_read(MII_AM79C874_DR), mii_parse_am79c874_dr },
		{ mk_mii_read(MII_AM79C874_ICSR), NULL },
		{ 0, NULL, }
	},
	(const phy_cmd_t []) {  /* shutdown - disable interrupts */
		{ mk_mii_write(MII_AM79C874_ICSR, 0x0000), NULL },
		{ 0, NULL, }
	},
};

/* ------------------------------------------------------------------------- */

static phy_info_t *phy_info[] = {
	&phy_info_lxt971,
	&phy_info_am79c874,
	NULL
};

/* ------------------------------------------------------------------------- */

static void
fec_enet_mii(struct device *dev)
{
	struct fec_enet_private *fep;
	volatile fec_t *ep;
	mii_list_t *mip;
	uint mii_reg;

	fep = (struct fec_enet_private *)dev->priv;
	ep = fec_hwp;
	mii_reg = ep->fec_mii_data;
	
	if ((mip = mii_head) == NULL) {
		printk("MII and no head!\n");
		return;
	}

	if (mip->mii_func != NULL)
		(*(mip->mii_func))(dev, mii_reg);

	mii_head = mip->mii_next;
	mip->mii_next = mii_free;
	mii_free = mip;

	if ((mip = mii_head) != NULL)
		ep->fec_mii_data = mip->mii_regval;
}

static int
mii_queue(struct device *dev, int regval, void (*func)(struct device *, uint))
{
	struct fec_enet_private *fep;
	unsigned long flags;
	mii_list_t *mip;
	int retval;

	retval = 0;

	/* Add PHY address to register command */
	fep = (struct fec_enet_private *)dev->priv;
	regval |= fep->phy_addr << 23;

	save_flags(flags);
	cli();

	if ((mip = mii_free) != NULL) {
		mii_free = mip->mii_next;
		mip->mii_regval = regval;
		mip->mii_func = func;
		mip->mii_next = NULL;
		if (mii_head) {
			mii_tail->mii_next = mip;
			mii_tail = mip;
		}
		else {
			mii_head = mii_tail = mip;
			fec_hwp->fec_mii_data = regval;
		}
	}
	else {
		retval = 1;
	}

	restore_flags(flags);

	return(retval);
}

static void
mii_found_phy(struct device *dev, uint mii_reg)
{
	struct fec_enet_private *fep;
	int i;

	fep = dev->priv;
	fep->phy_id |= (mii_reg & 0xffff);
	printk("fec: PHY @ 0x%x, ID 0x%08x", fep->phy_addr, fep->phy_id);

	for(i = 0; phy_info[i]; i++) {
                if(phy_info[i]->id == (fep->phy_id >> 4))
                        break;
        }

        fep->phy_infp = phy_info[i];
        if (phy_info[i]) {
                printk(" -- %s\n", phy_info[i]->name);
		mii_do_cmd(dev, phy_info[i]->config);
        } else {
                printk(" -- unknown PHY!\n");
	}
}

static void
mii_discover_phy(struct device *dev, uint mii_reg)
{
	struct fec_enet_private *fep;
	uint phytype;

	fep = (struct fec_enet_private *)dev->priv;

	if (fep->phy_addr < 32) {
		phytype = mii_reg & 0xffff;
		if ((phytype != 0) && (phytype !=0xffff)) {
			fep->phy_id = phytype << 16;
			mii_queue(dev, mk_mii_read(MII_REG_PHYIR2), mii_found_phy);
		} else {
			fep->phy_addr++;
			mii_queue(dev, mk_mii_read(MII_REG_PHYIR1), mii_discover_phy);
		}
	} else {
		printk("fec: no PHY device found.\n");
	}
}

static int
fec_enet_close(struct device *dev)
{
	/* Don't know what to do yet.
	*/

	return 0;
}

static struct enet_statistics *fec_enet_get_stats(struct device *dev)
{
	struct fec_enet_private *fep = (struct fec_enet_private *)dev->priv;

	return &fep->stats;
}

/* Set or clear the multicast filter for this adaptor.
 * Skeleton taken from sunlance driver.
 * The CPM Ethernet implementation allows Multicast as well as individual
 * MAC address filtering.  Some of the drivers check to make sure it is
 * a group multicast address, and discard those that are not.  I guess I
 * will do the same for now, but just remove the test if you want
 * individual filtering as well (do the upper net layers want or support
 * this kind of feature?).
 */

#define HASH_BITS	6		/* #bits in hash */
#define CRC32_POLY	0xEDB88320

static void set_multicast_list(struct device *dev)
{
	struct fec_enet_private *fep;
	volatile fec_t *ep;
	struct dev_mc_list *dmi;
	unsigned int i, j, bit, data, crc;
	unsigned char hash;

	fep = (struct fec_enet_private *)dev->priv;
	ep = fec_hwp;

	if (dev->flags&IFF_PROMISC) {
	  
		/* Log any net taps. */
		printk("%s: Promiscuous mode enabled.\n", dev->name);
		ep->fec_r_cntrl |= 0x0008;
	} else {

		ep->fec_r_cntrl &= ~0x0008;

		if (dev->flags & IFF_ALLMULTI) {
			/* Catch all multicast addresses, so set the
			 * filter to all 1's.
			 */
			ep->fec_hash_table_high = 0xffffffff;
			ep->fec_hash_table_low = 0xffffffff;
		} else {
			/* Clear filter and add the addresses in hash register.
			*/
			ep->fec_hash_table_high = 0;
			ep->fec_hash_table_low = 0;
            
			dmi = dev->mc_list;

			for (j = 0; j < dev->mc_count; j++, dmi = dmi->next)
			{
				/* Only support group multicast for now.
				*/
				if (!(dmi->dmi_addr[0] & 1))
					continue;
			
				/* calculate crc32 value of mac address
				*/
				crc = 0xffffffff;

				for (i = 0; i < dmi->dmi_addrlen; i++)
				{
					data = dmi->dmi_addr[i];
					for (bit = 0; bit < 8; bit++, data >>= 1)
					{
						crc = (crc >> 1) ^
						(((crc ^ data) & 1) ? CRC32_POLY : 0);
					}
				}

				/* only upper 6 bits (HASH_BITS) are used
				   which point to specific bit in he hash registers
				*/
				hash = (crc >> (32 - HASH_BITS)) & 0x3f;
			
				if (hash > 31)
					ep->fec_hash_table_high |= 1 << (hash - 32);
				else
					ep->fec_hash_table_low |= 1 << hash;
			}
		}
	}
}

/* Initialize the FEC Ethernet on ColdFire 5272.
 */
int fec_init(void)
{
	struct device *dev;
	struct fec_enet_private *fep;
	int i, j;
	unsigned char	*eap;
	unsigned long	mem_addr;
#ifndef CONFIG_UCLINUX
	pte_t		*pte;
#endif
	volatile cbd_t	*bdp;
	cbd_t		*cbd_base;
	volatile fec_t	*fecp;

#ifdef CONFIG_M5272
	/* Check if we have already found the internal ethernet */
	if (fec_hwfound)
		return(ENXIO);
#endif

	/* Allocate some private information.
	*/
	fep = (struct fec_enet_private *)kmalloc(sizeof(*fep), GFP_KERNEL);
	__clear_user(fep,sizeof(*fep));

	/* Create an Ethernet device instance.
	*/
	dev = init_etherdev(0, 0);

	fecp = fec_hwp;

	/* Whack a reset.  We should wait for this.
	*/
	fecp->fec_ecntrl = 1;
	udelay(10);

	/* Enable interrupts we wish to service.
	*/
	fecp->fec_imask = (FEC_ENET_TXF | FEC_ENET_TXB |
				FEC_ENET_RXF | FEC_ENET_RXB | FEC_ENET_MII);

	/* Clear any outstanding interrupt.
	*/
	fecp->fec_ievent = 0xffc0;

#ifndef CONFIG_M5272
	fecp->fec_ivec = (FEC_INTERRUPT/2) << 29;
#endif

#if defined(CONFIG_NETtel) || defined(CONFIG_GILBARCONAP) || defined(CONFIG_HW_FEITH)
    {
	unsigned char *ep;
#if defined(CONFIG_GILBARCONAP) || defined(CONFIG_SCALES)
	ep = (unsigned char *) 0xf0006000;
#elif defined(CONFIG_CANCam)
	ep = (unsigned char *) 0xf0020000;
#else
	ep = (unsigned char *) 0xf0006006;
#endif
	/*
	 * MAC address should be in FLASH, check that it is valid.
	 * If good use it, otherwise use the default.
	 */
	if (((ep[0] == 0xff) && (ep[1] == 0xff) && (ep[2] == 0xff) &&
	    (ep[3] == 0xff) && (ep[4] == 0xff) && (ep[5] == 0xff)) ||
	    ((ep[0] == 0) && (ep[1] == 0) && (ep[2] == 0) &&
	    (ep[3] == 0) && (ep[4] == 0) && (ep[5] == 0))) {
		ep = (unsigned char *) &fec_defethaddr[0];
	}
	fecp->fec_addr_low = (ep[0] << 24) | (ep[1]  << 16) |
		(ep[2]  << 8) | ep[3];
	fecp->fec_addr_high = (ep[4] << 24) | (ep[5] << 16);
    }
#endif	/* CONFIG_NETtel || CONFIG_GILBARCONAP */

	/* Read MAC address from FEC controller */
	my_enet_addr[0] = fecp->fec_addr_low >> 16;
	my_enet_addr[1] = fecp->fec_addr_low;
	my_enet_addr[2] = fecp->fec_addr_high >> 16;

	eap = (unsigned char *)&my_enet_addr[0];
	for (i=0; i<6; i++)
		dev->dev_addr[i] = *eap++;

	/* Reset all multicast.
	*/
	fecp->fec_hash_table_high = 0;
	fecp->fec_hash_table_low = 0;

	/* Set maximum receive buffer size.
	*/
	fecp->fec_r_buff_size = PKT_MAXBLR_SIZE;
#ifndef CONFIG_M5272
	fecp->fec_r_hash = PKT_MAXBUF_SIZE;
#endif

	/* Allocate memory for buffer descriptors.
	*/
	if (((RX_RING_SIZE + TX_RING_SIZE) * sizeof(cbd_t)) > PAGE_SIZE) {
		printk("FECC init error.  Need more space.\n");
		printk("FECC initialization failed.\n");
		return 1;
	}
	mem_addr = __get_free_page(GFP_KERNEL);
	cbd_base = (cbd_t *)mem_addr;

#ifndef CONFIG_UCLINUX
	/* Make it uncached.
	*/
	pte = va_to_pte(&init_task, (int)mem_addr);
	pte_val(*pte) |= _PAGE_NO_CACHE;
	flush_tlb_page(current->mm->mmap, mem_addr);
#endif /* CONFIG_UCLINUX */

	/* Set receive and transmit descriptor base.
	*/
	fecp->fec_r_des_start = __pa(mem_addr);
	fep->rx_bd_base = cbd_base;
	fecp->fec_x_des_start = __pa((unsigned long)(cbd_base + RX_RING_SIZE));
	fep->tx_bd_base = cbd_base + RX_RING_SIZE;

	fep->dirty_tx = fep->cur_tx = fep->tx_bd_base;
	fep->cur_rx = fep->rx_bd_base;

	fep->skb_cur = fep->skb_dirty = 0;

	/* Initialize the receive buffer descriptors.
	*/
	bdp = fep->rx_bd_base;
	for (i=0; i<FEC_ENET_RX_PAGES; i++) {

		/* Allocate a page.
		*/
		mem_addr = __get_free_page(GFP_KERNEL);

#ifndef CONFIG_UCLINUX
		/* Make it uncached.
		*/
		pte = va_to_pte(&init_task, mem_addr);
		pte_val(*pte) |= _PAGE_NO_CACHE;
		flush_tlb_page(current->mm->mmap, mem_addr);
#endif

		/* Initialize the BD for every fragment in the page.
		*/
		for (j=0; j<FEC_ENET_RX_FRPPG; j++) {
			bdp->cbd_sc = BD_ENET_RX_EMPTY;
			bdp->cbd_bufaddr = __pa(mem_addr);
			mem_addr += FEC_ENET_RX_FRSIZE;
			bdp++;
		}
	}

	/* Set the last buffer to wrap.
	*/
	bdp--;
	bdp->cbd_sc |= BD_SC_WRAP;

	/* ...and the same for transmmit.
	*/
	bdp = fep->tx_bd_base;
	for (i=0; i<TX_RING_SIZE; i++) {

		/* Initialize the BD for every fragment in the page.
		*/
		bdp->cbd_sc = 0;
		bdp->cbd_bufaddr = 0;
		bdp++;
	}

	/* Set the last buffer to wrap.
	*/
	bdp--;
	bdp->cbd_sc |= BD_SC_WRAP;

	/* Enable MII mode, half-duplex until we know better..
	*/
	fecp->fec_r_cntrl = 0x04;
	fecp->fec_x_cntrl = 0x00;

#ifdef CONFIG_M5272
	/* Set MII speed (66 MHz core).
	*/
	fecp->fec_mii_speed = 0x0e;

	/* Setup interrupt handlers. */
	if (request_irq(86, fec_enet_interrupt, 0, "fec(RX)", dev) != 0)
		printk("FEC: Could not allocate FEC(RC) IRQ(86)!\n");
	if (request_irq(87, fec_enet_interrupt, 0, "fec(TX)", dev) != 0)
		printk("FEC: Could not allocate FEC(RC) IRQ(87)!\n");
	if (request_irq(88, fec_enet_interrupt, 0, "fec(OTHER)", dev) != 0)
		printk("FEC: Could not allocate FEC(OTHER) IRQ(88)!\n");

	/* Unmask interrupt at ColdFire 5272 SIM */
	{
		volatile unsigned long	*icrp;
		icrp = (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR3);
		*icrp = 0x00000ddd;
	}
#else
	/* Enable big endian and don't care about SDMA FC.
	*/
	fecp->fec_fun_code = 0x78000000;

	/* Set MII speed (50 MHz core).
	*/
	fecp->fec_mii_speed = 0x14;

	/* Configure all of port D for MII.
	*/
	immap->im_ioport.iop_pdpar = 0x1fff;
	immap->im_ioport.iop_pddir = 0x1c58;

	/* Install our interrupt handlers.  The 860T FADS board uses
	 * IRQ2 for the MII interrupt.
	 */
	if (request_irq(FEC_INTERRUPT, fec_enet_interrupt, 0, "fec", dev) != 0)
		panic("Could not allocate FEC IRQ!");
	if (request_irq(SIU_IRQ2, mii_link_interrupt, 0, "mii", dev) != 0)
		panic("Could not allocate MII IRQ!");
#endif

	dev->base_addr = (unsigned long)fecp;
	dev->priv = fep;

	ether_setup(dev);

	/* The FEC Ethernet specific entries in the device structure. */
	dev->open = fec_enet_open;
	dev->hard_start_xmit = fec_enet_start_xmit;
	dev->stop = fec_enet_close;
	dev->get_stats = fec_enet_get_stats;
	dev->set_multicast_list = set_multicast_list;

	/* And last, enable the transmit and receive processing.
	*/
	fecp->fec_ecntrl = 2;
	fecp->fec_r_des_active = 0x01000000;

	printk("FEC ENET Version 0.1, ");
	for (i=0; i<5; i++)
		printk("%02x:", dev->dev_addr[i]);
	printk("%02x\n", dev->dev_addr[5]);

	for (i=0; i<NMII-1; i++)
		mii_cmds[i].mii_next = &mii_cmds[i+1];
	mii_free = mii_cmds;

	fep->phy_id = 0;
	fep->phy_addr = 0;
	mii_queue(dev, mk_mii_read(MII_REG_PHYIR1), mii_discover_phy);

	fec_hwfound++;
	return 0;
}

