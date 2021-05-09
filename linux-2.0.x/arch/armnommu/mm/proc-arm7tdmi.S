/*
 * arch/armnommu/mm/proc-arm7tdmi.S
 *
 * based upon:
 *
 * linux/arch/arm/mm/arm6.S: MMU functions for ARM6
 *
 * (C) 1997 Russell King
 *
 * data abort fixups and tdmi extensions:
 *
 * Copyright (C) 2000 WireSpeed Communications Corp
 * Copyright (C) 2000 NETsilicon, Inc.
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
#include "../lib/constants.h"
#define current current_set

	.macro misaligned_load, to_reg, from_addr, temp_reg
	ldrb	\to_reg, [\from_addr, #0]
	ldrb	\temp_reg, [\from_addr, #1]
	orr	\to_reg, \to_reg, \temp_reg, lsl #8
	ldrb	\temp_reg, [\from_addr, #2]
	orr	\to_reg, \to_reg, \temp_reg, lsl #16
	ldrb	\temp_reg, [\from_addr, #3]
	orr	\to_reg, \to_reg, \temp_reg, lsl #24
	.endm
	
	.macro misaligned_store, from_reg, to_addr, temp_reg
	mov	\temp_reg, \from_reg, lsr #24
	strb	\temp_reg, [\to_addr, #3]	
	mov	\temp_reg, \from_reg, lsr #16
	and	\temp_reg, \temp_reg, #255
	strb	\temp_reg, [\to_addr, #2]	
	mov	\temp_reg, \from_reg, lsr #8
	and	\temp_reg, \temp_reg, #255
	strb	\temp_reg, [\to_addr, #1]	
	and	\temp_reg, \from_reg, #255
	strb	\temp_reg, [\to_addr, #0]	
	.endm
	
	.macro misaligned_load_half, to_reg, from_addr, temp_reg
	ldrb	\to_reg, [\from_addr, #0]
	ldrb	\temp_reg, [\from_addr, #1]
	orr	\to_reg, \to_reg, \temp_reg, lsl #8
	.endm
	
	.macro misaligned_store_half, from_reg, to_addr, temp_reg
	mov	\temp_reg, \from_reg, lsr #8
	and	\temp_reg, \temp_reg, #255
	strb	\temp_reg, [\to_addr, #1]	
	and	\temp_reg, \from_reg, #255
	strb	\temp_reg, [\to_addr, #0]	
	.endm
	

/*
 * Function: arm6_7_flush_cache_all (void)
 *	   : arm6_7_flush_cache_page (unsigned long address, int size, int flags)
 *
 * Params  : address	Area start address
 *	   : size	size of area
 *	   : flags	b0 = I cache as well
 *
 * Purpose : Flush all cache lines
 */
arm6_7_flush_cache:
		mov	r0, #0
#ifndef NO_MM
		mcr	p15, 0, r0, c7, c0, 0		@ flush cache
#endif
arm6_7_null:
		mov	pc, lr

/*
 * Function: arm6_7_flush_tlb_all (void)
 *
 * Purpose : flush all TLB entries in all caches
 */
arm6_7_flush_tlb_all:
		mov	r0, #0
#ifndef NO_MM
		mcr	p15, 0, r0, c5, c0, 0		@ flush TLB
#endif
		mov	pc, lr

/*
 * Function: arm6_7_flush_tlb_page (unsigned long address, int end, int flags)
 *
 * Params  : address	Area start address
 *	   : end	Area end address
 *	   : flags	b0 = I cache as well
 *
 * Purpose : flush a TLB entry
*/
arm6_7_flush_tlb_area:
#ifndef NO_MM
1:		mcr	p15, 0, r0, c6, c0, 0		@ flush TLB
		add	r0, r0, #4096
		cmp	r0, r1
		blt	1b
#endif
		mov	pc, lr

LC0:		.word	current


/*
 * Function: arm6_7_switch_to (struct task_struct *prev, struct task_struct *next)
 *
 * Params  : prev	Old task structure
 *	   : next	New task structure for process to run
 *
 * Purpose : Perform a task switch, saving the old processes state, and restoring
 *	     the new.
 *
 * Notes   : We don't fiddle with the FP registers here - we postpone this until
 *	     the new task actually uses FP.  This way, we don't swap FP for tasks
 *	     that do not require it.
 */
