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
#define CMDLINE		0x004000		/* Command line in rom2 */

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

/*******************************************************************************/

.text
_start:

	nop

	/* The architecture depending setup
	 * is allready done by gdb...
	 */

	movel	#STACK_START, %sp	/* init stack pointer */

	movel	%sp,	_ramend

	move	#0x2700, %sr		/* mask interrupts */

	/* map vector table */
	movel	#VBR_BASE, %d0
	movecl	%d0, %vbr

	movew	#16384, %d0  			/* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0

	movel	#ROM_SIZE, rom_length

#ifndef CONFIG_ROOT_NFS

/*
 * Copy the root fs to ram if requested
 *
 */

#ifndef CONFIG_ROMFS_FROM_ROM

	leal	_sbss,	%a0			/* a0: start of romfs/bss */
	leal	_ebss,	%a1			/* a1: end of bss */
	movel	%a0,	%a2			/* a2: copy start of romfs/bss */

	movel	8(%a0),	%d0			/* length of romfs */
	addql	#8,	%d0			
	andl	#0xfffffc, %d0			/* mask you the 2 lowest bits */

	addl	%d0,	%a0			/* a0: end of romfs/bss */
	addl	%d0,	%a1			/* a1: new end of bss */
	movel	%a1,	_ramstart

_copy_romfs:					/* Copy the romfs to the end of */
	movel	-(%a0),	%d0			/* the bss section and clear bss out */
	movel	%d0,	-(%a1)
	cmpl	%a0,	%a2
	bne	_copy_romfs

	leal	_sbss, %a0
	leal	_ebss, %a1

#endif

#else
	leal	_ebss, %a2
	movel	%a2, _ramstart
#endif

/*
 * Clear out bss section
 */

_clear_bss:
	movel	#0,	%a0@+
	cmpl	%a0,	%a1
	bne	_clear_bss

#if 0
/*
 * Copy command_line from flash to ram
 */

	leal	command_line, %a0		/* a0: command_line in ram */
	movel	#CMDLINE, %a1			/* a1: command_line in flash */

_copy_cmdline:
	moveb	%a1@+, %d0
	moveb	%d0, %a0@+
	cmpb	#0, %d0
	bne	_copy_cmdline
#endif
	
	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	move	#0,	%d0

	jsr	start_kernel

_exit:	jmp	_exit				/* should never get here */

.data

env:	
	.long	0
