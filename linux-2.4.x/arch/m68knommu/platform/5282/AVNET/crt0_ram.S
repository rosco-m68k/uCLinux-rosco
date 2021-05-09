/*****************************************************************************/

/*
 *	crt0_ram.S -- startup code for MCF5282 ColdFire based MOTOROLA boards.
 *
 *	(C) Copyright 1999-2003, Greg Ungerer (gerg@snapgear.com).
 */

/*****************************************************************************/

#include "linux/autoconf.h"
#include "asm/coldfire.h"
#include "asm/mcfsim.h"

/*****************************************************************************/

/*
 *	Motorola M5282C3 ColdFire eval board, chip select and memory setup.
 */

#define	MEM_BASE	0x00000000	/* Memory base at address 0 */
#define	VBR_BASE	MEM_BASE	/* Vector address */

#if defined(CONFIG_RAM4MB)
#define	MEM_SIZE	0x00400000	/* Memory size 4Mb */
#elif defined(CONFIG_RAM16MB)
#define	MEM_SIZE	0x01000000	/* Memory size 16Mb */
#else
#define	MEM_SIZE	0x00800000	/* Memory size 8Mb */
#endif

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

#if CONFIG_BLK_DEV_INITRD
/*
 *	Setup initial RAM disk limits of using INITRD.
 */
.extern	initrd_start
.extern initrd_end
#endif

/*****************************************************************************/

.text

/*
 *	This is the codes first entry point. This is where it all
 *	begins...
 */

_start:
	nop					/* Filler */
	move.w	#0x2700, %sr			/* No interrupts */

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
	 *	Set memory size.
	 */
	move.l	#MEM_SIZE, %a0

	move.l	%a0, %d0			/* Mem end addr is in a0 */
	move.l	%d0, %sp			/* Set up initial stack ptr */
	move.l	%d0, _ramend			/* Set end ram addr */

#if 0
	/*
	 *	Enable CPU internal cache.
	 */
	move.l	#0x01000000, %d0
	movec	%d0, %CACR			/* Invalidate cache */
	nop

	move.l	#0x0000c000, %d0		/* Set SDRAM cached only */
	movec	%d0, %ACR0
	move.l	#0x00000000, %d0		/* No other regions cached */
	movec	%d0, %ACR1

	move.l	#0x00000000, %d0		/* Setup cache mask */
	movec	%d0, %CACR			/* Enable cache */
	nop
#endif

	/*
	 *	Move ROM filesystem above bss :-)
	 */
	lea.l	_sbss, %a0			/* Get start of bss */
	lea.l	_ebss, %a1			/* Set up destination  */
	move.l	%a0, %a2			/* Copy of bss start */

#if CONFIG_BLK_DEV_INITRD
	move.l	%a1, %d2
	add.l	#0xfff, %d2			/* Round ROMfs start to page */
	and.l	#0xfffff000, %d2
	move.l	%d2, %a1			/* Save result for later */
#endif

	move.l	8(%a0), %d1			/* Get size of ROMFS */
	addq.l	#8, %d1				/* Allow for rounding */
	and.l	#0xfffffffc, %d1		/* Whole words */

	add.l	%d1, %a0			/* Copy from end */
	add.l	%d1, %a1			/* Copy from end */
	move.l	%a1, _ramstart			/* Set start of ram */

_copy_romfs:
	move.l	-(%a0), %d0			/* Copy dword */
	move.l	%d0, -(%a1)
	cmp.l	%a0, %a2			/* Check if at end */
	bne	_copy_romfs

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

#if CONFIG_BLK_DEV_INITRD
	/*
	 *	Setup up RAMdisk info if using it.
	 *	(Must do this after clearing the bss :-)
	 */
	move.l	%d2, initrd_start		/* Set up start of initrd */
	add.l	%d1, %d2			/* Calculate end of initrd */
	move.l	%d2, initrd_end
#endif

	/*
	 *	Load the current task pointer and stack.
	 */
	lea	init_task_union, %a0
	movel	%a0, _current_task
	lea	0x2000(%a0), %sp

	/*
	 *	Assember start up done, start code proper.
	 */
	jsr	start_kernel			/* Start Linux kernel */

_exit:
	jmp	_exit				/* Should never get here */

/*****************************************************************************/

//	// ================================================================
//	// LED test code
//
//	//GPTASYSCR1  @ 0x401a0006        // GPTA Control reg 1
//	//GPTBSYSCR1  @ 0x401b0006
//	//GPTADR      @ 0x401a001d        // GPTA Data Reg
//	//GPTBDR      @ 0x401b001d
//	//GPTADDR     @ 0x401a001e        // GPTA Data Direction
//	//GPTBDDR     @ 0x401b001e
//	//PORT_TC     @ 0x4010000f
//	//PORT_TD     @ 0x40100010
//	//DDR_TC      @ 0x40100023
//	//DDR_TD      @ 0x40100024
//	//DR_TC       @ 0x40100037
//	//DR_TD       @ 0x40100038
//
//	// LED (GPIO) initialization sequence:
//	moveq  #0x00, d0
//	move.b d0, 0x401a0006 // Write GPTASYSCR1 := 0x00
//	moveq  #0x00, d0
//	move.b d0, 0x401b0006 // Write GPTBSYSCR1 := 0x00
//
//	moveq  #0x00, d0
//	move.b d0, 0x401a001e // Write GPTADDR    := 0x00
//	moveq  #0x0f, d0
//	move.b d0, 0x401b001e // Write GPTBDDR    := 0x0f
//
//	moveq  #0x05, d0
//	move.b d0, 0x40100023 // Write DDR_TC     := 0x05
//	moveq  #0x05, d0
//	move.b d0, 0x40100024 // Write DDR_TD     := 0x05
//
//	// ----------------------------------------------------------------
//	// Set some LED bits:
//	move.b   0x401b001d,d0   // Read GPTBDR
//	ori.l    #0xe,d0         // Set bits
//	andi.l   #0xfffffffe,d0  // Clear bits
//	move.b   d0,0x401b001d   // Write GPTBDR
//	// ----------------------------------------------------------------
//