arm6_7_switch_to:
		stmfd	sp!, {r4 - r10, fp, lr}		@ Store most regs on stack
		mrs	ip, cpsr
		stmfd	sp!, {ip}			@ Save cpsr_SVC
		str	sp, [r0, #TSS_SAVE]		@ Save sp_SVC
		ldr	r2, LC0
		str	r1, [r2]
		ldr	sp, [r1, #TSS_SAVE]		@ Get saved sp_SVC
#ifndef NO_MM 
		ldr	r0, [r1, #TSS_MEMMAP]		@ Page table pointer
		mov	r1, #0
		mcr	p15, 0, r1, c7, c0, 0		@ flush cache
		mcr	p15, 0, r0, c2, c0, 0		@ update page table ptr
		mcr	p15, 0, r1, c5, c0, 0		@ flush TLBs
#endif
		ldmfd	sp!, {ip}
		msr	spsr, ip			@ Save tasks CPSR into SPSR for this return
		ldmfd	sp!, {r4 - r10, fp, pc}^		@ Load all regs saved previously

/*
 * Function: arm6_7_data_abort ()
 *
 * Params  : r0 = address of aborted instruction
 *
 * Purpose : obtain information about current aborted instruction
 *
 * Returns : r0 = address of abort
 *	   : r1 = FSR
 *	   : r2 != 0 if writing
 *	   : sp = pointer to registers
 *
 * Notes   : data abort *** NEEDS WORK ***
 */

Lukabttxt:	.ascii	"Unknown data abort code %d [pc=%p, *pc=%p] LR=%p\0"
		.align

msg:		.ascii	"DA*%p=%p\n\0"
		.align

Ldata_unknown:	@ Part of jumptable
		ldr	r3, [sp, #15 * 4]		@ Get PC
		str	r3, [sp, #-4]!
		mov	r1, r1, lsr #2
		mov	r3, r4
		mov	r2, r0
		adr	r0, Lukabttxt
		bl	panic
Lstop:		b	Lstop

/* for the 7tdmi case, this routine attepts to recover from misaligned reads */
/* if an alignment problem is not detected, we force the program to abort.   */
/* all errors are assumed to be alignment problems, since them mmu would     */
/* implement the abort status, and we have no mmu therefore we guess         */

/* Another issue which I did not address here is that the value in the       */
/* index register after a data abort is "IMPLEMENTATION DEFINED", so on some */
/* arm processors it gets updated even if the load failed. The flavor I'm    */
/* using updates the index, This code has only been tested on such an        */
/* architecture! I'm not sure if all 7tdmi's are the same on this issue...   */
/* --Joe deBlaquiere ( so you know who to blame ) */

arm7_data_abort:
		ldr	r4, [r0]			@ read instruction causing problem
@		mov	r2, r4, lsr #19			@ r2 b1 = L
		and	r1, r4, #15 << 24
		add	pc, pc, r1, lsr #22		@ Now branch to the relevent processing routine
		movs	pc, lr
							@ branch table based on
							@ bits [27:24] of inst
		b	Ldata_halfword			@ 0 ldrh,ldrsb,ldrsh
		b	Ldata_halfword			@ 1 ldrh,ldrsb,ldrsh
		b	Ldata_unknown			@ 2
		b	Ldata_unknown			@ 3
		b	Ldata_lateldrpostconst		@ 4 ldr(b) rd, [rn], #m
		b	Ldata_lateldrpreconst		@ 5 ldr(b) rd, [rn, #m]	@ RegVal
		b	Ldata_lateldrpostreg		@ 6 ldr(b) rd, [rn], rm
		b	Ldata_lateldrprereg		@ 7 ldr(b) rd, [rn, rm]
		b	Ldata_unknown			@ 8 ldm*a	rn, <rlist>
		b	Ldata_unknown			@ 9 ldm*b	rn, <rlist>
		b	Ldata_unknown			@ A
		b	Ldata_unknown			@ B
		b	Ldata_unknown			@ C ldc	rd, [rn], #m	@ Same as ldr	rd, [rn], #m
		b	Ldata_unknown			@ D ldc	rd, [rn, #m]
		b	Ldata_unknown			@ E 
		b	Ldata_unknown			@ F


Ldata_halfword:
		tst	r4, #1 << 5			@ test halfword bit
		beq	Lret_fail			@ return fail if byte op
		and	r1, r4, #15 << 21
		add	pc, pc, r1, lsr #19		@ Now branch to the relevent processing routine
		movs	pc, lr
							@ branch table based on
							@ bits [27:24] of inst

@ NOTE : if the P bit is clear, then the W bit must be clear, however
@ writeback is still implied! ( if that isn't confusing, don't know what is )

		b	Ldata_halfpostreg		@ 0 sub reg implied wb
		b	Ldata_unknown			@ 1 sub reg 	INVALID
		b	Ldata_halfpostimm		@ 2 sub imm implied wb
		b	Ldata_unknown			@ 3 sub imm 	INVALID
		b	Ldata_halfpostreg		@ 4 add reg implied wb
		b	Ldata_unknown			@ 5 add reg 	INVALID
		b	Ldata_halfpostimm		@ 6 add imm implied wb
		b	Ldata_unknown			@ 7 add imm 	INVALID
		b	Ldata_halfprereg		@ 8 pre sub reg
		b	Ldata_halfprewb			@ 9 pre sub reg wb
		b	Ldata_halfpreimm		@ A pre sub imm
		b	Ldata_halfprewb			@ B pre sub imm wb
		b	Ldata_halfprereg		@ C pre add reg
		b	Ldata_halfprewb			@ D pre add reg wb
		b	Ldata_halfpreimm		@ E pre add imm
		b	Ldata_halfprewb			@ F pre add imm wb

		mov	r0, #1				@ return fail
		mov	pc, lr

Ldata_halfprereg:
		and	r6, r4, #15			@ Get Rm - reg num
		ldr	r1, [sp, r6, lsl #2]		@ r1 = Rm value
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
		tst	r4, #1 << 23		
		subeq	r0, r0, r1			@ apply pre-index
		addne	r0, r0, r1
		b	Ldata_half_fini
				

Ldata_halfpostreg:
		and	r6, r4, #15			@ Get Rm - reg num
		ldr	r1, [sp, r6, lsl #2]			@ r1 = Rm value
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
		tst	r4, #1 << 23		
		addeq	r0, r0, r1			@ reverse post-index
		subne	r0, r0, r1
		b	Ldata_half_fini
				

Ldata_halfpostimm:
		and	r1, r4, #15			@ get imm val LSnibble
		and	r6, r4, #15 << 8		@ get imm val MSnibble
		orr	r1, r1, r6, lsr #4		@ r1 = 8 bit imm val
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
		tst	r4, #1 << 23		
		addeq	r0, r0, r1			@ reverse post-index
		subne	r0, r0, r1
		b	Ldata_half_fini
		
Ldata_halfpreimm:
		and	r1, r4, #15			@ get imm val LSnibble
		and	r6, r4, #15 << 8		@ get imm val MSnibble
		orr	r1, r1, r6, lsr #4		@ r1 = 8 bit imm val
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
		tst	r4, #1 << 23		
		subeq	r0, r0, r1			@ effect pre-index
		addne	r0, r0, r1
		b	Ldata_half_fini
		
Ldata_halfprewb:
		and	r1, r4, #15			@ get imm val LSnibble
		and	r6, r4, #15 << 8		@ get imm val MSnibble
		orr	r1, r1, r6, lsr #4		@ r1 = 8 bit imm val
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
Ldata_half_fini:
		tst	r0, #3				@ test for misalignment
		beq	Lret_fail			@ return error if aligned
		and	r3, r4, #15 << 12		@ get Rd - reg num
		tst	r4, #1 << 20			@ check load bit
		beq	1f				@ branch for store
		misaligned_load_half r1, r0, r2		@ ld r1 from r0 addr,r2tmp
		tst	r4, #1 << 6			@ test signed bit
		movne	r1, r1, lsl #16			@ if signed hword xfer
		movne	r1, r1, asr #16			@ fix sign bit - arith
		str	r1, [sp, r3, lsr #10]		@ update destination reg
		b	2f
1:
		ldr	r1, [sp, r3, lsr #10]		@ load source reg
		misaligned_store_half r1, r0, r2	@ str r1 to r0 addr,r2tmp
2:
		ldr	r1, [sp, #60]			@ load pc
		add	r1, r1, #4			@ point to next inst
		str	r1, [sp, #60]			@ fix context
		mov	r0, #0
		mov	pc, lr				@ return success
		

/* post-increment index register with a constant */

Ldata_lateldrpostconst:
		tst	r4, #1 << 22			@ check for ubyte op
		bne	Lret_fail			@ if ubyte ret fail
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
		mov	r1, r4, lsl #20			@ Get offset
		tst	r4, #1 << 23			
		addeq	r0, r0, r1, lsr #20		@ reverse effect of inst
		subne	r0, r0, r1, lsr #20
		b	Ldata_word_fini


Ldata_lateldrpreconst:
		tst	r4, #1 << 22			@ check for ubyte op
		bne	Lret_fail			@ if ubyte ret fail
		movs	r1, r4, lsl #20			@ Get offset
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = reg val
		tst	r4, #1 << 21			@ check writeback bit
		bne	Ldata_word_fini			@ register already update
		tst	r4, #1 << 23			@ U bit +/- offset ?
		addne	r0, r0, r1, lsr #20		@ U = 0 -> Ad = Rn - off
		subeq	r0, r0, r1, lsr #20		@ U = 1 -> Ad = Rn + off
		b	Ldata_word_fini


Ldata_lateldrpostreg:
		tst	r4, #1 << 22			@ check for ubyte op
		bne	Lret_fail			@ if ubyte ret fail
		and	r5, r4, #15			@ Get Rm - reg num
		ldr	r1, [sp, r5, lsl #2]		@ r1 = Rm value
		mov	r3, r4, lsr #7
		ands	r3, r3, #31			@ r3 = shift count
		and	r6, r4, #0x70			@ r6 = shift type
		orreq	r6, r6, #8			@ if shift=0, set 1<<3 
		add	pc, pc, r6
		mov	r0, r0

		mov	r1, r1, lsl r3			@ 0: LSL #!0
		b	1f
		b	1f				@ 1: LSL #0
		mov	r0, r0
		b	Lret_fail			@ 2: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ 3: illegal inst?
		mov	r0, r0
		mov	r1, r1, lsr r3			@ 4: LSR #!0
		b	1f
		mov	r1, r1, lsr #32			@ 5: LSR #32
		b	1f
		b	Lret_fail			@ 6: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ 7: illegal inst?
		mov	r0, r0
		mov	r1, r1, asr r3			@ 8: ASR #!0
		b	1f
		mov	r1, r1, asr #32			@ 9: ASR #32
		b	1f
		b	Lret_fail			@ A: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ B: illegal inst?
		mov	r0, r0
		mov	r1, r1, ror r3			@ C: ROR #!0
		b	1f
		mov	r1, r1, rrx			@ D: RRX
		b	1f
		b	Lret_fail			@ E: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ F: illegal inst?
		mov	r0, r0

1:
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
		tst	r4, #1 << 23			@ U bit
		subne	r0, r0, r1			@ U = 0 -> Ad = Rn - off
		addeq	r0, r0, r1			@ U = 1 -> Ad = Rn + off
		b	Ldata_word_fini


Ldata_lateldrprereg:
		tst	r4, #1 << 22			@ check for ubyte op
		bne	Lret_fail			@ if ubyte ret fail
		and	r5, r4, #15			@ Get Rm - reg num
		ldr	r1, [sp, r5, lsl #2]		@ r1 = Rm value
		mov	r3, r4, lsr #7
		ands	r3, r3, #31			@ r3 = shift count
		and	r6, r4, #0x70			@ r6 = shift type
		orreq	r6, r6, #8			@ if shift=0, set 1<<3
		add	pc, pc, r6
		mov	r0, r0

		mov	r1, r1, lsl r3			@ 0: LSL #!0
		b	1f
		b	1f				@ 1: LSL #0
		mov	r0, r0
		b	Lret_fail			@ 2: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ 3: illegal inst?
		mov	r0, r0
		mov	r1, r1, lsr r3			@ 4: LSR #!0
		b	1f
		mov	r1, r1, lsr #32			@ 5: LSR #32
		b	1f
		b	Lret_fail			@ 6: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ 7: illegal inst?
		mov	r0, r0
		mov	r1, r1, asr r3			@ 8: ASR #!0
		b	1f
		mov	r1, r1, asr #32			@ 9: ASR #32
		b	1f
		b	Lret_fail			@ A: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ B: illegal inst?
		mov	r0, r0
		mov	r1, r1, ror r3			@ C: ROR #!0
		b	1f
		mov	r1, r1, rrx			@ D: RRX
		b	1f
		b	Lret_fail			@ E: illegal inst?
		mov	r0, r0
		b	Lret_fail			@ F: illegal inst?
		mov	r0, r0

1:
		and	r5, r4, #15 << 16		@ Get Rn - reg num
		ldr	r0, [sp, r5, lsr #14]		@ r0 = Rn value
		tst	r4, #1 << 21			@ check writeback bit
		bne	Ldata_word_fini			@ register already update
		tst	r4, #1 << 23			@ U bit +/- offset?
							@ reverse update
		addne	r0, r0, r1			@ U = 0 -> Ad = Rn - off
		subeq	r0, r0, r1			@ U = 1 -> Ad = Rn + off
Ldata_word_fini:
		tst	r0, #3				@ if the addr misaligned?
		beq	Lret_fail			@ return error if not
		and	r3, r4, #15 << 12		@ get Rd - reg num
		tst	r4, #1 << 20			@ check load bit
		beq	1f				@ branch for store
		misaligned_load r1, r0, r2		@ ld r1 from r0 addr,r2tmp
		str	r1, [sp, r3, lsr #10]		@ update destination reg
		b	2f
1:
		ldr	r1, [sp, r3, lsr #10]		@ load source reg
		misaligned_store r1, r0, r2		@ str r1 to r0 addr,r2tmp
2:
		ldr	r1, [sp, #60]			@ load pc
		add	r1, r1, #4			@ point to next inst
		str	r1, [sp, #60]			@ fix context
		mov	r0, #0
		mov	pc, lr				@ return success
Lret_fail:
		mov	r0, #1				@ return fail
		mov	pc, lr				@

/*
 * Function: arm6_7_check_bugs (void)
 *	   : arm6_7_proc_init (void)
 *	   : arm6_7_proc_fin (void)
 *
 * Notes   : This processor does not require these
 */
arm6_7_check_bugs:
		mrs	ip, cpsr
		bic	ip, ip, #F_BIT
		msr	cpsr, ip
arm6_7_proc_init:
arm6_7_proc_fin:
		mov	pc, lr


/*
 * Function: arm7_set_pmd ()
 *
 * Params  : r0 = Address to set
 *	   : r1 = value to set
 *
 * Purpose : Set a PMD and flush it out of any WB cache
 */
arm7_set_pmd:
#ifndef NO_MM
	orr	r1, r1, #16		@ Updatable bit is always set on ARM7
		str	r1, [r0]
#endif
		mov	pc, lr

/*
 * Function: arm6_7_reset
 *
 * Notes   : This sets up everything for a reset
 */
arm6_7_reset:	mrs	r1, cpsr
		orr	r1, r1, #F_BIT|I_BIT
		msr	cpsr, r1
		mov	r0, #0
#ifndef NO_MM
		mcr	p15, 0, r0, c7, c0, 0		@ flush cache
		mcr	p15, 0, r0, c5, c0, 0		@ flush TLB
#endif
		mov	r1, #F_BIT | I_BIT | 3
		mov	pc, lr

/*
 * Purpose : Function pointers used to access above functions - all calls
 *	     come through these
 */
arm7_name:	.ascii	"arm7\0"
		.align

		.globl	arm7_processor_functions
arm7_processor_functions:
		.word	arm7_name			@  0
		.word	arm6_7_switch_to		@  4
		.word	arm7_data_abort			@  8
		.word	arm6_7_check_bugs		@ 12
		.word	arm6_7_proc_init		@ 16
		.word	arm6_7_proc_fin			@ 20
		.word	arm6_7_flush_cache		@ 24
		.word	arm6_7_flush_cache		@ 28
		.word	arm6_7_flush_cache		@ 32
		.word	arm6_7_null			@ 36
		.word	arm6_7_flush_cache		@ 40
		.word	arm6_7_flush_tlb_all		@ 44
		.word	arm6_7_flush_tlb_area		@ 48
		.word	arm7_set_pmd			@ 52
		.word	arm6_7_reset			@ 54
