/*****************************************************************************/

/*
 *	ds1302.c -- driver for Dallas Semiconductor DS1302 Real Time Clock.
 *
 * 	(C) Copyright 2001, Greg Ungerer (gerg@snapgear.com)
 */

/*****************************************************************************/

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/wait.h>
#include <asm/param.h>
#include <asm/coldfire.h>
#include <asm/mcfsim.h>

/*****************************************************************************/

/*
 *	Same major/minor used by x86 type RTC.
 */
#define	DS1302_MAJOR	10
#define	DS1302_MINOR	135

/*
 *	Size of RTC region. 32 bytes of calender and 32 bytes RAM.
 *	Not all addresses are valid in this range...
 */
#define	DS1302_MSIZE	0x3f

/*****************************************************************************/

/*
 *	DS1302 defines.
 */
#define	RTC_CMD_READ	0x81		/* Read command */
#define	RTC_CMD_WRITE	0x80		/* Write command */

#define	RTC_ADDR_YEAR	0x06		/* Address of year register */
#define	RTC_ADDR_DAY	0x05		/* Address of day of week register */
#define	RTC_ADDR_MON	0x04		/* Address of month register */
#define	RTC_ADDR_DATE	0x03		/* Address of day of month register */
#define	RTC_ADDR_HOUR	0x02		/* Address of hour register */
#define	RTC_ADDR_MIN	0x01		/* Address of minute register */
#define	RTC_ADDR_SEC	0x00		/* Address of second register */

/*****************************************************************************/

/*
 *	Architecture specific code. Accessing the DS1302 will depend
 *	on exactly how it is wried up. This section specifically coded
 *	to match the Gilbarco/NAP board based on a 5272 ColdFire CPU.
 */
static volatile unsigned short	*ds1302_dirp =
	(volatile unsigned short *) (MCF_MBAR + MCFSIM_PADDR);
static volatile unsigned short	*ds1302_dp =
	(volatile unsigned short *) (MCF_MBAR + MCFSIM_PADAT);

#define	RTC_RESET	0x0020		/* PA5 of M5272 */
#define	RTC_IODATA	0x0040		/* PA6 of M5272 */
#define	RTC_SCLK	0x0080		/* PA7 of M5272 */

void ds1302_sendbits(unsigned int val)
{
	int	i;

	for (i = 8; (i); i--, val >>= 1) {
		*ds1302_dp = (*ds1302_dp & ~RTC_IODATA) |
			 ((val & 0x1) ? RTC_IODATA : 0);
		*ds1302_dp |= RTC_SCLK;
		*ds1302_dp &= ~RTC_SCLK;
	}
}

unsigned int ds1302_recvbits(void)
{
	unsigned int	val;
	int		i;

	for (i = 0, val = 0; (i < 8); i++) {
		*ds1302_dp |= RTC_SCLK;
		val |= (((*ds1302_dp & RTC_IODATA) ? 1 : 0) << i);
		*ds1302_dp &= ~RTC_SCLK;
	}
	return(val);
}

unsigned int ds1302_readbyte(unsigned int addr)
{
	unsigned int	val;
	unsigned long	flags;

#if 0
	printk("ds1302_readbyte(addr=%x)\n", addr);
#endif

	save_flags(flags); cli();
	*ds1302_dirp |= RTC_RESET | RTC_IODATA | RTC_SCLK;
	*ds1302_dp &= ~(RTC_RESET | RTC_IODATA | RTC_SCLK);

	*ds1302_dp |= RTC_RESET;
	ds1302_sendbits(((addr & 0x3f) << 1) | RTC_CMD_READ);
	*ds1302_dirp &= ~RTC_IODATA;
	val = ds1302_recvbits();
	*ds1302_dp &= ~RTC_RESET;
	restore_flags(flags);

	return(val);
}

