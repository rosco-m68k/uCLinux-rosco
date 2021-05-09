/*
 * linux/arch/arm/lib/segment.S
 *
 * Copyright (C) 1995, 1996 Russell King
 * Except memcpy/memmove routine.
 */

#include <asm/assembler.h>
		.text
#define ENTER	\
		MOV	ip,sp	;\
		STMFD	sp!,{r4-r9,fp,ip,lr,pc}	;\
		SUB	fp,ip,#4

#define EXIT	\
		LOADREGS(ea, fp, {r4 - r9, fp, sp, pc})

#define EXITEQ	\
		LOADREGS(eqea, fp, {r4 - r9, fp, sp, pc})

# Prototype: void memcpy_tofs(void *to,const void *from,unsigned long n);

Lmtfs_dest_not_aligned:
		rsb	ip, ip, #4
		cmp	ip, #2
		ldrb	r3, [r1], #1
		strbt	r3, [r0], #1
		ldrgeb	r3, [r1], #1
		strgebt	r3, [r0], #1
		ldrgtb	r3, [r1], #1
		strgtbt	r3, [r0], #1
		sub	r2, r2, ip
		b	Lmtfs_dest_aligned

		.global	___memcpy_tofs,__memcpy_tofs
__memcpy_tofs:
___memcpy_tofs:
		cmp	r0, #0x02000000			@ PHYS check
		bge	_memcpy_fromfs
		stmfd	sp!, {lr}
		cmp	r2, #4
		blt	Lmtfs_not_enough
		ands	ip, r0, #3
		bne	Lmtfs_dest_not_aligned
Lmtfs_dest_aligned:
		stmfd	sp!, {r4 - r7}
		ands	ip, r1, #3
		bne	Lmtfs_src_not_aligned
/*
 * Seeing as there has to be at least 8 bytes to copy, we can
 * copy one word, and force a user-mode page fault...
 */

Lmtfs_0fupi:	subs	r2, r2, #4
		addmi	ip, r2, #4
		bmi	Lmtfs_0nowords
		ldr	r3, [r1], #4
		strt	r3, [r0], #4
		mov	ip, r0, lsl #17
		rsb	ip, ip, #0
		movs	ip, ip, lsr #17
		beq	Lmtfs_0fupi
/*
 * ip = max no. of bytes to copy before needing another "strt" insn
 */
		cmp	r2, ip
		movlt	ip, r2
		sub	r2, r2, ip
		subs	ip, ip, #32
		blt	Lmtfs_0rem8lp

Lmtfs_0cpy8lp:	ldmia	r1!, {r3 - r6}
		stmia	r0!, {r3 - r6}
		ldmia	r1!, {r3 - r6}
		stmia	r0!, {r3 - r6}
		subs	ip, ip, #32
		bpl	Lmtfs_0cpy8lp
Lmtfs_0rem8lp:	cmn	ip, #16
		ldmgeia	r1!, {r3 - r6}
		stmgeia	r0!, {r3 - r6}
		tst	ip, #8
		ldmneia	r1!, {r3 - r4}
		stmneia	r0!, {r3 - r4}
		tst	ip, #4
		ldrne	r3, [r1], #4
		strnet	r3, [r0], #4
		ands	ip, ip, #3
		beq	Lmtfs_0fupi
Lmtfs_0nowords:	ldmfd	sp!, {r4 - r7}
		teq	ip, #0
		LOADREGS(eqfd,sp!,{pc})
Lmtfs_nowords:	cmp	ip, #2
		ldrb	r3, [r1], #1
		strbt	r3, [r0], #1
		ldrgeb	r3, [r1], #1
		strgebt	r3, [r0], #1
		ldrgtb	r3, [r1], #1
		strgtbt	r3, [r0], #1
		LOADREGS(fd,sp!,{pc})

Lmtfs_not_enough:
		movs	ip, r2
		bne	Lmtfs_nowords
		LOADREGS(fd,sp!,{pc})

