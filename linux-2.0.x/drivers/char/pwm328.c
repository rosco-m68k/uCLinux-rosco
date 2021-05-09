/*****************************************************************************/

/*
 *      pwm328.c -- Driver for MC328EZ PWM
 *      By David Beckemeyer
 *
 *      Derived from:
 *
 *	dac0800.c -- driver for NETtel DMA audio
 *
 *	(C) Copyright 1999, greg Ungerer (gerg@moreton.com.au)
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
#include <asm/system.h>


/*****************************************************************************/

#define	PWM_MAJOR	120

#define	PWM_BUFSIZE	(16 * 1024)

#define PWM_BUSY	1
#define PWM_OPEN	2

//#define DEBUG_STARTPWM
//#define DEBUG_PWM_ISR
//#define DEBUG_PWM_BUF
//#define DEBUG_PWM_OPEN

/*****************************************************************************/

/*
 *	Double buffers for storing audio samples.
 */
unsigned short	pwm_buf0[PWM_BUFSIZE/2];
unsigned short	pwm_buf1[PWM_BUFSIZE/2];
unsigned int	pwm_bufsize;
unsigned int	pwm_buf0cnt;
unsigned int	pwm_buf1cnt;

unsigned int	pwm_flags;
unsigned int	pwm_curbuf;
unsigned int 	pwm_bufbusy;

#define	PWM_BUF0		0x1
#define	PWM_BUF1		0x2

#define	PWM_ALLBUFS		0x3

struct wait_queue	*pwm_waitchan;

static unsigned short *pwm_pos;
static unsigned short *pwm_end;

#define PWMC_CLKSEL	(0<<PWMC_CLKSEL_SHIFT) /* /2 32KHz rate */
#define PWMC_PRESCALER	(0<<PWMC_PRESCALER_SHIFT) /* voice mode */
#define PWMC_REPEAT	(2<<PWMC_REPEAT_SHIFT) /* repeat 3 times */

#define	PWMC_INIT	(PWMC_CLKSRC       *0| \
			 PWMC_PRESCALER      | \
			 PWMC_IRQ          *0| \
			 PWMC_IRQEN          | \
			 PMNC_FIFOAV       *0| \
 			 PWMC_EN             | \
			 PWMC_REPEAT         | \
			 PWMC_CLKSEL            )

#define PWMC_RESET	0x0020
			

/*****************************************************************************/

/*****************************************************************************/

void pwm_startpwm(unsigned char *buf, unsigned int size)
{

#ifdef DEBUG_STARTPWM
	printk("PWM: starting curbuf=%x\n", pwm_curbuf);
#endif
	/* Set the vars */
	pwm_pos = buf;
	pwm_end = buf + size;

	/* mark the PWM BUSY */
	pwm_flags |= PWM_BUSY;

	/* Turn on the hardware */
	PWMC = PWMC_INIT;

}

/*****************************************************************************/

/*
 *	If there is a buffer ready to go then start sending it
 */

void pwm_startbuf(void)
{
	unsigned char	*bufp;
	unsigned int	flag, cnt;

	/* If already sending nothing to do */
	if (pwm_curbuf)
		return;

	/* Simple double buffered arrangement... */
	if (pwm_bufbusy & PWM_BUF0) {
		bufp = pwm_buf0;
		cnt = pwm_buf0cnt;
		flag = PWM_BUF0;
	} else if (pwm_bufbusy & PWM_BUF1) {
		bufp = pwm_buf1;
		cnt = pwm_buf1cnt;
		flag = PWM_BUF1;
	} else {
		return;
	}

	pwm_curbuf = flag;
	pwm_startpwm(bufp, cnt);
}

/*****************************************************************************/

int pwm_open(struct inode *inode, struct file *filp)
{
#ifdef DEBUG_PWM_OPEN
	printk("%s(%d): pwm_open() flags=%d\n", __FILE__, __LINE__, pwm_flags);
#endif

	if (pwm_flags)
		return -EBUSY;
	pwm_flags |= PWM_OPEN;
	pwm_curbuf = 0;
	return(0);
}

/*****************************************************************************/

void pwm_release(struct inode *inode, struct file *filp)
{
#ifdef DEBUG_PWM_RELEASE
	printk("%s(%d): pwm_close()\n", __FILE__, __LINE__);
#endif
	pwm_flags &= ~PWM_OPEN;
}

/*****************************************************************************/

