/*
 * linux/arch/arm/lib/iputils.S
 *
 * Copyright (C) 1995, 1996 Russell King
 */

#include <asm/assembler.h>
		.text
@ r0 = buffer
@ r1 = len
@ r2 = sum
		.global	_csum_partial,csum_partial
csum_partial:
_csum_partial:	tst	r0, #2
		beq	Lalignok
		subs	r1, r1, #2
		addmi	r1, r1, #2
		bmi	Lnobits32
		bic	r0, r0, #3
		ldr	r3, [r0], #4
		adds	r2, r2, r3, lsr #16
		adcs	r2, r2, #0
Lalignok:	adds	r2, r2, #0
		bics	ip, r1, #31
		beq	Lnobits32
		stmfd	sp!, {r4 - r6}
Llp32:		ldmia	r0!, {r3 - r6}
		adcs	r2, r2, r3
		adcs	r2, r2, r4
		adcs	r2, r2, r5
		adcs	r2, r2, r6
		ldmia	r0!, {r3 - r6}
		adcs	r2, r2, r3
		adcs	r2, r2, r4
		adcs	r2, r2, r5
		adcs	r2, r2, r6
		sub	ip, ip, #32
		teq	ip, #0
		bne	Llp32
		adcs	r2, r2, #0
		ldmfd	sp!, {r4 - r6}
Lnobits32:	ands	ip, r1, #0x1c
		beq	Lnobits4
Lbits32lp:	ldr	r3, [r0], #4
		adcs	r2, r2, r3
		sub	ip, ip, #4
		teq	ip, #0
		bne	Lbits32lp
		adcs	r2, r2, #0
Lnobits4:	ands	ip, r1, #3
		moveq	r0, r2
		RETINSTR(moveq,pc,lr)
		mov	ip, ip, lsl #3
		rsb	ip, ip, #32
		ldr	r3, [r0]
		mov	r3, r3, lsl ip
		adds	r2, r2, r3, lsr ip
		adc	r0, r2, #0
		RETINSTR(mov,pc,lr)

@ r0 = src
@ r1 = dst	(should be aligned on 16-bit)
@ r2 = len
@ r3 = sum
		.global	___csum_partial_copy_fromuser,__csum_partial_copy_fromuser
__csum_partial_copy_fromuser:
___csum_partial_copy_fromuser:
		mov	ip, sp
		stmfd	sp!, {r4 - r8, fp, ip, lr, pc}
		sub	fp, ip, #4
		cmp	r2, #4
		blt	Ltoo_small_user
		tst	r1, #2			@ Test destination alignment
		beq	Ldst_aligned_user
		subs	r2, r2, #2		@ We dont know if SRC is aligned...
		ldrbt	ip, [r0], #1
		ldrbt	r8, [r0], #1
		orr	ip, ip, r8, lsl #8
		adds	r3, r3, ip
		adcs	r3, r3, #0
		strb	ip, [r1], #1
		mov	ip, ip, lsr #8
		strb	ip, [r1], #1		@ Destination now aligned
Ldst_aligned_user:
		tst	r0, #3
		bne	Lsrc_not_aligned_user
		adds	r3, r3, #0
		bics	ip, r2, #15		@ Routine for src & dst aligned
		beq	2f
1:		ldrt	r4, [r0], #4
		ldrt	r5, [r0], #4
		ldrt	r6, [r0], #4
		ldrt	r7, [r0], #4
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
2:		ands	ip, r2, #12
		beq	4f
		tst	ip, #8
		beq	3f
		ldrt	r4, [r0], #4
		ldrt	r5, [r0], #4
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		tst	ip, #4
		beq	4f
3:		ldrt	r4, [r0], #4
		str	r4, [r1], #4
		adcs	r3, r3, r4
4:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		ldrt	r4, [r0], #4
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
Lexit:		tst	r2, #1
		strneb	r4, [r1], #1
		andne	r4, r4, #255
		adcnes	r3, r3, r4
		adcs	r0, r3, #0
		LOADREGS(ea,fp,{r4 - r8, fp, sp, pc})