Lmtfs_src_not_aligned:
		bic	r1, r1, #3
		ldr	r7, [r1], #4
		cmp	ip, #2
		bgt	Lmtfs_3fupi
		beq	Lmtfs_2fupi
Lmtfs_1fupi:	subs	r2, r2, #4
		addmi	ip, r2, #4
		bmi	Lmtfs_1nowords
		mov	r3, r7, lsr #8
		ldr	r7, [r1], #4
		orr	r3, r3, r7, lsl #24
		strt	r3, [r0], #4
		mov	ip, r0, lsl #17
		rsb	ip, ip, #0
		movs	ip, ip, lsr #17
		beq	Lmtfs_1fupi
		cmp	r2, ip
		movlt	ip, r2
		sub	r2, r2, ip
		subs	ip, ip, #16
		blt	Lmtfs_1rem8lp

Lmtfs_1cpy8lp:	mov	r3, r7, lsr #8
		ldmia	r1!, {r4 - r7}
		orr	r3, r3, r4, lsl #24
		mov	r4, r4, lsr #8
		orr	r4, r4, r5, lsl #24
		mov	r5, r5, lsr #8
		orr	r5, r5, r6, lsl #24
		mov	r6, r6, lsr #8
		orr	r6, r6, r7, lsl #24
		stmia	r0!, {r3 - r6}
		subs	ip, ip, #16
		bpl	Lmtfs_1cpy8lp
Lmtfs_1rem8lp:	tst	ip, #8
		movne	r3, r7, lsr #8
		ldmneia	r1!, {r4, r7}
		orrne	r3, r3, r4, lsl #24
		movne	r4, r4, lsr #8
		orrne	r4, r4, r7, lsl #24
		stmneia	r0!, {r3 - r4}
		tst	ip, #4
		movne	r3, r7, lsr #8
		ldrne	r7, [r1], #4
		orrne	r3, r3, r7, lsl #24
		strnet	r3, [r0], #4
		ands	ip, ip, #3
		beq	Lmtfs_1fupi
Lmtfs_1nowords:	teq	ip, #0
		LOADREGS(eqfd,sp!,{r4 - r7, pc})
		mov	r3, r7, lsr #8
		cmp	ip, #2
		strbt	r3, [r0], #1
		movge	r3, r3, lsr #8
		strgebt	r3, [r0], #1
		movgt	r3, r3, lsr #8
		strgtbt	r3, [r0], #1
		LOADREGS(fd,sp!, {r4 - r7, pc})

Lmtfs_2fupi:	subs	r2, r2, #4
		addmi	ip, r2, #4
		bmi	Lmtfs_2nowords
		mov	r3, r7, lsr #16
		ldr	r7, [r1], #4
		orr	r3, r3, r7, lsl #16
		strt	r3, [r0], #4
		mov	ip, r0, lsl #17
		rsb	ip, ip, #0
		movs	ip, ip, lsr #17
		beq	Lmtfs_2fupi
		cmp	r2, ip
		movlt	ip, r2
		sub	r2, r2, ip
		subs	ip, ip, #16
		blt	Lmtfs_2rem8lp

Lmtfs_2cpy8lp:	mov	r3, r7, lsr #16
		ldmia	r1!, {r4 - r7}
		orr	r3, r3, r4, lsl #16
		mov	r4, r4, lsr #16
		orr	r4, r4, r5, lsl #16
		mov	r5, r5, lsr #16
		orr	r5, r5, r6, lsl #16
		mov	r6, r6, lsr #16
		orr	r6, r6, r7, lsl #16
		stmia	r0!, {r3 - r6}
		subs	ip, ip, #16
		bpl	Lmtfs_2cpy8lp
