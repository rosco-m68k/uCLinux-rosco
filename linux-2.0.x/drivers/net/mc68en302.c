/* mcen302.c: A Linux network driver for Mototrola 68EN302 MCU
 *
 *	Copyright (C) 1999 Aplio S.A.
 *  Written by Vadim Lebedev
 *
 *  based on the skeleton.c by Donald Becker
 *	This software may be used and distributed according to the terms
 *	of the GNU Public License, incorporated herein by reference.
 *
 */

static const char *version =
	"$Version$\n";

/*
 *  Sources:
 *	List your sources of programming information to document that
 *	the driver is your own creation, and give due credit to others
 *	that contributed to the work. Remember that GNU project code
 *	cannot use proprietary or trade secret information. Interface
 *	definitions are generally considered non-copyrightable to the
 *	extent that the same names and structures must be used to be
 *	compatible.
 *
 *	Finally, keep in mind that the Linux kernel is has an API, not
 *	ABI. Proprietary object-code-only distributions are not permitted
 *	under the GPL.
 */

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <linux/errno.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <asm/m68en302.h>
#include <asm/irq.h>

/*
 * The name of the card. Is used for messages and in the requests for
 * io regions, irqs and dma channels
 */
static const char* cardname = "en302";

/* First, a few definitions that the brave might change. */

/* A zero-terminated list of I/O addresses to be probed. */
static unsigned int netcard_portlist[] =
   { ECNTRL0 };

#define NETCARD_IO_EXTENT sizeof(struct _68EN302_ETHR_BLOCK)



/* use 0 for production, 1 for verification, >2 for debug */
#ifndef NET_DEBUG
#define NET_DEBUG 2
#endif
static unsigned int net_debug = NET_DEBUG;


#define MAX_FRAME_SIZE 0x600
#define RX_QUEUE_SIZE 8
#define TX_QUEUE_SIZE 8
#define TX_QUEUE_MASK 7
#define RX_QUEUE_MASK 7
#define HW_MAX_ADDRS (_68EN302_MAX_CET-1)

#define TX_BD_START 0
#define RX_BD_START 64

/* Information that need to be kept for each board. */
struct net_local {
	struct enet_statistics stats;
	long open_time;			/* Useless example local info. */
	struct sk_buff* rx_skb[RX_QUEUE_SIZE];
	struct sk_buff* tx_skb[TX_QUEUE_SIZE];
	struct _68EN302_ETHR_BLOCK* eblk;
	int		tx_running;
	int		tx_next;
	int		tx_next_done;
	int		tx_count;
	int		rx_next;		/* check this RX descriptor on the next interrupt */
};

/* The station (ethernet) address prefix, used for IDing the board. */
#if 0
#define SA_ADDR0 0x00
#define SA_ADDR1 0x42
#define SA_ADDR2 0x65
#endif

/* Index to functions, as function prototypes. */

extern int netcard_probe(struct device *dev);

static int netcard_probe1(struct device *dev, int ioaddr);
static int net_open(struct device *dev);
static int	net_send_packet(struct sk_buff *skb, struct device *dev);
static void net_interrupt(int irq, void *dev_id, struct pt_regs *regs);
static void net_rx(struct device *dev);
static void net_tx(struct device* dev);
static int net_close(struct device *dev);
static struct enet_statistics *net_get_stats(struct device *dev);
static void set_multicast_list(struct device *dev);



#define tx_done(dev) 1



static void en302_set_filter(struct net_local* lp, struct dev_mc_list* list)
{
	int i = 1;
	struct _68EN302_ETHR_BLOCK* eblk = lp->eblk;

	while(list)
	{
		memcpy(eblk->CET[i], list->dmi_addr, ETH_ALEN);
		list = list->next;
		i++;
	}

	for(;  i < _68EN302_MAX_CET; i++)
		memset(eblk->CET[i], 0, ETH_ALEN);
}

static void en302_stop(volatile struct net_local *lp)
{

	lp->eblk->info.ECNTRL.GTS = 1;

	while(lp->tx_running)
		;

	lp->eblk->info.ECNTRL.ETHER_EN = 0;
	lp->rx_next = lp->tx_next = lp->tx_next_done = 0;

}


