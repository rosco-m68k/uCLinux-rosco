/*  $Id: nfa.c,v 1.2 2001/05/30 05:24:54 gerg Exp $
 *
 *  Copyright (C) 2000 Jeffrey L. Smith (flexiblepenguin@hotmail.com)
 *
 *  ECC Code 
 *  Copyright (c) 2000   Toshiba America Electronics Components, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file contains the driver for the NFAdisk code that converts
 * a NAND Flash Array into a linear, contiguous Solid State Floppy Disk  
 * Drive Work-A-Like. This driver is independant of the FileSystem that
 * operates on it, and provides for Wear Leveling, and Failure Redundancy. 
 * It is designed for High Availibility Embedded Systems, and provides a 
 * a robust mechanizim for Power Fail recovery durring all operations, 
 * while maintaining a small system memory foot print, and low CPU 
 * CPU bandwidth. Speed On Read/Write Accesses Should be comperable to 
 * Rotational Media.
 *
 * This file contains an ECC algorithm from Toshiba that detects and
 * corrects 1 bit errors in a 256 byte block of data.
 *
 * Contact flexiblepenguin@msn.com for more info...
 *
 */

/* 
 *
 * B A S I C  I N F O R M A T I O N . . . 
 *
 *
 *  1.0 HISTORY
 *     I was getting fed up with looking for a driver, and wrote my own...
 *     This is how the GNU community works. This driver has been designed for
 *     DEEPLY EMBEDDED Single Board Computers running Linux or uClinux.
 *
 *  2.0 FEATURES
 *     This driver has a number of unique features that are not 
 *     standard in the GNU sources publicly avalible. 
 *
 *         a. Single 'C' Source File.
 *         b. Small Code Profile.
 *         c. Fast Operation.
 *         d. Stand Alone Operation.
 *         e. It's ENDIAN aware.
 *         f. Provides for linux Kernel Access & BootLoadder Access.
 *         g. Places NO restrictions on Who's "chips" it can be used with.
 *         h. Runtime Geometry Configuration.
 *         i. Works with ALL linux file utils...
 *         j. No special Formatting / Partitoning Software.
 *         k. Easily Customizable.
 *
 *  3.0 MTD DRIVERS
 *     This driver was developed SPECIFICALLY to avoid the use of the linux
 *     MTD drivers. Why?, I needed a small fast driver to create a Silicon
 *     Based Disk Drive, and needed to be able to boot from the "drive" 
 *     with the boot loader and BIOS for the target in a VERY SMALL standard
 *     flash memory. This driver can be compiled as a linux Kernel Driver 
 *     AND compiled into a boot loader WITH NO CHANGES. I simply have a 
 *     simbolic link from "uClinux-coldfire/linux/drivers/block/nfa.c" to
 *     my BootLoader source tree. The makefile for the BootLoadder defines
 *     "BOOTLOADER_BUILD" and everything works...
 *     Should this code become "part" of the MTD driver project ? I don't 
 *     know, and think that it "fits" better under the standard block dev
 *     source tree. I have GREAT respect for the MTD Driver folks, I spent
 *     MUCH time reading and re-reading the MTD driver source BEFORE I even
 *     started on this code. 
 *
 *  4.0 BAD BLOCK HANDLING.
 *     This driver handles bad blocks by NOT handling them... Sounds dumb,
 *     but it works. Once a Block is marked BAD (6th byte in 1st OR 2nd page
 *     of block's redundant data is NON FFh) it will never be used again.
 *     bad blocks are shipped in the powe on array scan, and become "invisible"
 *     to the system. There is NO NEED to "map'em out" as we never make them
 *     "accessable". Bad blocks are only marked as such in the following 
 *     circumstances. #1 By the chip Mfg, #2 On ERASE failure, #3 on write
 *     failure where the ECC correction CANNOT recover the "bad" bit(s).
 *
 *  5.0 WEAR LEVELING
 *     This driver performs wear leveling by tracking the total number of write
 *     cycles to each "block" of pages (1 page == 1 virtual sector). Each time
 *     the block is ERASED the count for that block is bumped by 1. The driver 
 *     maintains a "runtime" table of all free blocks, ordered in least to most
 *     written. As a block is ERASED and returned into the free-list, it is 
 *     inserted into the list in the 1st "slot" where the NEXT block in the list
 *     has more writes than the block being added. As Blocks need to be allocated
 *     the TOP block on the list is pop'ed and retuned as the block to use.
 *
 *  6.0 VIRTUAL SECTORS
 *     Virtual sectors are handled in an "LBA" or Logical Block Address manner. 
 *     This is the method that SCSI disks use. Heads/Cyl/Track's have no "real"
 *     meaning here. The driver handles READ/WRITE calls of Virtual Sectors by
 *     using a virtiual block number as the index into the Virtual-To-Physical
 *     block table. At driver init time all blocks are scanned and any block that
 *     id NOT bad, and has a "virtual" block address in it's redundant data area
 *     is added to the table. The process works as follows:
 *
 *           VirtualBlock = VirtualSectorNumber / PagesPerBlock
 * 
 *           PhysicalBlock = VirtualToPhysicalTable[  VirtualBlock ]
 *
 *           IF 
 *              PhysicalBlock == 0xFFFFFFFF, then NO Physical Block Allocated
 *
 *           ELSE
 *             PhysicalChip = PhysicalBlock / BlocksPerChip
 *
 *           PageInChip = (PhysicalBlock % BlocksPerChip) * PagesPerBlock
 *
 *     As a side note, when a READ operation is performed for a VIRTUAL sector
 *     that has NO physical sector, we return sector buffer with 00h and return.
 *     You may ask why?, simple, Physical Hard Disk drives when formatted, are 
 *     filled with 00h... Think about it... Cool... This make is possible to have
 *     a Virtual Disk where it "looks" like the whole thing is there to fdisk and
 *     freinds, even when it has not yet been written... 
 *         
 *
 *  7.0 VIRTUAL DISK SIZE
 *     When the driver is initialized, it scans for "MAX_CHIPS_IN_ARRAY" looking
 *     for NAND Flash Chips. The 1st chip located is used as the "master" type, with
 *     ALL chips in the array being REQUIRED to be the same MFG and Part#. Once we
 *     know What "type" of chip, and how many are present, we setup some parameters
 *     that are utilized latter on like, Virtual Cylinders/Heads/Sectors (CHS), Total
 *     Sectors, Sector Size, etc... The parameters for CHS and related have been set
 *     using "optimal" settings for the "virtual" disk as compared to physical disks.
 *
 *
 *  8.0 PARTITIONING
 *     Yes Virgina, the disk CAN be partitioned... Use linux fdisk to partition the 
 *     virtual disk just as you would a "real" hard disk... Oh, there ARE a 'couple'
 *     of suggestions. 
 *
 *         a. Use FAT16 as the 'type' if you want to use the companion BootLoader.
 *         b. If you want more than 4 partitions modify the code... (hard coded @ 4)
 *         c. Don't forget to set the "Bootable" flag for the BootLoader (Duh...)
 *         d. Don't complain about "strange stuff" on weird settings, just be happy it works at all!
 * 
 *
 *  9.0 FORMATTING
 *     GeeWiz are you suggesting that this will work like a real hard disk ?
 *     Well, it works with mkdosfs & mke2fs! Yepper's... Shur Duz...
 *
 *
 *  10.0 CUSTOM HARDWARE
 *     Ok, I know we don't have the same hardware. I *attempted* to make it as easy as
 *     possible to port to "new" platforms. The code is broken down in "layers" with
 *     the lowest being "bit" twiddling of the actual chip pins... The following areas
 *     should be addressed:
 *  
 *         a. Change 'MAX_CHIPS_IN_ARRAY' to the MAX that can be SOLDERED in...
 *
 *         b. If the chips are RUN TIME REMOVABLE then this driver is NOT for you,
 *            I am currently developing a "Removable Media" Version, be calm...
 *
 *         c. If you have a controller chip, then you can GUT most of the low level
 *            stuff including the ECC code.
 *     
 *         e. DO NOT USE with M-Systems parts... BE FORE WARNED!!!
 *         
 *         f. The interface this code uses as "delivered" is a pair of 8 bit wide
 *            Read/Writeable latches, controlled by an FPGA (for address decode
 *            etc...) with ALL connections to the chips "bit banged"....
 * 
 * 
 *  11.0 SPEED
 *     The speed of the disk *could* be optimized by delaying erases via an erase queue,
 *     etc... I chose to NOT do that for a number of VERY good reasons. Please Follow:
 *
 *          a. This interface is for DEEPLY embedded systems.
 *          b. The "Primary" filesystem *SHOULD* be a RAM Disk, with this disk under "/mnt/..."
 *          c. You should only be using this device for Boot/ ParameterSave / Data Logging.
 *          d. This filesystem will survive POWER FAIL under operation!
 *          e. KISS (Keep It Simple Stupid..), and it's already "Fast!"
 *
 *
 *  12.0 USE
 *     To use this driver under uClinux (or 'real' Linux) you will have to do the following:
 *     
 *          a. Build a new kernel with this driver in the /dev/block portion of the source tree
 *
 *          b. Create the device handles (as root on uClinux):
 *                            'mknod romfs/dev/nfa  b 86 0'
 *                            'mknod romfs/dev/nfa1 b 86 1'
 *                            'mknod romfs/dev/nfa2 b 86 2'
 *                            'mknod romfs/dev/nfa3 b 86 3'
 *                            'mknod romfs/dev/nfa4 b 86 4'
 *                            'chown username.group romfs/dev/nfa*'
 *
 *          b. Boot the system.
 *
 *          c. run fdisk /dev/nfa  (create a single bootable partition of FAT16 and mark 'bootable')
 *
 *          d. sync the disk (sync command OR reboot )
 *
 *          e. run mkdosfs /dev/nfa1
 *
 *          f. Reboot... That's IT!
 *
 *
 *  13.0 MODIFICATION
 *     Feel free to modify all you want. If you happen to find a bug :-) PLEASE send me
 *     your patch for the next release. I will answer all e-mail about this driver, so 
 *     if you get stuck, just ask... We are ALL still learning. This code is STILL dirty
 *     and I have taken no time to "pretty it up". I do this for the simple reason, that
 *     I felt that time was of the essance. I would rather get it into as many hands as
 *     as possible, as soon as possible, instead of spending weeks "tweeking" it, or making
 *     it "look" better. Please don't critisize, it's already embarrasing just sending it
 *     "as is", Later releases will "look" better.
 *
 *
 *    Good Luck
 *    Jeff Smith
 *
 *
 *  02/10/2001 - JLS
 * 
 *    New Hardware platform was deliverd to me last week, The Interface has changed (for the better)
 *    and now is controlled by a "more intelegent" FPGA. The completely bit-banged stuff has now been
 *    replaced by a "port" style interface, but remains availible via the BIT_BANGED ifdef...
 *
 *  02/27/2001 - JLS
 * 
 *    First 'Public' Release.
 *
 */


#define DEBUG      1
#ifdef BOOTLOADER_BUILD
#include <stdlib.h>
#include <stdio.h>
#include "platform.h"
#include "bsp.h"

#define FALSE 0
#define TRUE 1
#define EBUSY -1

#define sti()
#define cli()

#else

#define NFA_MINOR    0
#define NFA_MAJOR    86
#define MAJOR_NR        86
#define MAX_PARTITIONS  4
#include <linux/config.h>
#include <linux/major.h>
#include <linux/malloc.h>
#define printf  printk
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifdef  DEBUG
#define PRINTK printk
#else
#define PRINTK
#endif

#include <linux/blk.h>
#include <asm/system.h>

#ifdef CONFIG_COLDFIRE
#include <asm/coldfire.h>
#include <asm/mcfsim.h>
//#ifdef CONFIG_SHARK
//#include <asm/shark.h>
//#endif
#ifdef CONFIG_SHARK_R1
#include <asm/shark_r1.h>
#endif
#endif
#endif /* BOOTLOADER_BUILD */

#define CF_LE_W(v) ((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))
#define CF_LE_L(v) (((unsigned)(v)>>24) | (((unsigned)(v)>>8)&0xff00) | (((unsigned)(v)<<8)&0xff0000) | ((unsigned)(v)<<24))
#define CT_LE_W(v) CF_LE_W(v)
#define CT_LE_L(v) CF_LE_L(v)

#define FAIL                    0
#define PASS	                1
#define ECCERROR                3
#define ECC_CORRECTED           4
#define NAND_CS_NONE            0xFF


#define MAX_CHIPS_IN_ARRAY      2

