/*****************************************************************************/

/*
 *	daci2s.c -- driver for i2s style DMA audio
 *
 *	Copyright (C) 1999 Rob Scott (rscott@mtrob.fdns.net)
 *
 *	Based on dac0800.c which is:
 *	(C) Copyright 1999, greg Ungerer (gerg@moreton.com.au)
 *	
 *      Which, in turn, is based loosely on lcddma.c which is:
 *
 *	Copyright (C) 1999 Rob Scott (rscott@mtrob.fdns.net)
 *
 *  General scheme:
 *    The I2S DAC is driven by a Scenix microcontroller. The microcontroller
 *  implements a 128 byte sample FIFO, and a control register.
 *
 *  The control register is at offset zero, and contains the following bits:
 *   <7>: 1 = update controls, reset counters
 *   <6>: 1 = enable DAC
 *   <5>: 1 = clear DMA request pin, starts DMA op
 *   <4>: 1 = turn LED on, 0 = turn LED off
 *   <2:0>: Select sample rate:
 *         0 - select 8 Khz 
 *         1 - select 16 KHz
 *         2 - select 22.05 KHz
 *         3 - select 24 KHz
 *         4 - select 32 KHz
 *         5 - select 44.1 KHz
 *         6 - select 48 KHz
 *
 *  The FIFO is at offset 4, and is write only.
 *
 *  When the FIFO has empty space, and the DAC is enabled, DMA request <0>
 *  is asserted.
 */



/*****************************************************************************/

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <asm/param.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#include <asm/mcfdma.h>
#include <asm/mcftimer.h>

/*****************************************************************************/

#define	DACI2S_MAJOR	120
#define	DACI2S_VEC	120

#define	DACI2S_DMA_CHAN	0
#define	DACI2S_CTL_ADDR     	0x30800000
#define	DACI2S_DMA_DESTADDR	0x30800004

#define	DACI2S_DMA_BUFSIZE	(16 * 1024)
//#define	DACI2S_DMA_BUFSIZE	(4 * 1024)

#define cmdUpd	(1 << 7)	// 1 = update controls, reset counters
#define cmdEna	(1 << 6)	// 1 = enable DAC
#define cmdGo	(1 << 5)	// 1 = starts DMA requests
#define cmdLED	(1 << 4)	// 1 = turn LED on, 0 = turn LED off

/*****************************************************************************/

/*
 *	Double buffers for storing audio samples.
 */
unsigned char	daci2s_buf0[DACI2S_DMA_BUFSIZE];
unsigned char	daci2s_buf1[DACI2S_DMA_BUFSIZE];
unsigned int	daci2s_bufsize = DACI2S_DMA_BUFSIZE;
unsigned int	daci2s_buf0cnt;
unsigned int	daci2s_buf1cnt;

unsigned int	daci2s_curbuf = 0;
unsigned int 	daci2s_bufbusy = 0;

unsigned char   daci2s_rate = 0; 

#define	DACI2S_BUF0		0x1
#define	DACI2S_BUF1		0x2

#define	DACI2S_ALLBUFS		0x3

struct wait_queue	*daci2s_waitchan = NULL;

/*****************************************************************************/

/*
 *	Set up sample rate
 */

#define	RATE_8   0
#define	RATE_11  1    
#define	RATE_16  2    
#define	RATE_22  3
#define	RATE_24  4
#define	RATE_32  5
#define	RATE_44  6
#define	RATE_48  7


int daci2s_setclk(unsigned int rate)
{
  int ret_val;
  int i, j;

  ret_val = 0;

#if 1
  printk("%s(%d): daci2s_setclk()\n", __FILE__, __LINE__);
#endif

  switch (rate) {
  case 8000: 
    daci2s_rate = RATE_8;
    break;
  case 11025: 
    daci2s_rate = RATE_11;
    break;
  case 16000:
    daci2s_rate = RATE_16;
    break;
  case 22050:
    daci2s_rate = RATE_22;
    break;
  case 24000:
    daci2s_rate = RATE_24;
    break;
  case 32000:
    daci2s_rate = RATE_32;
    break;
  case 44100:
    daci2s_rate = RATE_44;
    break;
  case 48000:
    daci2s_rate = RATE_48;
    break;
  default:
    printk("DACI2S - unsupported rate of: %d\n", rate);
    ret_val = -1;
    break;
  }

  /* Make sure no DMAs are currently executing: */
  i = 1;
  //j = 16384;
  j = 10;
  while (i && (j > 0)) {
    i = (daci2s_bufbusy != 0) || (get_dma_residue(DACI2S_DMA_CHAN) != 0);
    j--;
    printk("setclk: busy flag: %x, j: %x\n", i, j);
  }  

  if (j == 0) {
    printk("%s(%d), %s: DMA failed to complete\n",
	   __FILE__,  __LINE__, __FUNCTION__);

    /* Clear DMA interrupt */
    disable_dma(DACI2S_DMA_CHAN);
    /* Pretend that DMA completed */
    set_dma_count(DACI2S_DMA_CHAN, 0);
  }	

  /* Tell DAC new speed, and enable it (resets internal FIFO) */
  *((volatile unsigned char *)DACI2S_CTL_ADDR) = cmdUpd | cmdEna | daci2s_rate;

  return(ret_val);
}