Lmtfs_2rem8lp:	tst	ip, #8
		movne	r3, r7, lsr #16
		ldmneia	r1!, {r4, r7}
		orrne	r3, r3, r4, lsl #16
		movne	r4, r4, lsr #16
		orrne	r4, r4, r7, lsl #16
		stmneia	r0!, {r3 - r4}
		tst	ip, #4
		movne	r3, r7, lsr #16
		ldrne	r7, [r1], #4
		orrne	r3, r3, r7, lsl #16
		strnet	r3, [r0], #4
		ands	ip, ip, #3
		beq	Lmtfs_2fupi
Lmtfs_2nowords:	teq	ip, #0
		LOADREGS(eqfd,sp!,{r4 - r7, pc})
		mov	r3, r7, lsr #16
		cmp	ip, #2
		strbt	r3, [r0], #1
		movge	r3, r3, lsr #8
		strgebt	r3, [r0], #1
		ldrgtb	r3, [r1], #0
		strgtbt	r3, [r0], #1
		LOADREGS(fd,sp!,{r4 - r7, pc})

Lmtfs_3fupi:	subs	r2, r2, #4
		addmi	ip, r2, #4
		bmi	Lmtfs_3nowords
		mov	r3, r7, lsr #24
		ldr	r7, [r1], #4
		orr	r3, r3, r7, lsl #8
		strt	r3, [r0], #4
		mov	ip, r0, lsl #17
		rsb	ip, ip, #0
		movs	ip, ip, lsr #17
		beq	Lmtfs_3fupi
		cmp	r2, ip
		movlt	ip, r2
		sub	r2, r2, ip
		subs	ip, ip, #16
		blt	Lmtfs_3rem8lp

Lmtfs_3cpy8lp:	mov	r3, r7, lsr #24
		ldmia	r1!, {r4 - r7}
		orr	r3, r3, r4, lsl #8
		mov	r4, r4, lsr #24
		orr	r4, r4, r5, lsl #8
		mov	r5, r5, lsr #24
		orr	r5, r5, r6, lsl #8
		mov	r6, r6, lsr #24
		orr	r6, r6, r7, lsl #8
		stmia	r0!, {r3 - r6}
		subs	ip, ip, #16
		bpl	Lmtfs_3cpy8lp
Lmtfs_3rem8lp:	tst	ip, #8
		movne	r3, r7, lsr #24
		ldmneia	r1!, {r4, r7}
		orrne	r3, r3, r4, lsl #8
		movne	r4, r4, lsr #24
		orrne	r4, r4, r7, lsl #8
		stmneia	r0!, {r3 - r4}
		tst	ip, #4
		movne	r3, r7, lsr #24
		ldrne	r7, [r1], #4
		orrne	r3, r3, r7, lsl #8
		strnet	r3, [r0], #4
		ands	ip, ip, #3
		beq	Lmtfs_3fupi
Lmtfs_3nowords:	teq	ip, #0
		LOADREGS(eqfd,sp!,{r4 - r7, pc})
		mov	r3, r7, lsr #24
		cmp	ip, #2
		strbt	r3, [r0], #1
		ldrge	r3, [r1], #0
		strgebt	r3, [r0], #1
		movgt	r3, r3, lsr #8
		strgtbt	r3, [r0], #1
		LOADREGS(fd,sp!, {r4 - r7, pc})

# Prototype: void memcpy_fromfs(void *to,const void *from,unsigned long n);
# ARM3: cant use memcopy here!!!

		.global	_memcpy,memcpy
		.global	_memmove,memmove
		.global	_memcpy_fromfs,memcpy_fromfs
_memcpy:
_memmove:
_memcpy_fromfs:
memcpy:
memmove:
memcpy_fromfs:

		ENTER
		cmp	r1, r0
		bcc	Lother_copy
		subs	r2, r2, #4
		blt	Lup_no_double_words
		ands	ip, r0, #3
		bne	Lup_dest_not_aligned
		ands	ip, r1, #3
		bne	Lup_src_not_aligned

Lup_rest:	subs	r2, r2, #8
		blt	Lup_cpy_2_lp
		subs	r2, r2, #0x14
		blt	Lup_not_long_copy