typedef struct {
  unsigned long  VirtualBlock;      /*  0-3  */
  unsigned char  DataStatus;        /*   4   */
  unsigned char  BlockStatus;       /*   5   */
  unsigned char  eccA_code[3];      /*  6-8  */
  unsigned char  eccB_code[3];      /*  9-11 */
  unsigned long  WriteCount;        /* 12-15 */
} rda_t;

typedef struct {
  unsigned long block;
  unsigned long writes;
} freeblock_t;

typedef struct {
  char  data[512];
  rda_t rda;
} page_t;

typedef struct {
  unsigned long Physical;
  unsigned long Virtual;
  int           Modified;
  int           Free;
  page_t        *Page;
} block_t;

typedef struct {
  int            ZapEnabled;
  int            verbosity;
  int            Chips;
  unsigned char  Mfg;
  unsigned char  Id;
  char           Manufacturer[24];
  /* Virtual Disk Parameters */
  unsigned short Cylinders;
  unsigned char  Heads;
  unsigned char  Sectors;
  unsigned long  TotalSectors;

  unsigned long  *Virt2Phys;
  freeblock_t    *FreePhys;

  unsigned long  FreeBlocks;

  unsigned char  ChipStatus;                  /* Last Chip Status Read */
  unsigned long  PagesPerBlock;               /* Number of Pages Per Block in Chip */
  unsigned long  PagesPerChip;                /* Number of Pages Per Chip */
  unsigned long  BlocksPerChip;               /* Number of Blocks Per Chip */
  unsigned long  BlockSize;                   /* Number of bytes per block */
  unsigned long  PageSize;                    /* Number of bytes per page */
  unsigned long  DataSize;                    /* Number of bytes in page data area */
  unsigned long  SpareSize;                   /* Number of bytes in page redundant area */
  unsigned long  PhysicalBlocksInArray;       /* Total Block in Chip Array */
  int            ChipMegs;                    /* Type of device */
  unsigned long  DiskSize;
  unsigned long  VirtualBlock;                /* Current Virtual Block (If non 0xFFFFFFFF ) */
  int            InUse;                       /* Thread Safe Lock */
  block_t        Block;
  rda_t          BlockInfo;                   /* Cache'ed Block Info */
} nfa_t;

nfa_t *nfa;

#define NOT_ALLOCATED       ((unsigned long)0xFFFFFFFFL)

#define PAGE_TO_CHIP(p)     ((p)/nfa->PagesPerChip) 
#define BLOCK_TO_CHIP(b)    ((b)/(nfa->BlocksPerChip))
#define PAGE_TO_BLOCK(p)    ((p)/nfa->PagesPerBlock)
#define PAGE_IN_BLOCK(p)    ((p)%nfa->PagesPerBlock)
#define BLOCK_TO_PAGE(b)    ((b)*nfa->PagesPerBlock)
#define BLOCK_IN_CHIP(b)    ((b)%nfa->BlocksPerChip)
#define PAGE_TO_VIRTADDR(p) ((p)*nfa->PageSize)
#define CHIP_MAX_ADDRESS()  (nfa->BlocksPerChip * nfa->BlockSize)

#define CHIP_ADDRESS(a)     ((BLOCK_TO_PAGE(BLOCK_IN_CHIP(PAGE_TO_BLOCK((a)))) + PAGE_IN_BLOCK((a)))*nfa->PageSize)

#define BAD_BLOCK()         ((nfa->BlockInfo.BlockStatus == (unsigned char)0xFF) ? 0 : 1)
#define VIRTUAL_BLOCK()     (nfa->BlockInfo.VirtualBlock)
#define WRITE_COUNT()       (nfa->BlockInfo.WriteCount)
#define FREE_PAGE()         ((nfa->BlockInfo.DataStatus == (unsigned char)0xFF) ? 1 : 0)
#define FREE_BLOCK()        ((nfa->BlockInfo.VirtualBlock == (unsigned long)0xFFFFFFFF) ? 1 : 0)


#ifdef DEBUG_INIT
#define DEBUG_MSG              printf
#else
#define DEBUG_MSG
#endif

#define ERROR_MSG              printf

#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READ3	        0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xD0
#define NAND_CMD_RESET		0xFF

#if defined(BIT_BANGED)
#define SYSLATCH_0             0x30000000;
#define SYSLATCH_1             0x30000004;

#define NAND                   *((volatile unsigned long  *)SYSLATCH_1)
#define NAND_CLE               (0x10000000)
#define NAND_ALE               (0x08000000)
#define NAND_WR                (0x01000000)
#define NAND_RD                (0x04000000)
#define NAND_WP                (0x02000000)
#define NAND_DATAMASK          (0x00FF0000)
#define NAND_CTRLMASK          (0xE0FFFFFF)
#define NAND_NCE_0             (0x20000000)
#define NAND_NCE_1             (0x40000000)
#define NAND_NCE_2             (0x80000000)
#define NAND_INIT              ( 0 | NAND_NCE_0 | NAND_NCE_1 | NAND_NCE_2 | NAND_WR | NAND_RD | NAND_WP )
#define NAND_MASK              (0x0000FFFF)

#define nfa_SetRD()          (*((volatile unsigned long  *)SYSLATCH_0) = (*((volatile unsigned long  *)SYSLATCH_0) & 0xFFFFF7FF))
#define nfa_SetWR()          (*((volatile unsigned long  *)SYSLATCH_0) = (*((volatile unsigned long  *)SYSLATCH_0) | 0x00000800))

#else // New 'Port' Style interface...

#define NAND_DATA              *((volatile unsigned char *)0x30000020)
#define NAND_CTRL              *((volatile unsigned char *)0x3000000E)

#define NAND_BUFWE             (0x80)
#define NAND_CLE               (0x40)
#define NAND_ALE               (0x20)
#define NAND_WP                (0x10)
#define NAND_CE(a)             ((a)&0x07)

#endif


/*
 * Pre-calculated 256-way 1 byte column parity
 */
static const unsigned char nand_ecc_precalc_table[] = {
  0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
  0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
  0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
  0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
  0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
  0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
  0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
  0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
  0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
  0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
  0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
  0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
  0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
  0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
  0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
  0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00
};


/*
 * Creates non-inverted ECC code from line parity
 */
void nand_trans_result(unsigned char reg2, unsigned char reg3, unsigned char *ecc_code)
{
  unsigned char a, b, i, tmp1, tmp2;
  
  /* Initialize variables */
  a = b = 0x80;
  tmp1 = tmp2 = 0;
  
  /* Calculate first ECC byte */
  for (i = 0; i < 4; i++) {
    if (reg3 & a)		/* LP15,13,11,9 --> ecc_code[0] */
      tmp1 |= b;
    b >>= 1;
    if (reg2 & a)		/* LP14,12,10,8 --> ecc_code[0] */
      tmp1 |= b;
    b >>= 1;
    a >>= 1;
  }
  
  /* Calculate second ECC byte */
  b = 0x80;
  for (i = 0; i < 4; i++) {
    if (reg3 & a)		/* LP7,5,3,1 --> ecc_code[1] */
      tmp2 |= b;
    b >>= 1;
    if (reg2 & a)		/* LP6,4,2,0 --> ecc_code[1] */
      tmp2 |= b;
    b >>= 1;
    a >>= 1;
  }
  
  /* Store two of the ECC bytes */
  ecc_code[0] = tmp1;
  ecc_code[1] = tmp2;
}
 
/*
 * Calculate 3 byte ECC code for 256 byte block
 */
void nand_calculate_ecc (const unsigned char *dat, unsigned char *ecc_code)
{
  unsigned char idx, reg1, reg2, reg3;
  int j;
  
  /* Initialize variables */
  reg1 = reg2 = reg3 = 0;
  ecc_code[0] = ecc_code[1] = ecc_code[2] = 0;
  
  /* Build up column parity */ 
  for(j = 0; j < 256; j++) {
    
    /* Get CP0 - CP5 from table */
    idx = nand_ecc_precalc_table[dat[j]];
    reg1 ^= (idx & 0x3f);
    
    /* All bit XOR = 1 ? */
    if (idx & 0x40) {
      reg3 ^= (unsigned char) j;
      reg2 ^= ~((unsigned char) j);
    }
  }
  
  /* Create non-inverted ECC code from line parity */
  nand_trans_result(reg2, reg3, ecc_code);
  
  /* Calculate final ECC code */
  ecc_code[0] = ~ecc_code[0];
  ecc_code[1] = ~ecc_code[1];
  ecc_code[2] = ((~reg1) << 2) | 0x03;
}

/*
 * Detect and correct a 1 bit error for 256 byte block
 */
int nand_correct_data (unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc)
{
  unsigned char a, b, c, d1, d2, d3, add, bit, i;
  
  /* Do error detection */ 
  d1 = calc_ecc[0] ^ read_ecc[0];
  d2 = calc_ecc[1] ^ read_ecc[1];
  d3 = calc_ecc[2] ^ read_ecc[2];
  
  if ((d1 | d2 | d3) == 0) {
    /* No errors */
    return 0;
  }
  else {
    a = (d1 ^ (d1 >> 1)) & 0x55;
    b = (d2 ^ (d2 >> 1)) & 0x55;
    c = (d3 ^ (d3 >> 1)) & 0x54;
    
    /* Found and will correct single bit error in the data */
    if ((a == 0x55) && (b == 0x55) && (c == 0x54)) {
      c = 0x80;
      add = 0;
      a = 0x80;
      for (i=0; i<4; i++) {
	if (d1 & c)
	  add |= a;
	c >>= 2;
	a >>= 1;
      }
      c = 0x80;
      for (i=0; i<4; i++) {
	if (d2 & c)
	  add |= a;
	c >>= 2;
	a >>= 1;
      }
      bit = 0;
      b = 0x04;
      c = 0x80;
      for (i=0; i<3; i++) {
	if (d3 & c)
	  bit |= b;
	c >>= 2;
	b >>= 1;
      }
      b = 0x01;
      a = dat[add];
      a ^= (b << bit);
      dat[add] = a;
      return add;
    }
    else {
      i = 0;
      while (d1) {
	if (d1 & 0x01)
	  ++i;
	d1 >>= 1;
      }
      while (d2) {
	if (d2 & 0x01)
	  ++i;
	d2 >>= 1;
      }
      while (d3) {
	if (d3 & 0x01)
	  ++i;
	d3 >>= 1;
      }
      if (i == 1) {
	/* ECC Code Error Correction */
	read_ecc[0] = calc_ecc[0];
	read_ecc[1] = calc_ecc[1];
	read_ecc[2] = calc_ecc[2];
      }
      else {
	/* Uncorrectable Error */
	return -1;
      }
    }
  }
  
  /* Should never happen */
  return -1;
}


static void nfa_ChipDelay( unsigned long delay ) 
{
  unsigned long i;
  /* Pause ~ 7us */
  for (i = 0; i < delay; i++) {}
}

static __inline__ void nfa_WriteChip(unsigned char x)
{
#if defined(BIT_BANGED)
  nfa_SetWR();
  NAND = ((NAND & ~NAND_DATAMASK) | (((unsigned long)(x)<<16) & NAND_DATAMASK));
  NAND &= ~NAND_WR;
  /*  nfa_ChipDelay(10); */
  NAND |=  NAND_WR;
  nfa_SetRD();
#else
  NAND_CTRL |= NAND_BUFWE;
  NAND_DATA = x;
#endif
}

static __inline__ unsigned char nfa_ReadChip(void)
{
  unsigned char val;
#if defined(BIT_BANGED)
  nfa_SetRD();
  NAND &= ~NAND_RD;
  /*  nfa_ChipDelay(10); */
  val = (unsigned char)((NAND & NAND_DATAMASK)>>16);
  NAND |=  NAND_RD;
#else
  NAND_CTRL &= ~NAND_BUFWE;
  val = NAND_DATA;
#endif
  return val;
}

static __inline__ void nfa_ChipSelect(nfa_t *nfa, int chip )
{
#if defined(BIT_BANGED)
  switch ( chip ) {
  case 0:
    NAND = ((NAND & ~NAND_NCE_0) | NAND_NCE_1 | NAND_NCE_2 );
    break;
  case 1:
    NAND = ((NAND & ~NAND_NCE_1) | NAND_NCE_0 | NAND_NCE_2 );
    break;
  case 2:
    NAND = ((NAND & ~NAND_NCE_2) | NAND_NCE_0 | NAND_NCE_1 );
    break;
  case 0xFF:
    NAND = (NAND | NAND_NCE_0 | NAND_NCE_1 | NAND_NCE_2 );
    break;
  }
#else
  if( chip > 1 ) {
    printf("NFA: ERROR, ChipSelect Invalid Chip %d !\n", chip );
    return;
  }
  NAND_CTRL = ((NAND_CTRL & 0xF8) | NAND_CE( (1<<chip) ));
#endif
}