/*****************************************************************************/

void daci2s_startdma(unsigned char *buf, unsigned int size)
{
	/* Do DMA write to i/o operation */
	set_dma_mode(DACI2S_DMA_CHAN, DMA_MODE_WRITE);
	set_dma_device_addr(DACI2S_DMA_CHAN, DACI2S_DMA_DESTADDR);
	set_dma_addr(DACI2S_DMA_CHAN, (unsigned int) buf);
	set_dma_count(DACI2S_DMA_CHAN, size);

	/* Fire it off! */
	enable_dma(DACI2S_DMA_CHAN);

	/* Tell DAC it's ok to do DMA now */
	*((volatile unsigned char *)DACI2S_CTL_ADDR) = cmdGo;
          
}

/*****************************************************************************/

/*
 *	If there is a buffer ready to go then start DMA from it.
 */

void daci2s_startbuf(void)
{
	unsigned char	*bufp;
	unsigned int	flag, cnt;

	/* If already DMAing nothing to do */
	if (daci2s_curbuf)
		return;

	/* Simple double buffered arrangement... */
	if (daci2s_bufbusy & DACI2S_BUF0) {
		bufp = daci2s_buf0;
		cnt = daci2s_buf0cnt;
		flag = DACI2S_BUF0;
	} else if (daci2s_bufbusy & DACI2S_BUF1) {
		bufp = daci2s_buf1;
		cnt = daci2s_buf1cnt;
		flag = DACI2S_BUF1;
	} else {
	  //	  printk("d\n");
	  return;
	}

	daci2s_curbuf = flag;
	daci2s_startdma(bufp, cnt);
}

/*****************************************************************************/

int daci2s_open(struct inode *inode, struct file *filp)
{
#if 0
	printk("%s(%d): daci2s_open()\n", __FILE__, __LINE__);
#endif

	/* Clear DMA interrupt */
        disable_dma(DACI2S_DMA_CHAN);
	
	/* Tell DAC it's ok to do DMA now */
	*((volatile unsigned char *)DACI2S_CTL_ADDR) = 
	  cmdUpd | cmdEna | daci2s_rate;
          
	daci2s_curbuf = 0;
	return(0);
}

/*****************************************************************************/

void daci2s_release(struct inode *inode, struct file *filp)
{
  int i, j;
#if 1
	printk("%s(%d): daci2s_close()\n", __FILE__, __LINE__);
#endif

  /* Make sure no DMAs are currently executing: */
  i = 1;
  //j = 16384;
  j = 10;
  while (i && (j > 0)) {
    i = (daci2s_bufbusy != 0) || (get_dma_residue(DACI2S_DMA_CHAN) != 0);
    j--;
    printk("Close: busy flag: %x, j: %x\n", i, j);
  }  

  if (j == 0) {
    printk("%s, %s(%d): DMA failed to complete\n",
	   __FILE__, __FUNCTION__, __LINE__ );

    /* Clear DMA interrupt */
    disable_dma(DACI2S_DMA_CHAN);
    /* Pretend that DMA completed */
    set_dma_count(DACI2S_DMA_CHAN, 0);
  }	
}

/*****************************************************************************/

int daci2s_write(struct inode *inode, struct file *filp, const char *buf, int count)
{
	unsigned char	*dacbufp;
	unsigned int	size, bufflag;
	unsigned long	flags;

#if 0
	printk("%s(%d): daci2s_write(buf=%x, count=%d)\n", __FILE__, __LINE__,
		(int) buf, count);
#endif

	size = (count > daci2s_bufsize) ? daci2s_bufsize : count;

	/* Check for empty DMA buffer, if not wait for one. */
	save_flags(flags); cli();
	while ((daci2s_bufbusy & DACI2S_ALLBUFS) == DACI2S_ALLBUFS) {
		if (current->signal & ~current->blocked) {
			restore_flags(flags);
			return(-ERESTARTSYS);
		}
		interruptible_sleep_on(&daci2s_waitchan);
	}

	if (daci2s_bufbusy & DACI2S_BUF0) {
		bufflag = DACI2S_BUF1;
		dacbufp = &daci2s_buf1[0];
		daci2s_buf1cnt = size;
	} else {
		bufflag = DACI2S_BUF0;
		dacbufp = &daci2s_buf0[0];
		daci2s_buf0cnt = size;
	}
	restore_flags(flags);

	/* Copy the buffer local and start DMAing it. */
	memcpy_fromfs(dacbufp, buf, size);

	save_flags(flags); cli();
	daci2s_bufbusy |= bufflag;
	daci2s_startbuf();
	restore_flags(flags);

	return(size);
}

