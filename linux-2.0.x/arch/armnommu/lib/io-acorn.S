/*
 * linux/arch/arm/lib/io.S
 *
 * Copyright (C) 1995, 1996 Russell King
 */
#include <linux/autoconf.h>
#include <asm/assembler.h>
#include <asm/hardware.h>

		.text
		.align

#define OUT(reg)						\
		mov	r8, reg, lsl $16			;\
		orr	r8, r8, r8, lsr $16			;\
		str	r8, [r3, r0, lsl $2]			;\
		mov	r8, reg, lsr $16			;\
		orr	r8, r8, r8, lsl $16			;\
		str	r8, [r3, r0, lsl $2]

#define IN(reg)							\
		ldr	reg, [r0]				;\
		and	reg, reg, ip				;\
		ldr	lr, [r0]				;\
		orr	reg, reg, lr, lsl $16

#if (IO_BASE == (PCIO_BASE & 0xff000000))
#define ADDR(off,reg)						\
		tst	off, $0x80000000			;\
		mov	reg, $IO_BASE				;\
		orreq	reg, reg, $(PCIO_BASE & 0x00ff0000)
#else
#define ADDR(off,reg)						\
		tst	off, $0x80000000			;\
		movne	reg, $IO_BASE				;\
		moveq	reg, $(PCIO_BASE & 0xff000000)		;\
		orreq	reg, reg, $(PCIO_BASE & 0x00ff0000)
#endif

@ purpose: read a byte from a hardware register.
@ Proto  : unsigned char inb(int address);

		.global	___inb
		.global	___inb_p
		.global	___inbc
		.global	___inbc_p
