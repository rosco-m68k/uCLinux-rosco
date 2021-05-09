/* flash.c,v 1.2 2001/01/11 03:56:48 gerg Exp
 *
 * Flash-memory char or block device
 *
 * Copyright (c) 1998, 1999, 2000 Axis Communications AB
 *
 * Authors:  Bjorn Wesen (bjornw@axis.com)
 *
 * This driver implements a read/write/erase interface to the flash chips.
 * It probes for chips, and tries to read a bootblock with partition info from
 * the bootblock on the first chip. This bootblock is given the device name
 * /dev/flash0, and the rest of the partitions are given flash1 and upwards.
 * If no bootblock is found, the entire first chip (including bootblock) is
 * set as /dev/flash1.
 *
 * TODO:
 *   implement Unlock Bypass programming, to double the write speed
 *   fix various loops for partitions than span more than one chip
 *   take advantage of newer chips with 2 kword sector erase sizes
 *
 * flash.c,v
 * Revision 1.2  2001/01/11 03:56:48  gerg
 * Imported 20000811 release of JFFS.
 *
 * Revision 1.2  2000/10/06 08:36:02  davidm
 *
 * Changes to get the flash driver used by jffs working with the toshiba
 * flash in a NETtel with 4Mb.
 *
 * This code based on elinux_20000801.tgz.
 *
 * Revision 1.52  2000/07/31 18:32:15  finn
 * flash_ioctl: Made FLASHIO_ERASEALL work better.
 *
 * Revision 1.51  2000/07/25 08:38:13  finn
 * Uncommented CONFIG_USE_FLASH_PARTITION_TABLE. Some cleanup.
 *
 * Revision 1.50  2000/07/25 00:24:37  danielj
 * Fixed so that a partition that start after the end of chip 0 starts in
 * chip 1.
 *
 * Revision 1.49  2000/07/18 12:21:51  finn
 * Whitespace police.
 *
 * Revision 1.48  2000/07/14 16:26:42  johana
 * Flash partitiontable in flash seem to works but is not enabled.
 * Local CONFIG_USE_FLASH_PARTITION_TABLE can be defined to enabe it.
 * Maybe we should have a better magic (not 0xbeef) and a better checksum?
 *
 */

/* define to make kernel bigger, but with verbose Flash messages */

//#define LISAHACK

/* if IRQ_LOCKS is enabled, interrupts are disabled while doing things
 * with the flash that cant have other accesses to the flash at the same
 * time. this is normally not needed since we have a wait_queue lock on the
 * flash chip as a mutex.
 */

#include <linux/config.h>
#include <linux/major.h>
#include <linux/malloc.h>

//#define IRQ_LOCKS

//#define FLASH_VERBOSE

#ifdef CONFIG_SED_SIOS
#define FLASH_8BITX4    //Using 4- 8 bit flash devices in parallel
#else
#ifndef	CONFIG_FLASH_DOUBLEWIDE
#define FLASH_16BIT
#endif
#endif

#ifdef FLASH_8BITX4
  #define 	FL_SIZEOF(x)    	((x) << 2)
  #define 	FLASH_WIDTH 4
  #define 	FLASH_ERASE_STATE	(0xFFFFFFFFU)
#elifdef FLASH_16BIT
  #define	FL_SIZEOF(x)		(x)
  #define 	FLASH_ERASE_STATE	(0xFFFFU)
  #define 	FLASH_WIDTH 2
#else
  #define	FL_SIZEOF(x)		((x) << 1)
  #define 	FLASH_ERASE_STATE	(0xFFFFFFFFU)
  #define 	FLASH_WIDTH 		2
#endif

/* these times are taken from AMD flash but should be pretty close*/
/* we _could_ use CFI routines to poll, but it gets messy and big */
#define NOM_BYTE_PROGRAM_TIME_uS	(5)	/* nominal program time is 5 micros */
#define MAX_BYTE_PROGRAM_TIME_uS	(200)	/* max program time is 150 micros */

/* since the sector erase algorithm may have multiple blocks we can't timeout until the whole chip time has passed */
#define SECTOR_ERASE_TIMEOUT_JIFS	(45*HZ)	/* max erase time (whole chip) is 45 seconds */

/* This should perhaps be in kernal config: ? */
/* Define it locally to test it - maybe we need a better checksum?
 * (it must work both with and without aptable defined and with a
 * trashed flash)
 */
#define CONFIG_USE_FLASH_PARTITION_TABLE

/* ROM and flash devices have major 31 (Documentation/devices.txt)
 * However, KROM is using that major now, so we use an experimental major = 60.
 * When this driver works, it will support read-only memory as well and we can
 * remove KROM.
 * Our minors start at: 16 = /dev/flash0       First flash memory card (rw)
 */
#ifdef CONFIG_SED_SIOS
#define MAJOR_NR 31
#else
#ifdef CONFIG_MWI
#define MAJOR_NR 6
#else
#define MAJOR_NR 60
#endif
#endif

#define ROM_MINOR   8   /* ro ROM card */
#define FLASH_MINOR 16  /* rw Flash card */

/* Don't touch the MAX_CHIPS until the hardcoded chip0 and chip1 are
   removed from the code.  */

#ifdef CONFIG_COLDFIRE
#define MAX_CHIPS 1
#elif CONFIG_SED_SIOS
#define MAX_CHIPS 2
#elif CONFIG_ARCH_NETARM
#define MAX_CHIPS 1
#else
#define MAX_CHIPS 2
#endif
#define MAX_PARTITIONS 8

#ifdef	CONFIG_ARCH_NETARM

#ifdef FLASH_16BIT
#define DEF_FLASH2_SIZE 0x300000   /* leave 1M for OS - the rest JFFS */
#else
#define DEF_FLASH2_SIZE 0x700000   /* leave 1M for OS - the rest JFFS */
#endif

#include <asm-armnommu/arch-netarm/netarm_mem_module.h>

#else	/* !CONFIG_ARCH_NETARM */

#define DEF_FLASH2_SIZE 0x50000   /* 5 sectors for JFFS partition per default */

#endif	/* else !CONFIG_ARCH_NETARM */

/* all this stuff needs to go in blk.h */
#ifndef CONFIG_SED_SIOS
#define DEVICE_NAME "Flash/ROM device"
#endif

#ifdef CONFIG_BLK_DEV_FLASH

#define FLASH_SECTSIZE 512 /* arbitrary, but Linux assumes 512 often it seems */
#ifndef CONFIG_SED_SIOS
#define DEVICE_REQUEST do_flash_request
#endif
#define DEVICE_NR(device) (MINOR(device))
#define DEVICE_ON(device)
#define DEVICE_OFF(device)

#include <linux/blk.h>

/* the device and block sizes for each minor */

static int flash_blk_sizes[32];

#endif

static int flash_sizes[32];

#include <asm/system.h>
#ifdef CONFIG_SVINTO
#include <asm/svinto.h>
#endif

#include <linux/flash.h>
#if defined(CONFIG_SED_SIOS) || defined(CONFIG_M68332)
#include<asm/delay.h>
#endif

