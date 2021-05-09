/****************************************************************************/
/** lirc_intr.c *************************************************************/
/****************************************************************************/
/*
 * lirc_serial - Device driver that records pulse- and pause-lengths
 *               (space-lengths) between DDCD event on a serial port.
 *
 * Copyright (C) 2001 Lineo Inc., <davidm@lineo.com>, based on work by
 *
 *           Ralph Metzler <rjkm@thp.uni-koeln.de>
 *           Trent Piepho <xyzzy@u.washington.edu>
 *           Ben Pfaff <blp@gnu.org>
 *           Christoph Bartelmus <lirc@bartelmus.de>
 */
/****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
 
#include <linux/version.h>
#if LINUX_VERSION_CODE >= 0x020100
#define KERNEL_2_1
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
#define KERNEL_2_3
#endif
#endif

#include <linux/config.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/major.h>
#ifndef CONFIG_COLDFIRE
#include <linux/serial_reg.h>
#endif
#include <linux/time.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/delay.h>
#ifdef KERNEL_2_1
#include <linux/poll.h>
#endif

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/fcntl.h>

#include <linux/lirc.h>

/****************************************************************************/
#ifdef CONFIG_COLDFIRE
/****************************************************************************/
/*
 * the reset button is our interrupt source
 */

#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#include <asm/mcftimer.h>
#include <asm/nettel.h>

#define UART_MSR_DCD	0x1
#define UART_MSR_DDCD	0x2

#ifdef CONFIG_M5272
#define LIRC_IRQ		65
#else /* M5307 */
#define LIRC_IRQ		??
#endif /* M5307 */

#define LIRC_MAJOR	61

static volatile struct timeval fast_tv;

#define	FAST_HZ		10000
#define	FAST_USEC	(1000000 / FAST_HZ)

/****************************************************************************/
/*
 *	Use the other timer to provide high accuracy profiling info.
 */

static void coldfire_fast_tick(int irq, void *dummy, struct pt_regs *regs)
{
	volatile unsigned char	*timerp;

	/* Reset the ColdFire timer2 */
	timerp = (volatile unsigned char *) (MCF_MBAR + MCFTIMER_BASE2);
	timerp[MCFTIMER_TER] = MCFTIMER_TER_CAP | MCFTIMER_TER_REF;

	fast_tv.tv_usec += FAST_USEC;
	if (fast_tv.tv_usec >= 1000000) {
		fast_tv.tv_usec -= 1000000;
		fast_tv.tv_sec++;
	}
}

/****************************************************************************/

void coldfire_fast_init(void)
{
	volatile unsigned short	*timerp;
#ifdef CONFIG_M5272
	volatile unsigned long	*icrp;
#else
	volatile unsigned char	*icrp;
#endif

	printk("FASTCLK: lodging timer2=%d as profile timer\n", FAST_HZ);

	/* Set up TIMER 2 as poll clock */
	timerp = (volatile unsigned short *) (MCF_MBAR + MCFTIMER_BASE2);
	timerp[MCFTIMER_TMR] = MCFTIMER_TMR_DISABLE;

	timerp[MCFTIMER_TRR] = (unsigned short) ((MCF_CLK / 16) / FAST_HZ);
	timerp[MCFTIMER_TMR] = MCFTIMER_TMR_ENORI | MCFTIMER_TMR_CLK16 |
		MCFTIMER_TMR_RESTART | MCFTIMER_TMR_ENABLE;

#ifdef CONFIG_M5272
	icrp = (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1);
	*icrp = 0x00000e00; /* TMR2 with priority 7 */
	request_irq(70, coldfire_fast_tick, (SA_INTERRUPT | IRQ_FLG_FAST),
			"Fast Timer", NULL);
#else
	icrp = (volatile unsigned char *) (MCF_MBAR + MCFSIM_TIMER2ICR);
	*icrp = MCFSIM_ICR_AUTOVEC | MCFSIM_ICR_LEVEL7 | MCFSIM_ICR_PRI3;
	request_irq(31, coldfire_fast_tick, (SA_INTERRUPT | IRQ_FLG_FAST),
		"Fast Timer", NULL);
	mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_TIMER2);
#endif
}

/****************************************************************************/

#define do_gettimeofday(tvp)	(*(tvp) = fast_tv)

/****************************************************************************/
#endif /* CONFIG_COLDFIRE */
/****************************************************************************/

static volatile int line_status = 0;

#define LIRC_SIGNAL_PIN UART_MSR_DCD
#define LIRC_SIGNAL_PIN_CHANGE UART_MSR_DDCD