___inb:
___inb_p:
___inbc:
___inbc_p:
		ADDR(r0,r2)
		ldrb	r0, [r2, r0, lsl#2]
		RETINSTR(mov,pc,lr)

@ purpose: read a short word from a hardware register.
@ Proto  : unsigned short inw(int address);

		.global	___inw
___inw:		ADDR(r0,r2)
		ldr	r0, [r2, r0, lsl#2]
		bic	r0, r0, #0xff000000
		bic	r0, r0, #0x00ff0000
		RETINSTR(mov,pc,lr)

@ Purpose: write a byte to a hardware register.
@ Proto  : outb(unsigned char c,int address);

		.global	___outb
		.global	___outb_p
		.global	___outbc
		.global	___outbc_p
___outb:
___outb_p:
___outbc:
___outbc_p:
		ADDR(r1,r2)
		strb	r0,[r2,r1,LSL#2]
		RETINSTR(mov,pc,lr)

@ Purpose: write a short to a hardware register.
@ Proto  : outw(unsigned short c,int address);

		.global	___outw
___outw:	ADDR(r1,r2)
		orr	r0, r0, r0, lsl#16
		str	r0, [r2, r1, lsl#2]
		RETINSTR(mov,pc,lr)

@ Purpose: read a block of data from a hardware register to memory.
@ Proto  : insw(int from_port, void *to, int len_in_words);
@ Proto  : inswb(int from_port, void *to, int len_in_bytes);
@ Notes  : increment to

		.global	_insw
		.global	_inswb
_insw:		mov	r2, r2, lsl#1
_inswb:		mov	ip, sp
		stmfd	sp!, {r4 - r10 ,fp ,ip ,lr ,pc}
		sub	fp, ip, #4
		ADDR(r0,r3)
		add	r0, r3, r0, lsl #2
		tst	r1, #3
		beq	Linswok
		tst	r1, #1
		bne	Linsw_notaligned
		cmp	r2, #1
		ldrge	r4, [r0]
		strgeb	r4, [r1], #1
		movgt	r4, r4, LSR#8
		strgtb	r4, [r1], #1
		ldmleea	fp, {r4 - r10, fp, sp, pc}^
		sub	r2, r2, #2
Linswok:	mov	ip, #0xFF
		orr	ip, ip, ip, lsl #8
Linswlp:	subs	r2, r2, #64
		bmi	Linsw_toosmall
		IN(r3)
		IN(r4)
		IN(r5)
		IN(r6)
		IN(r7)
		IN(r8)
		IN(r9)
		IN(r10)
		stmia	r1!, {r3 - r10}
		IN(r3)
		IN(r4)
		IN(r5)
		IN(r6)
		IN(r7)
		IN(r8)
		IN(r9)
		IN(r10)
		stmia	r1!, {r3 - r10}
		bne	Linswlp
		LOADREGS(ea, fp, {r4 - r10, fp, sp, pc})
Linsw_toosmall:
		adds	r2, r2, #32
		bmi	Linsw_toosmall2
Linsw2lp:	IN(r3)
		IN(r4)
		IN(r5)
		IN(r6)
		IN(r7)
		IN(r8)
		IN(r9)
		IN(r10)
		stmia	r1!, {r3 - r10}
		LOADREGS(eqea, fp, {r4 - r10, fp, sp, pc})
		b	Linsw_notaligned
Linsw_toosmall2:
		add	r2, r2, #32
Linsw_notaligned:
		cmp	r2, #1
		LOADREGS(ltea, fp, {r4 - r10, fp, sp, pc})
		ldr	r4, [r0]
		strb	r4, [r1], #1
		movgt	r4, r4, LSR#8
		strgtb	r4, [r1], #1
		subs	r2, r2, #2
		bgt	Linsw_notaligned
		LOADREGS(ea, fp, {r4 - r10, fp, sp, pc})

@ Purpose: write a block of data from memory to a hardware register.
@ Proto  : outsw(int to_reg, void *from, int len_in_words);
@ Proto  : outswb(int to_reg, void *from, int len_in_bytes);
@ Notes  : increments from

		.global	_outsw
		.global	_outswb
_outsw:		mov	r2, r2, LSL#1
_outswb:	mov	ip, sp
		stmfd	sp!, {r4 - r8, fp, ip, lr, pc}
		sub	fp, ip, #4
		ADDR(r0,r3)
		tst	r1, #2
		beq	Loutsw32lp
		ldr	r4, [r1], #2
		mov	r4, r4, lsl #16
		orr	r4, r4, r4, lsr #16
		str	r4, [r3, r0, lsl #2]
		sub	r2, r2, #2
		teq	r2, #0
		LOADREGS(eqea, fp, {r4 - r8, fp, sp, pc})
Loutsw32lp:	subs	r2,r2,#32
		blt	Loutsw_toosmall
		ldmia	r1!,{r4,r5,r6,r7}
		OUT(r4)
		OUT(r5)
		OUT(r6)
		OUT(r7)
		ldmia	r1!,{r4,r5,r6,r7}
		OUT(r4)
		OUT(r5)
		OUT(r6)
		OUT(r7)
		LOADREGS(eqea, fp, {r4 - r8, fp, sp, pc})
		b	Loutsw32lp
Loutsw_toosmall:
		adds	r2,r2,#32
		LOADREGS(eqea, fp, {r4 - r8, fp, sp, pc})
Llpx:		ldr	r4,[r1],#2
		mov	r4,r4,LSL#16
		orr	r4,r4,r4,LSR#16
		str	r4,[r3,r0,LSL#2]
		subs	r2,r2,#2
		bgt	Llpx
		LOADREGS(ea, fp, {r4 - r8, fp, sp, pc})

@ Purpose: write a memc register
@ Proto  : void memc_write(int register, int value);
@ Returns: nothing

#ifndef CONFIG_ARCH_RPC
		.globl	_memc_write
_memc_write:	cmp	r0, #7
		RETINSTR(movgt,pc,lr)
		mov	r0, r0, lsl #17
		mov	r1, r1, lsl #15
		mov	r1, r1, lsr #17
		orr	r0, r0, r1, lsl #2
		add	r0, r0, #0x03600000
		strb	r0, [r0]
		RETINSTR(mov,pc,lr)
#define CPSR2SPSR(rt)
#else
#define CPSR2SPSR(rt) \
		mrs	rt, cpsr; \
		msr	spsr, rt
#endif

@ Purpose: call an expansion card loader to read bytes.
@ Proto  : char read_loader(int offset, char *card_base, char *loader);
@ Returns: byte read

		.global	_ecard_loader_read
_ecard_loader_read:
		stmfd	sp!, {r4 - r12, lr}
		mov	r11, r1
		mov	r1, r0
		CPSR2SPSR(r0)
		mov	lr, pc
		mov	pc, r2
		LOADREGS(fd, sp!, {r4 - r12, pc})

@ Purpose: call an expansion card loader to reset the card
@ Proto  : void read_loader(int card_base, char *loader);
@ Returns: byte read

		.global	_ecard_loader_reset
_ecard_loader_reset:
		stmfd	sp!, {r4 - r12, lr}
		mov	r11, r0
		CPSR2SPSR(r0)
		mov	lr, pc
		add	pc, r1, #8
		LOADREGS(fd, sp!, {r4 - r12, pc})