static void en302_send_packet(struct _68EN302_ETHR_BLOCK* eblk, void* buf, unsigned short length, int index)
{
	struct _68EN302_ETHR_TXBD* bd;

	bd = &eblk->BD[TX_BD_START+index].tx;
	bd->length = length;
	bd->address = (unsigned long) buf;
	bd->flags.w |=  0x9C00; /* R=1, I=1, L=1 TC=1 */
#if 0
	bd->flags.s.L = 1;
	bd->flags.s.I = 1;
	bd->flags.s.TC = 1;
	bd->flags.s.R = 1;
#endif
}


static void en302_flush_tx_ring(struct net_local* lp)
{
	int i;

	for(i = 0; i < TX_QUEUE_SIZE; i++)
	{
		if (lp->tx_skb[i])
			dev_kfree_skb (lp->tx_skb[i], FREE_WRITE);
		lp->tx_skb[i] = 0;
	}
	
	lp->tx_count = 0;
}

static void en302_flush_rx_ring(struct net_local* lp)
{
	int i;

	for(i = 0; i < RX_QUEUE_SIZE; i++)
	{
		if (lp->rx_skb[i])
			dev_kfree_skb (lp->rx_skb[i], FREE_READ);
		lp->rx_skb[i] = 0;
	}

}

static void en302_load_rx_ring(struct net_local *lp)
{
	struct _68EN302_ETHR_BLOCK* eblk = lp->eblk;
	int i;


	for(i = 0; i < RX_QUEUE_SIZE; i++)
	{
		struct sk_buff *skb;		

		if (!lp->rx_skb[i])
		{
			struct  _68EN302_ETHR_RXBD* bd;
			skb = dev_alloc_skb(MAX_FRAME_SIZE);

			if (skb == NULL)
				break;

			lp->rx_skb[i] = skb;
			bd = &eblk->BD[RX_BD_START+i].rx;			
			bd->length = MAX_FRAME_SIZE;
			bd->address.l = (unsigned long) skb->tail;
			bd->flags.s.I = 1;
			bd->flags.s.E = 1;
		}
	}
}




static void en302_init(struct net_local* lp, int flag)
{
	struct _68EN302_ETHR_BLOCK* eblk = lp->eblk;
	
	eblk->BD[TX_BD_START+TX_QUEUE_SIZE-1].tx.flags.s.W = 1;
	eblk->BD[RX_BD_START+RX_QUEUE_SIZE-1].rx.flags.s.W = 1;

	lp->rx_next = 0;
	lp->tx_next = 0;
	lp->tx_next_done = 0;
	lp->tx_count = 0;
	
	en302_load_rx_ring(lp);
	eblk->info.ECNTRL.GTS = 0;
	eblk->info.ECNTRL.ETHER_EN = 1;


	
}
		

		
		



/*
 * Check for a network adaptor of this type, and return '0' iff one exists.
 * If dev->base_addr == 0, probe all likely locations.
 * If dev->base_addr == 1, always return failure.
 * If dev->base_addr == 2, allocate space for the device and return success
 * (detachable devices only).
 */
#ifdef HAVE_DEVLIST
/*
 * Support for a alternate probe manager,
 * which will eliminate the boilerplate below.
 */
struct netdev_entry netcard_drv =
{cardname, netcard_probe1, NETCARD_IO_EXTENT, netcard_portlist};
#else
int
en302_probe(struct device *dev)
{
	int i;
	int base_addr = dev ? dev->base_addr : 0;

	if (base_addr > 0x1ff)    /* Check a single specified location. */
		return netcard_probe1(dev, base_addr);
	else if (base_addr != 0)  /* Don't probe at all. */
		return -ENXIO;

	for (i = 0; netcard_portlist[i]; i++) {
		int ioaddr = netcard_portlist[i];
		if (check_region(ioaddr, NETCARD_IO_EXTENT))
			continue;
		if (netcard_probe1(dev, ioaddr) == 0)
			return 0;
	}

	return -ENODEV;
}
#endif

/*
 * This is the real probe routine. Linux has a history of friendly device
 * probes on the ISA bus. A good device probes avoids doing writes, and
 * verifies that the correct device exists and functions.
 */