#ifndef MEM_NON_CACHEABLE
#define MEM_NON_CACHEABLE 0
#endif

#ifndef MEM_DRAM_START
#define MEM_DRAM_START 0
#endif

// #define DEBUG

#ifdef DEBUG
#define FDEBUG(x) x
#else
#define FDEBUG(x)
#endif

#if defined(__CRIS__) && !defined(DEBUG)
/* breaks badly if we use printk before we flash an image... */
extern void console_print_etrax(const char *b, ...);
#define safe_printk console_print_etrax
#else
#define safe_printk printk
#endif

int flash_write(unsigned char *ptr, const unsigned char *source, unsigned int size);
void flash_init_erase(unsigned char *ptr, unsigned int size);
void flash_finalize_erase(unsigned char *ptr, unsigned int size);
static struct flashchip *getchip(unsigned char *ptr);
static void flash_init_partitions(void);

#ifdef FLASH_8BITX4     //4 8 bit flash devices in parallel
enum {
    unlockAddress1          = 0xAAA,
    unlockData1             = 0xAAAAAAAA,
    unlockAddress2          = 0x555,
    unlockData2             = 0x55555555,
    manufacturerUnlockData  = 0x90909090,
    manufacturerAddress     = 0x00,
    deviceIdAddress         = 0x02,
    programUnlockData       = 0xA0A0A0A0,
    resetData               = 0xF0F0F0F0,
    sectorEraseUnlockData   = 0x80808080,
    sectorEraseUnlockData2  = 0x30303030 };

typedef volatile unsigned long *flashptr;
enum {
	D5_MASK           = 0x20202020,
	D6_MASK           = 0x40404040,
	D7_MASK           = 0x80808080
};
#elifdef FLASH_16BIT
/* 16-bit wide Flash-ROM */
enum {  unlockAddress1          = 0x555,
        unlockData1             = 0xAAAA,
        unlockAddress2          = 0x2AA,
        unlockData2             = 0x5555,
        manufacturerUnlockData  = 0x9090,
        manufacturerAddress     = 0x00,
        deviceIdAddress         = 0x01,
        programUnlockData       = 0xA0A0,
        resetData               = 0xF0F0,
        sectorEraseUnlockData   = 0x8080,
        sectorEraseUnlockData2  = 0x3030 };

typedef unsigned volatile short *flashptr;
enum {
	D5_MASK           = 0x2020,
	D6_MASK           = 0x4040,
	D7_MASK           = 0x8080
};
#else
/* 32-bit wide Flash-ROM
 * Since we have two devices in parallell, we need to duplicate
 * the special _data_ below so it reaches both chips.
 */
enum {  unlockAddress1          = 0x555,
        unlockData1             = 0x00AA00AA,
        unlockAddress2          = 0x2AA,
        unlockData2             = 0x00550055,
        manufacturerUnlockData  = 0x00900090,
        manufacturerAddress     = 0x00,
        deviceIdAddress         = 0x01,
        programUnlockData       = 0x00A000A0,
        resetData               = 0x00F000F0,
        sectorEraseUnlockData   = 0x00800080,
        sectorEraseUnlockData2  = 0x00300030 };

typedef volatile unsigned long *flashptr;
enum {
	D5_MASK           = 0x2020,
	D6_MASK           = 0x4040,
	D7_MASK           = 0x8080
};
#endif

enum {  ManufacturerAMD   = 0x01,
    AM29LV400BB       = 0x22BA,
	AM29F800BB        = 0x2258,
	AM29F800BT        = 0x22D6,
	AM29LV800BB       = 0x225B,
	AM29LV800BT       = 0x22DA,
	AM29LV160BT       = 0x22C4,
	AM29LV160BB       = 0x2249,
	AM29LV322DT       = 0x2255,
	AM29LV322DB       = 0x2256,
	AM29LV323DT       = 0x2250,
	AM29LV323DB       = 0x2253
};

enum {  ManufacturerToshiba = 0x0098,
	TC58FVT160          = 0x00c2,
	TC58FVB160          = 0x0043
};

enum {
	/* ST - www.st.com */
	ManufacturerST    = 0x0020,
	M29W800T          = 0x00D7 /* Used in 5600, similar to AM29LV800,
				    * but no unlock bypass */
};

enum {
	/* SST*/
	ManufacturerSST   = 0x00BF,
	SST39LF800        = 0x2781,
	SST39LF160        = 0x2782
};


enum {  maxNumberOfBootSectors = 8 };

/* internal structure for keeping track of flash devices */

struct flashchip {
	unsigned char *start;
	unsigned char *bootsector;
	unsigned int bootsectorsize[maxNumberOfBootSectors];
	unsigned short manufacturer_id;
	unsigned short device_id;
	unsigned short isValid;
	int size, sectorsize;
	/* to block accesses during erase and writing etc */
	int busy;
	struct wait_queue *wqueue;
};

/* and partitions */

struct flashpartition {
	struct flashchip *chip;   /* the chip we reside in */
	unsigned char *start;     /* starting flash mem address */
	int size;                 /* size of partition in bytes */
	unsigned int flags;       /* protection bits etc */
	__u16 type;               /* type of partition */
};


static struct flashchip chips[MAX_CHIPS];
static struct flashpartition partitions[MAX_PARTITIONS];

/* check if flash is busy */

static inline int
flash_is_busy(flashptr ptr)
{
	/* this should probably be protected! */
	return ((*ptr & D7_MASK) != (0xffffffff & D7_MASK));
}


static inline int
flash_pos_is_clean(unsigned char *ptr)
{
	ptr = (unsigned char *)((unsigned long)ptr | MEM_NON_CACHEABLE);
	return (*((__u32 *)ptr) == 0xffffffff);
}


/* Open the device. We don't need to do anything really.  */
static int
flash_open(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_CHR_DEV_FLASH

	int minor = MINOR(inode->i_rdev);
	struct flashpartition *part;

	if(minor < FLASH_MINOR)
		return -ENODEV;

	part = &partitions[minor - FLASH_MINOR];

	if(!part->start)
		return -ENODEV;

	filp->private_data = (void *)part;  /* remember for the future */

#endif

	return 0; /* everything went ok */
}

static void
flash_release(struct inode *inode, struct file *filp)
{
#ifdef CONFIG_BLK_DEV_FLASH
	sync_dev(inode->i_rdev);
#endif
	return;
}

#ifdef CONFIG_BLK_DEV_FLASH

