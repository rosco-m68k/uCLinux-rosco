/*****************************************************************************/

/*
 *	dac0800.c -- driver for NETtel DMA audio
 *
 *	(C) Copyright 1999, greg Ungerer (gerg@snapgear.com)
 * 	(C) Copyright 2000, Lineo Inc. (www.lineo.com) 
 *	
 *	Based loosely on lcddma.c which is:
 *
 *	Copyright (C) 1999 Rob Scott (rscott@mtrob.fdns.net)
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

#define	DAC0800_MAJOR	120
#define	DAC0800_VEC	120

#define	DAC0800_DMA_CHAN	1
#define	DAC0800_DMA_DESTADDR	0x30400000

#define	DAC0800_DMA_BUFSIZE	(16 * 1024)

/*****************************************************************************/

/*
 *	Double buffers for storing audio samples.
 */
unsigned char	dac0800_buf0[DAC0800_DMA_BUFSIZE];
unsigned char	dac0800_buf1[DAC0800_DMA_BUFSIZE];
unsigned int	dac0800_bufsize = DAC0800_DMA_BUFSIZE;
unsigned int	dac0800_buf0cnt;
unsigned int	dac0800_buf1cnt;

unsigned int	dac0800_curbuf = 0;
unsigned int 	dac0800_bufbusy = 0;

#define	DAC0800_BUF0		0x1
#define	DAC0800_BUF1		0x2

#define	DAC0800_ALLBUFS		0x3

struct wait_queue	*dac0800_waitchan = NULL;

/*****************************************************************************/

/*
 *	Set up internal timer1 for our sample rate.
 */

void dac0800_setclk(unsigned int rate)
{
	volatile unsigned short	*timerp;
	unsigned long		flags;

	timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE2);
	timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;

	save_flags(flags); cli();
	timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;
	timerp[MCFTIMER_TER] = MCFTIMER_TER_CAP | MCFTIMER_TER_REF;
        timerp[MCFTIMER_TRR] = (unsigned short) (MCF_CLK / rate);
        timerp[MCFTIMER_TMR] = (0 << 8) | /* set prescaler to divide by 1 */
                        MCFTIMER_TMR_DISCE | MCFTIMER_TMR_DISOM |
                        MCFTIMER_TMR_ENORI | MCFTIMER_TMR_RESTART |
                        MCFTIMER_TMR_CLK1  | MCFTIMER_TMR_ENABLE;
	restore_flags(flags);
}

/*****************************************************************************/

void dac0800_startdma(unsigned char *buf, unsigned int size)
{
	/* Clear DMA interrupt */
	disable_dma(DAC0800_DMA_CHAN);

	/* Do DMA write to i/o operation */
	set_dma_mode(DAC0800_DMA_CHAN, DMA_MODE_WRITE);
	set_dma_device_addr(DAC0800_DMA_CHAN, DAC0800_DMA_DESTADDR);
	set_dma_addr(DAC0800_DMA_CHAN, (unsigned int) buf);
	set_dma_count(DAC0800_DMA_CHAN, size);

	/* Fire it off! */
	enable_dma(DAC0800_DMA_CHAN);
}

/*****************************************************************************/

/*
 *	If there is a buffer ready to go then start DMA from it.
 */

void dac0800_startbuf(void)
{
	unsigned char	*bufp;
	unsigned int	flag, cnt;

	/* If already DMAing nothing to do */
	if (dac0800_curbuf)
		return;

	/* Simple double buffered arrangement... */
	if (dac0800_bufbusy & DAC0800_BUF0) {
		bufp = dac0800_buf0;
		cnt = dac0800_buf0cnt;
		flag = DAC0800_BUF0;
	} else if (dac0800_bufbusy & DAC0800_BUF1) {
		bufp = dac0800_buf1;
		cnt = dac0800_buf1cnt;
		flag = DAC0800_BUF1;
	} else {
		return;
	}

	dac0800_curbuf = flag;
	dac0800_startdma(bufp, cnt);
}

/*****************************************************************************/

int dac0800_open(struct inode *inode, struct file *filp)
{
#if 0
	printk("%s(%d): dac0800_open()\n", __FILE__, __LINE__);
#endif

	dac0800_curbuf = 0;
	return(0);
}

/*****************************************************************************/

void dac0800_release(struct inode *inode, struct file *filp)
{
#if 0
	printk("%s(%d): dac0800_close()\n", __FILE__, __LINE__);
#endif
}

/*****************************************************************************/

