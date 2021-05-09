/*
 *  Dallas Semiconductor DS1743/DS1743P Y2KC Nonvolatile Timekeeping RAM
 *  device driver.
 *
 *  Copyright (C) 2002 SED Systems, a Division of Calian Ltd.
 *      <hamilton@sedsystems.ca>. Operation based on linux 2.4.18 nvram driver.
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
#ifndef __DS1743_H__
#define __DS1743_H__
#ifdef __KERNEL__

#include <linux/config.h>
#if defined(CONFIG_DS1743)
extern int ds1743_init(void);

struct ds1743_memmap
{
	size_t filesize;
	unsigned char century;
	unsigned char seconds;
	unsigned char minutes;
	unsigned char hours;
	unsigned char weekday;
	unsigned char date;
	unsigned char month;
	unsigned char year;
};
extern void ds1743_gettod(int *yearp, int *monp, int *dayp, int *hourp, int *minp,
		int *secp);
extern int ds1743_set_clock_mmss(unsigned long nowtime);

#else
static __inline__ int ds1743_init(void)
{
	return(0);
}
#endif

#endif
#endif