#define LIRC_DRIVER_NAME "lirc_intr"

#define RS_ISR_PASS_LIMIT 256

/* A long pulse code from a remote might take up to 300 bytes.  The
   daemon should read the bytes as soon as they are generated, so take
   the number of keys you think you can push before the daemon runs
   and multiply by 300.  The driver will warn you if you overrun this
   buffer.  If you have a slow computer or non-busmastering IDE disks,
   maybe you will need to increase this.  */

/* This MUST be a power of two!  It has to be larger than 1 as well. */

#define RBUF_LEN 256
#define WBUF_LEN 256

static int sense = -1;   /* -1 = auto, 0 = active high, 1 = active low */

#ifdef KERNEL_2_3
static DECLARE_WAIT_QUEUE_HEAD(lirc_wait_in);
#else
static struct wait_queue *lirc_wait_in = NULL;
#endif

#ifdef KERNEL_2_1
static spinlock_t lirc_lock = SPIN_LOCK_UNLOCKED;
#endif

#ifndef MODULE
static int opened = 0;
#undef MOD_IN_USE
#undef MOD_INC_USE_COUNT
#undef MOD_DEC_USE_COUNT
#define MOD_IN_USE			(opened)
#define MOD_INC_USE_COUNT	(opened = 1)
#define MOD_DEC_USE_COUNT	(opened = 0)
#endif

static lirc_t rbuf[RBUF_LEN];
static int rbh, rbt;

#define LIRC_OFF (UART_MCR_RTS|UART_MCR_OUT2)
#define LIRC_ON  (LIRC_OFF|UART_MCR_DTR)

/****************************************************************************/

static void inline
rbwrite(lirc_t l)
{
	unsigned int nrbt;

	nrbt = (rbt + 1) & (RBUF_LEN - 1);
	if (nrbt==rbh) { /* no new signals will be accepted */
#ifdef DEBUG
		printk(KERN_WARNING  LIRC_DRIVER_NAME  ": Buffer overrun\n");
#endif
		return;
	}
	rbuf[rbt] = l;
	rbt = nrbt;
}

/****************************************************************************/

static void inline
frbwrite(lirc_t l)
{
	/* simple noise filter */
	static lirc_t pulse=0L,space=0L;
	static unsigned int ptr=0;
	
	if (ptr>0 && (l&PULSE_BIT)) {
		pulse += l & PULSE_MASK;
		if (pulse > 250) {
			rbwrite(space);
			rbwrite(pulse|PULSE_BIT);
			ptr = 0;
			pulse = 0;
		}
		return;
	}
	if (!(l & PULSE_BIT)) {
		if (ptr == 0) {
			if (l > 20000) {
				space = l;
				ptr++;
				return;
			}
		} else {
			if (l > 20000) {
				space += pulse;
				if (space > PULSE_MASK)
					space = PULSE_MASK;
				space += l;
				if (space > PULSE_MASK)
					space = PULSE_MASK;
				pulse = 0;
				return;
			}
			rbwrite(space);
			rbwrite(pulse|PULSE_BIT);
			ptr = 0;
			pulse = 0;
		}
	}
	rbwrite(l);
}

/****************************************************************************/

void irq_handler(int i, void *blah, struct pt_regs *regs)
{
	lirc_t data;
	unsigned long flags;

	save_flags(flags); cli();

	data = (lirc_t) (fast_tv.tv_sec * 1000000 + fast_tv.tv_usec);
	line_status = (line_status + 1) & 0x1;
/*
 *	we always get interrupts in pairs,  a 1/2 second is considered
 *	too long,  so we must have somehow missed an interrupt,  so count
 *	this as the first interrupt in a pair.
 */
	if (data > 100000 && line_status == 0) {/* 1/2 second fixup if broken */
		line_status = 1;
		printk("F");
	}
	restore_flags(flags);

/*
 *	New mode, written by Trent Piepho <xyzzy@u.washington.edu>.
 *
 *	The old format was not very portable.
 *	We now use the type lirc_t to pass pulses
 *	and spaces to user space.
 *
 *	If PULSE_BIT is set a pulse has been received, otherwise a space has been
 *	received.  The driver needs to know if your receiver is active high or
 *	active low, or the space/pulse sense could be inverted. The bits denoted
 *	by PULSE_MASK are the length in microseconds. Lengths greater than or
 *	equal to 16 seconds are clamped to PULSE_MASK.  All other bits are unused.
 *	This is a much simpler interface for user programs, as well as
 *	eliminating "out of phase" errors with space/pulse autodetection.
 */

	if (fast_tv.tv_sec > 15) {
#ifdef DEBUG
		printk(KERN_WARNING LIRC_DRIVER_NAME
			   ": AIEEEE: %d %d %lx %lx\n",
			   line_status, sense, fast_tv.tv_sec, fast_tv.tv_usec);
#endif
		data = PULSE_MASK; /* really long time */
		if (!(line_status ^ sense)) { /* sanity check */
			/* detecting pulse while this MUST be a space! */
			sense = sense ? 0:1;
		}
	}
	frbwrite(line_status ^ sense ? data : (data|PULSE_BIT));
	fast_tv.tv_sec = fast_tv.tv_usec = 0;
	wake_up_interruptible(&lirc_wait_in);

#ifdef CONFIG_M5272
	* (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1) = 0xe0000000;
#endif
}
/****************************************************************************/