static void
do_flash_request()
{
	while(1) {
		int minor, fsize, opsize;
		struct flashchip *chip;
		struct flashpartition *part;
		unsigned char *fptr;
		unsigned long flags;

		INIT_REQUEST;

		minor = DEVICE_NR(CURRENT_DEV);

		minor -= FLASH_MINOR;

		/* for now, just handle requests to the flash minors */

		if(minor < 0 || minor >= MAX_PARTITIONS ||
		   !partitions[minor].start) {
			printk(KERN_WARNING "flash: bad minor %d.", minor);
			end_request(0);
			continue;
		}

		part = partitions + minor;

		/* get the actual memory address of the sectors requested */

		fptr = part->start + CURRENT->sector * FLASH_SECTSIZE;
		fsize = CURRENT->current_nr_sectors * FLASH_SECTSIZE;

		/* check so it's not totally out of bounds */

		if(fptr + fsize > part->start + part->size) {
			printk(KERN_WARNING "flash: request past end "
			       "of partition\n");
			end_request(0);
			continue;
		}

		/* actually do something, but get a lock on the chip first.
		 * since the partition might span several chips, we need to
		 * loop and lock each chip in turn
		 */

		while(fsize > 0) {

			chip = getchip(fptr);

			/* how much fits in this chip ? */

			opsize = (fptr + fsize) > (chip->start + chip->size) ?
				 (chip->start + chip->size - fptr) : fsize;

			/* lock the chip */

			save_flags(flags);
			cli();
			while(chip->busy)
				sleep_on(&chip->wqueue);
			chip->busy = 1;
			restore_flags(flags);

			switch(CURRENT->cmd) {
			case READ:
				memcpy(CURRENT->buffer, fptr, opsize);
				FDEBUG(printk("flash read from %p to %p "
					      "size %d\n", fptr,
					      CURRENT->buffer, opsize));
				break;
			case WRITE:
				FDEBUG(printk("flash write block at 0x%p\n",
					      fptr));
				flash_write(fptr,
					    (unsigned char *)
					    CURRENT->buffer,
					    opsize);
				break;
			default:
				/* Shouldn't happen.  */
				chip->busy = 0;
				wake_up(&chip->wqueue);
				end_request(0);
				continue;
			}

			/* release the lock */

			chip->busy = 0;
			wake_up(&chip->wqueue);

			/* see if there is anything left to write in the next chip */

			fsize -= opsize;
			fptr += opsize;
		}

		/* We have a liftoff! */

		end_request(1);
	}
}

#endif /* CONFIG_BLK_DEV_FLASH */

void
flash_safe_acquire(void *part)
{
	struct flashchip *chip = ((struct flashpartition *)part)->chip;
	while (chip->busy)
		sleep_on(&chip->wqueue);
	chip->busy = 1;
}

void
flash_safe_release(void *part)
{
	struct flashchip *chip = ((struct flashpartition *)part)->chip;
	chip->busy = 0;
	wake_up(&chip->wqueue);
}

/* flash_safe_read and flash_safe_write are used by the JFFS flash filesystem */

int
flash_safe_read(void *_part, unsigned char *fptr,
		unsigned char *buf, int count)
{
	struct flashpartition *part = (struct flashpartition *)_part;

	/* Check so it's not totally out of bounds.  */

	if(fptr + count > part->start + part->size) {
		printk(KERN_WARNING "flash: read request past "
		       "end of device (address: 0x%p, size: %d)\n",
		       fptr, count);
		return -EINVAL;
	}

	FDEBUG(printk("flash_safe_read: %d bytes from 0x%p to 0x%p\n",
		      count, fptr, buf));

	/* Actually do something, but get a lock on the chip first.  */

	flash_safe_acquire(part);

	memcpy(buf, fptr, count);

	/* Release the lock.  */

	flash_safe_release(part);

	return count; /* success */
}


int
flash_safe_write(void *_part, unsigned char *fptr,
		 const unsigned char *buf, int count)
{
	struct flashpartition *part = (struct flashpartition *)_part;
	int err;

	/* Check so it's not totally out of bounds.  */

	if(fptr + count > part->start + part->size) {
		printk(KERN_WARNING "flash: write operation past "
		       "end of device (address: 0x%p, size: %d)\n",
		       fptr, count);
		return -EINVAL;
	}

	FDEBUG(printk("flash_safe_write: %d bytes from 0x%p to 0x%p\n",
		      count, buf, fptr));

	/* Actually do something, but get a lock on the chip first.  */

	flash_safe_acquire(part);

	if ((err = flash_write(fptr, buf, count)) < 0) {
		count = err;
	}

	/* Release the lock.  */

	flash_safe_release(part);

	return count; /* success */
}


#ifdef CONFIG_CHR_DEV_FLASH

static int
flash_char_read(struct inode *inode, struct file *filp,
		char *buf, int count)
{
	int rlen;
	struct flashpartition *part = (struct flashpartition *)filp->private_data;

	FDEBUG(printk("flash_char_read\n"));
	rlen = flash_safe_read(part,
			       (unsigned char *)part->start + filp->f_pos,
			       (unsigned char *)buf, count);

	/* advance file position pointer */

	if(rlen >= 0)
		filp->f_pos += rlen;

	return rlen;
}

static int
flash_char_write(struct inode *inode, struct file *filp,
		 const char *buf, int count)
{
	int wlen;
	struct flashpartition *part = (struct flashpartition *)filp->private_data;

	FDEBUG(printk("flash_char_write\n"));
	wlen = flash_safe_write(part,
				(unsigned char *)part->start + filp->f_pos,
				(unsigned char *)buf, count);

	/* advance file position pointer */

	if(wlen >= 0)
		filp->f_pos += wlen;

	return wlen;
}

#endif /* CONFIG_CHR_DEV_FLASH */


static int
flash_ioctl(struct inode *inode, struct file *file,
	    unsigned int cmd, unsigned long arg)
{
	int minor;
	struct flashpartition *part;
	struct flashchipinfo *finfo;

	if (!inode || !inode->i_rdev)
		return -EINVAL;

	minor = MINOR(inode->i_rdev);

	if(minor < FLASH_MINOR)
		return -EINVAL; /* only ioctl's for flash devices */

	part = &partitions[minor - FLASH_MINOR];

	switch(cmd) {
	case FLASHIO_CHIPINFO:

		if(!suser())
			return -EACCES;

		if(arg == 0)
			return -EINVAL;

		finfo = (struct flashchipinfo *)arg;

		/* TODO: verify arg */

		finfo->isValid = chips[0].isValid;
		finfo->manufacturer_id = chips[0].manufacturer_id;
		finfo->device_id = chips[0].device_id;
		finfo->size = chips[0].size;
		finfo->sectorsize = chips[0].sectorsize;

		return 0;

	case FLASHIO_ERASEALL:
		FDEBUG(printk("flash_ioctl(): Got FLASHIO_ERASEALL request.\n"));

		if(!part->start)
			return -EINVAL;

		if(!suser())
			return -EACCES;

		/* Invalidate all pages and buffers */

		invalidate_inodes(inode->i_rdev);
		invalidate_buffers(inode->i_rdev);

		/*
		 * Start the erasure, then sleep and wake up now and
		 * then to see if it's done. We use the waitqueue to
		 * make sure we don't start erasing in the middle of
		 * a write, or that nobody start using the flash while
		 * we're erasing.
		 *
		 * TODO: break up partition erases that spans more than one
		 *       chip.
		 */

		flash_safe_acquire(part);

		flash_init_erase(part->start, part->size);
#if 1
		flash_finalize_erase(part->start, part->size);
#else
		while (flash_is_busy(part->start)
		       || !flash_pos_is_clean(part->start)) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + HZ / 2;
			schedule();
		}
#endif

		flash_safe_release(part);

		return 0;

	default:
		return -EPERM;
	}

	return -EPERM;
}

