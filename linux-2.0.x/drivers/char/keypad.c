/*****************************************************************************/

/*
 *	keypad.c -- simple keypad driver.
 *
 *	(C) Copyright 2001, Greg Ungerer (gerg@snapgear.com)
 *	(C) Copyright 2001, Lineo Inc. (www.lineo.com) 
 */

/*****************************************************************************/

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <asm/param.h>

#include <asm/coldfire.h>
#include <asm/mcfsim.h>
#define POLLING 1
#define SLEEP_FRACTION 15

/*****************************************************************************/

/*
 *	Define driver major number.
 */
#define	KEYPAD_MAJOR	121

/*
 *  Define interrupt
 */
#define KEYPAD_IRQ		25

/*
 *	Place to store key codes at interrupt/poll time.
 */
#define	KEYPAD_BUFSIZE		8

unsigned char	keypad_buf[KEYPAD_BUFSIZE];
int		keypad_bufhead = 0;
int		keypad_bufcount;

int		keypad_isopen;
int		keypad_blocking;
static int		new_input;
char	old_kval;

struct wait_queue	*keypad_waitq = NULL;
static struct timer_list poll_timer;

/*****************************************************************************/

int keypad_open(struct inode *inode, struct file *filp)
{
#if KEYPAD_DEBUG
	printk("keypad_open()\n");
#endif

	keypad_blocking = (filp->f_flags & O_NONBLOCK) ? 0 : 1;
	keypad_isopen++;
	keypad_bufcount = 0;
	keypad_bufhead = 0;
	return(0);
}

/*****************************************************************************/

void keypad_release(struct inode *inode, struct file *filp)
{
#if KEYPAD_DEBUG
	printk("keypad_close()\n");
#endif
	keypad_isopen = 0;
}

/*****************************************************************************/

int keypad_read(struct inode *inode, struct file *filp, char *buf, int count)
{
#if POLLING
	unsigned short	kval;
	int		pos;
#else
	unsigned long	flags;
	int		i;
#endif
	if (count == 0)
		return 0;

#if KEYPAD_DEBUG
	printk("keypad_read(buf=%x,count=%d)\n", (int) buf, count);
#endif

#if POLLING
	del_timer(&poll_timer);

	put_user(keypad_buf[keypad_bufhead], buf);
	if (++keypad_bufhead >= KEYPAD_BUFSIZE)
		keypad_bufhead = 0;
	keypad_bufcount--;
	if (keypad_bufcount < 0)
		keypad_bufcount = 0;

	poll_timer.expires = jiffies + HZ/SLEEP_FRACTION;
	add_timer(&poll_timer);
	return(1);
#else
	save_flags(flags); cli();
	while ((keypad_bufcount == 0) && keypad_blocking) {
		if (current->signal & ~current->blocked)
			return(-ERESTARTSYS);
		interruptible_sleep_on(&keypad_waitq);
	}
	if (count > keypad_bufcount)
		count = keypad_bufcount;
	for (i = 0; i < count; i++) {
		put_user(keypad_buf[keypad_bufhead], buf);
		if (++keypad_bufhead >= KEYPAD_BUFSIZE)
			keypad_bufhead = 0;
	}
	keypad_bufcount -= i;
	restore_flags(flags);
	return(i);
#endif
}

/*****************************************************************************/
static int keypad_select(struct inode *inode, struct file *file,
				              int sel_type, select_table * wait)
{
	switch (sel_type) {
			case SEL_IN:
					if (keypad_bufcount > 0)
						return 1;
					select_wait(&keypad_waitq, wait);
					if (keypad_bufcount > 0)
						return 1;
					break;
			default:
					return -1;
	}
	return 0;
}
/*****************************************************************************/
void keypad_poll(void) {
	unsigned short	kval;
	int pos;

	del_timer(&poll_timer);
	kval = *((volatile unsigned short *) (MCF_MBAR + MCFSIM_PADAT));
	kval = ~(kval >> 10) & 0x003f;

	if (kval == old_kval) {
		new_input = 0;
	} else {
		pos = (keypad_bufhead + keypad_bufcount);
		while (pos >= KEYPAD_BUFSIZE)
			pos -= KEYPAD_BUFSIZE;
		keypad_buf[pos] = (unsigned char) kval;
		keypad_bufcount++;
		if (keypad_bufcount >= KEYPAD_BUFSIZE)
			keypad_bufcount--;
		old_kval = kval;
		new_input = 1;
		wake_up_interruptible(&keypad_waitq);
	}
	poll_timer.expires = jiffies + HZ/SLEEP_FRACTION;
	add_timer(&poll_timer);
}

/*****************************************************************************/

int keypad_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int	rc = 0;

	switch (cmd) {
	default:
		rc = -EINVAL;
		break;
	}

	return(rc);
}

/*****************************************************************************/

void keypad_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned short	kval;
	int		pos;
	/* FIXME HERE SOMEWHERE
	 * currently this code only works if KEYPAD_DEBUG is defined
	 * go figure...
	 */

#if KEYPAD_DEBUG
	printk("keypad_isr(irq=%d)\n", irq);
#endif
	if (keypad_isopen) {
		kval = *((volatile unsigned short *) (MCF_MBAR + MCFSIM_PADAT));
		kval = ~(kval >> 10) & 0x003f;
#if KEYPAD_DEBUG
		printk("keypad_read(kval = %c)\n", kval);
#endif
		pos = (keypad_bufhead + keypad_bufcount) % KEYPAD_BUFSIZE;
		keypad_buf[pos] = (unsigned char) kval;
		keypad_bufcount++;
		wake_up_interruptible(&keypad_waitq);
	}

}

/*****************************************************************************/

/*
 *	Exported file operations structure for driver...
 */

struct file_operations	keypad_fops = {
	NULL,		/* lseek */
	keypad_read,	/* read */
	NULL,		/* write */
	NULL,		/* readdir */
	keypad_select,		/* poll */
	keypad_ioctl,		/* ioctl */
	NULL,		/* mmap */
	keypad_open,	/* open */
	keypad_release,	/* release */
	NULL,		/* fsync */
	NULL,		/* fasync */
	NULL,		/* check_media_change */
	NULL		/* revalidate */
};

/*****************************************************************************/

void keypad_init(void)
{
	int	rc;

	/* Register keypad as character device */
	if ((rc = register_chrdev(KEYPAD_MAJOR, "keypad", &keypad_fops)) < 0) {
		printk(KERN_WARNING "KEYPAD: can't get major %d\n",
			KEYPAD_MAJOR);
		return;
	}
	keypad_isopen = 0;
	new_input = 0;
	poll_timer.expires = jiffies + HZ/SLEEP_FRACTION;
	poll_timer.function = keypad_poll;
	add_timer(&poll_timer);


	printk("KEYPAD: Copyright (C) 2000-2001, Greg Ungerer "
		"(gerg@snapgear.com)\n");

#ifndef POLLING
	/* register IRQ */
	mcf_autovector(KEYPAD_IRQ);
	rc = request_irq(KEYPAD_IRQ, keypad_isr, SA_INTERRUPT, "KEYPAD", NULL);
	if (rc) {
		printk("Keypad irq not registered.  Error: %d\n", rc);
	}
#endif

}

/*****************************************************************************/