static int netcard_probe1(struct device *dev, int ioaddr)
{
	static unsigned version_printed = 0;
	/* address of the INTERRUPT EXTENSION REGISTER */
	unsigned long ier = 2 + (((unsigned long) (REG16(MOBARREG) & ~0xf000)) << 12);


	/* Allocate a new 'dev' if needed. */
	if (dev == NULL) {
		/*
		 * Don't allocate the private data here, it is done later
		 * This makes it easier to free the memory when this driver
		 * is used as a module.
		 */
		dev = init_etherdev(0, 0);
		if (dev == NULL)
			return -ENOMEM;
	}

	if (net_debug  &&  version_printed++ == 0)
		printk(KERN_DEBUG "%s", version);

	printk(KERN_INFO "%s: %s found at %#3x, ", dev->name, cardname, ioaddr);

	/* Fill in the 'dev' fields. */
	dev->base_addr = ioaddr;
	if (REG16(ier) & 0x0800) /* MIL bit is set */
	{
		dev->irq = IRQ3;
	}
	else
	{
		dev->irq = IRQ5;
	}
	

	/* Initialize the device structure. */
	if (dev->priv == NULL) {
		dev->priv = kmalloc(sizeof(struct net_local), GFP_KERNEL);
		if (dev->priv == NULL)
			return -ENOMEM;
	}

	memset(dev->priv, 0, sizeof(struct net_local));

	/* Grab the region so that no one else tries to probe our ioports. */
	request_region(ioaddr, NETCARD_IO_EXTENT, cardname);

	dev->open		= net_open;
	dev->stop		= net_close;
	dev->hard_start_xmit = net_send_packet;
	dev->get_stats	= net_get_stats;
	dev->set_multicast_list = &set_multicast_list;

	/* Fill in the fields of the device structure with ethernet values. */
	ether_setup(dev);

	return 0;
}

/*
 * Open/initialize the board. This is called (in the current kernel)
 * sometime after booting when the 'ifconfig' program is run.
 *
 * This routine should set everything up anew at each open, even
 * registers that "should" only need to be set once at boot, so that
 * there is non-reboot way to recover if something goes wrong.
 */
static int
net_open(struct device *dev)
{
	struct net_local *lp = (struct net_local *)dev->priv;
	struct _68EN302_ETHR_BLOCK *eblk = (struct _68EN302_ETHR_BLOCK*) dev->base_addr;
	unsigned short vecBase = REG16(GIMR) & 0xE0;   /* interrupt vector base  */
	
	/*
	 * This is used if the interrupt line can turned off (shared).
	 * See 3c503.c for an example of selecting the IRQ at config-time.
	 */
	if (request_irq(dev->irq, &net_interrupt, 0, cardname, NULL)) {
		return -EAGAIN;
	}


	irq2dev_map[dev->irq] = dev;

	
	lp->eblk = eblk;
	
	eblk->info.ECNTRL.RESET = 1;
	eblk->info.EDMA.MBZ = 0;
	eblk->info.EDMA.BLIM = 3;
	eblk->info.EDMA.WMRK = 1;
	eblk->info.EDMA.BDSIZE = 3;


	eblk->info.EMRBRL = MAX_FRAME_SIZE;
	eblk->info.INTR_VEC.VG = 0;
	eblk->info.INTR_VEC.INV = vecBase + dev->irq;
	eblk->info.INTR_VEC.MBZ = 0;

	eblk->info.INTR_MASK = 0x07BC;
	eblk->info.ECNFIG = ETHR_FDEN;

	eblk->info.AR_CNTRL.MBZ = 0;
	eblk->info.AR_CNTRL.MULT = 0;
	eblk->info.AR_CNTRL.PROM = 0;
	eblk->info.AR_CNTRL.PA_REJ = 0;

	

	memset(eblk->BD,  0, sizeof(eblk->BD));
	en302_set_filter(lp, NULL);
	memcpy(eblk->CET[0], dev->dev_addr, ETH_ALEN); 

	lp->open_time = jiffies;

	dev->tbusy = 0;
	dev->interrupt = 0;
	dev->start = 1;
	lp->tx_running = 1;
	en302_init(lp, 1);
	MOD_INC_USE_COUNT;

	return 0;
}

