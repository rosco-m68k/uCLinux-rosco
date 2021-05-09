/*
 *  Dallas Semiconductor DS1743/DS1743P Y2KC Nonvolatile Timekeeping RAM
 *  device driver.
 *
 *  Copyright (C) 2002 SED Systems, a Division of Calian Ltd.
 *	<hamilton@sedsystems.ca>. Operation based on linux 2.4.18 nvram driver.
 *      Some code from the 2.4.18 driver was copied.
 *
 *  The data are supplied as a (seekable) character device, /dev/nvram. The
 *  size of the file is almost 2k (2k - 8bytes). The device uses the same
 *  minor number as the nvram device in the 2.4.18 kernel.
 *
 *  The driver does not use checksums. Instead, it relies on the battery flag.
 *  if the battery flag indicates the battery is bad, reads and writes fail.
 *  The clear nvram ioctl can be issued and clears the nvram. The NVRAM_SETCKS
 *  ioctl can also be issued but does nothing.
 */

#include <linux/config.h>
#include <linux/proc_fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/mc146818rtc.h> /* I use one of its structures and IOCTLs */
#include <linux/nvram.h>
#include <linux/ds1743.h>
#include <linux/time.h>

#if 0
#define PRINTK(expr, args...) printk(expr, ## args)
#else
#define PRINTK(expr, args...)
#endif

/*
 * The compiler or linker needs to tell this code where the nvram is and how
 * big it is. (If this is a DS1743 the size should be 2k but other chips may be
 * able to use this driver with a different size.) The address of _start_nvram
 * should be address of the first byte of the NVRAM. The address of _end_nvram
 * should be the address after the last byte of the device. &_end_nvram -
 * &_start_nvram is the size of the device. To look at how I have the linker
 * set these values for my platform look at
 * arch/m68knommu/platform/68360/sed_sios_master/ram.ld.
 */
extern const unsigned long _start_nvram;
extern const unsigned long _end_nvram;

static int nvram_read(struct inode *, struct file *, char *, int);
static int nvram_write(struct inode *, struct file *, char const *, int);
static void nvram_release(struct inode *, struct file *);
static int nvram_ioctl(struct inode *, struct file *, unsigned int,
		unsigned long);
static int nvram_open(struct inode *, struct file *);
static int nvram_lseek(struct inode *, struct file *, off_t, int);
static struct file_operations nvram_fops =
{
	lseek:		nvram_lseek,
	read:		nvram_read,
	write:		nvram_write,
	ioctl:		nvram_ioctl,
	open:		nvram_open,
	release:	nvram_release,
};

static struct miscdevice nvram_dev =
{
	NVRAM_MINOR,
	"ds174x_nvram",
	&nvram_fops
};

static int rtc_ioctl(struct inode *, struct file *, unsigned int,
		unsigned long);
static int rtc_lseek(struct inode *, struct file *, off_t, int);
static struct file_operations rtc_fops =
{
	lseek:		rtc_lseek,
	ioctl:		rtc_ioctl,
};
static struct miscdevice rtc_dev =
{
	RTC_MINOR,
	"ds174x_rtc",
	&rtc_fops
};


static int nvram_open_cnt;	/* number times opened */
static int nvram_open_mode;	/* special open modes. */
const static int NVRAM_WRITE = O_WRONLY; /* opened for writing (exclusive) */
const static int NVRAM_EXCL  = O_EXCL;   /* opened with O_EXCL */

static const char DRIVER_VERSION[] = "0.01";

static const unsigned char BATTERY_FLAG = 0x01 << 7;
static const unsigned char READ_FLAG = 0x01 << 6;
static const unsigned char WRITE_FLAG = 0x01 << 7;
static const unsigned char STOP_OSCILLATOR = 0x01 << 7;

static struct ds1743_memmap volatile *ds1743 = NULL;

