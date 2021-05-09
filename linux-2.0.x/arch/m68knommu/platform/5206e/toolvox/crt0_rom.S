/*****************************************************************************/

/*
 *	crt0_ram.S -- startup code for MCF5206e ColdFire based ToolVox boards.
 *
 *	(C) Copyright 1999, Greg Ungerer.
 *
 * 	2000/01/28 Modified for the Omnia -
 *		'ToolVox' Microphone Processor by James D. Schettine
 */

/*
 * Telos-systems Inc. has many twisty little architectures- all different.
 * The first coldfire board in the Omnia product line is the ToolVox.
 * The Toolvox is a Microphone processor with many features for the high-
 * end audio broadcaster or recording studio.  The actual audio processing
 * is performed by an on-board DSP.  (Really cool stuff!)
 * See:	 www.nogrunge.com/product/toolvox
 * Linux support is for in-house use for now, but hopefully soon I can
 * justify using a *real* kernel for the entire product line.
 */

/*****************************************************************************/

#include "linux/autoconf.h"
#include "asm/coldfire.h"
#include "asm/mcfsim.h"

/*****************************************************************************/

/*
 *	Omnia ToolVox M5206e ColdFire board, chip select and memory setup.
 */

#define	MEM_BASE	0x00100000	/* Memory base at address 1Mb */
#define	MEM_SIZE	0x00100000	/* Memory size 1Mb */
#define	VBR_BASE	MEM_BASE	/* Vector address */

/*****************************************************************************/

.global	_start
.global _rambase
.global _ramvec
.global	_ramstart
.global	_ramend

/*****************************************************************************/

.data

/*
 *	Set up the usable of RAM stuff. Size of RAM is determined then
 *	an initial stack set up at the end.
 */
_rambase:
.long	0
_ramvec:
.long	0
_ramstart:
.long	0
_ramend:
.long	0

/*****************************************************************************/

.text

/*
 *	This is the codes first entry point. This is where it all
 *	begins...
 */

_start:
	nop					/* Filler */
	move.w	#0x2700, %sr			/* No interrupts */

	/* assume boot-rom setup ram, xilinx, periperals, etc... */

	/* copy data segment from ROM to RAM */
	move.l	#_etext, %a0
	move.l	#_sdata, %a1
	move.l	#_edata, %a2
_copy_data:
	move.l	(%a0)+,%d0
	move.l	%d0,(%a1)+
	cmp.l	%a1,%a2
	bhi	_copy_data

	/*
	 * Setup VBR here, otherwise buserror remap will not work.
	 * if dBug was active before (on my SBC with dBug 1.1 of Dec 16 1996)
	 *
	 * bkr@cut.de 19990306
	 *
	 * Note: this is because dBUG points VBR to ROM, making vectors read
	 * only, so the bus trap can't be changed. (RS)
	 */
	move.l	#VBR_BASE, %a7			/* Note VBR can't be read */
	movec   %a7, %VBR
	move.l	%a7, _ramvec			/* Set up vector addr */
	move.l	%a7, _rambase			/* Set up base RAM addr */

	/*
	 *	Set to 1 meg for the ToolVox board (m5206e).
	 */
	move.l	#MEM_BASE+MEM_SIZE, %sp		/* Set up initial stack ptr */
	move.l	%sp, _ramend			/* Set end ram addr */
	lea.l	_ebss, %a0			/* Set start of ram */
	move.l	%a0, _ramstart

	/*
	 *	Enable CPU internal cache.
	 */
	move.l	#0x01000000, %d0		/* Invalidate cache cmd */
	movec	%d0, %CACR			/* Invalidate cache */
	move.l	#0x80000100, %d0		/* Setup cache mask */
	movec	%d0, %CACR			/* Enable cache */

	/*
	 *	Zero out the bss region.
	 */
	lea.l	_sbss, %a0			/* Get start of bss */
	lea.l	_ebss, %a1			/* Get end of bss */
	clr.l	%d0				/* Set value */
_clear_bss:
	move.l	%d0, (%a0)+			/* Clear each word */
	cmp.l	%a0, %a1			/* Check if at end */
	bne	_clear_bss

	/*
	 *	Assember start up done, start code proper.
	 */
	jsr	start_kernel			/* Start Linux kernel */

_exit:
	jmp	_exit				/* Should never get here */

/*****************************************************************************/
