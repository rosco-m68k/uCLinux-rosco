/*
 * linux/arch/arm/lib/memfastset.S
 *
 * Copyright (C) 1995, 1996 Russell King
 */

#include <asm/assembler.h>
		.text
@ Prototype: void memsetl (unsigned long *d, unsigned long c, size_t n);

		.global	_memsetl,memsetl

memsetl:
_memsetl:	stmfd	sp!, {lr}
		cmp	r2, #16
		blt	Lmemfastsetlp
		mov	r3, r1
		mov	ip, r1
		mov	lr, r1
		subs	r2, r2, #32
		bmi	Lmemfastl32
Lmemfast32setlp:
		stmia	r0!, {r1, r3, ip, lr}
		stmia	r0!, {r1, r3, ip, lr}
		LOADREGS(eqfd, sp!, {pc})
		subs	r2, r2, #32
		bpl	Lmemfast32setlp
Lmemfastl32:	adds	r2, r2, #16
		bmi	Lmemfastl16
Lmemfast16setlp:
		stmia	r0!, {r1, r3, ip, lr}
		LOADREGS(eqfd, sp!, {pc})
		subs	r2, r2, #16
		bpl	Lmemfast16setlp
Lmemfastl16:	add	r2, r2, #16
Lmemfastsetlp:	subs	r2, r2, #4
		strge	r1, [r0], #4
		bgt	Lmemfastsetlp
		LOADREGS(fd, sp!, {pc})
