/*
 * linux/arch/arm/lib/irqs.S
 *
 * Copyright (C) 1995, 1996 Russell King. (rmk@ecs.soton.ac.uk)
 *
 * Interrupt wrappers - handles disabling & re-enabling interrupts
 */

#include <asm/assembler.h>

@ IRQ stubs

@ IRQ stubs entered with:
@ r0 = IRQ number
@ r1 = regs
@ r4 = IRQ_MSKD		(only for CTB_OS)
@ return 0 for normal irq

		.text
/*
 * Disable interrupt & print a message
 */
LC0:		.long	_intr_count

		.globl	_bad_IRQ_interrupt
_bad_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0
		strb	r5, [r4]
		bl	_bad_IRQ
		mov	r0, #0
		mov	pc, r9

		.globl	_fast_IRQ_interrupt
_fast_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0
		strb	r5, [r4]
		ldr	r6, LC0
		ldr	r7, [r6]
		add	r2, r7, #1
		str	r2, [r6]
		bl	_do_IRQ
		mrs	r2, cpsr
		orr	r2, r2, #I_BIT
		msr	cpsr, r2
		str	r7, [r6]
		sub	r4, r4, #0x400000
		strb	r5, [r4]
		mov	r0, #1
		mov	pc, r9

		.globl	_IRQ_interrupt
_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0
		strb	r5, [r4]			@ Disable interrupt source
		ldr	r6, LC0
		ldr	r7, [r6]
		add	r2, r7, #1
		str	r2, [r6]
		mrs	r8, cpsr			@ Enable interrupts
		bic	r2, r8, #I_BIT
		msr	cpsr, r2
		bl	_do_IRQ
		msr	cpsr, r8			@ Restore interrupts
		str	r7, [r6]
		sub	r4, r4, #0x400000
		strb	r5, [r4]			@ Re-enable interrupt source
		mov	r0, #0
		mov	pc, r9

		.globl	_timer_IRQ_interrupt
_timer_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0
		strb	r5, [r4]			@ Disable interrupt source
		ldr	r6, LC0
		ldr	r7, [r6]
		add	r2, r7, #1
		str	r2, [r6]
		mrs	r8, cpsr
		bl	_do_IRQ
		msr	cpsr, r8			@ Restore interrupts
		str	r7, [r6]
		sub	r4, r4, #0x400000
		strb	r5, [r4]			@ Re-enable interrupt source
		mov	r0, #0
		mov	pc, r9

		.globl	_probe_IRQ_interrupt
_probe_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0
		add	r6, r4, #0x400000
		strb	r5, [r6]
		ldr	r6, LC0+4
		ldrb	r4, [r6]
		bic	r4, r4, r5
		strb	r4, [r6]
		mov	pc, r9

		.global	_bad_IRQ
_bad_IRQ:	mov	r1, r0
		adr	r0, Lmsg
		b	_printk

Lmsg:		.ascii	"Bad interrupt %d received!\n\0"
		.align