static int
init_hw(void)
{
	unsigned long flags;

	save_flags(flags); cli();
#ifdef CONFIG_COLDFIRE
/*
 *	we need a fast timer
 */
	coldfire_fast_init();
#endif
	restore_flags(flags);
	
	sense = 0;
	return(0);
}

/****************************************************************************/

static int lirc_open(struct inode *ino, struct file *filep)
{
	int result;
	unsigned long flags;
	
#ifdef KERNEL_2_1
	spin_lock(&lirc_lock);
#endif
	if (MOD_IN_USE) {
#ifdef KERNEL_2_1
		spin_unlock(&lirc_lock);
#endif
		return(-EBUSY);
	}
	
	result=request_irq(LIRC_IRQ, irq_handler, SA_INTERRUPT|IRQ_FLG_FAST,
			LIRC_DRIVER_NAME, NULL);
	switch(result) {
	case -EBUSY:
		printk(KERN_ERR LIRC_DRIVER_NAME ": IRQ %d busy\n", LIRC_IRQ);
#ifdef KERNEL_2_1
		spin_unlock(&lirc_lock);
#endif
		return(-EBUSY);

	case -EINVAL:
		printk(KERN_ERR LIRC_DRIVER_NAME
		       ": Bad irq number or handler\n");
#ifdef KERNEL_2_1
		spin_unlock(&lirc_lock);
#endif
		return(-EINVAL);

	default:
#ifdef DEBUG
		printk(KERN_INFO LIRC_DRIVER_NAME
		       ": Interrupt %d, port %04x obtained\n", LIRC_IRQ, port);
#endif
		break;
	};

	/* finally enable interrupts. */
	save_flags(flags); cli();
	
#ifdef CONFIG_M5272
	/*
	 * INT1 ipl set
	 */
	* (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1) = 0xe0000000;
#endif
	
	restore_flags(flags);
	
	/* Init read buffer pointers. */
	rbh = rbt = 0;
	
	MOD_INC_USE_COUNT;
#ifdef KERNEL_2_1
	spin_unlock(&lirc_lock);
#endif
	return 0;
}

/****************************************************************************/

#ifdef KERNEL_2_1
static int
lirc_close(struct inode *node, struct file *file)
#else
static void
lirc_close(struct inode *node, struct file *file)
#endif
{	unsigned long flags;
	
	save_flags(flags); cli();
	
#ifdef CONFIG_M5272
	/* INT1 ipl cleared */
	* (volatile unsigned long *) (MCF_MBAR + MCFSIM_ICR1) = 0x80000000;
#endif

	restore_flags(flags);
	
	free_irq(LIRC_IRQ, NULL);
#ifdef DEBUG
	printk(KERN_INFO  LIRC_DRIVER_NAME  ": freed IRQ %d\n", LIRC_IRQ);
#endif
	MOD_DEC_USE_COUNT;
#ifdef KERNEL_2_1
	return(0);
#endif
}

/****************************************************************************/
#ifdef KERNEL_2_1

static unsigned int
lirc_poll(struct file *file, poll_table * wait)
{
	poll_wait(file, &lirc_wait_in, wait);
	if (rbh != rbt)
		return POLLIN | POLLRDNORM;
	return(0);
}

#else

static int
lirc_select(struct inode *node, struct file *file,
	int sel_type, select_table * wait)
{
	if (sel_type != SEL_IN)
		return(0);
	if (rbh != rbt)
		return(1);
	select_wait(&lirc_wait_in, wait);
	return(0);
}

#endif
/****************************************************************************/

