/*****************************************************************************/

/*
 *	(C) Copyright 1999-2002, Greg Ungerer.
 *      (C) Copyright 1999, D. Jeff Dionne
 *
 * 	1999/02/24 Modified for the 5307 processor David W. Miller
 *      1999/11/20 Support for the 5204 by D. Jeff Dionne
 */

/*****************************************************************************/

.global	_start
.global _stext
.global out_char

/*****************************************************************************/

.section .ramvec
.long 0

.text
/*
 *	This is the codes first entry point. This is where it all
 *	begins...
 */

_start:
_stext:
	nop					/* Filler */
	move.w	#0x2700, %sr			/* No interrupts */
#if 0
	/*
	 * Setup vbr here, otherwise buserror remap will not work.
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
#endif
	pea	65
	jsr	out_char
	pea	65
	jsr	out_char
	pea	65
	jsr	out_char
	pea	65
	jsr	out_char
	jsr start_kernel
_exit:
	rte

out_char:
	move.l	4(%sp), %d1
	move.l	#0x13, %d0
	trap #15
	rts

/*****************************************************************************/