int pwm_write(struct inode *inode, struct file *filp, const char *buf, int count)
{
	unsigned char	*pwmbufp;
	unsigned int	size, bufflag;
	unsigned long	flags;

#ifdef DEBUG_PWM_WRITE
	printk("%s(%d): pwm_write(buf=%x,count=%d)\n", __FILE__, __LINE__,
		(int) buf, count);
#endif

	size = (count > pwm_bufsize) ? pwm_bufsize : count;

	/* Check for empty buffer, if not wait for one. */
	save_flags(flags); cli();
	while ((pwm_bufbusy & PWM_ALLBUFS) == PWM_ALLBUFS) {
		interruptible_sleep_on(&pwm_waitchan);
		if (current->signal & ~current->blocked) {
			restore_flags(flags);
			return(-ERESTARTSYS);
		}
	}

	if (pwm_bufbusy & PWM_BUF0) {
		bufflag = PWM_BUF1;
		pwmbufp = &pwm_buf1[0];
		pwm_buf1cnt = size;
	} else {
		bufflag = PWM_BUF0;
		pwmbufp = &pwm_buf0[0];
		pwm_buf0cnt = size;
	}
	restore_flags(flags);

#ifdef DEBUG_PWM_BUF
	printk("PWM: filling %d\n", bufflag);
#endif
	/* Copy the buffer local and start sending it. */
	memcpy_fromfs(pwmbufp, buf, size);

	save_flags(flags); cli();
	pwm_bufbusy |= bufflag;
	pwm_startbuf();
	restore_flags(flags);

	return(size);
}

/*****************************************************************************/

int pwm_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int	rc = 0;

	switch (cmd) {
	case 1:
		printk("PWM: pwm_curbuf=%x pwm_pos=%x\n", pwm_curbuf, pwm_pos);
		break;
	default:
		rc = -EINVAL;
		break;
	}

	return(rc);
}

/*****************************************************************************/

void pwm_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long	flags;
	unsigned int	status;

	/* read status to clear IRQ */
	status = PWMC;

	if (pwm_curbuf) {

		*((volatile unsigned short *) 0xFFFFF502) = *(pwm_pos++);

		if (pwm_pos < pwm_end) 
			*((volatile unsigned short *) 0xFFFFF502) = *(pwm_pos++);

		/* Handle end-of-buffer */
		if (pwm_pos >= pwm_end) {
#ifdef DEBUG_PWM_ISR
			printk("PWM: buffer %x done\n", pwm_curbuf);
#endif
			save_flags(flags); cli();

			/* Mark buffer no longer in use */
			pwm_bufbusy &= ~pwm_curbuf;
			pwm_curbuf = 0;

			/* Start other waiting buffers! */
			pwm_startbuf();

			if (!pwm_curbuf) {
				pwm_flags &= ~PWM_BUSY;
				PWMC = PWMC_RESET;
			}

			/* Wake up writers */
			wake_up_interruptible(&pwm_waitchan);

			restore_flags(flags);

		}
	}
	else {
		pwm_flags &= ~PWM_BUSY;
		PWMC = PWMC_RESET;
	}
}

/*****************************************************************************/

/*
 *	Exported file operations structure for driver...
 */

struct file_operations	pwm_fops = {
	NULL,          /* lseek */
	NULL,          /* read */
	pwm_write,
	NULL,          /* readdir */
	NULL,          /* poll */
	pwm_ioctl,
	NULL,          /* mmap */
	pwm_open,
	pwm_release,
	NULL,          /* fsync */
	NULL,          /* fasync */
	NULL,          /* check_media_change */
	NULL           /* revalidate */
};

/*****************************************************************************/

void pwm_init(void)
{
	int			result;

	/* Register pwm as character device */
	result = register_chrdev(PWM_MAJOR, "pwm", &pwm_fops);
	if (result < 0) {
		printk(KERN_WARNING "PWM: can't get major %d\n",
			PWM_MAJOR);
		return;
	}

	printk ("PWM: Copyright (C) 2001, David Beckemeyer "
		"(david@bdt.com)\n");

	/* reset the hardware */
	PWMC = PWMC_RESET;

	pwm_bufsize = PWM_BUFSIZE;
	pwm_flags = 0;
	pwm_curbuf = 0;
	pwm_bufbusy = 0;
	pwm_waitchan = NULL;

	/* Install ISR (interrupt service routine) */
	result = request_irq(IRQ_MACHSPEC | PWM_IRQ_NUM, pwm_isr, IRQ_FLG_STD,
		"PWM", NULL);
	if (result) {
		printk ("PWM: IRQ %d already in use\n", PWM_IRQ_NUM);
		return;
	}

}

/*****************************************************************************/