static int
net_send_packet(struct sk_buff *skb, struct device *dev)
{
	struct net_local *lp = (struct net_local *)dev->priv;

	if (dev->tbusy) {
		/*
		 * If we get here, some higher level has decided we are broken.
		 * There should really be a "kick me" function call instead.
		 */
		int tickssofar = jiffies - dev->trans_start;
		if (tickssofar < 5)
			return 1;
		printk(KERN_WARNING "%s: transmit timed out, %s?\n", dev->name,
			   tx_done(dev) ? "IRQ conflict" : "network cable problem");
		/* Try to restart the adaptor. */
		en302_stop(lp);
		en302_flush_rx_ring(lp);
		en302_flush_tx_ring(lp);
		en302_init(lp, 1);
		
		dev->tbusy=0;
		dev->trans_start = 0;
	}
	/*
	 * If some higher layer thinks we've missed an tx-done interrupt
	 * we are passed NULL. Caution: dev_tint() handles the cli()/sti()
	 * itself.
	 */
	if (skb == NULL) {
		dev_tint(dev);
		return 0;
	}
	

	{
		short length = ETH_ZLEN < skb->len ? skb->len : ETH_ZLEN;
		unsigned char *buf = skb->data;
		unsigned long flags;
		
		save_flags(flags); cli();
		if (lp->tx_count < TX_QUEUE_SIZE)
		{
			lp->tx_skb[lp->tx_next] = skb;
			en302_send_packet(lp->eblk, buf, length, lp->tx_next);
			lp->tx_next = (lp->tx_next+1) & TX_QUEUE_MASK;
			if (++lp->tx_count == TX_QUEUE_SIZE)
			{
				set_bit(0, (void*)&dev->tbusy);
				dev->trans_start = jiffies;
			}
			
		}
			
		restore_flags(flags);

	}

	return 0;
}

/*
 * The typical workload of the driver:
 *   Handle the network interface interrupts.
 */
static void
net_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	struct device *dev = (struct device *)(irq2dev_map[irq]);
	struct net_local *lp;
	int  status;
	struct _68EN302_ETHR_BLOCK* eblk;
	
	if (dev == NULL) {
		printk(KERN_WARNING "%s: irq %d for unknown device.\n", cardname, irq);
		return;
	}
	dev->interrupt = 1;

	lp = (struct net_local *)dev->priv;
	eblk = lp->eblk;
	
	status = eblk->info.INTR_EVENT;

	while(status)
	{
		eblk->info.INTR_EVENT |= status;
	
		if (status & ETHR_RFINT) {
			/* Got a packet(s). */
			net_rx(dev);
		}
		if (status & ETHR_TFINT) {
			net_tx(dev);
			dev->tbusy = 0;
			mark_bh(NET_BH);	/* Inform upper layers. */
		}
		
		if (status & ETHR_BSY)
		{
			if (!(status & ETHR_RFINT))
			{
				net_rx(dev);
			}
			else
			{
				en302_load_rx_ring(lp);
			}
		}
			

		
		if (status & ETHR_HBERR)
		{
		}

		if (status & ETHR_BABR)
		{
		}

		if (status & ETHR_BABT)
		{
		}

		if (status & ETHR_GRA)
		{
			eblk->info.ECNTRL.GTS = 0;
			lp->tx_running = 0;
		}

		if (status & ETHR_EBERR)
		{
		
			printk(KERN_WARNING "Ethernet buss error: BD %d\n", eblk->info.EDMA.BDERR);
		}


		status = eblk->info.INTR_EVENT;

		
	}

	dev->interrupt = 0;
	return;
}

static void dummy_call()
{
}

		
	