Lup_cpy_8_lp:	ldmia	r1!,{r3 - r9, ip}
		stmia	r0!,{r3 - r9, ip}
		subs	r2, r2, #32
		bge	Lup_cpy_8_lp
		cmn	r2, #16
		ldmgeia	r1!, {r3 - r6}
		stmgeia	r0!, {r3 - r6}
		subge	r2, r2, #0x10
Lup_not_long_copy:
		adds	r2, r2, #0x14

Lup_cpy_3_lp:	ldmgeia	r1!, {r3 - r5}
		stmgeia	r0!, {r3 - r5}
		subges	r2, r2, #12
		bge	Lup_cpy_3_lp

Lup_cpy_2_lp:	adds	r2, r2, #8
		blt	Lup_no_double_words
		subs	r2, r2, #4
		ldrlt	r3, [r1], #4
		strlt	r3, [r0], #4
		ldmgeia	r1!, {r3, r4}
		stmgeia	r0!, {r3, r4}
		subge	r2, r2, #4

Lup_no_double_words:
		adds	r2, r2, #4
		EXITEQ
		cmp	r2, #2
		ldrb	r3, [r1], #1
		strb	r3, [r0], #1
		ldrgeb	r3, [r1], #1
		strgeb	r3, [r0], #1
		ldrgtb	r3, [r1], #1
		strgtb	r3, [r0], #1
		EXIT
Lup_dest_not_aligned:
		rsb	ip, ip, #4
		cmp	ip, #2
		ldrb	r3, [r1], #1
		strb	r3, [r0], #1
		ldrgeb	r3, [r1], #1
		strgeb	r3, [r0], #1
		ldrgtb	r3, [r1], #1
		strgtb	r3, [r0], #1
		subs	r2, r2, ip
		blt	Lup_no_double_words
		ands	ip, r1, #3
		beq	Lup_rest
Lup_src_not_aligned:
		bic	r1, r1, #3
		ldr	r7, [r1], #4
		cmp	ip, #2
		bgt	Lup_cpy_4_3
		beq	Lup_cpy_4_2
		cmp	r2, #12
		blt	Lup_cpy_x_1_lp
		sub	r2, r2, #12

Lup_cpy_4_1_lp:	mov	r3, r7, lsr #8
		ldmia	r1!, {r4 - r7}
		orr	r3, r3, r4, lsl #24
		mov	r4, r4, lsr #8
		orr	r4, r4, r5, lsl #24
		mov	r5, r5, lsr #8
		orr	r5, r5, r6, lsl #24
		mov	r6, r6, lsr #8
		orr	r6, r6, r7, lsl #24
		stmia	r0!, {r3 - r6}
		subs	r2, r2, #16
		bge	Lup_cpy_4_1_lp
		adds	r2, r2, #12
		blt	Lup_rest1

Lup_cpy_x_1_lp:	mov	r3, r7, lsr #8
		ldr	r7, [r1], #4
		orr	r3, r3, r7, lsl #24
		str	r3, [r0], #4
		subs	r2, r2, #4
		bge	Lup_cpy_x_1_lp

Lup_rest1:	sub	r1, r1, #3
		b	Lup_no_double_words

Lup_cpy_4_2:	cmp	r2, #12
		blt	Lup_cpy_x_2_lp
		sub	r2, r2, #12

Lup_cpy_4_2_lp:	mov	r3, r7, lsr #16
		ldmia	r1!, {r4 - r7}
		orr	r3, r3, r4, lsl #16
		mov	r4, r4, lsr #16
		orr	r4, r4, r5, lsl #16
		mov	r5, r5, lsr #16
		orr	r5, r5, r6, lsl #16
		mov	r6, r6, lsr #16
		orr	r6, r6, r7,LSL#16
		stmia	r0!, {r3 - r6}
		subs	r2, r2, #16
		bge	Lup_cpy_4_2_lp
		adds	r2, r2, #12
		blt	Lup_rest2