static __inline__ void nfa_SetChipCLE(void)
{
#if defined(BIT_BANGED)
  NAND |=  NAND_CLE;
#else
  NAND_CTRL |= NAND_CLE;
#endif
}

static __inline__ void nfa_ClearChipCLE(void)
{
#if defined(BIT_BANGED)
  NAND &= ~NAND_CLE;
#else
  NAND_CTRL &= ~NAND_CLE;
#endif
}

static __inline__ void nfa_SetChipALE(void)
{
#if defined(BIT_BANGED)
  NAND |=  NAND_ALE;
#else
  NAND_CTRL |= NAND_ALE;
#endif
}

static __inline__ void nfa_ClearChipALE(void)
{
#if defined(BIT_BANGED)
  NAND &= ~NAND_ALE;
#else
  NAND_CTRL &= ~NAND_ALE;
#endif
}

static __inline__ void nfa_SetChipWP(void)
{
#if defined(BIT_BANGED)
  /* NAND &=  ~NAND_WP; JLS: NO WRITE PROTECT NEEDED FOR NOW  */
  NAND |=  NAND_WP;
#else
  NAND_CTRL |= NAND_WP;
#endif
}

static __inline__ void nfa_ClearChipWP(void)
{
#if defined(BIT_BANGED)
  NAND |=  NAND_WP;
#else
  NAND_CTRL |= NAND_WP;
#endif
}

#if 0
static __inline__ void nfa_WaitChipBusy(void)
{
  int x;
  for( x=0; x<20; x++);
}
#endif

static __inline__ int nfa_ChipFail( nfa_t *nfa )
{
  if( nfa->ChipStatus & 0x01 )
    return  1;
  else 			 
    return  0;
}

#if 0
static __inline__ int nfa_ChipWriteProtected( nfa_t *nfa )
{
  nfa_SetChipCLE();
  nfa_WriteChip( NAND_CMD_STATUS );
  nfa_ClearChipCLE();
  if ( nfa_ReadChip() & 0x80 )      
 if( nfa->ChipStatus & 0x80 )
   return  0;
 else 			 
   return  1;
}
#endif

static __inline__ int nfa_ChipReady( nfa_t *nfa )
{
  nfa_SetChipCLE();
  nfa_WriteChip( NAND_CMD_STATUS );
  nfa_ClearChipCLE();
  nfa->ChipStatus = nfa_ReadChip();
  if ( nfa->ChipStatus  & 0x40 )      
    return  1;
  else 			 
    return  0;
}

static __inline__ void nfa_ChipReset( nfa_t *nfa )                             
{
  nfa_SetChipCLE();
  /*Reset command (ffh)*/
  nfa_WriteChip( NAND_CMD_RESET );   
  nfa_ClearChipCLE();
  nfa_ChipDelay(200);		     
  /* Waiting for tRST */
  while( !nfa_ChipReady( nfa ) );
}

static __inline__ void nfa_ChipCommand( unsigned char cmd )
{
  nfa_SetChipCLE();
  nfa_WriteChip( cmd );
  nfa_ClearChipCLE();
}

static __inline__ void nfa_ChipAddress( nfa_t *nfa, unsigned long addr, unsigned char offset )
{
  unsigned long block;
  unsigned long page;

  union {
    unsigned long b32;
    unsigned char b8[4];
  } address;
  
 //==============================================================
 //     
 //                         +-+++------------ PAGE ADDRESS BITS
 //                         | |||
 //                         V VVV  
 //  ----0---- ----1---- ----2---- ----3----
 //  XXXX XXXX XXXX XXXX XXXx xxx0 0000 0000
 //  ^^^^ ^^^^ ^^^^ ^^^^ ^^^                
 //  |||| |||| |||| |||| |||
 //  +++++++++++++++++++++++----------------  BLOCK ADDRESS BITS
 //
 //==============================================================
 
  block = BLOCK_IN_CHIP(PAGE_TO_BLOCK( addr ));
  page  = PAGE_IN_BLOCK( addr );

 /* 
  * Address bits A13 and above are for BLOCK, 
  * A8-A12 for PAGE in BLOCK 
  */
  address.b32 = (0|(((block<<12)&0x003FF000)|((page<<8)&0x00000F00)));

  nfa_SetChipALE(); 
  /* Address bits 0-7 */
  nfa_WriteChip( offset ); 
  /* Address bits 9-16  */
  nfa_WriteChip( address.b8[2] );    
  /* Address bits 17-24  */
  nfa_WriteChip( address.b8[1] );    
  /* Address bits 25-31 */ 
  if(nfa->ChipMegs >= 64) 
    nfa_WriteChip( address.b8[0] ); 
  nfa_ClearChipALE();
}

static __inline__ void nfa_ChipBlockAddress( nfa_t *nfa, unsigned long addr )
{
  union {
    unsigned long b32;
    unsigned char b8[4];
  } address;
  
  //==============================================================
  //     
  //                         +-+++------------ PAGE ADDRESS BITS
  //                         | |||
  //                         V VVV  
  //  ----0---- ----1---- ----2---- ----3----
  //  XXXX XXXX XXXX XXXX XXXx xxxx xxxx xxxx
  //  ^^^^ ^^^^ ^^^^ ^^^^ ^^^                
  //  |||| |||| |||| |||| |||
  //  +++++++++++++++++++++++----------------  BLOCK ADDRESS BITS
  //
  //==============================================================


  address.b32 = BLOCK_IN_CHIP( addr );
                  
  /* Only A13 and UP are Valid for Block Addresses */
  address.b32 = ((address.b32<<12)&0x003FF000);

  nfa_SetChipALE();
  /* Address bits 9-16  */
  nfa_WriteChip( address.b8[2] );    
  /* Address bits 17-24  */
  nfa_WriteChip( address.b8[1] );    
  /* Address bits 25-31 */ 
  if(nfa->ChipMegs >= 64) 
    nfa_WriteChip( address.b8[0] ); 
  nfa_ClearChipALE();
}

static __inline__ void nfa_ChipRead( unsigned char *buffer, int len )
{
  while( len-- > 0 )
    *buffer++ = nfa_ReadChip();             
} 

static __inline__ void nfa_ChipWrite( unsigned char *buffer, int len )
{
  while( len-- > 0 )
    nfa_WriteChip( *buffer++ );             
}

static int nfa_ReadPageRda( nfa_t *nfa, unsigned long page, rda_t *rda )
{
  /* Make sure required chip selected */
  nfa_ChipSelect( nfa, PAGE_TO_CHIP(page) );
  /* Set Pointer to 'SPARE' array */
  nfa_ChipCommand( 0x50 );
  /* Write the page address */
  nfa_ChipAddress( nfa, page, 0); 
  /* Wait for READ to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Set Pointer to 'SPARE' array */
  nfa_ChipCommand( 0x50 );
  /* Read the data */
  nfa_ChipRead( (unsigned char *)rda, sizeof( rda_t ) );
  /* Negate the Chip Select */
  //  nfa_ChipSelect( nfa, NAND_CS_NONE ); 
  /* return o.k. */
  return PASS;
}

static int nfa_WritePageRda( nfa_t *nfa, unsigned long page, rda_t *rda )
{
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Make sure WP is disabled */
  nfa_ClearChipWP();
  /* Make sure required chip selected */
  nfa_ChipSelect( nfa, PAGE_TO_CHIP(page) );
  /* Set Pointer to 'SPARE' array */
  nfa_ChipCommand( 0x50 );
  /* Send command 80h (Sequential Data In) */
  nfa_ChipCommand( 0x80 );
  /* Write the page address */
  nfa_ChipAddress( nfa, page, 0);
  /* Write the data */
  nfa_ChipWrite( (unsigned char *)rda, sizeof( rda_t ) );
  /* Send command 10h (Page Program) */
  nfa_ChipCommand( 0x10 );
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Check For Page Program Failure */
  if( nfa_ChipFail( nfa ) ) {
    /* Negate the Chip Select */
    //    nfa_ChipSelect( nfa, NAND_CS_NONE );
    printf("NFA: ERROR, PageWriteRda Fail!\n");
    return FAIL;
  }
  /* Write Protect Chip */
  nfa_SetChipWP();
  /* Negate the Chip Select */
  //  nfa_ChipSelect( nfa, NAND_CS_NONE );
  /* return o.k. */
  return PASS;
}

int nfa_SetWriteCount( nfa_t *nfa, unsigned long page, unsigned long count )
{
  union {
    unsigned char b8[4];
    unsigned long b32;
  } writecount;

  /* break 1 32bit var into 4 8bit var's */
  writecount.b32 = count;
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Make sure WP is disabled */
  nfa_ClearChipWP();
  /* Make sure required chip selected */
  nfa_ChipSelect( nfa, PAGE_TO_CHIP(page) );
  /* Set Pointer to 'SPARE' array */
  nfa_ChipCommand( 0x50 );
  /* Send command 80h (Sequential Data In) */
  nfa_ChipCommand( 0x80 );
  /* Write the page address */
  nfa_ChipAddress( nfa, page, 12);
  /* Write the data */
  nfa_ChipWrite( writecount.b8, sizeof( unsigned long ) );
  /* Send command 10h (Page Program) */
  nfa_ChipCommand( 0x10 );
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Check For Page Program Failure */
  if( nfa_ChipFail( nfa ) ) {
    /* Negate the Chip Select */
    //    nfa_ChipSelect( nfa, NAND_CS_NONE );
    printf("NFA: ERROR, SetWriteCount Fail!\n");
    return FAIL;
  }
  /* Write Protect Chip */
  nfa_SetChipWP();
  /* Negate the Chip Select */
  //  nfa_ChipSelect( nfa, NAND_CS_NONE );
  /* return o.k. */
  return PASS;
}

static unsigned long nfa_PageWriteCount( nfa_t *nfa, unsigned long page )
{
  rda_t rda;
  nfa_ReadPageRda( nfa, page, &rda );
  return rda.WriteCount; 
}

static unsigned long nfa_BlockWriteCount( nfa_t *nfa, unsigned long block )
{
  unsigned long count;
#ifdef PARANOID
  unsigned long index;
  for( index = 0; index < nfa->PagesPerBlock; index++ ) {
    count = nfa_PageWriteCount( nfa,  BLOCK_TO_PAGE(block)+index );
    if ( count != NOT_ALLOCATED ) 
      break;
  }
#else
  count = nfa_PageWriteCount( nfa,  BLOCK_TO_PAGE(block) );
#endif
  return count;
}

/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_PageRead( nfa_t *nfa, unsigned char *page_buff, unsigned long page_address )                */
/*                                                                                                     */
/* Passed  : nfa, Pointer to Page Buffer[sizeof page], Physical Page Start Address in Array            */
/*                                                                                                     */
/* Returns : PASS | FAIL | ECC_CORRECTED                                                               */
/*                                                                                                     */
/* Notes   : This procedure utilizes the ECC correction methods. The passed buffer is filled with      */
/*           the complete page data, including the data area, and the redundant data area. All data    */
/*           in the page (data area) is passed though the ECC correct procudure, if bad                */
/*           bits are detected they will be repaired if possible.                                      */
/*                                                                                                     */
/*                                                                                                     */
/*=====================================================================================================*/
static int nfa_PageRead( nfa_t *nfa, unsigned char *buffer, unsigned long page )
{
  unsigned char eccA_calc[3]; 
  unsigned char eccB_calc[3]; 
  int ecc_result[2];
  page_t pageimg;

  /* Make sure required chip selected */
  nfa_ChipSelect( nfa, PAGE_TO_CHIP(page) );
  /* Set Pointer to 'DATA' array */
  nfa_ChipCommand( 0x00 );
  /* Write the page address */
  nfa_ChipAddress( nfa, page, 0); 
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Set Pointer to 'DATA' array */
  nfa_ChipCommand( 0x00 );
  /* Read the data */
  nfa_ChipRead( (unsigned char *)&pageimg.data[0], sizeof( page_t ) );
  /* Negate the Chip Select */
  //  nfa_ChipSelect( nfa, NAND_CS_NONE );
  /* Calculate the ECC */ 
  nand_calculate_ecc (&pageimg.data[0],   &eccA_calc[0]);
  nand_calculate_ecc (&pageimg.data[256], &eccB_calc[0]);
  /* Attempt to corect any bad bits.. */
  ecc_result[0] = nand_correct_data (&pageimg.data[0],   &pageimg.rda.eccA_code[0], &eccA_calc[0]);
  ecc_result[1] = nand_correct_data (&pageimg.data[256], &pageimg.rda.eccB_code[0], &eccB_calc[0]);
  /* Copy to caller's buffer */
  memcpy( buffer, &pageimg.data[0], nfa->DataSize );
  /* Handle bit failures if present */
  if ((ecc_result[0] == -1) || (ecc_result[1] == -1)) 
    return FAIL;
  if( ecc_result[0] || ecc_result[0] )
    return ECC_CORRECTED;
  /* return o.k. */
  return PASS; 
}