Ltoo_small_user:
		teq	r2, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		cmp	r2, #2
		blt	Ltoo_small_user1
		ldrbt	ip, [r0], #1
		ldrbt	r8, [r0], #1
		orr	ip, ip, r8, lsl #8
		adds	r3, r3, ip
		strb	ip, [r1], #1
		strb	r8, [r1], #1
		tst	r2, #1
Ltoo_small_user1:
		ldrnebt	ip, [r0], #1
		strneb	ip, [r1], #1
		adcnes	r3, r3, ip
		adcs	r0, r3, #0
		LOADREGS(ea,fp,{r4 - r8, fp, sp, pc})

Lsrc_not_aligned_user:
		cmp	r2, #4
		blt	Ltoo_small_user
		and	ip, r0, #3
		bic	r0, r0, #3
		ldrt	r4, [r0], #4
		cmp	ip, #2
		beq	Lsrc2_aligned_user
		bhi	Lsrc3_aligned_user
		mov	r4, r4, lsr #8
		adds	r3, r3, #0
		bics	ip, r2, #15
		beq	2f
1:		ldrt	r5, [r0], #4
		ldrt	r6, [r0], #4
		ldrt	r7, [r0], #4
		ldrt	r8, [r0], #4
		orr	r4, r4, r5, lsl #24
		mov	r5, r5, lsr #8
		orr	r5, r5, r6, lsl #24
		mov	r6, r6, lsr #8
		orr	r6, r6, r7, lsl #24
		mov	r7, r7, lsr #8
		orr	r7, r7, r8, lsl #24
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		mov	r4, r8, lsr #8
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
2:		ands	ip, r2, #12
		beq	4f
		tst	ip, #8
		beq	3f
		ldrt	r5, [r0], #4
		ldrt	r6, [r0], #4
		orr	r4, r4, r5, lsl #24
		mov	r5, r5, lsr #8
		orr	r5, r5, r6, lsl #24
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		mov	r4, r6, lsr #8
		tst	ip, #4
		beq	4f
3:		ldrt	r5, [r0], #4
		orr	r4, r4, r5, lsl #24
		str	r4, [r1], #4
		adcs	r3, r3, r4
		mov	r4, r5, lsr #8
4:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		b	Lexit

Lsrc2_aligned_user:
		mov	r4, r4, lsr #16
		adds	r3, r3, #0
		bics	ip, r2, #15
		beq	2f
1:		ldrt	r5, [r0], #4
		ldrt	r6, [r0], #4
		ldrt	r7, [r0], #4
		ldrt	r8, [r0], #4
		orr	r4, r4, r5, lsl #16
		mov	r5, r5, lsr #16
		orr	r5, r5, r6, lsl #16
		mov	r6, r6, lsr #16
		orr	r6, r6, r7, lsl #16
		mov	r7, r7, lsr #16
		orr	r7, r7, r8, lsl #16
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		mov	r4, r8, lsr #16
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
2:		ands	ip, r2, #12
		beq	4f
		tst	ip, #8
		beq	3f
		ldrt	r5, [r0], #4
		ldrt	r6, [r0], #4
		orr	r4, r4, r5, lsl #16
		mov	r5, r5, lsr #16
		orr	r5, r5, r6, lsl #16
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		mov	r4, r6, lsr #16
		tst	ip, #4
		beq	4f
3:		ldrt	r5, [r0], #4
		orr	r4, r4, r5, lsl #16
		str	r4, [r1], #4
		adcs	r3, r3, r4
		mov	r4, r5, lsr #16
4:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		strb	r4, [r1], #1
		ldrb	r4, [r0], #1
		b	Lexit

Lsrc3_aligned_user:
		mov	r4, r4, lsr #24
		adds	r3, r3, #0
		bics	ip, r2, #15
		beq	2f