static int check_battery(void)
{
	struct ds1743_memmap *ds1743 =
		(void *)&_end_nvram - sizeof(struct ds1743_memmap);

	if((ds1743->weekday & BATTERY_FLAG) == 0)
	{
		printk(KERN_WARNING "DS1743: battery low, time and NVRAM "
			       "contents are questionable.\n");
		return(-1);
	}
	else
	{
		return(0);
	}
}

static __inline__ size_t nvram_size(void)
{
	return(&_end_nvram - &_start_nvram - sizeof(struct ds1743_memmap));
}

/*
 *  Convert a binary number to BCD form.
 *  Assumption: The value passed in is less then 100
 */
static __inline__ unsigned char binary_to_bcd8(unsigned char i)
{
	unsigned char retval = 0;
	retval = i / 10;
	retval = retval << 4;
	retval |= i % 10;
	return(retval);
}

static __inline__ unsigned char bcd_to_binary8(unsigned char i)
{
	unsigned char retval = 0;
	retval = 0xf0 & i;
	retval = retval >> 4;
	retval *= 10;
	retval += (i & 0x0f);
	return(retval);
}

int ds1743_init(void)
{
	int ret;

	ds1743 = (void *)&_end_nvram - sizeof(struct ds1743_memmap);

	ret = misc_register( &nvram_dev );
	if(ret)
	{
		printk(KERN_ERR "DS174x: can't misc_register on minor=%d\n",
				nvram_dev.minor);
		return(ret);
	}

	ret = misc_register( &rtc_dev );
	if(ret)
	{
		printk(KERN_ERR "DS174x: can't misc_register on minor=%d\n",
				rtc_dev.minor);
		misc_deregister( &nvram_dev );
		return(ret);
	}

	nvram_open_cnt = 0;
	nvram_open_mode = 0;
	printk(KERN_INFO "DS1743 Clock/Non-volatile memory driver v%s\n",
			DRIVER_VERSION);
	check_battery();

	/*
	 *  If the filesize is larger then the device it must not have been
	 *  initialized so initialize it.
	 */
	if(ds1743->filesize > nvram_size())
		ds1743->filesize = 0;
	return(0);
}

void ds1743_cleanup(void)
{
	misc_deregister( &nvram_dev );
	misc_deregister( &rtc_dev );
}

static int nvram_lseek(struct inode *inode, struct file *filp, off_t offset,
		int mode)
{
	off_t newpos = 0;

	if(check_battery())
		return(-EIO);

	switch(mode)
	{
		case 0: /* Absolute seek on device */
			newpos = offset;
			break;

		case 1: /* Seek relative to current posistion */
			newpos = filp->f_pos + offset;
			break;
			
		case 2: /* Seek relative to end of file */
			newpos = ds1743->filesize + offset;
			break;
	}
	if((newpos < 0) || (newpos >= nvram_size()))
		return(-EINVAL);
	filp->f_pos = newpos;
	return(newpos);
}


static int nvram_read(struct inode *inode, struct file *filp, char *buf,
		int count)
{
	unsigned char *nvram = (unsigned char *)&_start_nvram;
	int amount;

	if(check_battery())
		return(-EIO);

	amount = (filp->f_pos + count) > ds1743->filesize ?
		ds1743->filesize - filp->f_pos : count;

	if(amount == 0)
		return(0);
	memcpy_tofs(buf, &nvram[filp->f_pos], amount);

	filp->f_pos += amount;
	return(amount);
}

static int nvram_write(struct inode *inode, struct file *filp, char const *buf,
		int count)
{
	unsigned char *nvram = (unsigned char *)&_start_nvram;
	int amount;

	if(check_battery())
		return(-EIO);

	if(count == 0)
		return(0);

	amount = (filp->f_pos + count) > nvram_size() ?
		nvram_size() - filp->f_pos : count;

	if(count == 0)
		return(-ENOSPC);
	memcpy_fromfs(&nvram[filp->f_pos], buf, amount);
	filp->f_pos += amount;
	
	if(filp->f_pos > ds1743->filesize)
		ds1743->filesize = filp->f_pos;
	return(amount);
}
static int nvram_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	if(check_battery())
		return(-EIO);

	switch(cmd)
	{
		case NVRAM_INIT:
			ds1743->filesize = 0;
			return(0);

		case NVRAM_SETCKS:
			return(0);

		default:
			return(-ENOTTY);
	}
}

