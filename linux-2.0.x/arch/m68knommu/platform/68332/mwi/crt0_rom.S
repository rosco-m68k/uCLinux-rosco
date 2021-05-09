/* linux/arch/m68knommu/platform/68332/mwi/crt0_ram.S:  A startup file for the MC68332 
 *
 * Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 *                     The Silver Hammer Group, Ltd.
 *
 * (c) 1995, Dionne & Associates
 * (c) 1995, DKG Display Tech.
 *
 * Cloned from linux/arch/m68knommu/platform/68332/generic-head.S
 * Gerold Boehler <gboehler@mail.austria.at>
 */

#include <linux/autoconf.h>

#ifdef CONFIG_RAM1MB
#define RAM_SIZE 0x100000 			/* 1 Megabyte */
#endif

#ifdef CONFIG_RAM05MB
#define RAM_SIZE 0x080000 			/* 1/2 Megabyte */
#endif

#define ROM_SIZE	0x100000		/* All mwi boards have 1MB Flash */
#define VBR_BASE	0x100000		/* this is the crt0 for ram */
#define STACK_START	(ROM_SIZE + RAM_SIZE) 	/* init sp to end of ram */
#define CMDLINE		0x004000		/* take the command line from here */
#define RAM_START	(ROM_SIZE + 0x400)

/*******************************************************************************/

.global _start
.global _ramvec
.global _ramstart
.global _ramend


/*******************************************************************************/

.data
	_ramstart:
		.long	0

	_ramend:
		.long	0

	_romfslen:
		.long	0

/*******************************************************************************/

.text
_start:
	.long	STACK_START
	.long	0x00000008

	nop

	/* 16.67 MHz and stuff... */
	movew	#0x004f, 0xfffa00
	movew	#0x7f80, 0xfffa04
	movew	#0x0004, 0xfffa20

	/* cspar */
	movew	#0xffff, 0xfffa44
	movew	#0x0001, 0xfffa46

	/* csboot */
	movew	#0x0007, 0xfffa48
	movew	#0x6830, 0xfffa4a

	/* csrom */
	movew	#0x0007, 0xfffa4c
	movew	#0x7430, 0xfffa4e

	/* csram */
	movew	#0x1007, 0xfffa50
	movew	#0x6c30, 0xfffa52

	/* csram lo */
	movew	#0x1007, 0xfffa54
	movew	#0x5030, 0xfffa56

	/* csram hi */
	movew	#0x1007, 0xfffa58
	movew	#0x3030, 0xfffa5a

	/* chip selects */
	movew	#0x0000, 0xfffa5c
	movew	#0x0000, 0xfffa5e
	movew	#0x0000, 0xfffa60
	movew	#0x0000, 0xfffa62
	movew	#0x0000, 0xfffa64
	movew	#0x0000, 0xfffa66
	movew	#0x0000, 0xfffa68
	movew	#0x0000, 0xfffa6a
	movew	#0x0000, 0xfffa6c
	movew	#0x0000, 0xfffa6e
	movew	#0x0000, 0xfffa70
	movew	#0x0000, 0xfffa72
	movew	#0x0000, 0xfffa74
	movew	#0x0000, 0xfffa76

	/* no ints on portf */
	movew	#0x0000, 0xfffa1e

/*
 * Copy data to begin of ram
 * This must be done before any write 
 * acces to any variables takes place
 * otherwise the value will be lost
 *
 */

	leal	_etext, %a0
	movel	#RAM_START, %a1
	leal	_edata, %a2

_copy_data:
	movel	%a0@+,	%d0
	movel	%d0,	%a1@+
	cmpal	%a0,	%a2
	bhi	_copy_data


	/* init stack */	
	movel	#STACK_START, %sp
	movel	%sp,	_ramend

	/* map vector table */
	movel	#VBR_BASE, %d0
	movecl	%d0, %vbr

	orb	#0x81, 0xfffa1d		/* set MODCLK = IRQ7 = output */
	andb	#0x7e, 0xfffa1f		/* assign MODCLK = IRQ7 to portf */

   	moveq   #0,%d0

	movew	#16384, %d0  			/* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0

	movel	#ROM_SIZE, rom_length

#ifndef CONFIG_ROOT_NFS

#ifdef CONFIG_ROMFS_FROM_ROM

/*
 * romfs gets loaded from __romfs
 */

	leal	_end, %a0
	movel	%a0, _ramstart
#else

/*
 * Init sci and prompt with '>' and wait for the
 * filesystem to arrive 
 *
 */

	movew	#0x00, 0xfffc00
	movew	#0x1b, 0xfffc08
	movew	#0x0c, 0xfffc0a

_load_romfs:

	moveq	#62, %d7		/* '>' */
	jsr	putc

	leal	_end, %a0		/* start of romfs */

	jsr	getc
	cmpb	#0x2d, %d7		/* '-' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	cmpb	#0x72, %d7		/* 'r' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	cmpb	#0x6f, %d7		/* 'o' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	cmpb	#0x6d, %d7		/* 'm' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	cmpb	#0x31, %d7		/* '1' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	cmpb	#0x66, %d7		/* 'f' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	cmpb	#0x73, %d7		/* 's' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	cmpb	#0x2d, %d7		/* '-' */
	bne	_load_romfs
	moveb	%d7, %a0@+

	jsr	getc
	moveb	%d7, %a0@+
	andl	#0xff, %d7
	movel	#24, %d6
	lsll	%d6, %d7
	movel	%d7, _romfslen

	jsr	getc
	moveb	%d7, %a0@+
	andl	#0xff, %d7
	movel	#16, %d6
	lsll	%d6, %d7
	addl	%d7, _romfslen
	
	jsr	getc
	moveb	%d7, %a0@+
	andl	#0xff, %d7
	lsll	#8, %d7
	addl	%d7, _romfslen
	
	jsr	getc
	moveb	%d7, %a0@+
	andl	#0xff, %d7
	addl	%d7, _romfslen

	subl	#12, _romfslen

	movel	_romfslen, %d0

_read_romfs:
	jsr	getc
	moveb	%d7, %a0@+
	subl	#1, %d0
	bne	_read_romfs

	/* finally allign start of ram */
	movel	%a0, %d7
	addl	#8, %d7
	andl	#0xfffffff8, %d7
	movel	%d7, _ramstart
#endif

#else
	leal	_ebss, %a0
	movel	%a0, _ramstart
#endif


/*
 * Clear out bss section
 *
 */
	leal	_sbss, %a0
	leal	_ebss, %a1

_clear_bss:
	movel	#0,	%a0@+
	cmpl	%a0,	%a1
	bne	_clear_bss

	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	move	#0,	%d0
#if 0
	move	CMDLINE, command_line		/* set command_line */
#endif
	jsr	start_kernel

_exit:	jmp	_exit				/* should never get here */


/*
 * simple putc
 */

putc:
	movew	%d7, 0xfffc0e
LSTL:
	movew	0xfffc0c,%d7
	andw	#0x0100,%d7
	beq	LSTL
	rts


/*
 * simple getc
 */

getc:
	movew	0xfffc0c, %d6
	andw	#0x0040, %d6
	beq	getc

	movew	0xfffc0e, %d7
	rts
.data

env:	
	.long	0
