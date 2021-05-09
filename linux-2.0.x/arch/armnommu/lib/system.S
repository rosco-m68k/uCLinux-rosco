/* 
 * linux/arch/arm/lib/system.S
 *
 * Copyright (C) 1995, 1996 Russell King
 *
 * 07/06/96: Now support tasks running in SVC mode.
 */

#include <linux/config.h>
#include <asm/assembler.h>

		.text
		.global	_abort
_abort:		adr	r0, Labort_msg
		mov	r1, lr
		b	_panic

Labort_msg:	.ascii	"Eek! Got to an abort() from %p! "
		.ascii	"(Please report to rmk@ecs.soton.ac.uk)\n\0"
		.align

#ifdef CONFIG_ARCH_EBSA
/*
 * The StrongARM evaluation board doesn't have these, so
 * we make dummy functions in here until we sort them out
 */
		.globl	_ecard_reset
		.globl	_ecard_init
_ecard_reset:
_ecard_init:
		mov	pc, lr
#endif