static int nvram_open(struct inode *inode, struct file *filp)
{
	if(((nvram_open_cnt) && (filp->f_flags & O_EXCL)) ||
	   (nvram_open_mode & NVRAM_EXCL) ||
	   ((filp->f_mode & O_WRONLY) && (nvram_open_mode & NVRAM_WRITE)))
		return(-EBUSY);

	if(filp->f_flags & O_EXCL)
		nvram_open_mode |= NVRAM_EXCL;
	if(filp->f_mode & O_WRONLY)
		nvram_open_mode |= NVRAM_WRITE;

	if((filp->f_mode & O_WRONLY) && (filp->f_mode & O_TRUNC))
		ds1743->filesize = 0;

	if((filp->f_mode & O_WRONLY) && (filp->f_mode & O_APPEND))
		filp->f_pos = ds1743->filesize;
	else
		filp->f_pos = 0;
	++nvram_open_cnt;
	return(0);
}

static void nvram_release(struct inode *inode, struct file *filp)
{
	if(filp->f_flags & O_EXCL)
		nvram_open_mode &= ~NVRAM_EXCL;
	if(filp->f_mode & O_WRONLY)
		nvram_open_mode &= ~NVRAM_WRITE;
	--nvram_open_cnt;
}

void ds1743_gettod(int *yearp, int *monp, int *dayp, int *hourp, int *minp,
		int *secp)
{
	int century, year, month, day, hour, min, sec;
	unsigned long flags;

	ds1743 = (void *)&_end_nvram - sizeof(struct ds1743_memmap);

	save_flags(flags);cli();

	ds1743->century |= READ_FLAG;

	century = bcd_to_binary8(ds1743->century & (~(READ_FLAG | WRITE_FLAG)));
	year = bcd_to_binary8(ds1743->year);
	/*
	 *  Kernel adds 1900 or 2000 to year
	 *  year += century * 100;
	 */

	sec = bcd_to_binary8(ds1743->seconds & (~STOP_OSCILLATOR));

	min = bcd_to_binary8(ds1743->minutes & 0x7f);

	hour = bcd_to_binary8(ds1743->hours & 0x3f);

	month = bcd_to_binary8(ds1743->month & 0x1f);
	/*
	 *  Kernel mktime assumes monthes are 01-12
	 *  month = month - 1;
	 */

	day = bcd_to_binary8(ds1743->date & 0x3f);

	check_battery();

	ds1743->century &= (~READ_FLAG);

	if((year + century * 100) < 1970)
	{
		printk(KERN_WARNING "DS1743: Setting time of day and"
			       " re-reading time\n");
		ds1743_set_clock_mmss(1);
		ds1743_gettod(yearp, monp, dayp, hourp, minp, secp);
	}
	else
	{
		*yearp = year;
		*monp = month;
		*dayp = day;
		*hourp = hour;
		*minp = min;
		*secp = sec;
	}
	PRINTK("DS1743: Date: y:%d, m:%d, d:%d; Time: %d:%d:%d\n",
			*yearp, *monp, *dayp, *hourp, *minp, *secp);
	restore_flags(flags);
}


/*
 * I need to convert time in seconds from 1970 to year/month/day/hour/min/sec
 * for the DS1743. Other things need it, they can use it.
 * This was copied from uClibc which was:
 *      This is adapted from glibc
 *      Copyright (C) 1991, 1993 Free Software Foundation, Inc
 */

#define SECS_PER_HOUR 3600L
#define SECS_PER_DAY  86400L

