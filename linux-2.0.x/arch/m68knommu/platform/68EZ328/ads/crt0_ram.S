/* arch/m68knommu/platform/68EZ328/ads_ram-head.S
 * 
 * Copyright (C) 1999  Vladimir Gurevich <vgurevic@cisco.com>
 *                     Bear & Hare Software, Inc.
 *
 * Based on: arch/m68knommu/platform/68EZ328/pilot-head.S
 * Copyright (C) 1998  D. Jeff Dionne <jeff@lineo.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 */

	.global _stext
	.global _start
	.global __bss_start
	.global _ramend
	.global _boot_stack

#define ASSEMBLY

#include <linux/config.h>
#include <asm/MC68EZ328.h>

	.text
_start:
_stext:
        /*
         * We do not need to program the basic registers here, since
         * this is an image which is loaded directly into DRAM. It
         * is the xxx-init.b file that will take care of that.
         */

        moveal  #_boot_stack, %sp                                               
	movew	#0x2700, %sr

	/*
	 * Here we start doing some real things
	 */
	moveq	#13, %d7		/* '\r' */
	jsr putc

	moveq	#10, %d7		/* '\n' */
	jsr putc

	moveq	#65, %d7		/* 'A' */
	jsr putc

	movew	#16384, %d0  /* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0

	moveq	#66, %d7		/* 'B' */
	jsr	putc

	moveal	#__bss_start, %a0
	moveal	#end, %a1

	/* Copy 0 to %a0 until %a0 == %a1 */
L1:
	movel	#0, %a0@+
	cmpal	%a0, %a1
	bhi	L1

	moveq	#67, %d7		/* 'C' */
	jsr	putc

	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	moveq	#68, %d7		/* 'D' */
	jsr	putc


lp:
	jsr	start_kernel
        jmp lp
_exit:

	jmp	_exit


putc:
	moveb	%d7,UTX_TXDATA_ADDR
pclp:
	movew	UTX_ADDR, %d7
	andw	#UTX_TX_AVAIL, %d7
	beq	pclp
	rts

	.data
env:
	.long	0
	.text