Lup_cpy_x_2_lp:	mov	r3, r7, lsr #16
		ldr	r7, [r1], #4
		orr	r3, r3, r7, lsl #16
		str	r3, [r0], #4
		subs	r2, r2, #4
		bge	Lup_cpy_x_2_lp

Lup_rest2:	sub	r1, r1, #2
		b	Lup_no_double_words

Lup_cpy_4_3:	cmp	r2, #12
		blt	Lup_cpy_x_3_lp
		sub	r2, r2, #12

Lup_cpy_4_3_lp:	mov	r3, r7, lsr #24
		ldmia	r1!,{r4 - r7}
		orr	r3, r3, r4, lsl #8
		mov	r4, r4, lsr #24
		orr	r4, r4, r5, lsl #8
		mov	r5, r5, lsr #24
		orr	r5, r5, r6, lsl #8
		mov	r6, r6, lsr #24
		orr	r6, r6, r7, lsl #8
		stmia	r0!, {r3 - r6}
		subs	r2, r2, #16
		bge	Lup_cpy_4_3_lp
		adds	r2, r2, #12
		blt	Lup_rest3

Lup_cpy_x_3_lp:	mov	r3, r7, lsr #24
		ldr	r7, [r1], #4
		orr	r3, r3, r7, lsl#8
		str	r3, [r0], #4
		subs	r2, r2, #4
		bge	Lup_cpy_x_3_lp

Lup_rest3:	sub	r1, r1, #1
		b	Lup_no_double_words


Lother_copy:	add	r1, r1, r2
		add	r0, r0, r2
		subs	r2, r2, #4
		blt	Ldown_no_double_words
		ands	ip, r0, #3
		bne	Ldown_dest_not_aligned
		ands	ip, r1, #3
		bne	Ldown_src_not_aligned

Ldown_rest:
		subs	r2, r2, #8
		blt	Ldown_cpy_2_lp
		subs	r2, r2, #0x14
		blt	Ldown_not_long_copy
Ldown_cpy_8_lp:
		ldmdb	r1!, {r3 - r9, ip}
		stmdb	r0!, {r3 - r9, ip}
		subs	r2, r2, #32
		bge	Ldown_cpy_8_lp

Ldown_not_long_copy:
		cmn	r2, #16
		ldmgedb	r1!, {r3 - r6}
		stmgedb	r0!, {r3 - r6}
		subge	r2, r2, #16
		adds	r2, r2, #20
		ldmgedb	r1!, {r3 - r5}
		stmgedb	r0!, {r3 - r5}
		subge	r2, r2, #12
