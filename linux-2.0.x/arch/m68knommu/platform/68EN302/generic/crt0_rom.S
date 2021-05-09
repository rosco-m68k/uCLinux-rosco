/* crt0_rom.S:  A startup file for the MC68EN302 
 *
 * Copyright (C) 1998-2000  D. Jeff Dionne <jeff@rt-control.com>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 *
 * (c) 1995, Dionne & Associates
 * (c) 1995, DKG Display Tech.
 */

#define ASSEMBLY

#include <linux/config.h>

	.global _stext
	.global _start
	.global main
	.global __bss_start
	.global _ramend
	.global penguin_bits
	
	.text
	
_start:
_stext:

	moveal	#0x400000, %a4
	moveal  #0x200000, %a3  /* set the rom length */
	moveal	8(%sp),    %a6
	moveal	#0x400000-4, %sp

	movew   #0x2700, %sr
	bsr 	pccheck
pccheck:
	movel	%sp@, %d7
	add.l	#4,%sp
	movel   #pccheck, %d6
	sub.l	%d6,%d7
	beq		correct_address

/* we're running in ROM memory we need to copy ourselves from ROM to RAM */
	moveal	#_start, %a1
	movel	%a1,%a0
	add.l  %d7, %a0
	moveal  #_end, %a2
	add.l  %d7, %a2
	

codetoram:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a0, %a2
	bhi codetoram
	
	jmp.l	_start

correct_address:





/*	lea	%a4@(-4), %sp */

	moveq	#13, %d7		/* '\r' */
	bsr putc

	moveq	#10, %d7		/* '\n' */
	jsr putc

	moveq	#65, %d7		/* 'A' */
	bsr putc

   	moveq   #0,%d0

	movew	#16384, %d0  /* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0

	moveq	#66, %d7
	bsr	putc


	/* Copy command line from the end of ram to end of bss */
	lea 	-512(%a4), %a0
	moveal	#end, %a1
	lea	%a1@(512), %a2
	moveq	#67, %d7
	bsr	putc

	/* Copy %a0 to %a1 until %a1 == %a2 */
L2:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	L2

	
	lea		running_in_rom(%pc),%a0
	cmp.l	#running_in_rom, %a0
	beq		LD2			/* if we're linked to RUN in RAM here we've already have all the stuff in RAM */ 


running_in_rom:

	/* Copy data segment from ROM to RAM */
	moveal	#__data_rom_start, %a0
	moveal	#__data_start, %a1
	moveal	#__data_end, %a2
	moveq	#68, %d7
	bsr	putc

	/* 
	    if data_rom_start == data_start then
            we're already executing from ram, no need to copy
	*/
	cmpal   %a0,%a1
	beq	LD2

	/* Copy %a0 to %a1 until %a1 == %a2 */
LD1:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	LD1

	moveq	#69, %d7
	bsr	putc

LD2:
	moveal	#__bss_start, %a0
	moveal	#end, %a1

	/* Copy 0 to %a0 until %a0 == %a1 */
L1:
	movel	#0, %a0@+
	cmpal	%a0, %a1
	bhi	L1

	moveq	#70, %d7
	bsr	putc

#if 1
	/* Copy command line from end of bss to command line */
	moveal	#end, %a0
	moveal	#command_line, %a1
	lea	%a1@(64), %a2
	moveq	#71, %d7
	bsr	putc

	/* Copy %a0 to %a1 until %a1 == %a2 */
L3:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	L3
#endif

	movel	%a4,	%d0
	subl	#4096,	%d0	/* Reserve 4K of stack */
	movel	%d0,	memory_end

	movel	%a3,	%d0
	movel	%d0,	rom_length

	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	moveq	#72, %d7
	jsr	putc

	bsr	start_kernel
_exit:

	jmp	_exit

	rts

	.data
env:
	.long	0
