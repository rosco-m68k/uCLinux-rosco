/*
 * linux/arch/arm/lib/delay.S
 *
 * Copyright (C) 1995, 1996 Russell King
 */

#include <asm/assembler.h>
		.text

LC0:		.word	loops_per_sec

		.global	_udelay,udelay
udelay:
_udelay:	mov	r2, #0x1000
		orr	r2, r2, #0x00c6
		mul	r1, r0, r2
		ldr	r2, LC0
		ldr	r2, [r2]
		mov	r1, r1, lsr #11
		mov	r2, r2, lsr #11
		mul	r0, r1, r2
		movs	r0, r0, lsr #10
		RETINSTR(moveq,pc,lr)

@ Delay routine
		.global	___delay,__delay
__delay:
___delay:	subs	r0, r0, #1
		RETINSTR(movcc,pc,lr)
		subs	r0, r0, #1
		RETINSTR(movcc,pc,lr)
		subs	r0, r0, #1
		RETINSTR(movcc,pc,lr)
		subs	r0, r0, #1
		RETINSTR(movcc,pc,lr)
		subs	r0, r0, #1
		RETINSTR(movcc,pc,lr)
		subs	r0, r0, #1
		RETINSTR(movcc,pc,lr)
		subs	r0, r0, #1
		RETINSTR(movcc,pc,lr)
		subs	r0, r0, #1
		bcs	___delay
		RETINSTR(mov,pc,lr)