Ldown_cpy_2_lp:
		adds	r2, r2, #8
		blt	Ldown_no_double_words
		subs	r2, r2, #4
		ldrlt	r3, [r1, #-4]!
		strlt	r3, [r0, #-4]!
		ldmgedb	r1!, {r3, r4}
		stmgedb	r0!, {r3, r4}
		subge	r2, r2, #4

Ldown_no_double_words:
		adds	r2, r2, #4
		EXITEQ
		cmp	r2, #2
		ldrb	r3, [r1, #-1]!
		strb	r3, [r0, #-1]!
		ldrgeb	r3, [r1, #-1]!
		strgeb	r3, [r0, #-1]!
		ldrgtb	r3, [r1, #-1]!
		strgtb	r3, [r0, #-1]!
		EXIT

Ldown_dest_not_aligned:
		CMP	ip,#2
		LDRB	r3,[r1,#-1]!
		STRB	r3,[r0,#-1]!
		LDRGEB	r3,[r1,#-1]!
		STRGEB	r3,[r0,#-1]!
		LDRGTB	r3,[r1,#-1]!
		STRGTB	r3,[r0,#-1]!
		SUBS	r2,r2,ip
		BLT	Ldown_no_double_words
		ANDS	ip,r1,#3
		BEQ	Ldown_rest

Ldown_src_not_aligned:
		BIC	r1,r1,#3
		LDR	r3,[r1],#0
		CMP	ip,#2
		BLT	Ldown_cpy_4_3
		BEQ	Ldown_cpy_4_2
		CMP	r2,#12
		BLT	Ldown_cpy_x_1_lp
		SUB	r2,r2,#12

Ldown_cpy_4_1_lp:
		MOV	r7,r3,LSL#8
		LDMDB	r1!,{r3,r4,r5,r6}
		ORR	r7,r7,r6,LSR#24
		MOV	r6,r6,LSL#8
		ORR	r6,r6,r5,LSR#24
		MOV	r5,r5,LSL#8
		ORR	r5,r5,r4,LSR#24
		MOV	r4,r4,LSL#8
		ORR	r4,r4,r3,LSR#24
		STMDB	r0!,{r4,r5,r6,r7}
		SUBS	r2,r2,#16
		BGE	Ldown_cpy_4_1_lp
		ADDS	r2,r2,#12
		BLT	Ldown_rest1

Ldown_cpy_x_1_lp:
		MOV	ip,r3,LSL#8
		LDR	r3,[r1,#-4]!
		ORR	ip,ip,r3,LSR#24
		STR	ip,[r0,#-4]!
		SUBS	r2,r2,#4
		BGE	Ldown_cpy_x_1_lp

Ldown_rest1:
		ADD	r1,r1,#3
		B	Ldown_no_double_words

Ldown_cpy_4_2:
		CMP	r2,#12
		BLT	Ldown_cpy_x_2_lp
		SUB	r2,r2,#12

Ldown_cpy_4_2_lp:
		MOV	r7,r3,LSL#16
		LDMDB	r1!,{r3,r4,r5,r6}
		ORR	r7,r7,r6,LSR#16
		MOV	r6,r6,LSL#16
		ORR	r6,r6,r5,LSR#16
		MOV	r5,r5,LSL#16
		ORR	r5,r5,r4,LSR#16
		MOV	r4,r4,LSL#16
		ORR	r4,r4,r3,LSR#16
		STMDB	r0!,{r4,r5,r6,r7}
		SUBS	r2,r2,#16
		BGE	Ldown_cpy_4_2_lp
		ADDS	r2,r2,#12
		BLT	Ldown_rest2

Ldown_cpy_x_2_lp:
		MOV	ip,r3,LSL#16
		LDR	r3,[r1,#-4]!
		ORR	ip,ip,r3,LSR#16
		STR	ip,[r0,#-4]!
		SUBS	r2,r2,#4
		BGE	Ldown_cpy_x_2_lp

Ldown_rest2:
		ADD	r1,r1,#2
		B	Ldown_no_double_words

Ldown_cpy_4_3:
		CMP	r2,#12
		BLT	Ldown_cpy_x_3_lp
		SUB	r2,r2,#12

Ldown_cpy_4_3_lp:
		MOV	r7,r3,LSL#24
		LDMDB	r1!,{r3,r4,r5,r6}
		ORR	r7,r7,r6,LSR#8
		MOV	r6,r6,LSL#24
		ORR	r6,r6,r5,LSR#8
		MOV	r5,r5,LSL#24
		ORR	r5,r5,r4,LSR#8
		MOV	r4,r4,LSL#24
		ORR	r4,r4,r3,LSR#8
		STMDB	r0!,{r4,r5,r6,r7}
		SUBS	r2,r2,#16
		BGE	Ldown_cpy_4_3_lp
		ADDS	r2,r2,#12
		BLT	Ldown_rest3
Ldown_cpy_x_3_lp:
		MOV	ip,r3,LSL#24
		LDR	r3,[r1,#-4]!
		ORR	ip,ip,r3,LSR#8
		STR	ip,[r0,#-4]!
		SUBS	r2,r2,#4
		BGE	Ldown_cpy_x_3_lp
Ldown_rest3:
		ADD	r1,r1,#1
		B	Ldown_no_double_words

		.align