/*****************************************************************************/

int daci2s_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
  int	rc = 0;

#if 0
	printk("%s(%d): daci2s_ioctl()\n", __FILE__, __LINE__);
#endif

  switch (cmd) {
  case 1:
    if (daci2s_setclk(arg)) rc = -EINVAL;
    break;
  default:
    rc = -EINVAL;
    break;
  }

  return(rc);
}

/*****************************************************************************/

void daci2s_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	/* Clear DMA interrupt */
	disable_dma(DACI2S_DMA_CHAN);

	/* Mark buffer no longer in use */
	daci2s_bufbusy &= ~daci2s_curbuf;
	daci2s_curbuf = 0;

	/* Start of any other waiting buffers! */
	daci2s_startbuf();

	/* Wake up writers */
	wake_up_interruptible(&daci2s_waitchan);
}

/*****************************************************************************/

/*
 *	Exported file operations structure for driver...
 */

struct file_operations	daci2s_fops = {
	NULL,          /* lseek */
	NULL,          /* read */
	daci2s_write,
	NULL,          /* readdir */
	NULL,          /* poll */
	daci2s_ioctl,
	NULL,          /* mmap */
	daci2s_open,
	daci2s_release,
	NULL,          /* fsync */
	NULL,          /* fasync */
	NULL,          /* check_media_change */
	NULL           /* revalidate */
};

/*****************************************************************************/

void daci2s_init(void)
{
	volatile unsigned char	*mbarp, *dmap;
	unsigned long		imr;
	unsigned int		icr;
	int			result;

	/* Make sure that the parallel port pin is set for DREQ */
	if (DACI2S_DMA_CHAN == 0)
	  /* Make parallel port pin DREQ[0] */
	  *(volatile unsigned short *)(MCF_MBAR + MCFSIM_PAR) |=
	                                                     MCFSIM_PAR_DREQ0;
	if (DACI2S_DMA_CHAN == 1)
	  /* Make parallel port pin DREQ[1] */
	  *(volatile unsigned short *)(MCF_MBAR + MCFSIM_PAR) |=
                                                             MCFSIM_PAR_DREQ1;

	/* Register DACI2S as character device */
	result = register_chrdev(DACI2S_MAJOR, "daci2s", &daci2s_fops);
	if (result < 0) {
		printk(KERN_WARNING "DACI2S: can't get major %d\n",
			DACI2S_MAJOR);
		return;
	}

	printk ("DACI2S: Copyright (C) 1999, Rob Scott"
		"(rscott@mtrob.fdns.net)\n");

	/* Install ISR (interrupt service routine) */
	result = request_irq(DACI2S_VEC, daci2s_isr, SA_INTERRUPT,
		"daci2s", NULL);
	if (result) {
		printk ("DACI2S: IRQ %d already in use\n", DACI2S_VEC);
		return;
	}

	/* Set interrupt vector location */
	dmap = (volatile unsigned char *) dma_base_addr[DACI2S_DMA_CHAN];
	dmap[MCFDMA_DIVR] = DACI2S_VEC;

	/* Set interrupt level and priority */
	switch (DACI2S_DMA_CHAN) {
	case 0:  icr = MCFSIM_DMA0ICR ; imr = MCFSIM_IMR_DMA0; break;
	case 1:  icr = MCFSIM_DMA1ICR ; imr = MCFSIM_IMR_DMA1; break;
	case 2:  icr = MCFSIM_DMA2ICR ; imr = MCFSIM_IMR_DMA2; break;
	case 3:  icr = MCFSIM_DMA3ICR ; imr = MCFSIM_IMR_DMA3; break;
	default: icr = MCFSIM_DMA0ICR ; imr = MCFSIM_IMR_DMA0; break;
	}

	mbarp = (volatile unsigned char *) MCF_MBAR;
	mbarp[icr] = MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI1;
	mcf_setimr(mcf_getimr() & ~imr);

	/* Request DMA channel */
	printk ("DACI2S: requesting DMA channel %d\n", DACI2S_DMA_CHAN);
	result = request_dma(DACI2S_DMA_CHAN, "daci2s");
	if (result) {
		printk ("DACI2S: dma channel %d already in use\n",
			DACI2S_DMA_CHAN);
		return;
	}

	/* Setup chip select for DAC */
	/* CS4 - DAC, address 0x30800000 */
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_CSMR4)) = 0x0001;
	// 8 bit, external ack, no burst read and write
	*((volatile unsigned short *) (MCF_MBAR + MCFSIM_CSCR4)) = 0x0040;
}

/*****************************************************************************/