/* We have a good packet(s), get it/them out of the buffers. */
static void
net_rx(struct device *dev)
{
	struct net_local *lp = (struct net_local *)dev->priv;
	struct _68EN302_ETHR_BLOCK* eblk = lp->eblk;
	int rxbd = 0;
	struct sk_buff *skb;
	int count = 0;

	for(rxbd = lp->rx_next;  count < RX_QUEUE_SIZE; rxbd = (rxbd + 1) & RX_QUEUE_MASK, count++)
	{
		struct  _68EN302_ETHR_RXBD* bd = &eblk->BD[RX_BD_START+rxbd].rx;
		union  _68EN302_ETHR_RXBD_FLAGS  flags;
		int bad = 0;


		skb = lp->rx_skb[rxbd];

		if (!skb)
		{
			skb = dev_alloc_skb(MAX_FRAME_SIZE);

			if (skb != NULL)
			{
				bd->length = MAX_FRAME_SIZE;
				bd->address.l = (unsigned long) skb->tail;
				bd->flags.s.I = 1;
				bd->flags.s.E = 1;
			}

			continue;

		}

		
		flags.w = bd->flags.w;

		if (flags.s.E)
		{
			break;
		}
		
		if (flags.s.SH)
		{
			bad = 1;
			lp->stats.rx_length_errors++;
		}

		if (flags.s.L)
		{
			if (flags.s.LG)
			{
				bad = 1;
				lp->stats.rx_length_errors++;
			}

			if (flags.s.NO)
			{
				bad = 1;
				lp->stats.rx_frame_errors++;
			}

			if (flags.s.CR)
			{
				bad = 1;
				lp->stats.rx_crc_errors++;
			}

			if (flags.s.OV)
			{
				bad = 1;
				lp->stats.rx_fifo_errors++;
			}

		}

		if (flags.s.CL)
		{
			bad = 1;
		}


		
		if (bad)
		{
			lp->stats.rx_errors++;
			bd->length = MAX_FRAME_SIZE;
			bd->address.l = (unsigned long) lp->rx_skb[rxbd]->tail;
			bd->flags.s.I = 1;
			bd->flags.s.E = 1;

			continue;
		}

		if (bd->length < 128) // small packet
		{
			struct sk_buff *nskb = dev_alloc_skb(bd->length);
			if (nskb)
			{
				memcpy(skb_put(nskb, bd->length), bd->address.l, bd->length);
				dummy_call();
				nskb->dev = dev;
				nskb->protocol = eth_type_trans(nskb,dev);
				netif_rx(nskb);
				lp->stats.rx_packets++;
			}
			else
			{
				lp->stats.rx_dropped++;
			}
		}
		else
		{
			skb->dev = dev;
			skb_put(skb, bd->length);
			dummy_call();
			skb->protocol = eth_type_trans(skb,dev);
			netif_rx(skb);
			lp->stats.rx_packets++;
		
			skb = dev_alloc_skb(MAX_FRAME_SIZE);
		}
		
		if (skb != NULL)
		{
			bd->length = MAX_FRAME_SIZE;
			bd->address.l = (unsigned long) skb->tail;
			bd->flags.s.I = 1;
			bd->flags.s.E = 1;
		}
		else
		{
			printk(KERN_WARNING "en302: can't get input packet buffer\n");
		}

		lp->rx_skb[rxbd] = skb;


	}

	lp->rx_next = rxbd;
	

	
				
}


static void 
net_tx(struct device *dev)
{
	struct net_local *lp = (struct net_local *)dev->priv;
	struct _68EN302_ETHR_BLOCK* eblk = lp->eblk;
	int txbd;
	int bad = 0;
	int count = 0;

	for(txbd = lp->tx_next_done;  count < TX_QUEUE_SIZE; txbd = (txbd + 1) & TX_QUEUE_MASK, count++)
	{
		struct  _68EN302_ETHR_TXBD* bd = &eblk->BD[TX_BD_START+txbd].tx;
		register union  _68EN302_ETHR_TXBD_FLAGS  flags;

		flags.w = bd->flags.w;

		if (!lp->tx_skb[txbd])
			break;
			
		if (flags.s.R)
			break;

		dev_kfree_skb (lp->tx_skb[txbd], FREE_WRITE);
		lp->tx_skb[txbd] = 0;
		lp->tx_count--;
		
		if (flags.s.CSL)
		{
			bad = 1;
			lp->stats.tx_carrier_errors++;
		}

		if (flags.s.UN)
		{
			bad = 1;
			lp->stats.tx_fifo_errors++;
		}

		if (flags.s.RL)
		{
			bad = 1;
			lp->stats.tx_aborted_errors++;
		}
		
		if (flags.s.HB)
		{
			bad = 1;
			lp->stats.tx_heartbeat_errors++;
		}
		
		if (flags.s.LC)
		{
			bad = 1;
			lp->stats.tx_window_errors++;
		}

		if (flags.w & 0x1C0)
		{
			bad = 1;
			lp->stats.tx_dropped++;
		}

		if (!bad)
			lp->stats.tx_packets++;
	}

	lp->tx_next_done = txbd;
		
}