/* probe for Flash RAM's - this isn't in the init function, because this needs
 * to be done really early in the boot, so we can use the device to burn an
 * image before the system is running.
 */

void
flash_probe()
{
	int i;

	/* start adresses for the Flash chips - these should really
	 * be settable in some other way.
	 */
#ifdef FLASH_VERBOSE
	safe_printk("Probing flash...\n");
#endif

#ifdef CONFIG_COLDFIRE
	chips[0].start = (unsigned char *)(0xf0200000);
#elif   CONFIG_SED_SIOS
    chips[0].start = (unsigned char *)(0x01400000);
    chips[1].start = (unsigned char *)(0x01600000);
#elif	CONFIG_ARCH_NETARM
	chips[0].start = (unsigned char *)(0x10000000);
#elif	CONFIG_MWI
	chips[0].start = (unsigned char *)(0x000000);
#else
	chips[0].start = (unsigned char *)(MEM_CSE0_START | MEM_NON_CACHEABLE);
	chips[1].start = (unsigned char *)(MEM_CSE1_START | MEM_NON_CACHEABLE);
#endif

	for(i = 0; i < MAX_CHIPS; i++) {
		struct flashchip *chip = chips + i;
		flashptr flashStart = (flashptr)chip->start;
#ifdef FLASH_8BITX4
        unsigned long manu_long;
#endif
		unsigned short manu;

#ifdef FLASH_VERBOSE
		safe_printk("Probing flash #%d at 0x%p\n", i, flashStart);
#endif

#ifdef CONFIG_SVINTO_SIM
		/* in the simulator, dont trash the flash ram by writing unlocks */
		chip->isValid = 1;
		chip->device_id = AM29LV160BT;
#else
		/* reset */

		flashStart[unlockAddress1] = unlockData1;
		flashStart[unlockAddress2] = unlockData2;
		flashStart[unlockAddress1] = resetData;

		/* read manufacturer */

		flashStart[unlockAddress1] = unlockData1;
		flashStart[unlockAddress2] = unlockData2;
		flashStart[unlockAddress1] = manufacturerUnlockData;
#ifdef FLASH_8BITX4
        manu_long = flashStart[manufacturerAddress];
        manu = manu_long & 0x000000ffU;
#else
		manu = flashStart[manufacturerAddress];
#endif
		chip->isValid = (manu == ManufacturerAMD ||
				 manu == ManufacturerToshiba ||
				 manu == ManufacturerST ||
				 manu == ManufacturerSST);
		chip->manufacturer_id = manu;

		if(!chip->isValid) {
#ifdef FLASH_VERBOSE
			safe_printk("Flash: No flash or unsupported "
				    "manufacturer.\n");
#endif
			continue;
		}
		/* reset */

		flashStart[unlockAddress1] = unlockData1;
		flashStart[unlockAddress2] = unlockData2;
		flashStart[unlockAddress1] = resetData;

		/* read device id */

		flashStart[unlockAddress1] = unlockData1;
		flashStart[unlockAddress2] = unlockData2;
		flashStart[unlockAddress1] = manufacturerUnlockData;
#ifdef FLASH_8BITX4
        manu_long = flashStart[deviceIdAddress];
        manu_long &= 0x000000ff;
        switch(chip->manufacturer_id)
        {
            case ManufacturerAMD:
                manu_long |= 0x00002200;
                break;
            case ManufacturerSST:
                manu_long |= 0x00002700;
                break;
            case ManufacturerToshiba:
            case ManufacturerST:
                break;
        }
        chip->device_id = manu_long;
#else
		chip->device_id = flashStart[deviceIdAddress];
#endif /*FLASH_8BITX4*/

		/* reset */

		flashStart[unlockAddress1] = unlockData1;
		flashStart[unlockAddress2] = unlockData2;
		flashStart[unlockAddress1] = resetData;
#endif /*CONFIG_SVINTO_SIM*/

		/* check device type and fill in correct sizes etc */

		switch(chip->device_id) {
		case AM29LV322DT:
		case AM29LV323DT:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 32Mb TB.\n");
#endif
			chip->size = FL_SIZEOF(0x00400000);
			chip->sectorsize = FL_SIZEOF(0x10000);
			chip->bootsector = chip->start + chip->size
					   - chip->sectorsize;
			chip->bootsectorsize[0] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[1] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[2] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[3] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[4] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[5] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[6] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[7] = FL_SIZEOF(0x2000);
			break;
		case AM29LV322DB:
		case AM29LV323DB:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 32Mb BB.\n");
#endif
			chip->size = FL_SIZEOF(0x00400000);
			chip->sectorsize = FL_SIZEOF(0x10000);
			chip->bootsector = chip->start;
			chip->bootsectorsize[0] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[1] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[2] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[3] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[4] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[5] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[6] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[7] = FL_SIZEOF(0x2000);
			break;
		case AM29LV160BT:
		case TC58FVT160:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 16Mb TB.\n");
#endif
			chip->size = FL_SIZEOF(0x00200000);
			chip->sectorsize = FL_SIZEOF(0x10000);
			chip->bootsector = chip->start + chip->size
					   - chip->sectorsize;
			chip->bootsectorsize[0] = FL_SIZEOF(0x8000);
			chip->bootsectorsize[1] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[2] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[3] = FL_SIZEOF(0x4000);
			break;
        case AM29LV400BB:
#ifdef FLASH_VERBOSE
            safe_printk("Flash: AMD AM 29LV400BB - 4Megabit.\n");
#endif
            chip->size= FL_SIZEOF(0x00080000);
			chip->sectorsize = FL_SIZEOF(0x10000);
            chip->bootsector = chip->start;
            chip->bootsectorsize[0] = FL_SIZEOF(0x4000);
            chip->bootsectorsize[1] = FL_SIZEOF(0x2000);
            chip->bootsectorsize[2] = FL_SIZEOF(0x2000);
            chip->bootsectorsize[3] = FL_SIZEOF(0x8000);
            break;
		case AM29LV160BB:
		case TC58FVB160:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 16Mb BB.\n");
#endif
			chip->size = FL_SIZEOF(0x00200000);
			chip->sectorsize = FL_SIZEOF(0x10000);
			chip->bootsector = chip->start;
			chip->bootsectorsize[0] = FL_SIZEOF(0x4000);
			chip->bootsectorsize[1] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[2] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[3] = FL_SIZEOF(0x8000);
			break;
		case AM29LV800BB:
		case AM29F800BB:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 8Mb BB.\n");
#endif
			chip->size = FL_SIZEOF(0x00100000);
			chip->sectorsize = FL_SIZEOF(0x10000);
			chip->bootsector = chip->start;
			chip->bootsectorsize[0] = FL_SIZEOF(0x4000);
			chip->bootsectorsize[1] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[2] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[3] = FL_SIZEOF(0x8000);
			break;
		case M29W800T:
		case AM29LV800BT:
		case AM29F800BT:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 8Mb TB.\n");
#endif
			chip->size = FL_SIZEOF(0x00100000);
			chip->sectorsize = FL_SIZEOF(0x10000);
			chip->bootsector = chip->start + chip->size
					   - chip->sectorsize;
			chip->bootsectorsize[0] = FL_SIZEOF(0x8000);
			chip->bootsectorsize[1] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[2] = FL_SIZEOF(0x2000);
			chip->bootsectorsize[3] = FL_SIZEOF(0x4000);
			break;
			//     case AM29LV800BB:

			/* the following supports smaller sector erases (like 2 Kword)
			 * so they dont use bootblocks. until we've implemented real
			 * support for the smaller erases, we just treat them as if they
			 * dont have bootblocks at all.
			 */

		case SST39LF800:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 8Mb No B\n");
#endif
			chip->size = FL_SIZEOF(0x00100000);
			chip->sectorsize = FL_SIZEOF(0x10000);

			chip->bootsector = (unsigned char *)MEM_DRAM_START; /* never a flash address (see above) */

			break;

		case SST39LF160:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: 16Mb No B\n");
#endif
			chip->size = FL_SIZEOF(0x00200000);
			chip->sectorsize = FL_SIZEOF(0x10000);

			chip->bootsector = (unsigned char *)MEM_DRAM_START; /* never a flash address */

			break;

		default:
#ifdef FLASH_VERBOSE
			safe_printk("Flash: Unknown device\n");
#endif
			chip->isValid = 0;
			break;
		}

		chip->busy = 0;
		init_waitqueue(&chip->wqueue);
	}
}

