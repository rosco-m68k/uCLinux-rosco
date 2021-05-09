/*
 * linux/arch/arm/lib/string.S
 *
 * Copyright (C) 1995, 1996 Russell King
 */

#include <asm/assembler.h>
		.text
# Prototype: char *strrchr(const char *s,char c);

@ r0 = pointer, r1 = length
		.global	_memzero,memzero
memzero:
_memzero:	stmfd	sp!, {lr}
		mov	r2, #0
		mov	r3, #0
		mov	ip, #0
		mov	lr, #0
Lgfpmemzerolp:	subs	r1, r1, #4*8
		stmgeia	r0!, {r2, r3, ip, lr}
		stmgeia	r0!, {r2, r3, ip, lr}
		bgt	Lgfpmemzerolp
		LOADREGS(fd, sp!, {pc})

		.global	___page_memcpy,__page_memcpy
__page_memcpy:
___page_memcpy:	stmfd	sp!, {r4, r5, lr}
Lpagememcpylp:	subs	r2, r2, #4*8
		ldmgeia	r1!, {r3, r4, r5, ip}
		stmgeia	r0!, {r3, r4, r5, ip}
		ldmgeia	r1!, {r3, r4, r5, ip}
		stmgeia	r0!, {r3, r4, r5, ip}
		bgt	Lpagememcpylp
		LOADREGS(fd, sp!, {r4, r5, pc})

		.global	_memset,memset
memset:
_memset:	mov	r3, r0
		cmp	r2, #16
		blt	Lmemsetbytelp
		ands	ip, r3, #3
		beq	Laligned
		cmp	ip, #2
		strltb	r1, [r3], #1			@ Align destination
		strleb	r1, [r3], #1
		strb	r1, [r3], #1
		rsb	ip, ip, #4
		sub	r2, r2, ip
Laligned:	orr	r1, r1, r1, lsl #8
		orr	r1, r1, r1, lsl #16
		cmp	r2, #256
		blt	Lmemsetnotopt1
		stmfd	sp!, {r4, r5, lr}
		mov	r4, r1
		mov	r5, r1
		mov	lr, r1
		mov	ip, r2, lsr #6
		sub	r2, r2, ip, lsl #6
Lmemset64lp1:	stmia	r3!, {r1, r4, r5, lr}		@ 64 bytes at a time.
		stmia	r3!, {r1, r4, r5, lr}
		stmia	r3!, {r1, r4, r5, lr}
		stmia	r3!, {r1, r4, r5, lr}
		subs	ip, ip, #1
		bne	Lmemset64lp1
		teq	r2, #0
		LOADREGS(eqfd, sp!, {r4, r5, pc})	@ Now <64 bytes to go.
		tst	r2, #32
		stmneia	r3!, {r1, r4, r5, lr}
		stmneia	r3!, {r1, r4, r5, lr}
		tst	r2, #16
		stmneia	r3!, {r1, r4, r5, lr}
		ldmia	sp!, {r4, r5}
Lmemsetexit:	tst	r2, #8
		stmneia	r3!, {r1, lr}
		tst	r2, #4
		strne	r1, [r3], #4
		tst	r2, #2
		strneb	r1, [r3], #1
		strneb	r1, [r3], #1
		tst	r2, #1
		strneb	r1, [r3], #1
		LOADREGS(fd, sp!, {pc})

Lmemsetnotopt1:	movs	ip, r2, lsr #3
		beq	Lmemsetexit
		sub	r2, r2, ip, lsl #3
		stmfd	sp!, {lr}
		mov	lr, r1
		subs	ip, ip, #4
Lmemset1616lp:	stmgeia	r3!, {r1, lr}
		stmgeia	r3!, {r1, lr}
		stmgeia	r3!, {r1, lr}
		stmgeia	r3!, {r1, lr}
		subges	ip, ip, #4
		bge	Lmemset1616lp
		tst	ip, #2
		stmneia	r3!, {r1, lr}
		stmneia	r3!, {r1, lr}
		tst	ip, #1
		stmneia	r3!, {r1, lr}
		teq	r2, #0
		LOADREGS(eqfd, sp!, {pc})
		b	Lmemsetexit

Lmemsetbytelp:
Lmemsetlp:	subs	r2, r2, #1
		strgeb	r1, [r3], #1
		bgt	Lmemsetlp
		RETINSTR(mov, pc, lr)

		.global	_strrchr,strrchr
strrchr:
_strrchr:	stmfd	sp!,{lr}
		mov	r3,#0
Lstrrchrlp:	ldrb	r2,[r0],#1
		teq	r2,r1
		moveq	r3,r0
		teq	r2,#0
		bne	Lstrrchrlp
		mov	r0,r3
		LOADREGS(fd, sp!, {pc})