/* The inverse routine to net_open(). */
static int
net_close(struct device *dev)
{
	struct net_local *lp = (struct net_local *)dev->priv;


	lp->open_time = 0;

	dev->tbusy = 1;
	dev->start = 0;

	en302_stop(lp);
	en302_flush_rx_ring(lp);
	en302_flush_tx_ring(lp);


	/* Update the statistics here. */

	MOD_DEC_USE_COUNT;

	return 0;

}

/*
 * Get the current statistics.
 * This may be called with the card open or closed.
 */
static struct enet_statistics *
net_get_stats(struct device *dev)
{
	struct net_local *lp = (struct net_local *)dev->priv;
	return &lp->stats;
}



	
/*
 * Set or clear the multicast filter for this adaptor.
 * num_addrs == -1	Promiscuous mode, receive all packets
 * num_addrs == 0	Normal mode, clear multicast list
 * num_addrs > 0	Multicast mode, receive normal and MC packets,
 *			and do best-effort filtering.
 */
static void
set_multicast_list(struct device *dev)
{
	struct net_local* lp = (struct net_local*) dev->priv;
	struct _68EN302_ETHR_BLOCK* eblk = lp->eblk;

	en302_stop(lp);
	en302_flush_tx_ring(lp);
	en302_flush_rx_ring(lp);
	
	memcpy(eblk->CET[0], dev->dev_addr, ETH_ALEN); 
	
	if (dev->flags&IFF_PROMISC)
	{
		/* Enable promiscuous mode */
		eblk->info.AR_CNTRL.PROM = 1;
	}
	else if((dev->flags&IFF_ALLMULTI) || dev->mc_count > HW_MAX_ADDRS)
	{

		/* Disable promiscuous mode, use normal mode. */
		eblk->info.AR_CNTRL.PROM = 0;
		eblk->info.AR_CNTRL.MULT = 2;
		en302_set_filter(lp, NULL);

	}
	else if(dev->mc_count)
	{
		/* Walk the address list, and load the filter */
		en302_set_filter(lp, dev->mc_list);
	}
	else
	{
		eblk->info.AR_CNTRL.MULT = 0;
		en302_set_filter(lp, NULL);		
	}

	en302_init(lp, 1);
}

#ifdef MODULE

static char devicename[9] = { 0, };
static struct device this_device = {
	devicename, /* will be inserted by linux/drivers/net/net_init.c */
	0, 0, 0, 0,
	0, 0,  /* I/O address, IRQ */
	0, 0, 0, NULL, netcard_probe };

static int io = ECTRL0;
static int irq = 0;
static int dma = 0;
static int mem = 0;

int init_module(void)
{
	int result;

	if (io == 0)
		printk(KERN_WARNING "%s: You shouldn't use auto-probing with insmod!\n",
			   cardname);

	/* Copy the parameters from insmod into the device structure. */
	this_device.base_addr = io;
	this_device.irq       = irq;
	this_device.dma       = dma;
	this_device.mem_start = mem;

	if ((result = register_netdev(&this_device)) != 0)
		return result;

	return 0;
}

void
cleanup_module(void)
{
	/* No need to check MOD_IN_USE, as sys_delete_module() checks. */
	unregister_netdev(&this_device);
	/*
	 * If we don't do this, we can't re-insmod it later.
	 * Release irq/dma here, when you have jumpered versions and
	 * allocate them in net_probe1().
	 */
	/*
	   free_irq(this_device.irq, NULL);
	   free_dma(this_device.dma);
	*/
	release_region(this_device.base_addr, NETCARD_IO_EXTENT);

	if (this_device.priv)
		kfree_s(this_device.priv, sizeof(struct net_local));
}

#endif /* MODULE */

/*
 * Local variables:
 *  compile-command:
 *	gcc -D__KERNEL__ -Wall -Wstrict-prototypes -Wwrite-strings
 *	-Wredundant-decls -O2 -m486 -c skeleton.c
 *  version-control: t
 *  kept-new-versions: 5
 *  tab-width: 4
 *  c-indent-level: 4
 * End:
 */
