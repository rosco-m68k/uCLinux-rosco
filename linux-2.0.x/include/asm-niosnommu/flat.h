
/*
 *  Copyright (C) 2001  Microtronix Datacom Ltd.  Heinz Zid <hzid@microtronix.com>
 *  Extensive changes for uClinux running under Altera Nios32
 *
 * Copyright (C) 1998  Kenneth Albanowski <kjahds@kjahds.com>
 *                     The Silver Hammer Group, Ltd.
 *
 */

#ifndef _LINUX_FLAT_H
#define _LINUX_FLAT_H

struct flat_hdr {
	char magic[4];
	unsigned long rev;
	unsigned long entry;       /* Offset of first executable instruction with text segment from beginning of file*/
	unsigned long data_start;  /* Offset of data segment from beginning of file*/
	
	unsigned long data_end;    /* Offset of end of data segment from beginning of file*/
	unsigned long bss_end;     /* Offset of end of bss segment from beginning of file*/
				   /* (It is assumed that data_end through bss_end forms the bss segment.) */
	unsigned long stack_size;  /* Size of stack, in bytes */
	unsigned long reloc_start; /* Offset of relocation records from beginning of file */
	
	unsigned long reloc_count; /* Number of relocation records */
	
	unsigned long flags;       
	
	unsigned long filler[6];   /* Reservered, set to zero */
};

#define FLAT_RELOC_TYPE_TEXT 0
#define FLAT_RELOC_TYPE_DATA 1
#define FLAT_RELOC_TYPE_BSS  2
#define FLAT_RELOC_EXTENDED  3	// First attempt to extend flat files w/o breaking existing code

struct flat_reloc {
	signed long offset : 30;
	unsigned long type : 2; 
};


#if defined(__nios32__)
#include <asm/nios-howto.h>  // copy of "nios.h" (from include/elf) from the Cygnus/Alter-Nios toolchain
#define EXTENDED_RELOC 1

#define PACKED __attribute__ ((packed))
struct flat_reloc_extended {
  	signed   long offset       : 30 PACKED;	// Signed offset (origin is .data section) into image of where to do the fixup: 
        unsigned long type         :  2 PACKED;	// == FLAT_RELOC_EXTENDED for this record


  	unsigned long reloc_type   :  8 PACKED; // Relocation type from ELF HOWTO
  	unsigned long dest_section :  2 PACKED;	// symbol section: What type field above had before extended records were
						// introduced. The section of the fixup target.
  	unsigned long src_section  :  2 PACKED; // Symbol section. The target of the fixup will be fixed up to refer to the src.
  	unsigned long reserved     : 20 PACKED;


        signed   long addend;
	signed   long src_offset;
	// Relocation is as follows: *(offset in dest_section) = addend + &(src_offset in src_section)
        // Note: The reloc_type indicates how to bit-fiddle the src address into the destination.
};

typedef struct flat_reloc_extended reloc_struct;

#else
typedef struct flat_reloc reloc_struct;

#endif // defined(__nios32__)

  	
/*
 *	Endien issues - blak!
 */
#define FLAT_FLAG_RAM  0x01000000    /* program should be loaded entirely into RAM */

#endif /* _LINUX_FLAT_H */
