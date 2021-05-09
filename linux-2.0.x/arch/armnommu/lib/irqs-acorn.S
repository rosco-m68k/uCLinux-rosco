/*
 * linux/arch/arm/lib/irqs.S
 *
 * Copyright (C) 1995, 1996 Russell King. (rmk@ecs.soton.ac.uk)
 *
 * Interrupt wrappers - handles disabling & re-enabling interrupts
 *
 * Changes:
 *  09/02/1996	RMK	Stream-lined normal interrupt wrapper - now uses 78
 *			cycles instead of 85 and 3 less instructions.
 *  08/09/1996	RMK	Changed spec - now does not allow for disable & enable
 *			of the executing interrupt.
 *			IRQ13 is special - it always is entered with IRQs disabled.
 *			IRQ14 & 15 are special - the IRQ is disabled in IOC
 *			 but interrupts are enabled.  It is expected that the
 *			 interrupt routine will re-enable the relevent IRQ...
 */

#include <asm/assembler.h>

@ IRQ stubs

@ IRQ stubs entered with:
@ r1 = IOC
@ r2 = IRQ number
@ return 0 for normal irq

		.text
#define BAD_IRQ(n, msk, reg)				\
		.global	_bad_IRQ##n##_interrupt		;\
_bad_IRQ##n##_interrupt:				;\
		stmfd	sp!, {lr}			;\
		ldrb	r0, [r4, $reg]			;\
		bic	r0, r0, $msk			;\
		strb	r0, [r4, $reg]			;\
		bl	_bad_IRQ			;\
		mov	r0, $1				;\
		LOADREGS(fd, sp!, {pc})

BAD_IRQ( 0, 0x01, 0x18)
BAD_IRQ( 1, 0x02, 0x18)
BAD_IRQ( 2, 0x04, 0x18)
BAD_IRQ( 3, 0x08, 0x18)
BAD_IRQ( 4, 0x10, 0x18)
BAD_IRQ( 5, 0x20, 0x18)
BAD_IRQ( 6, 0x40, 0x18)
BAD_IRQ( 7, 0x80, 0x18)
BAD_IRQ( 8, 0x01, 0x28)
BAD_IRQ( 9, 0x02, 0x28)
BAD_IRQ(10, 0x04, 0x28)
BAD_IRQ(11, 0x08, 0x28)
BAD_IRQ(12, 0x10, 0x28)
BAD_IRQ(13, 0x20, 0x28)
BAD_IRQ(14, 0x40, 0x28)
BAD_IRQ(15, 0x80, 0x28)
BAD_IRQ(16, 0x01, 0x1f8)
BAD_IRQ(17, 0x02, 0x1f8)
BAD_IRQ(18, 0x04, 0x1f8)
BAD_IRQ(19, 0x08, 0x1f8)
BAD_IRQ(20, 0x10, 0x1f8)
BAD_IRQ(21, 0x20, 0x1f8)
BAD_IRQ(22, 0x40, 0x1f8)
BAD_IRQ(23, 0x80, 0x1f8)

#define FAST_IRQ8(n)					\
		.globl	_fast_IRQ##n##_interrupt	;\
_fast_IRQ##n##_interrupt:				;\
		mov	r9, lr				;\
		ldrb	r2, [r4, $0x18]			;\
		bic	r2, r2, $1 << n			;\
		strb	r2, [r4, $0x18]			;\
		mov	r6, $1 << n			;\
		strb	r6, [r4, $0x14]			;\
		ldr	r5, LC1				;\
		ldr	r7, [r5]			;\
		add	r2, r7, $1			;\
		str	r2, [r5]			;\
		bl	_do_fast_IRQ			;\
		DISABLEIRQS(r0)				;\
		str	r7, [r5]			;\
		ldrb	r0, [r4, $0x18]			;\
		orr	r0, r0, $1 << n			;\
		strb	r0, [r4, $0x18]			;\
		mov	r0, $1				;\
		RETINSTR(mov,pc,r9)

#define FAST_IRQ16(n)					\
		.globl	_fast_IRQ##n##_interrupt	;\
_fast_IRQ##n##_interrupt:				;\
		mov	r9, lr				;\
		ldrb	r2, [r4, $0x28]			;\
		bic	r2, r2, $1 << (n - 8)		;\
		strb	r2, [r4, $0x28]			;\
		ldr	r5, LC1				;\
		ldr	r7, [r5]			;\
		add	r2, r7, $1			;\
		str	r2, [r5]			;\
		bl	_do_fast_IRQ			;\
		DISABLEIRQS(r0)				;\
		str	r7, [r5]			;\
		ldrb	r0, [r4, $0x28]			;\
		orr	r0, r0, $1 << (n - 8)		;\
		strb	r0, [r4, $0x28]			;\
		mov	r0, $1				;\
		RETINSTR(mov,pc,r9)

#define FAST_IRQ24(n)					\
		.globl	_fast_IRQ##n##_interrupt	;\