/* locate the flashchip structure associated with the given adress */

static struct flashchip *
getchip(unsigned char *ptr)
{
	int i;

	for (i = 0; i < MAX_CHIPS; i++) {
		if (ptr >= chips[i].start
		    && ptr < (chips[i].start + chips[i].size)) {
			return &chips[i];
		}
	}

	printk("flash: Illegal adress: getchip(0x%p)!\n", ptr);
	return (void *)0;
}

void *
flash_getpart(kdev_t dev)
{
	struct flashpartition *part;

	if (MINOR(dev) < FLASH_MINOR) {
		return 0;
	}

	part = &partitions[MINOR(dev) - FLASH_MINOR];

	if (!part->start) {
		return 0;
	}

	return (void *)part;
}


unsigned char *
flash_get_direct_pointer(kdev_t dev, __u32 offset)
{
	struct flashpartition *part;

	if (MINOR(dev) < FLASH_MINOR) {
		return 0;
	}

	part = &partitions[MINOR(dev) - FLASH_MINOR];

	if (!part->start) {
		return 0;
	}

	return (unsigned char *) ((__u32) part->start + offset);
}


/* start erasing flash-memory at ptr of a certain size
 * this does not wait until the erasing is done
 */

void
flash_init_erase(unsigned char *ptr, unsigned int size)
{
	struct flashchip *chip;
	int bootSectorCounter = 0;
	unsigned int erasedSize = 0;
	flashptr flashStart;
#ifdef IRQ_LOCKS
	unsigned long flags;
#endif

	ptr = (unsigned char *)((unsigned long)ptr | MEM_NON_CACHEABLE);
	chip = getchip(ptr);
	flashStart = (flashptr)chip->start;

	FDEBUG(safe_printk("Flash: erasing memory at 0x%p, size 0x%x.\n", ptr, size));

	/* need to disable interrupts, to avoid possible delays between the
	 * unlocking and erase-init
	 */

#ifdef IRQ_LOCKS
	save_flags(flags);
	cli();
#endif

	/* Init erasing of the number of sectors needed */

	flashStart[unlockAddress1] = unlockData1;
	flashStart[unlockAddress2] = unlockData2;
	flashStart[unlockAddress1] = sectorEraseUnlockData;
	flashStart[unlockAddress1] = unlockData1;
	flashStart[unlockAddress2] = unlockData2;

	while (erasedSize < size) {
		*(flashptr)ptr = sectorEraseUnlockData2;

		/* make sure we erase the individual bootsectors if in that area */
		/* TODO this BREAKS if we start erasing in the middle of the bootblock! */

		if (ptr < chip->bootsector || ptr >= (chip->bootsector +
						      chip->sectorsize)) {
			erasedSize += chip->sectorsize;
			ptr += chip->sectorsize;
		}
		else {
			erasedSize += chip->bootsectorsize[bootSectorCounter];
			ptr += chip->bootsectorsize[bootSectorCounter++];
		}
	}

#ifdef IRQ_LOCKS
	restore_flags(flags);
#endif
	/* give the busy signal time enough to activate (tBusy, 90 ns) */

#if 0
	nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
#else
	udelay(1);
#endif
}


/* certain products require the individual sectors to be individually polled 
 */

void
flash_finalize_erase(unsigned char *ptr, unsigned int size)
{
	struct flashchip *chip;
	int bootSectorCounter = 0;
	unsigned int erasedSize = 0;
	flashptr flashStart;
	unsigned long tcount;
#ifdef IRQ_LOCKS
	unsigned long flags;
#endif

	ptr = (unsigned char *)((unsigned long)ptr | MEM_NON_CACHEABLE);
	chip = getchip(ptr);
	flashStart = (flashptr)chip->start;

	FDEBUG(safe_printk("Flash: erasing memory at 0x%p, size 0x%x.\n", ptr, size));

	/* need to disable interrupts, to avoid possible delays between the
	 * unlocking and erase-init
	 */

	while(erasedSize < size) {
		tcount = SECTOR_ERASE_TIMEOUT_JIFS ;

		while ((*(flashptr)ptr != FLASH_ERASE_STATE) && (--tcount > 0)) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + 1 ;
			schedule();
		}
		if (tcount==0)
		{
			printk("Erase of sector at ptr=0x%p, erasedSize=0x%x TIMED OUT!\n", 
			       ptr, erasedSize);
		}

		/* make sure we erase the individual bootsectors if in that area */
		/* TODO this BREAKS if we start erasing in the middle of the bootblock! */
		if(ptr < chip->bootsector || ptr >= (chip->bootsector +
						     chip->sectorsize)) {
			erasedSize += chip->sectorsize;
			ptr += chip->sectorsize;
		} else {
			erasedSize += chip->bootsectorsize[bootSectorCounter];
			ptr += chip->bootsectorsize[bootSectorCounter++];
		}
		FDEBUG(printk("Erase of sector at ptr=0x%p, (erasedSize = 0x%x) \n", 
			ptr, erasedSize));
	}

}


