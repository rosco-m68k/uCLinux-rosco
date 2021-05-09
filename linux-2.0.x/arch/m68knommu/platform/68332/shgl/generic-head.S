/* linux/arch/m68knommu/kernel/head.S:  A startup file for the MC68332 
 *
 * Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 *                     The Silver Hammer Group, Ltd.
 *
 * (c) 1995, Dionne & Associates
 * (c) 1995, DKG Display Tech.
 */

#define ASSEMBLY

#include <linux/config.h>
#include <asm/shglcore.h>

	.global _stext
	.global _start
	.global main
	.global __bss_start
	.global _ramend
	.global penguin_bits
	
.section .ramvec
    .long       _endram
    .long       _start

	
	.text

_start:
_stext:


	nop
#if 0
        movew   #0x0007, 0xfffa48 /* CSBARBT */
        movew   #0x7b70, 0xfffa4a 
	movew	#0x0007, 0xfffa4c
#endif
	
        movew   #0x7f00, 0xfffa04 /* 512*32768Hz =16.67MHz */

	movew	#0x100c, 0xfffc0a /* SCI 8n1, ints off */

	movew	#13, 0xfffc08 /* 38400 BPS */
	movew	#27, 0xfffc08 /* 19200 BPS */
	movew	#54, 0xfffc08 /* 9600 BPS */

	orib	#0x80, 0xfffa13

	
#if 0
	movew   #0x2700, %sr
	lea	%a4@(-4), %sp
#endif

	moveq	#13, %d7		/* '\r' */
	jsr putc

	moveq	#10, %d7		/* '\n' */
	jsr putc

	moveq	#65, %d7		/* 'A' */
	jsr putc

   	moveq   #0,%d0

	movew	#16384, %d0  /* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0

	moveq	#66, %d7
	jsr	putc

#if 0
	/* Copy command line from beginning of RAM (+16) to end of bss */
	movel	#_ramvec, %d7
	addl	#16, %d7
	moveal	%d7, %a0
	moveal	#end, %a1
	lea	%a1@(512), %a2
	moveq	#67, %d7
	jsr	putc

	/* Copy %a0 to %a1 until %a1 == %a2 */
L2:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	L2

	/* Copy data segment from ROM to RAM */
	moveal	#__data_rom_start, %a0
	moveal	#__data_start, %a1
	moveal	#__data_end, %a2
	moveq	#68, %d7
	jsr	putc

	/* Copy %a0 to %a1 until %a1 == %a2 */
LD1:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	LD1

	moveq	#69, %d7
	jsr	putc

	moveal	#__bss_start, %a0
	moveal	#end, %a1

	/* Copy 0 to %a0 until %a0 == %a1 */
L1:
	movel	#0, %a0@+
	cmpal	%a0, %a1
	bhi	L1

	moveq	#70, %d7
	jsr	putc

	/* Copy command line from end of bss to command line */
	moveal	#end, %a0
	moveal	#command_line, %a1
	lea	%a1@(512), %a2
	moveq	#71, %d7
	jsr	putc

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

	jsr	start_kernel
_exit:

	jmp	_exit


putc:

#ifdef CONFIG_68332_SERIAL
	movew	%d7,0xfffc0e
LSTL:
	movew	0xfffc0c,%d7
	andw	#0x0100,%d7
	beq	LSTL
#endif
	rts

	.data
env:
	.long	0