_fast_IRQ##n##_interrupt:				;\
		mov	r9, lr				;\
		ldrb	r2, [r4, $0x1f8]		;\
		bic	r2, r2, $1 << (n - 16)		;\
		strb	r2, [r4, $0x1f8]		;\
		ldr	r5, LC1				;\
		ldr	r7, [r5]			;\
		add	r2, r7, $1			;\
		str	r2, [r5]			;\
		bl	_do_fast_IRQ			;\
		DISABLEIRQS(r0)				;\
		str	r7, [r5]			;\
		ldrb	r0, [r4, $0x1f8]		;\
		orr	r0, r0, $1 << (n - 16)		;\
		strb	r0, [r4, $0x1f8]		;\
		mov	r0, $1				;\
		RETINSTR(mov,pc,r9)

FAST_IRQ8 ( 0)
FAST_IRQ8 ( 1)
FAST_IRQ8 ( 2)
FAST_IRQ8 ( 3)
FAST_IRQ8 ( 4)
FAST_IRQ8 ( 5)
FAST_IRQ8 ( 6)
FAST_IRQ8 ( 7)
FAST_IRQ16( 8)
FAST_IRQ16( 9)
FAST_IRQ16(10)
FAST_IRQ16(11)
FAST_IRQ16(12)

LC1:		.word	_intr_count

		.globl	_fast_IRQ13_interrupt
_fast_IRQ13_interrupt:
		mov	r9, lr
		ldr	r5, LC1
		ldr	r7, [r5]
		add	r2, r7, #1
		str	r2, [r5]
		bl	_do_fast_IRQ
		str	r7, [r5]
		mov	r0, #1
		RETINSTR(mov,pc,r9)

		.globl	_fast_IRQ14_interrupt