int
flash_erase_region(kdev_t dev, __u32 offset, __u32 size)
{
	int minor;
	struct flashpartition *part;
	unsigned char *erase_start;
	short retries = 5;
	short success;

	minor = MINOR(dev);
	if (minor < FLASH_MINOR) {
		return -EINVAL;
	}

	part = &partitions[minor - FLASH_MINOR];
	if (!part->start) {
		return -EINVAL;
	}

	/* Start the erasure, then sleep and wake up now and then to see
	 * if it's done.
	 */

	erase_start = part->start + offset;

	flash_safe_acquire(part);
	do {
		flash_init_erase(erase_start, size);

		while (flash_is_busy((flashptr)erase_start)) {
			current->state = TASK_INTERRUPTIBLE;
			current->timeout = jiffies + HZ / 2;
			schedule();
		}

		success = ((flashptr)erase_start)[0] == FLASH_ERASE_STATE
			  && ((flashptr)erase_start)[1] == FLASH_ERASE_STATE
			  && ((flashptr)erase_start)[2] == FLASH_ERASE_STATE
			  && ((flashptr)erase_start)[3] == FLASH_ERASE_STATE;

		if (!success) {
			printk(KERN_NOTICE "flash: erase of region "
			       "[0x%p, 0x%p] failed once\n",
			       erase_start, erase_start + size);
		}

	} while (retries-- && !success);

	flash_safe_release(part);

	if (retries == 0 && !success) {
		printk(KERN_WARNING "flash: erase of region "
			       "[0x%p, 0x%p] totally failed\n",
			       erase_start, erase_start + size);
		return -1;
	}

	return 0;
}


/* wait until the erase operation is finished, in a CPU hogging manner */

void
flash_busy_wait_erase(unsigned char *ptr)
{
	flashptr flashStart;

	ptr = (unsigned char *)((unsigned long)ptr | MEM_NON_CACHEABLE);
	flashStart = (flashptr)getchip(ptr)->start;

	/* busy-wait for flash completion - when D6 stops toggling between
	 * reads.
	 */
	while (flash_is_busy(flashStart))
		/* nothing */;
}

/* erase all flashchips and wait until operation is completed */

void
flash_erase_all(void)
{
	/* TODO: we should loop over chips, not just try the first two! */

	if(chips[0].isValid)
		flash_init_erase(chips[0].start, chips[0].size);

	if(chips[1].isValid)
		flash_init_erase(chips[1].start, chips[0].size);

	if(chips[0].isValid)
		flash_busy_wait_erase(chips[0].start);

	if(chips[1].isValid)
		flash_busy_wait_erase(chips[1].start);

#ifdef FLASH_VERBOSE
	safe_printk("Flash: full erasure completed.\n");
#endif
}

/* Write a block of Flash. The destination Flash sectors need to be erased
 * first. If the size is larger than the Flash chip the block starts in, the
 * function will continue flashing in the next chip if it exists.
 * Returns 0 on success, -1 on error.
 */

int
flash_write(unsigned char *ptr, const unsigned char *source, unsigned int size)
{
#ifndef CONFIG_SVINTO_SIM
	struct flashchip *chip;
	flashptr theData = (flashptr)source;
	flashptr flashStart;
	flashptr programAddress;
	int i, fsize;
	int odd_size;

	ptr = (unsigned char *)((unsigned long)ptr | MEM_NON_CACHEABLE);

	while(size > 0) {
		chip = getchip(ptr);

		if(!chip) {
			printk("Flash: illegal ptr 0x%p in flash_write.\n", ptr);
			return -EINVAL;
		}

		flashStart = (flashptr)chip->start;
		programAddress = (flashptr)ptr;

		/* if the block doesn't fit in this flash chip, clamp the size */

		fsize = (ptr + size) > (chip->start + chip->size) ?
			(chip->start + chip->size - ptr) : size;

		ptr += fsize;
		size -= fsize;
        odd_size = fsize & (FLASH_WIDTH - 1);

#ifdef FLASH_16BIT
		fsize >>= 1; /* We write one word at a time.  */
#else
		fsize >>= 2; /* We write one dword at a time.  */
#endif

		FDEBUG(printk("flash_write (flash start 0x%p) %d words to 0x%p\n",
			      flashStart, fsize, programAddress));

		for (i = 0; i < fsize; i++) {
			int retries = 0;

			do {
				int timeout;
				/* Start programming sequence.
				 */
#ifdef IRQ_LOCKS
				unsigned long flags;
				save_flags(flags);
				cli();
#endif
				flashStart[unlockAddress1] = unlockData1;
				flashStart[unlockAddress2] = unlockData2;
				flashStart[unlockAddress1] = programUnlockData;
				*programAddress = *theData;
				/* give the busy signal time to activate (tBusy, 90 ns) */
#if 0
				nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
#else
				/* wait half a byte delay - more elegant than the nop,nop,nop */
				udelay(NOM_BYTE_PROGRAM_TIME_uS / 2);
#endif
				/* Wait for programming to finish.  */
				timeout = MAX_BYTE_PROGRAM_TIME_uS;
                while((timeout > 0) && (*programAddress != *theData))
                {
                    --timeout;
                    udelay(1);
                }
#ifdef IRQ_LOCKS
				restore_flags(flags);
#endif
				if(timeout <= 0)
					printk("flash: write timeout 0x%p\n",
					       programAddress);
                {
                    unsigned long flash_check = *programAddress;
                    if (flash_check != *theData)
                    {
                        printk("Flash: verify error 0x%p. (flash_write() %d)\n",
                                programAddress, __LINE__);
#if (FLASH_WIDTH==4)
                        printk("*programAddress = 0x%08lx, *theData = 0x%08lx\n",
                                flash_check, *theData);
#else
                        printk("*programAddress = 0x%04x, *theData = 0x%04x\n",
                                flash_check, *theData);
#endif
                    }
                    else
                    {
                        break;
                    }
                }
			} while(++retries < 5);

			if(retries >= 5) {
				printk("FATAL FLASH ERROR (1)\n");
				return -EIO; /* we failed... */
			}

			++programAddress;
			++theData;
		}

        if(odd_size) {
            unsigned char last_byte[FLASH_WIDTH];
            int retries = 0;

            memcpy(last_byte, (const char *)theData, odd_size);
            memcpy(&last_byte[odd_size],
                    &(((unsigned char *)programAddress)[odd_size]),
                    (FLASH_WIDTH - odd_size));
            theData = (flashptr)last_byte;
			do {
				int timeout = MAX_BYTE_PROGRAM_TIME_uS;
#ifdef IRQ_LOCKS
				unsigned long flags;
				save_flags(flags);
				cli();
#endif
				flashStart[unlockAddress1] = unlockData1;
				flashStart[unlockAddress2] = unlockData2;
				flashStart[unlockAddress1] = programUnlockData;
				*programAddress = *theData;
				/* give the busy signal time enough to activate (tBusy, 90 ns) */
#if 0
				nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
#else
				/* wait half a byte delay - more elegant than the nop,nop,nop */
				udelay(NOM_BYTE_PROGRAM_TIME_uS / 2);
#endif
				/* Wait for programming to finish */
				timeout = MAX_BYTE_PROGRAM_TIME_uS;
                while((timeout > 0) && (*programAddress != *theData))
                {
                    --timeout;
                    udelay(1);
                }
#ifdef IRQ_LOCKS
				restore_flags(flags);
#endif
				if(timeout <= 0)
					printk("flash: write timeout 0x%p\n",
					       programAddress);
				if (*programAddress == *theData)
					break;

				printk("Flash: verify error 0x%p. "
				       "(flash_write() %d)\n",
				       programAddress, __LINE__);
			} while(++retries < 5);

			if(retries >= 5) {
				printk("FATAL FLASH ERROR (%d)\n", __LINE__);
				return -EIO; /* we failed... */
			}
		}
	}

#else
	/* in the simulator we simulate flash as ram, so we can use a simple memcpy */
	printk("flash write, source %p dest %p size %d\n", source, ptr, size);
	memcpy(ptr, source, size);
#endif
	return 0;
}


