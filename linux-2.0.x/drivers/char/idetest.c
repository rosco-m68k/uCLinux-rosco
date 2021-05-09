/*****************************************************************************/

/*
 *	idetest.c -- A simple driver to exercise the Lineo IDE test jig.
 *
 * 	(C) Copyright 2001, Lineo Inc. (www.lineo.com) 
 *
 */

/*****************************************************************************/

#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>

#include <asm/mcfsim.h>

#if LINUX_VERSION_CODE < 0x020100
#define copy_from_user(a, b, c)		memcpy_fromfs(a, b, c)
#define copy_to_user(a, b, c)		memcpy_tofs(a, b, c)
#endif


#define IDETEST_NAME	"idetest"
#define IDETEST_MAJOR	122

/*
 *	Hardware resources used by IDE test driver. This is the
 *	setup on the Lineo MP3 hardware.
 */
#define	IDETEST_BASE	0x30800000
#define IDETEST_SIZE	16
#if 0
#define	IDETEST_IRQ		29
#endif

static int idetest_isopen;
#if 0
static int idetest_gotint;
#endif

/*****************************************************************************/

/* Replace standard IO functions for funky mapping of MP3 board */
#undef outb
#undef inb

#define outb    ide_outb
#define inb     ide_inb

/* The IDE interface uses a 16 bit data port.
 *
 * When the ColdFire does transfers using a 16 bit
 * port, the byte lane used depends on A0, so that even
 * and odd addresses use different byte lanes.  The byte
 * lane to use is normally communicated using BEW0 and BEW1.
 *
 * However, the IDE test rig does not have access to the
 * BEW0 and BEW1 signals, and uses the same byte lane for
 * both even and odd addresses.
 *
 * To solve this, we only access even addresses from the
 * Coldfire (to keep the byte lane constant), but use A15
 * as the least significant bit of the address presented
 * to the IDE interface (so that the IDE test rig can
 * still see odd addresses).
 */
#define ADDR8_PTR(addr)		(((addr) & 0x1) ? (0x8000 + (addr) - 1) : (addr))

/* For the IDE test rig, the byte lane used depends on A3.
 * We do 8 bit reads/writes as 16-bit transfers so that we
 * can use either byte lane. */
#define SWAP8(addr, w)		(((addr) & 0x8) ? (w) : ((((w) & 0xffff) << 8) | (((w) & 0xffff) >> 8)))


static void ide_outb(unsigned int val, unsigned int addr)
{
	volatile unsigned short	*rp;

	rp = (volatile unsigned short *) ADDR8_PTR(addr);
	val = SWAP8(addr, val);
#if 0
	printk("ide_outb(addr=%x,val=%x)\n", (int) rp, (int)val);
#endif
	*rp = val;
}


static int ide_inb(unsigned int addr)
{
	volatile unsigned short	*rp, val;

	rp = (volatile unsigned short *) ADDR8_PTR(addr);
	val = *rp;
#if 0
	printk("ide_inb(addr=%x)=%x\n", (int) rp, (int) val);
#endif
	return(SWAP8(addr, val));
}

/*****************************************************************************/

static int idetest_open(struct inode *inode, struct file *filp)
{
#ifdef DEBUG_FUNCS
	printk("idetest_open()\n");
#endif

	if (idetest_isopen)
		return -EBUSY;

	idetest_isopen++;

	MOD_INC_USE_COUNT;

	return 0;
}

/*****************************************************************************/

static
#if LINUX_VERSION_CODE < 0x020100
void
#else
int
#endif
idetest_close(struct inode *inode, struct file *filp)
{
#ifdef DEBUG_FUNCS
	printk("idetest_close()\n");
#endif

	idetest_isopen--;

	MOD_DEC_USE_COUNT;

#if LINUX_VERSION_CODE >= 0x020100
	return 0;
#endif
}

/*****************************************************************************/