1:		ldrt	r5, [r0], #4
		ldrt	r6, [r0], #4
		ldrt	r7, [r0], #4
		ldrt	r8, [r0], #4
		orr	r4, r4, r5, lsl #8
		mov	r5, r5, lsr #24
		orr	r5, r5, r6, lsl #8
		mov	r6, r6, lsr #24
		orr	r6, r6, r7, lsl #8
		mov	r7, r7, lsr #24
		orr	r7, r7, r8, lsl #8
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		mov	r4, r8, lsr #24
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
2:		ands	ip, r2, #12
		beq	4f
		tst	ip, #8
		beq	3f
		ldrt	r5, [r0], #4
		ldrt	r6, [r0], #4
		orr	r4, r4, r5, lsl #8
		mov	r5, r5, lsr #24
		orr	r5, r5, r6, lsl #8
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		mov	r4, r6, lsr #24
		tst	ip, #4
		beq	4f
3:		ldrt	r5, [r0], #4
		orr	r4, r4, r5, lsl #8
		str	r4, [r1], #4
		adcs	r3, r3, r4
		mov	r4, r5, lsr #24
4:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		ldrt	r4, [r0], #4
		strb	r4, [r1], #1
		adcs	r3, r3, r4, lsl #24
		mov	r4, r4, lsr #8
		b	Lexit

@ r0 = src
@ r1 = dst
@ r2 = len
@ r3 = sum

		.global	_csum_partial_copy,csum_partial_copy
csum_partial_copy:
_csum_partial_copy:
		mov	ip, sp
		stmfd	sp!, {r4 - r8, fp, ip, lr, pc}
		sub	fp, ip, #4
		cmp	r2, #4
		blt	Ltoo_small
		tst	r1, #2			@ Test destination alignment
		beq	Ldst_aligned
		subs	r2, r2, #2		@ We dont know if SRC is aligned...
		ldrb	ip, [r0], #1
		ldrb	r8, [r0], #1
		orr	ip, ip, r8, lsl #8
		adds	r3, r3, ip
		adcs	r3, r3, #0
		strb	ip, [r1], #1
		mov	ip, ip, lsr #8
		strb	ip, [r1], #1		@ Destination now aligned
Ldst_aligned:	tst	r0, #3
		bne	Lsrc_not_aligned
		adds	r3, r3, #0
		bics	ip, r2, #15		@ Routine for src & dst aligned
		beq	3f
1:		ldmia	r0!, {r4, r5, r6, r7}
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
3:		ands	ip, r2, #12
		beq	5f
		tst	ip, #8
		beq	4f
		ldmia	r0!, {r4, r5}
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		tst	ip, #4
		beq	5f
4:		ldr	r4, [r0], #4
		str	r4, [r1], #4
		adcs	r3, r3, r4
5:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		ldr	r4, [r0], #4
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		b	Lexit

Ltoo_small:	teq	r2, #0
		LOADREGS(eqea,fp,{r8, fp, sp, pc})
		cmp	r2, #2
		blt	Ltoo_small1
		ldrb	ip, [r0], #1
		ldrb	r8, [r0], #1
		orr	ip, ip, r8, lsl #8
		adds	r3, r3, ip
		strb	ip, [r1], #1
		strb	r8, [r1], #1
		tst	r2, #1
Ltoo_small1:	ldrneb	ip, [r0], #1
		strneb	ip, [r1], #1
		adcnes	r3, r3, ip
		adcs	r0, r3, #0
		LOADREGS(ea,fp,{r8, fp, sp, pc})

Lsrc_not_aligned:
		cmp	r2, #4
		blt	Ltoo_small
		and	ip, r0, #3
		bic	r0, r0, #3
		ldr	r4, [r0], #4
		cmp	ip, #2
		beq	Lsrc2_aligned
		bhi	Lsrc3_aligned
		mov	r4, r4, lsr #8
		adds	r3, r3, #0
		bics	ip, r2, #15
		beq	2f
