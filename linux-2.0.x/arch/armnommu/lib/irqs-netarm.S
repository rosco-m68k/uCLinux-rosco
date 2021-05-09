/*
 * linux/arch/armnommu/lib/irqs-netarm.S
 *
 * based upon:
 *
 *   linux/arch/arm/lib/irqs.S
 *   Copyright (C) 1995, 1996 Russell King. (rmk@ecs.soton.ac.uk)
 *   Interrupt wrappers - handles disabling & re-enabling interrupts
 *
 * modifications for NET+ARM are subject to:
 *
 * Copyright (C) 2000 NETsilicon, Inc.
 * Copyright (C) 2000 WireSpeed Communications Corporation
 *
 * This software is copyrighted by WireSpeed. LICENSEE agrees that
 * it will not delete this copyright notice, trademarks or protective
 * notices from any copy made by LICENSEE.
 *
 * This software is provided "AS-IS" and any express or implied 
 * warranties or conditions, including but not limited to any
 * implied warranties of merchantability and fitness for a particular
 * purpose regarding this software. In no event shall WireSpeed
 * be liable for any indirect, consequential, or incidental damages,
 * loss of profits or revenue, loss of use or data, or interruption
 * of business, whether the alleged damages are labeled in contract,
 * tort, or indemnity.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * author(s) : Joe deBlaquiere
 */

#include <asm/assembler.h>
#include <asm/arch/netarm_registers.h>

#define	_do_IRQ		do_IRQ
#define	_printk		printk
#define	_intr_count	intr_count

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

		.globl	_bad_IRQ_interrupt, bad_IRQ_interrupt
_bad_IRQ_interrupt:
bad_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0
		strb	r5, [r4]
		bl	_bad_IRQ
		mov	r0, #0
		mov	pc, r9

		.globl	_fast_IRQ_interrupt, fast_IRQ_interrupt
_fast_IRQ_interrupt:
fast_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0

		@ Disable interrupt source

		ldr	r2, =(NETARM_GEN_MODULE_BASE + NETARM_GEN_INTR_ENABLE_CLR)
		str	r5, [r2]

		ldr	r6, LC0
		ldr	r7, [r6]
		add	r2, r7, #1
		str	r2, [r6]
		mrs	r8, cpsr
		bl	_do_IRQ
		msr	cpsr, r8			@ Restore interrupts
		str	r7, [r6]

		@ Re-enable interrupt source

		ldr	r2, =(NETARM_GEN_MODULE_BASE + NETARM_GEN_INTR_ENABLE_SET)
		str	r5, [r2]

		mov	r0, #0
		mov	pc, r9

		.globl	_IRQ_interrupt, IRQ_interrupt
_IRQ_interrupt:
IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0

		@ Disable interrupt source

		ldr	r2, =(NETARM_GEN_MODULE_BASE + NETARM_GEN_INTR_ENABLE_CLR)
		str	r5, [r2]

		ldr	r6, LC0
		ldr	r7, [r6]
		add	r2, r7, #1
		str	r2, [r6]
		mrs	r8, cpsr
		bl	_do_IRQ
		msr	cpsr, r8			@ Restore interrupts
		str	r7, [r6]

		@ Re-enable interrupt source

		ldr	r2, =(NETARM_GEN_MODULE_BASE + NETARM_GEN_INTR_ENABLE_SET)
		str	r5, [r2]

		mov	r0, #0
		mov	pc, r9

		.globl	_timer_IRQ_interrupt, timer_IRQ_interrupt
_timer_IRQ_interrupt:
timer_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0

		@ Disable interrupt source

		ldr	r2, =(NETARM_GEN_MODULE_BASE + NETARM_GEN_INTR_ENABLE_CLR)
		str	r5, [r2]
		ldr	r2, =(NETARM_GEN_MODULE_BASE + NETARM_GEN_TIMER2_STATUS)

		mov	r5, #NETARM_GEN_TSTAT_INTPEN	@ clear timer INTPEN
		str	r5, [r2]
		mov	r5, #1				@ restore mask
		mov	r5, r5, lsl r0		

		ldr	r6, LC0
		ldr	r7, [r6]
		add	r2, r7, #1
		str	r2, [r6]
		mrs	r8, cpsr
		bl	_do_IRQ
		msr	cpsr, r8			@ Restore interrupts
		str	r7, [r6]

		@ Re-enable interrupt source

		ldr	r2, =(NETARM_GEN_MODULE_BASE + NETARM_GEN_INTR_ENABLE_SET)
		str	r5, [r2]

		mov	r0, #0
		mov	pc, r9

		.globl	_probe_IRQ_interrupt, probe_IRQ_interrupt
_probe_IRQ_interrupt:
probe_IRQ_interrupt:
		mov	r9, lr
		mov	r5, #1
		mov	r5, r5, lsl r0
@		add	r6, r4, #0x400000
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