int dac0800_write(struct inode *inode, struct file *filp, const char *buf, int count)
{
	unsigned char	*dacbufp;
	unsigned int	size, bufflag;
	unsigned long	flags;

#if 0
	printk("%s(%d): dac0800_write(buf=%x,count=%d)\n", __FILE__, __LINE__,
		(int) buf, count);
#endif

	size = (count > dac0800_bufsize) ? dac0800_bufsize : count;

	/* Check for empty DMA buffer, if not wait for one. */
	save_flags(flags); cli();
	while ((dac0800_bufbusy & DAC0800_ALLBUFS) == DAC0800_ALLBUFS) {
		if (current->signal & ~current->blocked) {
			restore_flags(flags);
			return(-ERESTARTSYS);
		}
		interruptible_sleep_on(&dac0800_waitchan);
	}

	if (dac0800_bufbusy & DAC0800_BUF0) {
		bufflag = DAC0800_BUF1;
		dacbufp = &dac0800_buf1[0];
		dac0800_buf1cnt = size;
	} else {
		bufflag = DAC0800_BUF0;
		dacbufp = &dac0800_buf0[0];
		dac0800_buf0cnt = size;
	}
	restore_flags(flags);

	/* Copy the buffer local and start DMAing it. */
	memcpy_fromfs(dacbufp, buf, size);

	save_flags(flags); cli();
	dac0800_bufbusy |= bufflag;
	dac0800_startbuf();
	restore_flags(flags);

	return(size);
}

/*****************************************************************************/

int dac0800_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	long	speed;
	int	rc = 0;

	switch (cmd) {
	case SNDCTL_DSP_SPEED:
		rc = verify_area(VERIFY_READ, (void *) arg, sizeof(speed));
		if (rc == 0) {
			speed = get_fs_long((unsigned long *) arg);
			dac0800_setclk(speed);
		}
		break;
	default:
		rc = -EINVAL;
		break;
	}

	return(rc);
}

/*****************************************************************************/

void dac0800_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	/* Clear DMA interrupt */
	disable_dma(DAC0800_DMA_CHAN);

	/* Mark buffer no longer in use */
	dac0800_bufbusy &= ~dac0800_curbuf;
	dac0800_curbuf = 0;

	/* Start of any other waiting buffers! */
	dac0800_startbuf();

	/* Wake up writers */
	wake_up_interruptible(&dac0800_waitchan);
}

/*****************************************************************************/

/*
 *	Exported file operations structure for driver...
 */

struct file_operations	dac0800_fops = {
	NULL,          /* lseek */
	NULL,          /* read */
	dac0800_write,
	NULL,          /* readdir */
	NULL,          /* poll */
	dac0800_ioctl,
	NULL,          /* mmap */
	dac0800_open,
	dac0800_release,
	NULL,          /* fsync */
	NULL,          /* fasync */
	NULL,          /* check_media_change */
	NULL           /* revalidate */
};

/*****************************************************************************/

void dac0800_init(void)
{
	volatile unsigned char	*mbarp, *dmap;
	unsigned long		imr;
	unsigned int		icr;
	int			result;

	/* Register dac0800 as character device */
	result = register_chrdev(DAC0800_MAJOR, "dac0800", &dac0800_fops);
	if (result < 0) {
		printk(KERN_WARNING "DAC0800: can't get major %d\n",
			DAC0800_MAJOR);
		return;
	}

	printk ("DAC0800: Copyright (C) 1999, Greg Ungerer "
		"(gerg@snapgear.com)\n");

	/* Install ISR (interrupt service routine) */
	result = request_irq(DAC0800_VEC, dac0800_isr, SA_INTERRUPT,
		"DAC0800", NULL);
	if (result) {
		printk ("DAC0800: IRQ %d already in use\n", DAC0800_VEC);
		return;
	}

	/* Set interrupt vector location */
	dmap = (volatile unsigned char *) dma_base_addr[DAC0800_DMA_CHAN];
	dmap[MCFDMA_DIVR] = DAC0800_VEC;

	/* Set interrupt level and priority */
	switch (DAC0800_DMA_CHAN) {
	case 1:  icr = MCFSIM_DMA1ICR ; imr = MCFSIM_IMR_DMA1; break;
	case 2:  icr = MCFSIM_DMA2ICR ; imr = MCFSIM_IMR_DMA2; break;
	case 3:  icr = MCFSIM_DMA3ICR ; imr = MCFSIM_IMR_DMA3; break;
	default: icr = MCFSIM_DMA0ICR ; imr = MCFSIM_IMR_DMA0; break;
	}

	mbarp = (volatile unsigned char *) MCF_MBAR;
	mbarp[icr] = MCFSIM_ICR_LEVEL6 | MCFSIM_ICR_PRI1;
	mcf_setimr(mcf_getimr() & ~imr);

	/* Request DMA channel */
	printk ("DAC0800: requesting DMA channel %d\n", DAC0800_DMA_CHAN);
	result = request_dma(DAC0800_DMA_CHAN, "dac0800");
	if (result) {
		printk ("DAC0800: dma channel %d already in use\n",
			DAC0800_DMA_CHAN);
		return;
	}

	/* Program default timer rate */
	dac0800_setclk(8000);
}

/*****************************************************************************/