/* "Memset" a chunk of memory on the flash.
 * do this by flash_write()'ing a pattern chunk.
 */

int
flash_memset(unsigned char *ptr, const __u8 c, unsigned long size)
{
#ifndef CONFIG_SVINTO_SIM

	static unsigned char pattern[16];
	int i;

	/* fill up pattern */

	for(i = 0; i < 16; i++)
		pattern[i] = c;

	/* write as many 16-byte chunks as we can */

	while(size >= 16) {
		flash_write(ptr, pattern, 16);
		size -= 16;
		ptr += 16;
	}

	/* and the rest */

	if(size)
		flash_write(ptr, pattern, size);
#else

	/* In the simulator, we simulate flash as ram, so we can use a
	   simple memset.  */
	printk("flash memset, byte 0x%x dest %p size %d\n", c, ptr, size);
	memset(ptr, c, size);

#endif

	return 0;
}


#ifdef CONFIG_BLK_DEV_FLASH
/* the operations supported by the block device */

static struct file_operations flash_block_fops =
{
	NULL,                   /* lseek - default */
	block_read,             /* read - general block-dev read */
	block_write,            /* write - general block-dev write */
	NULL,                   /* readdir - bad */
	NULL,                   /* poll */
	flash_ioctl,            /* ioctl */
	NULL,                   /* mmap */
	flash_open,             /* open */
	flash_release,          /* release */
	block_fsync,            /* fsync */
	NULL,			/* fasync */
	NULL,			/* check media change */
	NULL			/* revalidate */
};
#endif

#ifdef CONFIG_CHR_DEV_FLASH
/* the operations supported by the char device */

static struct file_operations flash_char_fops =
{
	NULL,                   /* lseek - default */
	flash_char_read,        /* read */
	flash_char_write,       /* write */
	NULL,                   /* readdir - bad */
	NULL,                   /* poll */
	flash_ioctl,            /* ioctl */
	NULL,                   /* mmap */
	flash_open,             /* open */
	flash_release,          /* release */
	NULL,                   /* fsync */
	NULL,			/* fasync */
	NULL,			/* check media change */
	NULL			/* revalidate */
};
#endif

/* Initialize the flash_partitions array, by reading the partition information from the
 * partition table (if there is any). Otherwise use a default partition set.
 *
 * The first partition is always sector 0 on the first chip, so start by initializing that.
 * TODO: partitions only reside on chip[0] now. check that.
 */

static void
flash_init_partitions(void)
{
	struct partitiontable_head *ptable_head;
	struct partitiontable_entry *ptable;
	int use_default_ptable = 1; /* Until proven otherwise */
	int pidx = 0;
	int pidx_before_probe = 0;
	const char *pmsg = "  /dev/flash%d at 0x%x, size 0x%x\n";

	/* if there is no chip 0, there is no bootblock => no partitions at all */

	if (chips[0].isValid) {

		printk("Checking flash partitions:\n");
		/* First sector in the flash is partition 0,
		   regardless of if it's a real flash "bootblock" or not.  */

		partitions[0].chip = &chips[0];
		partitions[0].start = chips[0].start;
#ifdef CONFIG_SED_SIOS
        /* The first two sectors of flash are partition 0. One sector is not
           large enough for the kernel. */
		partitions[0].size = chips[0].sectorsize * 2;
#else
		partitions[0].size = chips[0].sectorsize;
#endif
		partitions[0].flags = 0;  /* FIXME */
		flash_sizes[FLASH_MINOR] =
			partitions[0].size >> BLOCK_SIZE_BITS;
		printk(pmsg, 0, partitions[0].start, partitions[0].size);
		pidx++;

		ptable_head = (struct partitiontable_head *)
		  (partitions[0].start + partitions[0].size
		   + PARTITION_TABLE_OFFSET);

#ifdef CONFIG_SVINTO_SIM
                /* If running in the simulator, do not scan nonexistent
		   memory.  Behave as when the bootblock is "broken".
                   ??? FIXME: Maybe there's something better to do. */
		partitions[pidx].chip = &chips[0];
		partitions[pidx].start = chips[0].start;
		partitions[pidx].size = chips[0].size;
		partitions[pidx].flags = 0; /* FIXME */
		flash_sizes[FLASH_MINOR + pidx] =
			partitions[pidx].size >> BLOCK_SIZE_BITS;
		printk(pmsg, pidx, partitions[pidx].start, partitions[pidx].size);
		pidx++;
#else  /* ! defined CONFIG_SVINTO_SIM */

		pidx_before_probe = pidx;

		/* TODO: until we've defined a better partition table,
		 * always do the default flash1 and flash2 partitions.
		 */

		if ((ptable_head->magic ==  PARTITION_TABLE_MAGIC)
		    && (ptable_head->size
			< (MAX_PARTITIONS
			   * sizeof(struct partitiontable_entry) + 4))
		    && (*(unsigned long*)
			((void*)ptable_head
			 + sizeof(*ptable_head)
			 + ptable_head->size - 4)
			==  PARTITIONTABLE_END_MARKER)) {
			/* Looks like a start, sane length and end of a
			 * partition table, lets check csum etc.
			 */
			int ptable_ok = 0;
			struct partitiontable_entry *max_addr =
			  (struct partitiontable_entry *)
			  ((unsigned long)ptable_head + sizeof(*ptable_head) +
				ptable_head->size);
			unsigned long offset = chips[0].sectorsize;
			unsigned char *p;
			unsigned long csum = 0;

			ptable = (struct partitiontable_entry *)
			  ((unsigned long)ptable_head + sizeof(*ptable_head));

			/* Lets be PARANOID, and check the checksum. */
			p = (unsigned char*) ptable;

			while (p <= (unsigned char*)max_addr) {
				csum += *p++;
				csum += *p++;
				csum += *p++;
				csum += *p++;
			}
			/* printk("  total csum: 0x%08X 0x%08X\n",
			   csum, ptable_head->checksum); */
			ptable_ok = (csum == ptable_head->checksum);

			/* Read the entries and use/show the info.  */
			ptable = (struct partitiontable_entry *)
			  ((unsigned long)ptable_head + sizeof(*ptable_head));

			printk(" Found %s partition table at 0x%08lX-0x%08lX.\n",
			       (ptable_ok ? "valid" : "invalid"),
			       (unsigned long)ptable_head,
			       (unsigned long)max_addr);

			/* We have found a working bootblock.  Now read the
			   partition table.  Scan the table.  It ends when
			   there is 0xffffffff, that is, empty flash.  */

			while (ptable_ok
			       && ptable->offset != 0xffffffff
			       && ptable < max_addr
			       && pidx < MAX_PARTITIONS) {
				partitions[pidx].chip = &chips[0];
				if ((offset + ptable->offset) >= chips[0].size) {
					partitions[pidx].start
					  = offset + chips[1].start
					    + ptable->offset - chips[0].size;
				}
				else {
					partitions[pidx].start
					  = offset + chips[0].start
					    + ptable->offset;
				}
				partitions[pidx].size = ptable->size;
				partitions[pidx].flags = ptable->flags;
				partitions[pidx].type = ptable->type;
				flash_sizes[FLASH_MINOR + pidx] =
					partitions[pidx].size >> BLOCK_SIZE_BITS;
				printk(pmsg, pidx, partitions[pidx].start,
				       partitions[pidx].size);
				pidx++;
				ptable++;
			}
#ifdef CONFIG_USE_FLASH_PARTITION_TABLE
			use_default_ptable = !ptable_ok;
#endif
		}
		if (use_default_ptable)
#ifdef CONFIG_SED_SIOS
        {
            /* the flash is split into flash1 and bootblock
             * (flash0)
             */
            pidx = pidx_before_probe;
            printk(" Using default flash1\n");

            /* flash1 starts after the first sector */ 
            partitions[pidx].chip = &chips[0];
            partitions[pidx].start = chips[0].start + partitions[0].size;
            partitions[pidx].size = chips[0].size - partitions[0].size;
            partitions[pidx].flags = 0; /* FIXME */
            flash_sizes[FLASH_MINOR + pidx] =
                partitions[pidx].size >> BLOCK_SIZE_BITS;
            printk(pmsg, pidx, partitions[pidx].start, partitions[pidx].size);
            pidx++;
	        if (chips[1].isValid)
            {
                partitions[pidx].chip = &chips[1];
                partitions[pidx].start = chips[1].start;
                partitions[pidx].size = chips[1].size;
                partitions[pidx].flags = 0; /* FIXME */
                flash_sizes[FLASH_MINOR + pidx] =
                    partitions[pidx].size >> BLOCK_SIZE_BITS;
                printk(pmsg, pidx, partitions[pidx].start,
                        partitions[pidx].size);
                pidx++;
            }
		}
#else
        {
			/* the flash is split into flash1, flash2 and bootblock
			 * (flash0)
			 */
			pidx = pidx_before_probe;
			printk(" Using default flash1 and flash2.\n");

			/* flash1 starts after the first sector */

			partitions[pidx].chip = &chips[0];
			partitions[pidx].start = chips[0].start + chips[0].sectorsize;
			partitions[pidx].size = chips[0].size - (DEF_FLASH2_SIZE +
				partitions[0].size);
			partitions[pidx].flags = 0; /* FIXME */
			flash_sizes[FLASH_MINOR + pidx] =
				partitions[pidx].size >> BLOCK_SIZE_BITS;
			printk(pmsg, pidx, partitions[pidx].start,
			       partitions[pidx].size);
			pidx++;

			/* flash2 starts after flash1. */

			partitions[pidx].chip = &chips[0];
			partitions[pidx].start = partitions[pidx - 1].start +
				partitions[pidx - 1].size;
			partitions[pidx].size = DEF_FLASH2_SIZE;
			partitions[pidx].flags = 0; /* FIXME */
			flash_sizes[FLASH_MINOR + pidx] =
				partitions[pidx].size >> BLOCK_SIZE_BITS;
			printk(pmsg, pidx, partitions[pidx].start,
			       partitions[pidx].size);
			pidx++;
		}
#endif


#endif /* ! defined CONFIG_SVINTO_SIM */
	}

	/* fill in the rest of the table as well */

	while (pidx < MAX_PARTITIONS) {
		partitions[pidx].start = 0;
		partitions[pidx].size = 0;
		partitions[pidx].chip = 0;
		pidx++;
	}

	/*flash_blk_sizes[FLASH_MINOR + i] = 1024; TODO this should be 512.. */
}