#ifdef KERNEL_2_1
static ssize_t
lirc_read(struct file *file, char *buf, size_t count, loff_t * ppos)
#else
static int
lirc_read(struct inode *node, struct file *file, char *buf, int count)
#endif
{
	int n=0, retval=0;
#ifdef KERNEL_2_3
	DECLARE_WAITQUEUE(wait,current);
#else
	struct wait_queue wait={current,NULL};
#endif
	
	if (n % sizeof(lirc_t))
		return(-EINVAL);
	
	add_wait_queue(&lirc_wait_in,&wait);
	while (n < count) {
		current->state = TASK_INTERRUPTIBLE;
		if (rbt != rbh) {
#ifdef KERNEL_2_1
			copy_to_user((void *) buf+n, (void *) &rbuf[rbh], sizeof(lirc_t));
#else
			memcpy_tofs((void *) buf+n, (void *) &rbuf[rbh], sizeof(lirc_t));
#endif
			rbh = (rbh + 1) & (RBUF_LEN - 1);
			n += sizeof(lirc_t);
		} else {
			if (file->f_flags & O_NONBLOCK) {
				retval = -EAGAIN;
				break;
			}
#ifdef KERNEL_2_1
			if (signal_pending(current)) {
				retval = -ERESTARTSYS;
				break;
			}
#else
			if (current->signal & ~current->blocked) {
				retval = -EINTR;
				break;
			}
#endif
			schedule();
		}
	}
	current->state = TASK_RUNNING;
	remove_wait_queue(&lirc_wait_in,&wait);
	return (n ? n : retval);
}

/****************************************************************************/

static int
lirc_ioctl(struct inode *node, struct file *filep, unsigned int cmd,
	unsigned long arg)
{
	int result;
	unsigned long value;
	unsigned int ivalue;
	unsigned long features = LIRC_CAN_REC_MODE2;
	
	switch (cmd) {
	case LIRC_GET_FEATURES:
#ifdef KERNEL_2_1
		result = put_user(features, (unsigned long *) arg);
		if (result)
			return(result); 
#else
		result = verify_area(VERIFY_WRITE, (unsigned long*) arg,
				   sizeof(unsigned long));
		if (result)
			return(result);
		put_user(features, (unsigned long *) arg);
#endif
		break;

	case LIRC_GET_REC_MODE:
#ifdef KERNEL_2_1
		result = put_user(LIRC_MODE_MODE2, (unsigned long *) arg);
		if (result)
			return(result); 
#else
		result = verify_area(VERIFY_WRITE, (unsigned long *) arg,
				   sizeof(unsigned long));
		if (result)
			return(result);
		put_user(LIRC_MODE_MODE2, (unsigned long *) arg);
#endif
		break;

	case LIRC_SET_REC_MODE:
#ifdef KERNEL_2_1
		result = get_user(value, (unsigned long *) arg);
		if (result)
			return(result);
#else
		result = verify_area(VERIFY_READ, (unsigned long *) arg,
				   sizeof(unsigned long));
		if (result)
			return(result);
		value = get_user((unsigned long *) arg);
#endif
		if (value != LIRC_MODE_MODE2)
			return(-ENOSYS);
		break;

	default:
		return(-ENOIOCTLCMD);
	}
	return(0);
}

/****************************************************************************/

static struct file_operations lirc_fops =
{
	read:    lirc_read,
#ifdef KERNEL_2_1
	poll:    lirc_poll,
#else
	select:  lirc_select,
#endif
	ioctl:   lirc_ioctl,
	open:    lirc_open,
	release: lirc_close
};

/****************************************************************************/

int lirc_init(void)
{
	int result;

	if ((result = init_hw()) < 0)
		return(result);
	if (register_chrdev(LIRC_MAJOR, LIRC_DRIVER_NAME, &lirc_fops) < 0) {
		printk(KERN_ERR  LIRC_DRIVER_NAME  
		       ": register_chrdev failed!\n");
		return(-EIO);
	}
	return(0);
}

/****************************************************************************/

#ifdef MODULE

#if LINUX_VERSION_CODE >= 0x020100
MODULE_AUTHOR("David McCullough");
MODULE_DESCRIPTION("Infrared receiver driver for raw interrupts.");

EXPORT_NO_SYMBOLS;
#endif

int init_module(void)
{
	return(lirc_init());
}

void cleanup_module(void)
{
	unregister_chrdev(LIRC_MAJOR, LIRC_DRIVER_NAME);
#ifdef DEBUG
	printk(KERN_INFO  LIRC_DRIVER_NAME  ": cleaned up module\n");
#endif
}

#endif

/****************************************************************************/