void ds1302_writebyte(unsigned int addr, unsigned int val)
{
	unsigned long	flags;

#if 0
	printk("ds1302_writebyte(addr=%x)\n", addr);
#endif

	save_flags(flags); cli();
	*ds1302_dirp |= RTC_RESET | RTC_IODATA | RTC_SCLK;
	*ds1302_dp &= ~(RTC_RESET | RTC_IODATA | RTC_SCLK);

	*ds1302_dp |= RTC_RESET;
	ds1302_sendbits(((addr & 0x3f) << 1) | RTC_CMD_WRITE);
	ds1302_sendbits(val);
	*ds1302_dp &= ~RTC_RESET;
	restore_flags(flags);
}

void ds1302_reset(void)
{
	/* Hardware dependant reset/init */
	*ds1302_dirp |= RTC_RESET | RTC_IODATA | RTC_SCLK;
	*ds1302_dp &= ~(RTC_RESET | RTC_IODATA | RTC_SCLK);
}

/*****************************************************************************/

int __inline__ bcd2int(int val)
{
	return((((val & 0xf0) >> 4) * 10) + (val & 0xf));
}

/*****************************************************************************/

/*
 *	This is called direct from the kernel timer handling code.
 *	It gets the current time from the RTC.
 */

void ds1302_gettod(int *year, int *mon, int *day, int *hour, int *min, int *sec)
{
#if 0
	printk("ds1302_gettod()\n");
#endif

	*year = bcd2int(ds1302_readbyte(RTC_ADDR_YEAR));
	*mon = bcd2int(ds1302_readbyte(RTC_ADDR_MON));
	*day = bcd2int(ds1302_readbyte(RTC_ADDR_DATE));
	*hour = bcd2int(ds1302_readbyte(RTC_ADDR_HOUR));
	*min = bcd2int(ds1302_readbyte(RTC_ADDR_MIN));
	*sec = bcd2int(ds1302_readbyte(RTC_ADDR_SEC));
}

/*****************************************************************************/

/*
 *	This is called direct from the kernel timer handling code.
 *	It is supposed to synchronize the kernel clock to the RTC.
 */

int ds1302_set_clock_mmss(unsigned long nowtime)
{
	unsigned long	m, s;

#if 1
	printk("ds1302_set_clock_mmss(nowtime=%d)\n", nowtime);
#endif

	/* FIXME: not implemented yet... */
	s = nowtime % 60;
	m = nowtime / 60;
	return(0);
}

/*****************************************************************************/

static int ds1302_read(struct inode *ip, struct file *fp, char *buf, int count)
{
	int	total;

#if 0
	printk("ds1302_read(buf=%x,count=%d)\n", (int) buf, count);
#endif

	if (fp->f_pos >= DS1302_MSIZE)
		return(0);
	if (count > (DS1302_MSIZE - fp->f_pos))
		count = DS1302_MSIZE - fp->f_pos;

	for (total = 0; (total < count); total++)
		put_user(ds1302_readbyte(fp->f_pos + total), buf++);

	fp->f_pos += total;
	return(total);
}

/*****************************************************************************/

int ds1302_write(struct inode *inode, struct file *fp, const char *buf, int count)
{
	int	total;

#if 0
	printk("ds1302_write(buf=%x,count=%d)\n", (int) buf, count);
#endif

	if (fp->f_pos >= DS1302_MSIZE)
		return(0);
	if (count > (DS1302_MSIZE - fp->f_pos))
		count = DS1302_MSIZE - fp->f_pos;

	for (total = 0; (total < count); total++)
		ds1302_writebyte((fp->f_pos + total), get_user(buf++));

	fp->f_pos += total;
	return(total);
}

/*****************************************************************************/

/*
 *	Exported file operations structure for driver...
 */

struct file_operations	ds1302_fops = {
	read:	ds1302_read,
	write:	ds1302_write,
};

/*****************************************************************************/

void ds1302_init(void)
{
	int	rc;

	if ((rc = register_chrdev(DS1302_MAJOR, "ds1302", &ds1302_fops)) < 0) {
		printk(KERN_WARNING "DS1302: can't get major %d\n",
			DS1302_MAJOR);
		return;
	}

	printk ("DS1302: Copyright (C) 2001, Greg Ungerer "
		"(gerg@snapgear.com)\n");

	ds1302_reset();
}

/*****************************************************************************/