static inline int __isleap(int year)
{
	if((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
		return(1);
	else
		return(0);
}

static const unsigned short int __mon_lengths[2][12] = {
	/* Normal years.  */
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	/* Leap years.  */
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

void tm_conv(struct rtc_time *tmbuf, time_t *t)
{
	long days, rem;
	register int y;
	register const unsigned short int *ip;

	days = *t / SECS_PER_DAY;
	rem = *t % SECS_PER_DAY;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmbuf->tm_hour = rem / SECS_PER_HOUR;
	rem %= SECS_PER_HOUR;
	tmbuf->tm_min = rem / 60;
	tmbuf->tm_sec = rem % 60;
	/* January 1, 1970 was a Thursday.  */
	tmbuf->tm_wday = (4 + days) % 7;
	if (tmbuf->tm_wday < 0)
		tmbuf->tm_wday += 7;
	y = 1970;
	while (days >= (rem = __isleap(y) ? 366 : 365)) {
		++y;
		days -= rem;
	}
	while (days < 0) {
		--y;
		days += __isleap(y) ? 366 : 365;
	}
	tmbuf->tm_year = y - 1900;
	tmbuf->tm_yday = days;
	ip = __mon_lengths[__isleap(y)];
	for (y = 0; days >= ip[y]; ++y)
		days -= ip[y];
	tmbuf->tm_mon = y;
	tmbuf->tm_mday = days + 1;
}

int ds1743_set_clock_mmss(unsigned long nowtime)
{
	struct rtc_time time;
	struct ds1743_memmap image;
	unsigned long flags;

	PRINTK("DS1743: Setting clock to %ld\n", nowtime);

	tm_conv(&time, &nowtime);
	ds1743 = (void *)&_end_nvram - sizeof(struct ds1743_memmap);

	image.century = (time.tm_year + 1900)/100;
	image.century = (binary_to_bcd8(image.century)) & (~READ_FLAG);
	image.century |= WRITE_FLAG;

	image.seconds = (binary_to_bcd8(time.tm_sec)) & (~STOP_OSCILLATOR);
	image.minutes = binary_to_bcd8(time.tm_min);
	image.hours = binary_to_bcd8(time.tm_hour);
	image.date = binary_to_bcd8(time.tm_mday);
	image.month = binary_to_bcd8(time.tm_mon + 1);
	image.weekday = binary_to_bcd8(time.tm_wday + 1);
	image.year = binary_to_bcd8(time.tm_year % 100);

	save_flags(flags);cli();
	ds1743->century |= WRITE_FLAG;
	ds1743->seconds = image.seconds;
	ds1743->minutes = image.minutes;
	ds1743->hours = image.hours;
	ds1743->date = image.date;
	ds1743->month = image.month;
	ds1743->weekday = image.weekday;
	ds1743->year = image.year;
	ds1743->century = image.century;
	image.century &= (~WRITE_FLAG);
	ds1743->century = image.century;
	PRINTK("DS1743 Image: Date: c:0x%x y:0x%x, m:0x%x, d:0x%x;"
			"Time: 0x%x:0x%x:0x%x\n",
			image.century, image.year, image.month,
		       	image.date,
		       	image.hours, image.minutes, image.seconds);
	PRINTK("DS1743: Date: c:0x%x y:0x%x, m:0x%x, d:0x%x;"
			"Time: 0x%x:0x%x:0x%x\n",
			ds1743->century, ds1743->year, ds1743->month,
		       	ds1743->date,
		       	ds1743->hours, ds1743->minutes, ds1743->seconds);
	restore_flags(flags);
	return(0);
}

static int rtc_lseek(struct inode *i, struct file *f, off_t t, int j)
{
	return(-ESPIPE);
}

static int rtc_ioctl(struct inode *inode, struct file *filp, unsigned int op,
		unsigned long arg)
{
	int retval = -ENOTTY;
	switch(op)
	{
		case RTC_SET_TIME:
			if(!suser())
				return(-EACCES);

			if(arg)
			{
				/*
				 * TODO: allow the user to pass in a pointer to
				 * a structure to set the time of data.
				 */
				return(-ENOTTY);
			}
			else
			{
				/*
				 * KGH: my extension, if the user passes in NULL
				 * set the rtc to the system time.
				 */
				return(ds1743_set_clock_mmss(xtime.tv_sec));
			}
			break;
	}
	return(retval);
}