static void nfa_MarkBadPage( nfa_t *nfa, unsigned long page, unsigned char bs )
{
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Make sure WP is disabled */
  nfa_ClearChipWP();
  /* Make sure required chip selected */
  nfa_ChipSelect( nfa, PAGE_TO_CHIP(page) );
  /* Set Pointer to 'SPARE' array */
  nfa_ChipCommand( 0x50 );
  /* Send command 80h (Sequential Data In) */
  nfa_ChipCommand( 0x80 );
  /* Write the page address */
  nfa_ChipAddress( nfa, page, 6);
  /* Write the data */
  nfa_WriteChip( bs );
  /* Send command 10h (Page Program) */
  nfa_ChipCommand( 0x10 );
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Write Protect Chip */
  nfa_SetChipWP();
  /* Negate the Chip Select */
  //  nfa_ChipSelect( nfa, NAND_CS_NONE );
}

/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_MarkBadBlock( nfa_t *nfa, unsigned long block_address )                                     */
/*                                                                                                     */
/* Passed  : nfa, Physical block address                                                               */
/*                                                                                                     */
/* Returns : PASS | FAIL                                                                               */
/*                                                                                                     */
/* Notes   : This procedure marks a block bad by writting 0x00 into each page's redundant area's bad-  */
/*           block marker.                                                                             */
/*                                                                                                     */
/*=====================================================================================================*/
static void nfa_MarkBadBlock( nfa_t *nfa, unsigned long block )
{
  unsigned long index;
  /* 
   * Note: SAMSUNG & TOSHIBA Specifically state that either the
   *       1st or 2nd page in a BAD block *WILL* contain a non-0xFF
   *       value in the 6th byte of the spare array. For our use
   *       I have decided to write ALL pages in the block...
   */
  for( index = 0; index < nfa->PagesPerBlock; index++ ) {
    nfa_MarkBadPage( nfa, BLOCK_TO_PAGE(block)+index, 0xF0 );
  }
}

/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_PageWrite( nfa_t *nfa, unsigned char *page_buff, unsigned long page_address )               */
/*                                                                                                     */
/* Passed  : nfa, Pointer to Page Buffer[sizeof page], Physical Page Start Address in Array            */
/*                                                                                                     */
/* Returns : PASS | FAIL | ECC_CORRECTED                                                               */
/*                                                                                                     */
/* Notes   : This procedure utilizes the ECC correction methods. The passed buffer data area is        */
/*           scanned and an ECC is generated for the data. The ECC will be inserted into the redundant */
/*           area placeholder(s) before the page is actually written.                                  */
/*                                                                                                     */
/*=====================================================================================================*/
static int nfa_PageWrite( nfa_t *nfa, unsigned char *buffer, unsigned long page, unsigned long virtblk )
{
  page_t pageimg;
  /* Make sure WP is disabled */
  nfa_ClearChipWP();
  /* Set the RDA to reflect current status */
  pageimg.rda.VirtualBlock = virtblk;
  pageimg.rda.DataStatus   = 0xA5;
  pageimg.rda.BlockStatus  = 0xFF;
  pageimg.rda.WriteCount   = NOT_ALLOCATED;
  /* Copy user data to page image struct */
  memcpy( (char *)&pageimg.data[0], buffer, nfa->DataSize );
  /* Calculate the ECC */
  nand_calculate_ecc (&pageimg.data[0],   &pageimg.rda.eccA_code[0]);
  nand_calculate_ecc (&pageimg.data[256], &pageimg.rda.eccB_code[0]);
  /* Make sure required chip selected */
  nfa_ChipSelect( nfa, PAGE_TO_CHIP(page) );
  /* Set Pointer to 'DATA' array */
  nfa_ChipCommand( 0x00 );
  /* Send command 80h (Sequential Data In) */
  nfa_ChipCommand( 0x80 );
  /* Write the page address */
  nfa_ChipAddress( nfa, page, 0); 
  /* Write the data */
  nfa_ChipWrite( (unsigned char *)&pageimg.data[0], (sizeof( page_t ) - sizeof( unsigned long) ));
  /* Send command 10h (Page Program) */
  nfa_ChipCommand( 0x10 );
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );
  /* Check For Page Program Failure */
  if( nfa_ChipFail( nfa ) ) {
    /* Negate the Chip Select */
    //    nfa_ChipSelect( nfa, NAND_CS_NONE );
    printf("NFA: ERROR, PageWrite Fail!\n");
    return FAIL;
  }
  /* Write Protect Chip */
  nfa_SetChipWP();
  /* Negate the Chip Select */
  //  nfa_ChipSelect( nfa, NAND_CS_NONE );
  /* return o.k. */
  return PASS;
}