1:		ldmia	r0!, {r5, r6, r7, r8}
		orr	r4, r4, r5, lsl #24
		mov	r5, r5, lsr #8
		orr	r5, r5, r6, lsl #24
		mov	r6, r6, lsr #8
		orr	r6, r6, r7, lsl #24
		mov	r7, r7, lsr #8
		orr	r7, r7, r8, lsl #24
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		mov	r4, r8, lsr #8
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
2:		ands	ip, r2, #12
		beq	4f
		tst	ip, #8
		beq	3f
		ldmia	r0!, {r5, r6}
		orr	r4, r4, r5, lsl #24
		mov	r5, r5, lsr #8
		orr	r5, r5, r6, lsl #24
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		mov	r4, r6, lsr #8
		tst	ip, #4
		beq	4f
3:		ldr	r5, [r0], #4
		orr	r4, r4, r5, lsl #24
		str	r4, [r1], #4
		adcs	r3, r3, r4
		mov	r4, r5, lsr #8
4:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		b	Lexit

Lsrc2_aligned:	mov	r4, r4, lsr #16
		adds	r3, r3, #0
		bics	ip, r2, #15
		beq	2f
1:		ldmia	r0!, {r5, r6, r7, r8}
		orr	r4, r4, r5, lsl #16
		mov	r5, r5, lsr #16
		orr	r5, r5, r6, lsl #16
		mov	r6, r6, lsr #16
		orr	r6, r6, r7, lsl #16
		mov	r7, r7, lsr #16
		orr	r7, r7, r8, lsl #16
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		mov	r4, r8, lsr #16
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
2:		ands	ip, r2, #12
		beq	4f
		tst	ip, #8
		beq	3f
		ldmia	r0!, {r5, r6}
		orr	r4, r4, r5, lsl #16
		mov	r5, r5, lsr #16
		orr	r5, r5, r6, lsl #16
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		mov	r4, r6, lsr #16
		tst	ip, #4
		beq	4f
3:		ldr	r5, [r0], #4
		orr	r4, r4, r5, lsl #16
		str	r4, [r1], #4
		adcs	r3, r3, r4
		mov	r4, r5, lsr #16
4:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		mov	r4, r4, lsr #8
		strb	r4, [r1], #1
		ldrb	r4, [r0], #1
		b	Lexit

Lsrc3_aligned:	mov	r4, r4, lsr #24
		adds	r3, r3, #0
		bics	ip, r2, #15
		beq	2f
1:		ldmia	r0!, {r5, r6, r7, r8}
		orr	r4, r4, r5, lsl #8
		mov	r5, r5, lsr #24
		orr	r5, r5, r6, lsl #8
		mov	r6, r6, lsr #24
		orr	r6, r6, r7, lsl #8
		mov	r7, r7, lsr #24
		orr	r7, r7, r8, lsl #8
		stmia	r1!, {r4, r5, r6, r7}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		adcs	r3, r3, r6
		adcs	r3, r3, r7
		mov	r4, r8, lsr #24
		sub	ip, ip, #16
		teq	ip, #0
		bne	1b
2:		ands	ip, r2, #12
		beq	4f
		tst	ip, #8
		beq	3f
		ldmia	r0!, {r5, r6}
		orr	r4, r4, r5, lsl #8
		mov	r5, r5, lsr #24
		orr	r5, r5, r6, lsl #8
		stmia	r1!, {r4, r5}
		adcs	r3, r3, r4
		adcs	r3, r3, r5
		mov	r4, r6, lsr #24
		tst	ip, #4
		beq	4f
3:		ldr	r5, [r0], #4
		orr	r4, r4, r5, lsl #8
		str	r4, [r1], #4
		adcs	r3, r3, r4
		mov	r4, r5, lsr #24
4:		ands	r2, r2, #3
		adceq	r0, r3, #0
		LOADREGS(eqea,fp,{r4 - r8, fp, sp, pc})
		tst	r2, #2
		beq	Lexit
		adcs	r3, r3, r4, lsl #16
		strb	r4, [r1], #1
		ldr	r4, [r0], #4
		strb	r4, [r1], #1
		adcs	r3, r3, r4, lsl #24
		mov	r4, r4, lsr #8
		b	Lexit
