/* $Id: flash.h,v 1.2 2000/10/06 08:50:37 davidm Exp $ */
/* Copyright (C) 1999, 2000 Axis Communications AB, Sweden */
/* 
 * $Log: flash.h,v $
 * Revision 1.2  2000/10/06 08:50:37  davidm
 *
 * Added latest code/patches from elinux_20000801.tgz for jffs support
 *
 * Revision 1.8  2000/07/14 16:23:27  johana
 * Flash partition in sync with tools/mkptable
 * 
 *
 */

#ifndef __LINUX_FLASH_H
#define __LINUX_FLASH_H

#include <asm/types.h>

/* the ioctl's supported */

#define FLASHIO_ERASEALL 0x1      /* erase the entire device, blocks until completion */
#define FLASHIO_CHIPINFO 0x2      /* reads internal chip info and store it in */

#ifdef __KERNEL__
extern int flash_init(void);
extern void flash_probe(void);
extern void flash_safe_acquire(void *part);
extern void flash_safe_release(void *part);
extern int flash_safe_read(void *part, unsigned char *fptr,
			   unsigned char *buf, int count);
extern int flash_safe_write(void *part, unsigned char *fptr,
			    const unsigned char *buf, int count);
extern void *flash_getpart(kdev_t dev);
extern unsigned char *flash_get_direct_pointer(kdev_t dev, __u32 offset);
extern int flash_erase_region(kdev_t dev, __u32 offset, __u32 size);
extern long flash_erasable_size(void *_part, __u32 offset, __u32 size);
extern int flash_memset(unsigned char *ptr, const __u8 c, unsigned long size);
extern void flash_erase_all(void);
#endif

/* the following are on-flash structures */

/* Bootblock parameters are stored at 0xc000 and has the FLASH_BOOT_MAGIC 
 * as start, it ends with 0xFFFFFFFF */
#define FLASH_BOOT_MAGIC 0xbeefcace
#define BOOTPARAM_OFFSET 0xc000
/* apps/bootblocktool is used to read and write the parameters,
 * and it has nothing to do with the partition table. 
 */


/* the partitiontable consists of some "jump over" code, a head and
 * then the actual entries.
 * tools/mkptable is used to generate the ptable. 
 */

/* The partition table start with kod to "jump over" it: */
#define PARTITIONTABLE_CODE_START { \
 0x0f, 0x05, /* nop 0 */\
 0x25, 0xf0, /* di  2 */\
 0xed, 0xff  /* ba  4 */ }

/* The actual offset depend on the number of entries */
#define PARTITIONTABLE_CODE_END { \
 0x00, 0x00, /* ba offset 6 */\
 0x0f, 0x0f  /* nop 8 */}

#define PARTITION_TABLE_OFFSET 10
#define PARTITION_TABLE_MAGIC 0xbeef /* Not a good magic */

/* The partitiontable_head is located at offset +10: */
struct partitiontable_head {
	__u16 magic; /* PARTITION_TABLE_MAGIC */ 
	__u16 size;  /* Length of ptable block (not header) */
	__u32 checksum; /* simple longword sum */
};

/* And followed by partition table entries */
struct partitiontable_entry {
	__u32 offset;   /* Offset is relative to the sector the ptable is in */
	__u32 size;
	__u32 checksum; /* simple longword sum */
	__u16 type;
	__u16 flags;   /* bit 0: ro/rw = 1/0 */
	__u32 future0; /* 16 bytes reserved for future use */
	__u32 future1;
	__u32 future2;
	__u32 future3;
};
/* ended by an end marker: */
#define PARTITIONTABLE_END_MARKER 0xFFFFFFFF

/*#define PARTITION_TYPE_RESCUE 0x0000?*/  /* Not used, maybe it should? */
#define PARTITION_TYPE_PARAM  0x0001 /* Hmm.. */
#define PARTITION_TYPE_KERNEL 0x0002
#define PARTITION_TYPE_JFFS   0x0003


/* the struct for ioctl FLASHIO_CHIPINFO */

struct flashchipinfo {
	unsigned short isValid;
	unsigned short manufacturer_id;
	unsigned short device_id;
	int size, sectorsize;
};

#endif /* __LINUX_FLASH_H */