static
#if LINUX_VERSION_CODE < 0x020100
int idetest_write(struct inode *inode, struct file *filp, const char *buf, int count)
#else
ssize_t idetest_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
#endif
{
#if LINUX_VERSION_CODE < 0x020100
	loff_t *ppos = &filp->f_pos;
#endif
	char p[IDETEST_SIZE];
	int i;

#ifdef DEBUG_FUNCS
	printk("idetest_write(buf=%x,count=%d)\n", (int) buf, count);
#endif

	if (*ppos + count > IDETEST_SIZE)
		return -ENOSPC;
	copy_from_user(p, buf, count);
	for (i=0; i<count; i++)
		outb(p[i], IDETEST_BASE+*ppos+i);
	*ppos += i;

	/* XXX: check interrupts? */

	return i;
}

/*****************************************************************************/

static
#if LINUX_VERSION_CODE < 0x020100
int idetest_read(struct inode *inode, struct file *filp, char *buf, int count)
#else
ssize_t idetest_read(struct file *filp, char *buf, size_t count, loff_t *ppos)
#endif
{
#if LINUX_VERSION_CODE < 0x020100
	loff_t *ppos = &filp->f_pos;
#endif
	char p[IDETEST_SIZE];
	int i;

#ifdef DEBUG_FUNCS
	printk("idetest_read(buf=%x,count=%d)\n", (int) buf, count);
#endif

	if (*ppos + count > IDETEST_SIZE)
		count = IDETEST_SIZE - *ppos;
	for (i=0; i<count; i++)
		p[i] = inb(IDETEST_BASE+*ppos+i);
	copy_to_user(buf, p, i);
	*ppos += i;

	/* XXX: check interrupts? */

	return i;
}

/*****************************************************************************/

static int idetest_lseek(struct inode * inode, struct file * file, off_t offset, int orig)
{
	loff_t newpos;
	
	switch (orig) {
	case 0:
		newpos = offset;
		break;
	case 1:
		newpos = file->f_pos + offset;
		break;
	default:
		return -EINVAL;
	}

	if (newpos<0 || newpos>=IDETEST_SIZE)
		return -EINVAL;

	file->f_pos = newpos;
	return file->f_pos;
}

/*****************************************************************************/

static int idetest_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

#ifdef DEBUG_FUNCS
	printk("idetest_ioctl(cmd=%x,arg=%x)\n", (int) cmd, (int) arg);
#endif

	switch(cmd) {
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

/*****************************************************************************/

#if 0
static void idetest_isr(int irq, void *dev_id, struct pt_regs *regs)
{
#ifdef DEBUG_FUNCS
	printk("idetest_isr(irq=%d)\n", irq);
#endif
	if (idetest_isopen) {
		idetest_gotint = 1;
	}
}
#endif

/*****************************************************************************/

static struct file_operations idetest_fops = {
	lseek: idetest_lseek,
	read: idetest_read,
	write: idetest_write,
	ioctl: idetest_ioctl,
	open: idetest_open,
	release: idetest_close
};

/*****************************************************************************/

void idetest_init(void)
{
	int			rc;

#ifdef DEBUG_FUNCS
	printk("idetest_init()\n");
#endif

	/* Register character device */
	if (register_chrdev(IDETEST_MAJOR, IDETEST_NAME, &idetest_fops) < 0) {
		printk(KERN_WARNING IDETEST_NAME": failed to allocate major %d\n",
			IDETEST_MAJOR);
		return;
	}

	idetest_isopen = 0;
#if 0
	idetest_gotint = 0;
#endif

	printk(IDETEST_NAME": Copyright (C) 2001, Lineo (www.lineo.com)\n");

#if 0
	/* Install interrupt handler */
	mcf_autovector(IDETEST_IRQ);
	rc = request_irq(IDETEST_IRQ, idetest_isr,
		SA_INTERRUPT, IDETEST_NAME, NULL);
	if (rc)
		printk(IDETEST_NAME": IRQ %d already in use\n", IDETEST_IRQ);
#endif
}

/*****************************************************************************/
#ifdef MODULE

int init_module(void)
{
	idetest_init();
	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(IDETEST_MAJOR, IDETEST_NAME);
}

#endif /* MODULE */
/*****************************************************************************/