#ifdef LISAHACK
static void
move_around_bootparams(void)
{
	unsigned long *newp = (unsigned long *)0x8000c000;  /* new bootsector */
	unsigned long *oldp = (unsigned long *)0x801fc000;  /* old bootsector */
	unsigned long *buf;
	unsigned long magic = 0xbeefcace;

	printk("Checking if we need to move bootparams...");

	/* First check if they are already moved.  */

	if(*newp == magic) {
		printk(" no\n");
		return;
	}

	printk(" yes. Moving...");

	buf = (unsigned long *)kmalloc(0x4000, GFP_KERNEL);

	memcpy(buf, oldp, 0x4000);

	flash_write((unsigned char *)newp, (unsigned char *)&magic, 4);
	flash_write((unsigned char *)(newp + 1), (unsigned char *)buf, 0x4000 - 4);

	/* Erase old boot block, so JFFS can expand into it.  */

	flash_init_erase((unsigned char *)0x801f0000, chips[0].sectorsize);
	flash_busy_wait_erase(chips[0].start);

	printk(" done.\n");

	kfree(buf);
}
#endif

/* register the device into the kernel - called at boot */

int
flash_init(void)
{
#ifdef CONFIG_BLK_DEV_FLASH
	/* register the block device major */

	if(register_blkdev(MAJOR_NR, DEVICE_NAME, &flash_block_fops )) {
		printk(KERN_ERR DEVICE_NAME ": Unable to get major %d\n",
		       MAJOR_NR);
		return -EBUSY;
	}

	/* register the actual block I/O function - do_flash_request - and the
	 * tables containing the device sizes (in 1kb units) and block sizes
	 */

	blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
	blk_size[MAJOR_NR] = flash_sizes;
	blksize_size[MAJOR_NR] = flash_blk_sizes;
	read_ahead[MAJOR_NR] = 1; /* fast device, so small read ahead */

	printk("Flash/ROM block device v2.1, (c) 1999 Axis Communications AB\n");
#endif

#ifdef CONFIG_CHR_DEV_FLASH
	/* register the char device major */

	if(register_chrdev(MAJOR_NR, DEVICE_NAME, &flash_char_fops )) {
		printk(KERN_ERR DEVICE_NAME ": Unable to get major %d\n",
		       MAJOR_NR);
		return -EBUSY;
	}

	printk("Flash/ROM char device v2.1, (c) 1999 Axis Communications AB\n");
#endif

	/* initialize partition table */

	flash_init_partitions();

#ifdef LISAHACK
	/* nasty hack to "upgrade" older beta units of Lisa into newer by
	 * moving the boot block parameters. will go away as soon as this
	 * build is done.
	 */

	move_around_bootparams();
#endif

	return 0;
}

/* check if it's possible to erase the wanted range, and if not, return
 * the range that IS erasable, or a negative error code.
 */

long
flash_erasable_size(void *_part, __u32 offset, __u32 size)
{
	struct flashpartition *part = (struct flashpartition *)_part;
	int ssize;

	if (!part->start) {
		return -EINVAL;
	}

	/* assume that sector size for a partition is constant even
	 * if it spans more than one chip (you usually put the same
	 * type of chips in a system)
	 */

	ssize = part->chip->sectorsize;

	if (offset % ssize) {
		/* The offset is not sector size aligned.  */
		return -1;
	}
	else if (offset > part->size) {
		return -2;
	}
	else if (offset + size > part->size) {
		return -3;
	}

	return (size / ssize) * ssize;
}