/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_BadBlock( nfa_t *nfa, unsigned long block_address )                                         */
/*                                                                                                     */
/* Passed  : nfa, Physical block address                                                               */
/*                                                                                                     */
/* Returns : non zero on BAD block                                                                     */
/*                                                                                                     */
/* Notes   : This procedure scans all pages in a block to see if it is a 'bad' block. Bad blocks       */
/*           are blocks whos redundant data area byte #6 are != 0xFF.                                  */
/*                                                                                                     */
/*=====================================================================================================*/
static int nfa_BadBlock( nfa_t *nfa, unsigned long block )
{
  rda_t rda;
  unsigned long index;
#ifdef PARANOID
  for( index = 0; index < nfa->PagesPerBlock; index++ ) {
#else
  /* 
   * Note: SAMSUNG & TOSHIBA Specifically state that either the
   *       1st or 2nd page in a BAD block *WILL* contain a non-0xFF
   *       value in the 6th byte of the spare array. This makes our
   *       job faster & less complex...
   */
  for( index = 0; index < 2; index++ ) {
#endif
    nfa_ReadPageRda( nfa,  BLOCK_TO_PAGE(block)+index, &rda );
    if( rda.BlockStatus != 0xFF ) {
      return 1;
    }
  }
  return 0;
}

/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_BlockInfo( nfa_t *nfa, unsigned long block_address )                                        */
/*                                                                                                     */
/* Passed  : nfa, Physical block address                                                               */
/*                                                                                                     */
/* Returns : N/A                                                                                       */
/*                                                                                                     */
/* Notes   : This procedure scans all pages in a block to see if it is a 'bad' block. Bad blocks       */
/*           are blocks whos redundant data area byte #6 are != 0xFF.                                  */
/*                                                                                                     */
/*=====================================================================================================*/
static void nfa_BlockInfo( nfa_t *nfa, unsigned long block )
{
  rda_t rda[16];
  unsigned long index;
#ifdef PARANOID
  for( index = 0; index < nfa->PagesPerBlock; index++ ) {
#else
  /* 
   * Note: SAMSUNG & TOSHIBA Specifically state that either the
   *       1st or 2nd page in a BAD block *WILL* contain a non-0xFF
   *       value in the 6th byte of the spare array. This makes our
   *       job faster & less complex...
   */
  for( index = 0; index < 2; index++ ) {
#endif
    nfa_ReadPageRda( nfa,  BLOCK_TO_PAGE(block)+index, &rda[index] );
    if( rda[index].BlockStatus != 0xFF ) {
      memcpy( (char *)&nfa->BlockInfo, (char *)&rda[index], sizeof( rda_t ));
      return;
    }
  }
  memcpy( (char *)&nfa->BlockInfo, (char *)&rda[0], sizeof( rda_t ));
}

/*=====================================================================================================*/
/*                                                                                                     */
/* unsigned long nfa_FetchPageVirtualBlock( nfa_t *nfa, unsigned long page_address )                   */
/*                                                                                                     */
/* Passed  : nfa,                                                                                      */
/*                                                                                                     */
/* Returns :                                                                                           */
/*                                                                                                     */
/* Notes   :                                                                                           */
/*                                                                                                     */
/*=====================================================================================================*/
static unsigned long nfa_FetchPageVirtualBlock( nfa_t *nfa, unsigned long page )
{
  rda_t rda;
  nfa_ReadPageRda( nfa, page, &rda );
  return rda.VirtualBlock; 
}

/*=====================================================================================================*/
/*                                                                                                     */
/* unsigned long nfa_FetchBlockVirtualBlock( nfa_t *nfa, unsigned long block_address )                 */
/*                                                                                                     */
/* Passed  : nfa, Physical block address                                                               */
/*                                                                                                     */
/* Returns :                                                                                           */
/*                                                                                                     */
/* Notes   :                                                                                           */
/*                                                                                                     */
/*                                                                                                     */
/*=====================================================================================================*/
static unsigned long nfa_FetchBlockVirtualBlock( nfa_t *nfa, unsigned long block )
{
  unsigned long address;
#ifdef PARANOID
  unsigned long index;
  for( index = 0; index < nfa->PagesPerBlock; index++ ) {
    address = nfa_FetchPageVirtualBlock( nfa,  BLOCK_TO_PAGE(block)+index );
    if ( address != NOT_ALLOCATED ) {
      return address;
    }
  }
#else
  address = nfa_FetchPageVirtualBlock( nfa,  BLOCK_TO_PAGE(block) );
  if ( address != NOT_ALLOCATED ) 
    return address;
  else
    return NOT_ALLOCATED;
#endif
}

/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_EraseBlock( nfa_t *nfa, unsigned long block_address )                                       */
/*                                                                                                     */
/* Passed  : nfa, Physical block address                                                               */
/*                                                                                                     */
/* Returns : non-zero on sucess                                                                        */
/*                                                                                                     */
/* Notes   : This procedure erases a complete block in the array. No makring is performed on the       */
/*           redundant data area.                                                                      */
/*                                                                                                     */
/*=====================================================================================================*/
static int nfa_EraseBlock( nfa_t *nfa, unsigned long block )
{
  unsigned long index;
  rda_t rda;

  /* 
   * Make sure we DONT erase a BAD block !
   */
  if( !nfa->ZapEnabled ) {
    /* Zap the RDA buffer */
    rda.VirtualBlock = NOT_ALLOCATED;
    rda.DataStatus   = 0xFF;
    rda.BlockStatus  = 0xFF;
    rda.eccA_code[0] = 0xFF;    
    rda.eccA_code[1] = 0xFF;    
    rda.eccA_code[2] = 0xFF;    
    rda.eccB_code[0] = 0xFF;    
    rda.eccB_code[1] = 0xFF;    
    rda.eccB_code[2] = 0xFF;    
    /* if block is already marked BAD, skip with 'NOT-FAIL' */
    if( nfa_BadBlock( nfa, block ) )
      return PASS;
    /* Determine howmany times this block has been written */
    rda.WriteCount = nfa_BlockWriteCount( nfa, block ) + 1;
  }
  
  /* Make sure WP is disabled */
  nfa_ClearChipWP();
  /* Make sure required chip selected */
  nfa_ChipSelect( nfa, BLOCK_TO_CHIP(block) );
  /* Send command 60h (Erase Block) */
  nfa_ChipCommand( 0x60 );
  /* Write the page address */
  nfa_ChipBlockAddress( nfa, block ); 
  /* 180 Roughly aproximates to 10uS */
  //  nfa_ChipDelay( 180 );
  /* Send command D0h (Erase Confirm) */
  nfa_ChipCommand( 0xD0 );
  /* 180 Roughly aproximates to 10uS */
  //  nfa_ChipDelay( 180 );
  /* Wait for Write to complete */
  while( !nfa_ChipReady( nfa ) );

  /* Check For Page Program Failure */
  if( nfa_ChipFail( nfa ) ) {
    /* Negate the Chip Select */
    //    nfa_ChipSelect( nfa, NAND_CS_NONE );
    printf("NFA: ERROR, EraseBlock Fail!\n");
    return FAIL;
  }

  if( !nfa->ZapEnabled ) {
    /* Set all pages the same */
    for( index = 0; index < nfa->PagesPerBlock; index++ ) {
      if( nfa_WritePageRda( nfa,  BLOCK_TO_PAGE(block)+index, &rda ) == FAIL ) {
	printf("FAILED TO SET INITIAL RDA INFO !\n");
	return FAIL;
      }
    }
  }

  /* Negate the Chip Select */
  //  nfa_ChipSelect( nfa, NAND_CS_NONE );

  /* Write Protect Chip */
  nfa_SetChipWP();
  /* return o.k. */ 
  return PASS;
}


/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_AllocateBlock( nfa_t *nfa, unsigned long virtual_block )                                    */
/*                                                                                                     */
/* Passed  : nfa, virtual_block                                                                        */
/*                                                                                                     */
/* Returns : non-zero on sucsess                                                                       */
/*                                                                                                     */
/* Notes   :                                                                                           */
/*                                                                                                     */
/*=====================================================================================================*/
static unsigned long nfa_AllocateBlock( nfa_t *nfa, unsigned long virtual_block )
{
  unsigned long PhysicalBlock = 0;
  /*
   * NOTE: This is an implementation of a FIFO list of Free Block
   *       management. The list *should* be loadded such that the
   *       blocks with the least number of writes are at the top.
   */

  /* Handle no free blocks... */
  if ( !nfa->FreeBlocks )
    return NOT_ALLOCATED;
  /* Grab the top FREE item in the list */
  PhysicalBlock = nfa->FreePhys[0].block;
  /* Reduce the FREE count by 1 */
  nfa->FreeBlocks--;
  /* Scoot the contents of the list up by one slot */
  memcpy( (unsigned char*)&nfa->FreePhys[0].block, (unsigned char*)&nfa->FreePhys[1].block, sizeof( unsigned long ) * nfa->FreeBlocks );
  /* Record Allocated block in Virt2Phys Table */
  nfa->Virt2Phys[ virtual_block ] = PhysicalBlock;
  /* Set our idea of the current VIRTUAL block */
  nfa->VirtualBlock = virtual_block;
  /* return the PHYSICAL block */
  return PhysicalBlock;
}


static void nfa_RecordFreeBlock( nfa_t *nfa, unsigned long block, unsigned long writes )
{
  unsigned long index;
  unsigned long from, to;

  for( index=0; index < nfa->FreeBlocks; index++ ) {
    /* IF the indexed block has been written more times, insert here... */
    if( writes < nfa->FreePhys[index].writes ) {

      /* We copy from the end, moving to the head of the list */
      from = nfa->FreeBlocks-1;
      to   = nfa->FreeBlocks-2;

      /* Scoot the contents of the list down by one slot */
      while( from > index ) {
	nfa->FreePhys[to].block  = nfa->FreePhys[from].block;
	nfa->FreePhys[to].writes = nfa->FreePhys[from].writes;
	/* Backup "UP" one slot */
	from--;
	to--;
      }
      /* Record the BLOCK */
      nfa->FreePhys[index].block  = block;
      /* Record the number of WTITES */
      nfa->FreePhys[index].writes = writes;
      /* Increment FREE count */
      nfa->FreeBlocks++;
      /* return */
      return;
    }
  }
  /*
   * IF we reach this point, we know that 
   * the current block has MORE write cycles than
   * any block in the list. Inset it at the END of the list.
   */
  /* Record the BLOCK */
  nfa->FreePhys[ nfa->FreeBlocks ].block  = block;
  /* Record the number of WTITES */
  nfa->FreePhys[ nfa->FreeBlocks ].writes = writes;
  /* Increment FREE count */
  nfa->FreeBlocks++;
}


/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_CreateVirt2PhysTable( nfa_t *nfa )                                                          */
/*                                                                                                     */
/* Passed  : nfa                                                                                       */
/*                                                                                                     */
/* Returns : non-zero on sucsess                                                                       */
/*                                                                                                     */
/* Notes   :                                                                                           */
/*                                                                                                     */
/*=====================================================================================================*/
static int nfa_CreateVirt2PhysTable( nfa_t *nfa ) 
{ 
  unsigned long block; 

  nfa->FreeBlocks = 0;

  /* Init the Virt2Phys Array as completely Unassigned */
  for ( block = 0; block < nfa->PhysicalBlocksInArray; block++ ) 
    nfa->Virt2Phys[ block ] = NOT_ALLOCATED;

  /* Init the FreePhys Array as completely Unassigned */
  for ( block = 0; block < nfa->PhysicalBlocksInArray; block++ ) {
    nfa->FreePhys[ block ].block  = NOT_ALLOCATED;
    nfa->FreePhys[ block ].writes = NOT_ALLOCATED;
  }

  block = 0;

  /* Scan though all physical blocks. */
  while ( block < nfa->PhysicalBlocksInArray ) {

    /* Read the status of the current block into the BlockInfo cache */
    nfa_BlockInfo( nfa, block );

    /* Skip all BAD Blocks */
    if( BAD_BLOCK() ) {
      block++;
      continue;
    }

    /* Handle Free Blocks by adding to Free Block Table */
    if( FREE_BLOCK() ) {
      nfa_RecordFreeBlock( nfa, block++, WRITE_COUNT() );
      continue;
    }

    /* Handle insane Virtual Address */
    if( VIRTUAL_BLOCK() > (nfa->PhysicalBlocksInArray-1) ) {
      ERROR_MSG("nfa: ERROR, Invalid virtual block, aborting.\n");
      return FAIL;
    }

    /* Handle Redundant Blocks (Power fail in last run cycle ?) */
    if( nfa->Virt2Phys[ VIRTUAL_BLOCK() ] != NOT_ALLOCATED ) {
      ERROR_MSG("nfa: ERROR, Redundant virtual block, aborting. T[0x%08lX] C[0x%08lX]\n",nfa->Virt2Phys[ VIRTUAL_BLOCK() ], VIRTUAL_BLOCK() );

      block++;
      continue;

      return FAIL;
    }

    /* Handle Duplicate Blocks (Power fail in last run cycle ?) */
    if( nfa->Virt2Phys[ VIRTUAL_BLOCK() ] == block ) {
      ERROR_MSG("nfa: ERROR, Duplicate virtual block, aborting.\n");
      return FAIL;
    }

    /* 
     * IF we get this far we know the following:
     * a. The block is good.
     * b. The Block has a sane virtual block number.
     * c. The block is not a redundant block.
     *
     * Therefore it is safe to ad it to the table.
     */
    nfa->Virt2Phys[ VIRTUAL_BLOCK() ] = block++; 
    continue;
  }
  return PASS;
}



static int nfa_CacheLoad( nfa_t *nfa, unsigned long block )
{
  unsigned long page;
  unsigned long index;

  nfa->Block.Free     = 1;
  page = BLOCK_TO_PAGE( block );
  nfa->Block.Physical = block;
  nfa->Block.Virtual = nfa_FetchBlockVirtualBlock( nfa, block );

  for(index = 0; index < nfa->PagesPerBlock; index++ ) {
    if( nfa_PageRead( nfa, &nfa->Block.Page[index].data[0], page++ ) == FAIL ) {
      ERROR_MSG("nfa: ERROR, CacheLoad: Page Failed to load, Chip: %ld Block: %ld Page: %ld\n", BLOCK_TO_CHIP(block), BLOCK_IN_CHIP(block), page-1); 
      return FAIL;
    }
  }
  nfa->Block.Free     = 0;
  nfa->Block.Modified = 0;
  return PASS;
}

static int nfa_CacheFlush( nfa_t *nfa )
{
  unsigned long temp;
  unsigned long page;
  unsigned long index;

  if( !nfa->Block.Modified ) {
    return PASS;
  }

  /* Do we need a Physical Block ? */
  if( nfa->Block.Physical == NOT_ALLOCATED ) {
    nfa->Block.Physical = nfa_AllocateBlock( nfa, nfa->Block.Virtual );
    if( nfa->Block.Physical == NOT_ALLOCATED ) {
      ERROR_MSG("nfa: ERROR, CacheFlush: Failed to allocate a block [0x%08lX]\n", nfa->Block.Physical );
      return FAIL;
    }
  }
  else {
    /* Nope, We modified an existing block in the array... */
    temp = nfa_AllocateBlock( nfa, nfa->Block.Virtual );
    if( temp == NOT_ALLOCATED ) {
      ERROR_MSG("nfa: ERROR, CacheFlush: Failed to allocate a REPLACEMENT block [0x%08lX]\n", temp );
      return FAIL;
    }
    if( nfa_EraseBlock( nfa, nfa->Block.Physical ) == FAIL ) {
      ERROR_MSG("nfa: ERROR, CacheFlush: Failed to erase stale block %ld, Marked BAD!\n", nfa->Block.Physical );
      nfa_MarkBadBlock( nfa, nfa->Block.Physical );
    }
    else {
      nfa->FreePhys[ nfa->FreeBlocks++ ].block = nfa->Block.Physical;
    }
    nfa->Block.Physical = temp;
  }

  page = BLOCK_TO_PAGE( nfa->Block.Physical );

  for(index = 0; index < nfa->PagesPerBlock; index++ ) {
    if( nfa_PageWrite( nfa, &nfa->Block.Page[index].data[0], page++, nfa->Block.Virtual ) == FAIL ) {
      ERROR_MSG("nfa: ERROR, CacheFlush: Failed to write Chip: %ld Block: %ld Page: %ld\n", 
		BLOCK_TO_CHIP(nfa->Block.Physical), nfa->Block.Physical, page-1);
      return FAIL;
    }
  }

  nfa->Block.Free     = 1;
  nfa->Block.Modified = 0;
  return PASS;
}

static int nfa_CacheHit( nfa_t *nfa, unsigned long sector )
{
  if( nfa->Block.Virtual == PAGE_TO_BLOCK( sector ) ) 
    return 1;
  else
    return 0;
}

static int nfa_CacheRead( nfa_t *nfa, unsigned long page, void *buffer )
{
  unsigned char *buff = (unsigned char *)buffer;
  if( !nfa_CacheHit( nfa, page ) )
    return FAIL;
  memcpy( buff, &nfa->Block.Page[ PAGE_IN_BLOCK(page) ].data[0], nfa->DataSize );
  return PASS;
}

static int nfa_CacheWrite( nfa_t *nfa, unsigned long page, void *buffer )
{
  unsigned char *buff = (unsigned char *)buffer;
  memcpy( &nfa->Block.Page[PAGE_IN_BLOCK(page)].data[0], buff, nfa->DataSize );
  nfa->Block.Modified = 1;
  return PASS;
}


static int nfa_CacheModified( nfa_t *nfa )
{
  if( nfa->Block.Free )
    return 0;
  if( nfa->Block.Modified )
    return 1;
  else
    return 0;
}

static void nfa_CacheFree( nfa_t *nfa )
{
  /* Set all to FFh */
  memset( &nfa->Block.Page[0].data[0], 0xFF, nfa->PagesPerBlock * nfa->PageSize );
  nfa->Block.Modified  = 0;
  nfa->Block.Free      = 1;
  nfa->Block.Physical  = NOT_ALLOCATED;
  nfa->Block.Virtual   = NOT_ALLOCATED;
}

static int nfa_CacheInit( nfa_t *nfa, unsigned long sector )
{
  /* Flush cache if modified */
  if( nfa_CacheModified( nfa) ) {
    if( nfa_CacheFlush( nfa ) == FAIL )
      ERROR_MSG("nfa: ERROR, CacheInit: Init Sector %ld, Cannot FLUSH cached sector %ld, Aborting!\n", sector, nfa->Block.Virtual );
      return FAIL;
  }
  /* ZAP the whole thing to FFh and set flags to inactive & free */
  nfa_CacheFree( nfa );
  /* Set the Virtual Block Address */
  nfa->Block.Virtual = PAGE_TO_BLOCK( sector );
  /* Mark it as 'IN-USE' */
  nfa->Block.Free         = 0;
  /* Is there currently a Physical Sector ? */
  if( nfa->Virt2Phys[ nfa->Block.Virtual ] != NOT_ALLOCATED ) {
    if( nfa_CacheLoad( nfa, nfa->Virt2Phys[ nfa->Block.Virtual ] ) == FAIL ) {
      ERROR_MSG("nfa: ERROR, CacheInit: Failed to load Physical Block %ld, Aborting!\n", nfa->Virt2Phys[ nfa->Block.Virtual ] );
      return FAIL;
    }
  }
  else {
    nfa->Block.Physical = NOT_ALLOCATED;
  }
  /* Exit... */
  return PASS;
}


int nfa_SectorWrite( nfa_t *nfa, unsigned long sector, void *buffer )
{
  if( !nfa_CacheHit( nfa, sector ) ) {
    if( nfa_CacheModified( nfa ) ) {
      if( nfa_CacheFlush( nfa ) == FAIL ) {
	ERROR_MSG("nfa: ERROR, SectorWrite: Failed to flush cache, Aborting!\n");
	return FAIL;
      }
    }
    if( nfa_CacheInit( nfa, sector ) == FAIL ) {
      ERROR_MSG("nfa: ERROR, SectorWrite: ERROR, Cache miss, and cannot, init sector %ld cache, Aborting!\n", sector ); 
      return FAIL;
    }
  }
  return nfa_CacheWrite( nfa, sector, buffer );
}

int nfa_SectorRead( nfa_t *nfa, unsigned long sector, void *buffer )
{
  unsigned long page;
  unsigned long block;

  if( nfa_CacheHit( nfa, sector ) ) {
    return nfa_CacheRead( nfa, sector, buffer );
  }
  else {
    if( nfa->Block.Free ) {
      if( nfa->Virt2Phys[ PAGE_TO_BLOCK( sector) ] != NOT_ALLOCATED ) {
	block = nfa->Virt2Phys[ PAGE_TO_BLOCK(sector) ];
	if( nfa_CacheLoad( nfa, block ) == FAIL ) {
	  ERROR_MSG("nfa: ERROR, SectorRead: Failed to load cache...\n");
	  return FAIL;
	}
	return nfa_CacheRead( nfa, sector, buffer );
      }
    }
  }

  if( nfa->Virt2Phys[ PAGE_TO_BLOCK( sector) ] != NOT_ALLOCATED ) {
    block = nfa->Virt2Phys[ PAGE_TO_BLOCK(sector) ];
    /* Add to the Physical address the page offset */
    page = (block * nfa->PagesPerBlock) + PAGE_IN_BLOCK(sector);
    if( nfa_PageRead( nfa, buffer, page ) == FAIL ) {
      ERROR_MSG("nfa: ERROR, Cannot read physical page [0x%08lX]\n", page);
      return FAIL;
    }
    return PASS;
  }
  else {
    memset( (char *)buffer, 0x00, nfa->DataSize );
    return PASS;
  }
}


#ifdef BOOTLOADER_BUILD  


#if 0
/*
 * DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER 
 *
 * This procedure does somthing ALL the NAND Flash Chip manufatures say to 
 * NEVER do... It Erases ALL chips reguardless of the BAD BLOCK markers that
 * may be present from the factory. I utilized this code in testing with
 * parts that DID HAVE bad blocks to check for error recovery. It would be
 * very stupid to use this procedure unless you are willing to de-solder
 * the chips afterwords, and THROW THEM AWAY!!!!
 *
 * DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER DANGER 
 */
int nfa_Zap( void )
{
  unsigned long VirtualBlocks;
  unsigned long PhysicalAddress;
  unsigned long block;
  int bad = 0;

  nfa->ZapEnabled  = 1;

  for( block = 0; block < nfa->PhysicalBlocksInArray; block++ ) {
    nfa_EraseBlock( nfa, block );
  }
  nfa->ZapEnabled = 0;
  return 0xDEADBEEF;
}
#endif


int nfa_Erase( void )
{
  unsigned long block;

  /* Loop though all blocks in array */
  for( block=0; block <  nfa->PhysicalBlocksInArray; block++ ) {
    /* Erase all known GOOD blocks only */
    if( nfa_EraseBlock( nfa, block ) == FAIL ) {
      /* Erase FAIL is condtion for marking block BAD */
      nfa_MarkBadBlock( nfa, block );
    }
  }
  /* No current Vitrual Block */
  nfa->VirtualBlock = NOT_ALLOCATED;
  /* Build the Lookup Tables */
  return nfa_CreateVirt2PhysTable( nfa );
}

int nfa_Read( unsigned long sector, void *buffer, int cnt )
{
  unsigned long xfercnt = 0;
  unsigned char *buff = (unsigned char *)buffer;

  while( cnt-- > 0 ) {
    if( nfa_SectorRead( nfa, sector++, &buff[xfercnt] ) == FAIL ) {
      return xfercnt = 0;
      break;
    }
    xfercnt += nfa->DataSize;
  }
  return xfercnt;
}


#ifdef TESTING
int nfa_Dump( unsigned long sector )
{
  int x;
  int y;
  unsigned char buffer[528];
  int status;

  printf("Requested Physical Sector %ld translates to:\n", sector );
  printf("Chip    : %ld\n", PAGE_TO_CHIP(sector) );
  printf("Block   : %ld\n", BLOCK_IN_CHIP(PAGE_TO_BLOCK(sector)) );
  printf("Page    : %ld\n", PAGE_IN_BLOCK(sector) );
  printf("Address : %ld\n", CHIP_ADDRESS( sector) );

  status = nfa_PageRead( nfa, buffer, sector);

  if( status == FAIL )
    printf("Page Read FAILED\n" );
  else if ( status == PASS )
    printf("Page Read PASSED\n" ); 
  else
    printf("Page Read Status unknown!: %d\n", status );

  for(x=0;x<nfa->DataSize;x+=16) {
    printf("\n0x%04X    ", x);
    for(y=0;y<16;y++) {
      printf("%02X ", (unsigned char)buffer[x+y] );
    }
    printf("  ");
    for(y=0;y<16;y++) {
      if ((((unsigned char)buffer[x+y] >= (unsigned char)0x20) && ((unsigned char)buffer[x+y] <= (unsigned char)0x7F)))
	printf("%c", buffer[x+y]);
      else   
	printf(".");
    }
  }
  printf("\n");
  return status;
}

int nfa_Rda( unsigned long sector )
{
  int x;
  int y;

  union {
    rda_t rda;
    unsigned char b8[16];
  } buffer;

  int status;

  

  printf("Requested Physical Sector %ld translates to:\n", sector );
  printf("Chip    : %ld\n", PAGE_TO_CHIP(sector) );
  printf("Block   : %ld\n", BLOCK_IN_CHIP(PAGE_TO_BLOCK(sector)) );
  printf("Page    : %ld\n", PAGE_IN_BLOCK(sector) );
  printf("Address : %ld\n", CHIP_ADDRESS( sector) );

  status = nfa_ReadPageRda( nfa, sector, &buffer.rda );

  if( status == FAIL )
    printf("RDA Read FAILED\n" );
  else if ( status == PASS )
    printf("RDA Read PASSED\n" );
  else
    printf("RDA Read Status unknown!: %d\n", status );

  for(x=0;x<nfa->SpareSize;x+=16) {
    printf("\n0x%04X    ", x);
    for(y=0;y<16;y++) {
      printf("%02X ", (unsigned char)buffer.b8[x+y] );
    }
    printf("  ");
    for(y=0;y<16;y++) {
      if ((((unsigned char)buffer.b8[x+y] >= (unsigned char)0x20) && ((unsigned char)buffer.b8[x+y] <= (unsigned char)0x7F)))
	printf("%c", buffer.b8[x+y]);
      else   
	printf(".");
    }
  }
  printf("\n");

  printf("Underwear Check: Page Size = %d, RDA Size = %d\n", sizeof( page_t), sizeof( rda_t ) );

  printf("Virtual Block :  0x%08lX\n", buffer.rda.VirtualBlock);
  printf("Data Status   :  0x%02X\n",  buffer.rda.DataStatus);
  printf("Block Status  :  0x%02X\n",  buffer.rda.BlockStatus);
  printf("ECC Array A   :  0x%02X 0x%02X 0x%02X\n", buffer.rda.eccA_code[0], buffer.rda.eccA_code[1], buffer.rda.eccA_code[2]);
  printf("ECC Array B   :  0x%02X 0x%02X 0x%02X\n", buffer.rda.eccB_code[0], buffer.rda.eccB_code[1], buffer.rda.eccB_code[2]);
  printf("WriteCount    :  0x%08lX\n", buffer.rda.WriteCount);

  printf("\n");

}
#endif /* TESTING */
#endif /* BOOTLOADER_BUILD */


/*=================================================================================*/
/* Chip Array Info:                                                                */
/*                                                                                 */
/* This procudure is called on module init to scan the array of NAND Flash chips   */
/* to determine the Manufacturer, Type, and Size of all the chips in the array.    */
/* It should be noted that at this time ALL chips in the array MUST be the same    */
/* part number, and that Manufacturer and Type CANNOT BE MIXED!.                   */
/*                                                                                 */
/*=================================================================================*/
static int nfa_ChipArrayInfo( nfa_t *nfa )
{
  //  unsigned short i;
  unsigned char mfg,id;
  int chips = 0;

  nfa->Chips = 0;

  nfa_ChipSelect( nfa, 0 );

  nfa_ChipReset( nfa );

  nfa_SetChipCLE();
  nfa_WriteChip( NAND_CMD_READID);   	/*Read ID command*/
  nfa_ClearChipCLE();
  /* Start ADDRESS cycle */
  nfa_SetChipALE();
  /* Address bits 0-7   */
  nfa_WriteChip( 0x00);
  /* End ADDRESS Cycle */
  nfa_ClearChipALE();

  nfa->Mfg = nfa_ReadChip();              /* Reading 2 byte data */
  nfa->Id  = nfa_ReadChip();


  //    printf("Manufacturer's code is %4x\n", nfa->Mfg);
  //    printf("Device code is %4x\n", nfa->Id);
  
  switch ( nfa->Mfg ) {
    
  case 0xEC:
    printf("SAMSUNG ");
    switch ( nfa->Id ) {
    case 0xA4:  printf("512 KB Nand Flash (3.3 V, 5 V)");  break;
    case 0x6e:  printf("  1 MB Widerange Nand Flash");     break;
    case 0xea:  printf("  2 MB Nand Flash (3.3 V)");       break;
    case 0x64:  printf("  2 MB Nand Flash (5 V)");         break;
    case 0xe3:  printf("  4 MB Nand Flash (3.3 V)");       break;
    case 0xe5:  printf("  4 MB Nand Flash (5 V)");         break;
    case 0xe6:  printf("  8 MB Nand Flash (U-ver)");       break;
    case 0x6d:  printf("  8 MB Nand Flash (5 V)");         break;
    case 0x73:  printf(" 16 MB Nand Flash (3.3 V)");       break;
    case 0x75:  printf(" 32 MB Nand Flash (3.3 V)");       break;
    case 0x76:  printf(" 64 MB Nand Flash (3.3 V)");       break;
    case 0x79:  printf("128 MB Nand Flash (3.3 V)");       break;
    default:    printf(" \nThe device cannot be identified.\n"); 
    }
    break;
    
  case 0x98:
    printf("Toshiba ");
    switch ( nfa->Id ) {
    case 0xec:  printf("  1 MB Nand Flash (3.3 V)");       break;
    case 0x6e:  printf("  1 MB Nand Flash (5 V )");        break;
    case 0xea:  printf("  2 MB Nand Flash (3.3 V)");       break;
    case 0x64:  printf("  2 MB Nand Flash (5 V)");         break;
    case 0xe5:  printf("  4 MB Nand Flash (3.3 V)");       break;
    case 0x6b:  printf("  4 MB Nand Flash (5 V)");         break;
    case 0xe6:  printf("  8 MB Nand Flash (3.3V)");        break;
    case 0x6d:  printf("  8 MB Nand Flash (5 V)");         break;
    case 0x73:  printf(" 16 MB Nand Flash (3.3 V)");       break;
    case 0x75:  printf(" 32 MB Nand Flash (3.3 V)");       break;
    case 0x76:  printf(" 64 MB Nand Flash (3.3 V)");       break;
    case 0x79:  printf("128 MB Nand Flash (3.3 V)");       break;
    default:    printf(" \nThe device cannot be identified..\n");
    }
    break;
  }


  /* 
   * Handle Chip Database Info...
   */
  switch (nfa->Id ) {
  case 0xA4:
    break;
 
  case 0x6E:
  case 0xEC:
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 1;          /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 256;        /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 16;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 264;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 256;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 8;          /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 8;          /* sector count	 value: sectors per block */
    break;
    
  case 0xEA:
  case 0x64:
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 2;          /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 512;        /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 16;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 264;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 256;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 8;          /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 8;          /* sector count	 value: sectors per block */
    break;
    
  case 0xE3:
  case 0xE5:
  case 0x6B:
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 4;          /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 512;        /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 16;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 528;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 512;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 16;         /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 16;         /* sector count	 value: sectors per block */
    break;

  case 0xE6:
  case 0x6D:
  case 0x70:	
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 8;          /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 1024;       /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 16;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 528;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 512;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 16;         /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 16;         /* sector count	 value: sectors per block */
    break;

  case 0x73:
  case 0xF2:
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 16;         /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 1024;       /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 32;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 528;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 512;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 16;         /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 32;         /* sector count	 value: sectors per block */
    break;

  case 0xF4:
  case 0x75:
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 32;         /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 2048;       /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 32;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 528;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 512;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 16;         /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 32;         /* sector count	 value: sectors per block */
    break;

  case 0xF7:
  case 0x76:
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 64;         /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 4096;       /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 32;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 528;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 512;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 16;         /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 32;         /* sector count	 value: sectors per block */
    break;

  case 0xF8:
  case 0x79:
    nfa->Chips         = 1;          /* 1st Chip Located */
    nfa->ChipMegs      = 128;        /* device_type	 1:1M, 2:2M, 4:4M, 8:8M, 16:16M, 32:32M ...bytes */
    nfa->BlocksPerChip = 8192;       /* blocks_per_chip	 value: blocks in a disk */
    nfa->PagesPerBlock = 32;         /* pages_block	 value: pages in a block */
    nfa->PageSize      = 528;        /* page_size	 value: page_size in bytes */
    nfa->DataSize      = 512;        /* data_size	 value: data_size in bytes */
    nfa->SpareSize     = 16;         /* spare_size	 value: spare_size in bytes */
    nfa->PagesPerBlock = 32;         /* sector count	 value: sectors per block */
    break;
  }


  /* 
   * Scan for addtional chips in the array.
   */
  if( nfa->Chips > 0 ) {

    for( chips = 1; chips < MAX_CHIPS_IN_ARRAY; chips++ ) {

      nfa_ChipSelect( nfa, chips);

      nfa_ChipReset( nfa );

      nfa_SetChipCLE();
      /*Read ID command*/
      nfa_WriteChip( NAND_CMD_READID);   	
      nfa_ClearChipCLE();

      /* Start ADDRESS cycle */
      nfa_SetChipALE();

      /* Address bits 0-7   */
      nfa_WriteChip( 0x00);

      /* End ADDRESS Cycle */
      nfa_ClearChipALE();

      mfg = nfa_ReadChip();            /* Reading 2 byte data */
      id  = nfa_ReadChip();

      if( (mfg == nfa->Mfg ) && (id == nfa->Id)) {
	nfa->Chips++;
      }
      else {
	break;
      }
    }

    /* Negate the Chip Selects */
    //    nfa_ChipSelect( nfa, NAND_CS_NONE );

    nfa->PagesPerChip          = nfa->PagesPerBlock * nfa->BlocksPerChip;
    nfa->BlockSize             = nfa->PagesPerBlock * nfa->PageSize;
    nfa->PhysicalBlocksInArray = nfa->BlocksPerChip * nfa->Chips;
    nfa->DiskSize              = nfa->PhysicalBlocksInArray * (nfa->PagesPerBlock * nfa->PageSize);

    /* Handle common array sizes */
    switch( (nfa->Chips * nfa->ChipMegs )) { 
    case 1:   /* Megabyte Disk */
      nfa->Cylinders     = 125L;
      nfa->Heads         = 4L;
      nfa->Sectors       = 4L;
      break; 
    case 2:   /* Megabyte Disk */
      nfa->Cylinders     = 125L;
      nfa->Heads         = 4L;
      nfa->Sectors       = 8L;
      break; 
    case 4:   /* Megabyte Disk */
      nfa->Cylinders     = 250L;
      nfa->Heads         = 4L;
      nfa->Sectors       = 8L;
      break; 
    case 8:   /* Megabyte Disk */
      nfa->Cylinders     = 250L;
      nfa->Heads         = 4L;
      nfa->Sectors       = 16L;
      break; 
    case 16:  /* Megabyte Disk */
      nfa->Cylinders     = 500L;
      nfa->Heads         = 4L;
      nfa->Sectors       = 16L;
      break; 
    case 32:  /* Megabyte Disk */
      nfa->Cylinders     = 500L;
      nfa->Heads         = 8L;
      nfa->Sectors       = 16L;
      break; 
    case 64:  /* Megabyte Disk */
      nfa->Cylinders     = 500L;
      nfa->Heads         = 8L;
      nfa->Sectors       = 32L;
      break; 
    case 128: /* Megabyte Disk */
      nfa->Cylinders     = 500L;
      nfa->Heads         = 16L;
      nfa->Sectors       = 32L;
      break; 

    case  12: /* T.B.D. */  /* Megabyte Disk ( 3   4Meg Chips ) */
    case  24: /* T.B.D. */  /* Megabyte Disk ( 3   8Meg Chips ) */
    case  48: /* T.B.D. */  /* Megabyte Disk ( 3  16Meg Chips ) */
    case  96: /* T.B.D. */  /* Megabyte Disk ( 3  32Meg Chips ) */
    case 192: /* T.B.D. */  /* Megabyte Disk ( 3  64Meg Chips ) */
    case 384: /* T.B.D. */  /* Megabyte Disk ( 3 128Meg Chips ) */

    default: 
      nfa->Chips = 0; 
      break;
    } 
    nfa->TotalSectors  = nfa->PhysicalBlocksInArray * nfa->PagesPerBlock;
  }
  
  return ( nfa->Chips );
}


#ifdef BOOTLOADER_BUILD  

/*=====================================================================================================*/
/*                                                                                                     */
/* int nfa_DiskInit( void )                                                                         */
/*                                                                                                     */
/* Passed  : Nothing                                                                                   */
/*                                                                                                     */
/* Returns : TRUE | FALSE                                                                              */
/*                                                                                                     */
/* Notes   : This procedure is called by the BIOS / OS to init the SmartDisk Drive.                    */
/*                                                                                                     */
/*=====================================================================================================*/
int nfa_DiskInit( void )
{
  int chips;
  unsigned long VirtualBlocks;
  static int initialized = 0;

  if( initialized )
    return 0;

  initialized = 1;

#if !defined(BIT_BANGED)
  NAND_CTRL =  (NAND_WP | NAND_CE(0));
#endif

  nfa = (nfa_t *)malloc( sizeof( nfa_t ));
  
  if( !nfa ) {
    printf("ERROR: Cannot malloc memory for NAND Flash Array driver\n");
    return FALSE;
  }

  /* Prevent ERASE ALL action */
  nfa->ZapEnabled = 0;

  /* Set VERBOSE to on for Mfg & Chip ID Info Print */
  nfa->verbosity = 5;

  /* Scan the array for chips */
  chips = nfa_ChipArrayInfo( nfa );

  /* Allocate memory for Virt2Phys Lookup Table */
  nfa->Virt2Phys = (unsigned long *)malloc( (nfa->PhysicalBlocksInArray * sizeof( unsigned long )));
  if(!nfa->Virt2Phys) {
    printf("NFA: ERROR, Insuffician Memory, Aborting [001]\n");
    return 0;
  }

  /* Allocate memory for FreePhys Lookup Table */
  nfa->FreePhys = (freeblock_t *)malloc( (nfa->PhysicalBlocksInArray * sizeof( freeblock_t )));
  if(!nfa->FreePhys) {
    printf("NFA: ERROR, Insuffician Memory, Aborting [002]\n");
    return 0;
  }

  /* No current Vitrual Block */
  nfa->VirtualBlock = NOT_ALLOCATED;

  nfa->Block.Page = (page_t *)malloc((nfa->PagesPerBlock * sizeof( page_t )));
  if( !nfa->Block.Page ) {
    printf("NFA: ERROR, Not enough memory to buffer 1 block, Aborting [004]\n");
    return 0;
  }

  nfa->Block.Free        = 1;
  nfa->Block.Modified    = 0;
  nfa->Block.Virtual     = NOT_ALLOCATED;
  nfa->Block.Physical    = NOT_ALLOCATED;

  /* Prevent ERASE ALL action */ 
  nfa->ZapEnabled = 0;

  /* Set VERBOSE to on for Mfg & Chip ID Info Print */
  nfa->verbosity = 0;

  /* Build the Lookup Tables */
  if( nfa_CreateVirt2PhysTable( nfa ) == FAIL) {
    return -EBUSY;
  }
  
  printf("\nInternal NAND Flash Drive, %d Chips, Diskette Size : %d Mb, %d total blocks\n", 
	 (int)chips, 
	 (int)((nfa->DiskSize/1024)/1024), 
	 (int)nfa->PhysicalBlocksInArray);


  return TRUE;
}

#endif

#ifndef BOOTLOADER_BUILD

#include <linux/genhd.h>
#include <linux/hdreg.h>


#define CF_LE_W(v) ((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))
#define CF_LE_L(v) (((unsigned)(v)>>24) | (((unsigned)(v)>>8)&0xff00) | \
               (((unsigned)(v)<<8)&0xff0000) | ((unsigned)(v)<<24))
#define CT_LE_W(v) CF_LE_W(v)
#define CT_LE_L(v) CF_LE_L(v)

/* Each memory region corresponds to a minor device */
typedef struct {
	unsigned char boot_ind;		/* 0x80 - active */
	unsigned char head;		/* starting head */
	unsigned char sector;		/* starting sector */
	unsigned char cyl;		/* starting cylinder */
	unsigned char sys_ind;		/* What partition type */
	unsigned char end_head;		/* end head */
	unsigned char end_sector;	/* end sector */
	unsigned char end_cyl;		/* end cylinder */
	unsigned int start_sect;	/* starting sector counting from 0 */
	unsigned int nr_sects;		/* nr of sectors in partition */
} __attribute((packed)) partition_t;	/* Give a polite hint to egcs/alpha to generate
				   unaligned operations */

static partition_t nfa_partition[5];

#define PART_TAB_OFFSET         0x1BE
#define PARTITION_ACTIVE        0x80
#define copy_to_user memcpy
#define NFA_MAX_PARTITION  NFA_MAX_CHIP

struct hd_struct part[5];
int nfa_partsizes[5]  ={ 0, /* ... */ };

static int nfa_sizes[5];
static int nfa_blocksizes[5];

void dummy_init(struct gendisk *notused)
{
}


struct gendisk nfa_gendisk={
  NFA_MAJOR,           /* Major number */      
  "nfa",               /* Major name */
  0,                      /* Bits to shift to get real from partition */
  4,                      /* Number of partitions per real */
  1,                      /* maximum number of real */
  dummy_init,             /* init function */
  part,                 /* hd struct */
  nfa_partsizes,       /* block sizes */
  0,                      /* number */
  NULL,                   /* internal use, not presently used */
  NULL                    /* next */
};

static int nfa_ioctl(struct inode *inode, struct file *file,u_int cmd, u_long arg);
static int nfa_open(struct inode *inode, struct file *file);
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
//static int nfa_close(struct inode *inode, struct file *file);
//#else
static void nfa_close(struct inode *inode, struct file *file);
//#endif

struct file_operations nfa_blk_fops = {
  NULL,		/* lseek */
  block_read,	/* read */
  block_write,	/* write */
  NULL,		/* readdir */
  NULL,		/* select */
  nfa_ioctl,	/* ioctl */
  NULL,		/* mmap */
  nfa_open,	/* open */
  //#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
  //  NULL,         /* flush */
  //#endif    
  nfa_close,	/* release */
  block_fsync	/* fsync */
};


//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
//#include <asm/uaccess.h>  /* for get_user and put_user */
//#else
/* Segmentation stuff for pre-2.1 kernels */
#include <asm/segment.h>

#if 0
static inline int copy_to_user(void *dst, void *src, unsigned long len)
{
	int rv = verify_area(VERIFY_WRITE, dst, len);
	if ( rv )
		return -1;
	memcpy_tofs(dst,src,len);
	return 0;
}

static inline int copy_from_user(void *dst, void *src, unsigned long len)
{
	int rv = verify_area(VERIFY_READ, src, len);
	if ( rv )
		return -1;
	memcpy_fromfs(dst,src,len);
	return 0;
}
#endif
//#endif


static inline unsigned long le32_to_cpu(volatile unsigned long v)
{
    return (((v & 0x000000ff) << 24) | ((v & 0x0000ff00) << 8) | ((v & 0x00ff0000) >> 8) | ((v & 0xff000000) >> 24));
}

int nfa_ReadPartitionTable( nfa_t *nfa , partition_t *part)
{
  unsigned char bs[512];
  int index = 0;
  int partitions=0;

  /* Read Sector #0 (MBR) */
  if( nfa_SectorRead( nfa, 0L, bs) == FAIL) {
    printk("nfa: ERROR, Cannot Read Master Boot Record!\n");
    return 0;
  }

  part[index].start_sect = 0L; 
  part[index].nr_sects   =  nfa->TotalSectors;

  /* 
   * We loop over all of the 4 possible entries 
   */
  for(index=0; index < 4; index++) {
    memcpy( (char *)&part[1+index], &bs[PART_TAB_OFFSET+(index*16)], 16 );

    /* Adjust for Intel LittleEndian Platform */
    part[1+index].start_sect = le32_to_cpu( part[1+index].start_sect );
    part[1+index].nr_sects   = le32_to_cpu( part[1+index].nr_sects );

#if 0
    printk("nfa: Partition Table Slot %d, BF:%02X H:%d S:%d C:%d Sys:%02X, eH:%d eS:%d eC:%d StartSect:0x%08lX NrSects:0x%08lX\n",
	   index,
	   part[1+index].boot_ind,
	   part[1+index].head,
	   part[1+index].sector,
	   part[1+index].cyl,
	   part[1+index].sys_ind,
	   part[1+index].end_head,
	   part[1+index].end_sector,
	   part[1+index].end_cyl,
	   part[1+index].start_sect,
	   part[1+index].nr_sects );
#endif

    /* if the length is > 0, then it *must* be a partition */
    if( part[1+index].nr_sects > 0)
      partitions++;
  }    
  /* 
   * If we got here no partition is marked active, 
   * return FALSE 
   */
  return partitions;
}


static int nfa_open(struct inode *inode, struct file *file)
{
  int minor = MINOR(inode->i_rdev);
  int i;
 
  if ( nfa_gendisk.part[minor].nr_sects == 0) {
    printk("nfa: ERROR, Open Failed, minor = %d\n", minor );
    for(i = 0; i < 5; i++) {
      printk("nfa: Partition %d start_sect = %ld nr_sects = %ld\n", i, nfa_gendisk.part[i].start_sect, nfa_gendisk.part[i].nr_sects ); 
    }
    return -ENXIO;
  }
  return 0;
} /* nfa_open */

/*====================================================================*/
//#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
//static int nfa_close(struct inode *inode, struct file *file)
//#else
static void nfa_close(struct inode *inode, struct file *file)
//#endif
{
  //  int minor = MINOR(inode->i_rdev);
  /* Flush all writes */
  fsync_dev(inode->i_rdev);

  /* invalidate_inodes(inode->i_rdev);*/
  invalidate_buffers(inode->i_rdev);

  nfa_CacheFlush( nfa );

} /* nfa_close */

int nfa_read(partition_t *part, caddr_t buffer, u_long sector, u_long nblocks)
{
  u_long i;
  
  //  printk(KERN_NOTICE "nfa_cs: nfa_read(0x%lx, %ld)\n",sector, nblocks);
  
  for (i = 0; i < nblocks; i++) {
    if ((sector+i) >= (part->start_sect + part->nr_sects)) {
      printk(KERN_NOTICE "nfa_cs: bad read offset\n");
      return -EIO;
    }
    
    if( nfa_SectorRead( nfa, (sector+i), buffer ) == FAIL ) {
      printk("nfa: Page Read FAILED\n" );
      return -EIO;
    }
    buffer += nfa->DataSize;
  }
  return 0;
} /* nfa_read */

static int nfa_write(partition_t *part, caddr_t buffer, u_long sector, u_long nblocks)
{
  u_long i;

  for (i = 0; i < nblocks; i++)	{
    if ((sector+i) >= (part->start_sect + part->nr_sects)) {
      printk(KERN_NOTICE "nfa_cs: bad write offset\n");
      return -EIO;
    }
    if( nfa_SectorWrite( nfa, (sector+i), buffer) == FAIL) {
      printk(KERN_NOTICE "nfa_cs: block write failed!\n");
      return -EIO;
    }
    buffer += nfa->DataSize;
  }
  return 0;
} /* nfa_write */


static int nfa_ioctl(struct inode *inode, struct file *file,	u_int cmd, u_long arg)
{
  int minor = MINOR(inode->i_rdev);
  struct hd_geometry loc;
  int ret = 0;
  int i = 0;
  int part_count = 0;

  //  printk(KERN_NOTICE "nfa_ioctl\n");
 
  switch (cmd) {
  case HDIO_GETGEO:
    /* Make sure cache is flushed (may have JUST written MBR in FSISK */
    if( nfa_CacheFlush( nfa ) == FAIL ) {
      printf("NFA: ERROR, Cache Flush Fail!\n");
      return -EIO;
    }

    loc.heads     = nfa->Heads;
    loc.sectors   = nfa->Sectors;
    loc.cylinders = nfa->Cylinders;
    loc.start     = nfa_gendisk.part[minor].start_sect;
    if (copy_to_user((void *)arg, &loc, sizeof(loc)) < 0)
      ret = -EFAULT;
    break;
 
  case BLKGETSIZE:
    if( nfa_gendisk.part[minor].nr_sects < 1 ) {
      for (i = 0; i < 5; i++) {
	printk("nfa: nfa%d Start = %d Len = %d\n", i, nfa_partition[i].start_sect, nfa_partition[i].nr_sects );
      }
      return -EIO;
    }
    ret = verify_area(VERIFY_WRITE, (long *)arg, sizeof(long));
    if (!ret) 
      put_user(nfa_gendisk.part[minor].nr_sects, (long *)arg);
    break;

  case BLKFLSBUF:
    if (!suser()) 
      return -EACCES;
    if (!(inode->i_rdev)) 
      return -EINVAL;
    fsync_dev(inode->i_rdev);
    invalidate_buffers(inode->i_rdev);
    break;

  case BLKRRPART:
    /* Make sure cache is flushed (may have JUST written MBR in FSISK */
    if( nfa_CacheFlush( nfa ) == FAIL ) {
      printf("NFA: ERROR, Cache Flush Fail!\n");
      return -EIO;
    }

    /* Read direct from hardware */
    part_count = nfa_ReadPartitionTable( nfa , nfa_partition);

    for (ret=0; ret<5; ret++) {
      nfa_gendisk.part[minor+ret].nr_sects=0;
      nfa_gendisk.part[minor+ret].start_sect=0;
    }

    nfa_gendisk.part[i].start_sect  =  0L;
    nfa_gendisk.part[i].nr_sects    =  nfa->TotalSectors;

    for (i = 0; i < 4; i++) {
      nfa_gendisk.part[1+i].start_sect  =  nfa_partition[i].start_sect;
      nfa_gendisk.part[1+i].nr_sects    =  nfa_partition[i].nr_sects;
    }

    for (ret = 0; ret < 4; ret++) 
      nfa_sizes[minor + ret] = nfa_gendisk.part[minor+ret].nr_sects >> (BLOCK_SIZE_BITS - 9);

    ret = 0;
    break;
    
  default:
    ret = -EINVAL;
  }
  //  printk(KERN_NOTICE "nfa_ioctl done\n");
  return ret;
} /* nfa_ioctl */

/*======================================================================
  
  Handler for block device requests
  
  ======================================================================*/

void do_nfa_request(void)
{
  int ret, minor;
  unsigned long start;

  partition_t *part = NULL;
  
  do {
    //    sti();
    INIT_REQUEST;
    minor = MINOR(CURRENT->rq_dev);
#if 0
    printk("nfa: do_nfa_request: minor %d \n", minor);
#endif
    part=&nfa_partition[minor];  
#if 0
    printk("nfa: Partition nfa%d, BF:%02X H:%d S:%d C:%d Sys:%02X, eH:%d eS:%d eC:%d StartSect:0x%08lX NrSects:0x%08lX\n",
	   minor,
	   part->boot_ind,
	   part->head,
	   part->sector,
	   part->cyl,
	   part->sys_ind,
	   part->end_head,
	   part->end_sector,
	   part->end_cyl,
	   part->start_sect,
	   part->nr_sects );
#endif
    if (!part) {
      printk("do_nfa_request: device %d doesn't exist\n",minor);
      end_request(1);
    }

    /* Get the partition starting offset */
    start = nfa_gendisk.part[minor].start_sect;
    ret = 0;
    
    switch (CURRENT->cmd) {
    case READ:
      ret = nfa_read(part, CURRENT->buffer, CURRENT->sector+start, CURRENT->current_nr_sectors);
      break;
    case WRITE:
      ret = nfa_write(part, CURRENT->buffer, CURRENT->sector+start, CURRENT->current_nr_sectors);
      break;
    default:
      panic("nfa_cs: unknown block command %d!\n", CURRENT->cmd);
    }
    if (ret == 0)
      end_request(1);
    else {
      static int ne = 0;
      if (++ne < 5)
	end_request(0);
      else
	end_request(1);
    }
  } while (1);
} /* do_nfa_request */


int init_nfa(void)
{
  static int initialized = 0;
  int partitions = 0;
  int i = 0;

  /* Prevent Re-Init... */
  if( initialized )
    return 0;

  printk(KERN_NOTICE "Internal NAND Flash array driver (nfa), (c) 2000, Jeff Smith, All Rights Reserved\n");

#if !defined(BIT_BANGED)
  NAND_CTRL = ( 0 |  NAND_WP | NAND_CE(0) );
#endif

  /* Grab kernel spave memory for this driver */
  nfa = (nfa_t *)kmalloc( sizeof( nfa_t ), GFP_KERNEL);
  if( !nfa ) {
    printf("ERROR: Cannot malloc memory for NAND Flash Array driver\n");
    return FALSE;
  }

  /* Prevent ERASE ALL action */
  nfa->ZapEnabled = 0;

  /* Set VERBOSE to on for Mfg & Chip ID Info Print */
  nfa->verbosity = 2;

  /* Scan the array for chips */
  if( !nfa_ChipArrayInfo( nfa ) ) 
    return -EIO;

  /* Set VERBOSE to on for Mfg & Chip ID Info Print */
  nfa->verbosity = 0;

  /* Allocate memory for Virt2Phys Lookup Table */
  nfa->Virt2Phys = (unsigned long *)kmalloc( (nfa->PhysicalBlocksInArray * sizeof( unsigned long )), GFP_KERNEL);
  if(!nfa->Virt2Phys) {
    printk("NFA: ERROR, Insuffician Memory, Aborting [001]\n");
    return 0;
  }

  /* Allocate memory for FreePhys Lookup Table */
  nfa->FreePhys = (freeblock_t *)kmalloc( (nfa->PhysicalBlocksInArray * sizeof( freeblock_t )), GFP_KERNEL);
  if(!nfa->FreePhys) {
    printk("NFA: ERROR, Insuffician Memory, Aborting [002]\n");
    return 0;
  }

  /* No current Vitrual Block */
  nfa->VirtualBlock = NOT_ALLOCATED;

  nfa->Block.Page = (page_t *)kmalloc((nfa->PagesPerBlock * sizeof( page_t )), GFP_KERNEL );
  if( !nfa->Block.Page ) {
    printk("NFA: ERROR, Not enough memory to buffer 1 block, Aborting [004]\n");
    return 0;
  }

  /* Setup Initial CACHE Flags */
  nfa->Block.Free        = 1;
  nfa->Block.Modified    = 0;
  nfa->Block.Virtual     = NOT_ALLOCATED;
  nfa->Block.Physical    = NOT_ALLOCATED;

  
  /* Build the Lookup Tables */
  if( nfa_CreateVirt2PhysTable( nfa ) == FAIL) {
    return -EBUSY;
  }

  /* Announce the creation of the Virtual Disk */
  printk(", Located %d chips in array, Virtual hard disk size : %d Mb, %d total blocks\n", 
	 (int)nfa->Chips, 
	 (int)((nfa->DiskSize/1024)/1024), 
	 (int)nfa->PhysicalBlocksInArray);

  if (register_blkdev(NFA_MAJOR,"nfa", &nfa_blk_fops)) {
    printk(KERN_NOTICE "nfa: unable to grab major device number %d!\n",NFA_MAJOR);
    return EAGAIN;
  }

  /* Set the pointer to the request handle for this MAJOR device */
  blk_dev[NFA_MAJOR].request_fn = DEVICE_REQUEST;

  /* Locate any partitions (fdisk created) */
  partitions = nfa_ReadPartitionTable( nfa , nfa_partition);

  /* Set the 1st Entry to 'WHOLE DISK' */
  nfa_gendisk.part[0].start_sect  =  nfa_partition[0].start_sect;
  nfa_gendisk.part[0].nr_sects    =  nfa_partition[0].nr_sects;

  /* Announce valid partitions as located */
  if( nfa_partition[1].start_sect ) {
    printk("nfa partitions: ");
    for (i = 1; i < 5; i++) {
      if( nfa_partition[i].start_sect ) {
	printk("nfa%d ", i );
      }
      nfa_gendisk.part[i].start_sect  =  nfa_partition[i].start_sect;
      nfa_gendisk.part[i].nr_sects    =  nfa_partition[i].nr_sects;
    }
    printk("\n");
  }
			
  /* Set Linux's view of the sizes of the partitions */
  for (i = 0; i < 5; i++) {
    nfa_sizes[i]                   = nfa_gendisk.part[i].nr_sects >> (BLOCK_SIZE_BITS - 9);
    nfa_blocksizes[i]              = nfa->DataSize;
  }

  /* We set read ahead to minimum (we *should* be fast!) */
  read_ahead[ MAJOR_NR ]           = 0;

  /* Fill in the BLK DEV structure stuff */
  blksize_size[NFA_MAJOR]          = nfa_blocksizes;
  blk_size[NFA_MAJOR]              = nfa_sizes;

  /* Return A.O.K. */
  return 0;
}
#endif