_fast_IRQ14_interrupt:
		mov	r9, lr
		ldrb	r2, [r4, #0x28]
		bic	r2, r2, #1 << 6
		strb	r2, [r4, #0x28]
		ldr	r5, LC1
		ldr	r7, [r5]
		add	r2, r7, #1
		str	r2, [r5]
		bl	_do_fast_IRQ
		str	r7, [r5]
		mov	r0, #1
		RETINSTR(mov,pc,r9)

FAST_IRQ16(15)
FAST_IRQ24(16)
FAST_IRQ24(17)
FAST_IRQ24(18)
FAST_IRQ24(19)
FAST_IRQ24(20)
FAST_IRQ24(21)
FAST_IRQ24(22)
FAST_IRQ24(23)

#define NORM_IRQ8(n)					\
		.global	_IRQ##n##_interrupt		;\
_IRQ##n##_interrupt:					;\
		mov	r9, lr				;\
		ldrb	r2, [r4, $0x18]			;\
		bic	r2, r2, $1 << n			;\
		strb	r2, [r4, $0x18]			;\
		mov	r6, $1 << n			;\
		strb	r6, [r4, $0x14]			;\
		ldr	r5, LC2				;\
		ldr	r7, [r5]			;\
		add	r2, r7, $1			;\
		str	r2, [r5]			;\
		ENABLEIRQS(r2)				;\
		bl	_do_IRQ				;\
		DISABLEIRQS(r0)				;\
		str	r7, [r5]			;\
		ldrb	r0, [r4, $0x18]			;\
		orr	r0, r0, $1 << n			;\
		strb	r0, [r4, $0x18]			;\
		mov	r0, $0				;\
		RETINSTR(mov,pc,r9)

#define NORM_IRQ16(n)					\
		.global	_IRQ##n##_interrupt		;\
_IRQ##n##_interrupt:					;\
		mov	r9, lr				;\
		ldrb	r2, [r4, $0x28]			;\
		bic	r2, r2, $1 << (n - 8)		;\
		strb	r2, [r4, $0x28]			;\
		ldr	r5, LC2				;\
		ldr	r7, [r5]			;\
		add	r2, r7, $1			;\
		str	r2, [r5]			;\
		ENABLEIRQS(r2)				;\
		bl	_do_IRQ				;\
		DISABLEIRQS(r2)				;\
		str	r7, [r5]			;\
		ldrb	r0, [r4, $0x28]			;\
		orr	r0, r0, $1 << (n - 8)		;\
		strb	r0, [r4, $0x28]			;\
		mov	r0, $0				;\
		RETINSTR(mov,pc,r9)

#define NORM_IRQ24(n)					\
		.globl	_IRQ##n##_interrupt		;\
_IRQ##n##_interrupt:					;\
		mov	r9, lr				;\
		ldrb	r2, [r4, $0x1f8]		;\
		bic	r2, r2, $1 << (n - 16)		;\
		strb	r2, [r4, $0x1f8]		;\
		ldr	r5, LC2				;\
		ldr	r7, [r5]			;\
		add	r2, r7, $1			;\
		str	r2, [r5]			;\
		ENABLEIRQS(r2)				;\
		bl	_do_IRQ				;\
		DISABLEIRQS(r2)				;\
		str	r7, [r5]			;\
		ldrb	r0, [r4, $0x1f8]		;\
		orr	r0, r0, $1 << (n - 16)		;\
		strb	r0, [r4, $0x1f8]		;\
		mov	r0, $0				;\
		RETINSTR(mov,pc,r9)

NORM_IRQ8 ( 0)
NORM_IRQ8 ( 1)
NORM_IRQ8 ( 2)
NORM_IRQ8 ( 3)
NORM_IRQ8 ( 4)

		.globl	_timer_IRQ_interrupt
_timer_IRQ_interrupt:
		mov	r9, lr
		ldrb	r2, [r4, #0x18]
		bic	r2, r2, #1 << 5
		strb	r2, [r4, #0x18]
		mov	r2, #1 << 5
		strb	r2, [r4, #0x14]
		ldr	r5, LC1
		ldr	r7, [r5]
		add	r2, r7, #1
		str	r2, [r5]
		bl	_do_IRQ
		str	r7, [r5]
		ldrb	r2, [r4, #0x18]
		orr	r2, r2, #1 << 5
		strb	r2, [r4, #0x18]
		mov	r0, #0
		RETINSTR(mov,pc,r9)

NORM_IRQ8 ( 6)
NORM_IRQ8 ( 7)
NORM_IRQ16( 8)
NORM_IRQ16( 9)
NORM_IRQ16(10)
NORM_IRQ16(11)
NORM_IRQ16(12)

LC2:		.word	_intr_count

		.globl	_IRQ13_interrupt
_IRQ13_interrupt:
		mov	r9, lr
		ldr	r5, LC2
		ldr	r7, [r5]
		add	r2, r7, #1
		str	r2, [r5]
		bl	_do_IRQ
		str	r7, [r5]
		mov	r0, #0
		RETINSTR(mov,pc,r9)

		.globl	_IRQ14_interrupt
_IRQ14_interrupt:
		mov	r9, lr
		ldrb	r2, [r4, #0x28]
		bic	r2, r2, #1 << 6
		strb	r2, [r4, #0x28]
		ldr	r5, LC2
		ldr	r7, [r5]
		add	r2, r7, #1
		str	r2, [r5]
		bl	_do_IRQ
		str	r7, [r5]
		mov	r0, #0
		RETINSTR(mov,pc,r9)

NORM_IRQ16(15)
NORM_IRQ24(16)
NORM_IRQ24(17)
NORM_IRQ24(18)
NORM_IRQ24(19)
NORM_IRQ24(20)
NORM_IRQ24(21)
NORM_IRQ24(22)
NORM_IRQ24(23)

#define PROBE_IRQ8(n, v1)				\
		.global _probe_IRQ##n##_interrupt	;\
_probe_IRQ##n##_interrupt:				;\
		ldrb	r0, [r4, $0x18]			;\
		bic	r0, r0, $ v1			;\
		strb	r0, [r4, $0x18]			;\
		mov	r0, $ v1			;\
		strb	r0, [r4, $0x14]			;\
		mov	r0, $1				;\
		RETINSTR(mov,pc,lr)

#define PROBE_IRQ16(n, v1)				\
		.global _probe_IRQ##n##_interrupt	;\
_probe_IRQ##n##_interrupt:				;\
		ldrb	r0, [r4, $0x28]			;\
		bic	r0, r0, $ v1			;\
		strb	r0, [r4, $0x28]			;\
		mov	r0, $1				;\
		RETINSTR(mov,pc,lr)

#define PROBE_IRQ24(n, v1)				\
		.global _probe_IRQ##n##_interrupt	;\
_probe_IRQ##n##_interrupt:				;\
		ldrb	r0, [r4, $0x1f8]		;\
		bic	r0, r0, $ v1			;\
		strb	r0, [r4, $0x1f8]		;\
		mov	r0, $1				;\
		RETINSTR(mov,pc,lr)

PROBE_IRQ8 ( 0,   1)
PROBE_IRQ8 ( 1,   2)
PROBE_IRQ8 ( 2,   4)
PROBE_IRQ8 ( 3,   8)
PROBE_IRQ8 ( 4,  16)
PROBE_IRQ8 ( 5,  32)
PROBE_IRQ8 ( 6,  64)
PROBE_IRQ8 ( 7, 128)
PROBE_IRQ16( 8,   1)
PROBE_IRQ16( 9,   2)
PROBE_IRQ16(10,   4)
PROBE_IRQ16(11,   8)
PROBE_IRQ16(12,  16)
PROBE_IRQ16(13,  32)
PROBE_IRQ16(14,  64)
PROBE_IRQ16(15, 128)
PROBE_IRQ24(16,   1)
PROBE_IRQ24(17,   2)
PROBE_IRQ24(18,   4)
PROBE_IRQ24(19,   8)
PROBE_IRQ24(20,  16)
PROBE_IRQ24(21,  32)
PROBE_IRQ24(22,  64)
PROBE_IRQ24(23, 128)

		.global	_bad_IRQ
_bad_IRQ:	adr	r0, Lmsg
		mov	r1, r2
		b	_printk

Lmsg:		.ascii	"Bad interrupt %d received!\n\0"
		.align
