/*
 * linux/arch/arm/lib/traps.S
 *
 * Copyright (C) 1995, 1996 Russell King.
 *
 * Low-level vector interface routines
 *
 * Design issues:
 *  - We have several modes that each vector can be called from,
 *    each with its own set of registers.  On entry to any vector,
 *    we *must* save the registers used in *that* mode.
 *
 *  - This code must be as fast as possible.
 *
 * There are a few restrictions on the vectors:
 *  - the SWI vector cannot be called from *any* non-user mode
 *
 *  - the FP emulator is *never* called from *any* non-user mode undefined
 *    instruction.
 *
 * Ok, so this file may be a mess, but its as efficient as possible while
 * adhering to the above criteria.
 */

#include <asm/assembler.h>
#include <asm/errno.h>
#include <asm/hardware.h>

#if 0
/*
 * Uncomment these if you wish to get more debugging into about data aborts.
 */
#define FAULT_CODE_LDRSTRPOST	0x80
#define FAULT_CODE_LDRSTRPRE	0x40
#define FAULT_CODE_LDRSTRREG	0x20
#define FAULT_CODE_LDMSTM	0x10
#define FAULT_CODE_LDCSTC	0x08
#endif
#define FAULT_CODE_PREFETCH	0x04
#define FAULT_CODE_WRITE	0x02
#define FAULT_CODE_USER		0x01

		.text

/* Offsets into task structure
 * ---------------------------
 */
#define STATE		0
#define COUNTER		4
#define PRIORITY	8
#define SIGNAL		12
#define BLOCKED		16
#define FLAGS		20
#define ERRNO		24

#define PF_TRACESYS	0x20

/* Bad Abort numbers
 * -----------------
 */
#define BAD_PREFETCH	0
#define BAD_DATA	1
#define BAD_ADDREXCPTN	2
#define BAD_IRQ		3

/* OS version number used in SWIs
 *  RISC OS is 0
 *  RISC iX is 8
 */
#define OS_NUMBER	9

/* Stack format (ensured by USER_* and SVC_*)
 */
#define S_OLD_R0	64
#define S_PC		60
#define S_LR		56
#define S_SP		52
#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#include "../lib/constants.h"

#define USER_SAVE_ALL				\
		str	r0, [sp, #-4]!		;\
		str	lr, [sp, #-4]!		;\
		sub	sp, sp, #15*4		;\
		stmia	sp, {r0 - lr}^		;\
		mov	r0, r0			;\
		mov	fp, #0

#define SVC_SAVE_ALL				\
		str	sp, [sp, #-16]!		;\
		str	lr, [sp, #8]		;\
		str	lr, [sp, #4]		;\
		stmfd	sp!, {r0 - r12}		;\
		mov	r0, #-1			;\
		str	r0, [sp, #S_OLD_R0]	;\
		mov	fp, #0

#define SVC_IRQ_SAVE_ALL			\
		str	sp, [sp, #-16]!		;\
		str	lr, [sp, #4]		;\
		ldr	lr, [pc, #LC4 - . - 8]	;\
		ldr	lr, [lr]		;\
		str	lr, [sp, #8]		;\
		stmfd	sp!, {r0 - r12}		;\
		mov	r0, #-1			;\
		str	r0, [sp, #S_OLD_R0]	;\
		mov	fp, #0

#define USER_RESTORE_ALL			\
		ldmia	sp, {r0 - lr}^		;\
		mov	r0, r0			;\
		add	sp, sp, #15*4		;\
		ldr	lr, [sp], #8		;\
		movs	pc, lr

#define SVC_RESTORE_ALL				\
		ldmfd	sp, {r0 - pc}^
		
		.global	_ret_from_sys_call

/*
 * Interrupt table (incorporates priority).  First set is indexed using
 * IRQSTATB.
 */

Lirq_prio_h:	.byte	 0, 8, 9, 8,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	12, 8, 9, 8,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	14,14,14,14,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	14,14,14,14,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	15,15,15,15,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	15,15,15,15,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	15,15,15,15,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	15,15,15,15,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10
		.byte	13,13,13,13,10,10,10,10,11,11,11,11,10,10,10,10

Lirq_prio_l:	.byte	 0, 0, 1, 0, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3
		.byte	 4, 0, 1, 0, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3
		.byte	 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
		.byte	 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
		.byte	 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 3, 3, 3, 3, 3
		.byte	 6, 6, 6, 6, 6, 6, 6, 6, 3, 3, 3, 3, 3, 3, 3, 3
		.byte	 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
		.byte	 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
		.byte	 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7

/**************************************************************************
 * Interrupt (IRQ) handler (r13 points to irq temp save area)
 * Note: if in user mode, then *no* kernel routine is running, so dont have
 *       to save svc lr
 */

LC4:		.word	Lirq_temp
LC5:		.word	_irqjump

Lvector_IRQ:	ldr	r13, [pc, #LC4 - . - 8]	@ Ill leave this one in just in case...
		sub	lr, lr, #4
		str	lr, [r13]
		tst	lr, #3
		bne	LIRQ_not_user
		teqp	pc, #0x08000003
		mov	r0, r0
		ldr	lr, [pc, #LC4 - . - 8]
		ldr	lr, [lr]
		USER_SAVE_ALL
		mov	r4, #IOC_BASE		@ point at IOC
		ldrb	r1, [r4, #0x24]		@ get high priority first
		adr	r2, Lirq_prio_h
		teq	r1, #0
		ldreqb	r1, [r4, #0x14]		@ get low priority
		adreq	r2, Lirq_prio_l
		teq	r1, #0
		beq	_ret_from_sys_call
		ldrb	r0, [r2, r1]		@ Get IRQ number
		ldr	r2, [pc, #LC5 - . - 8]
		mov	r1, sp
		mov	lr, pc
	/*
	 * routine gets called with r0 = interrupt number, r1 = struct pt_regs *
	 */
		ldr	pc, [r2, r0, lsl#2]
		mov	r2, #0
		teq	r0, #0			@ Check to see if it is a fast IRQ
		beq	_ret_from_sys_call
		USER_RESTORE_ALL

LC65:
		.word	_intr_count		@ -8
		.word	_bh_mask		@ -4
		.word	_bh_active		@ -0

Lirq_illegal_mode:
		mov	r0, sp
		mov	r1, #BAD_IRQ
		b	_bad_mode

LIRQ_not_user:	teqp	pc, #0x08000003
		mov	r0, r0
		SVC_IRQ_SAVE_ALL
                and	r2, lr, #3
		teq	r2, #3
		bne	Lirq_illegal_mode
Lsrepeat:	mov	r4, #IOC_BASE		@ point at IOC
		ldrb	r1, [r4, #0x24]		@ get high priority first
		adr	r2, Lirq_prio_h
		teq	r1, #0
		ldreqb	r1, [r4, #0x14]		@ get low priority
		adreq	r2, Lirq_prio_l
		teq	r1, #0
		beq	Lno_irq2
		ldrb	r0, [r2, r1]		@ Get IRQ number
		ldr	r2, [pc, #LC5 - . - 8]
		mov	r1, sp
		mov	lr, pc
	/*
	 * routine gets called with r0 = interrupt number, r1 = struct pt_regs *
	 */
		ldr	pc, [r2, r0, lsl#2]
		mov	r2, #1
		teq	r0, #0			@ Check to see if it is a fast IRQ
		bne	Lsrepeat
		ldr	r0, [pc, #LC65 - . - 8]
		ldr	r1, [r0]
		teq	r1, #0
		bne	Lsrepeat
		mov	r4, r0
		mov	r5, r1
		ldr	r6, [pc, #LC65 - . - 4]
		ldr	r7, [pc, #LC65 - . - 0]
Lrecheck_bh2:	ldr	r0, [r6]
		ldr	r1, [r7]
		tst	r0, r1
		beq	Lsrepeat
		add	r0, r5, #1
		str	r0, [r4]
		mov	r8, pc
		teqp	pc, #0x00000003
		bl	_do_bottom_half
		teqp	r8, #0
		str	r5, [r4]
		b	Lrecheck_bh2
Lno_irq2:	SVC_RESTORE_ALL

/**************************************************************************
 * Trap initialisation
 *
 * Note - FIQ code has changed.  The default is a couple of words in 0x1c,
 * 0x20 that call _unexp_fiq.  Nowever, we now copy the FIQ routine to
 * 0x1c (removes some excess cycles).
 */

Lswi_reset:	swi	SYS_ERROR0
		ldr	pc, . + 4
		.long	_unexp_fiq

Lfiqmsg:	.ascii	"*** Unexpeced FIQ\n\0"
		.align

_unexp_fiq:	mov	r12, #IOC_BASE
		strb	r12, [r12, #0x38]	@ Disable FIQ register
		teqp	pc, #0x0c000003
		mov	r0, r0
		stmfd	sp!, {r0 - r3, ip, lr}
		adr	r0, Lfiqmsg
		bl	_printk
		ldmfd	sp!, {r0 - r3, ip, lr}
		teqp	pc, #0x0c000001
		mov	r0, r0
		movs	pc, lr

Lvectors:	.long	Lvector_undefinstr - Lvectors - 12
		.long	Lvector_swi - Lvectors - 16
		.long	Lvector_prefetch - Lvectors - 20
		.long	Lvector_data - Lvectors - 24
		.long	Lvector_addrexcptn - Lvectors - 28
		.long	Lvector_IRQ - Lvectors - 32
		.long	0xea000000

		.global	_trap_init
_trap_init:	stmfd	sp !,{r4 - r9,lr}	@ Save link register
		teqp	pc, #0x0c000003
		mov	r0, #IOC_BASE
		str	r0, [r0, #0x18]
		str	r0, [r0, #0x28]
		str	r0, [r0, #0x38]
		mov	r0, #0			@ Lowest location
		adr	r1, Lvectors
		ldmia	r1, {r2, r3, r4, r5, r6, r7, ip}
		add	r1, ip, r1, asr #2
		add	r2, r1, r2, asr #2
		add	r3, r1, r3, asr #2
		add	r4, r1, r4, asr #2
		add	r5, r1, r5, asr #2
		add	r6, r1, r6, asr #2
		add	r7, r1, r7, asr #2
		adr	r1, Lswi_reset
		ldmia	r1, {r1, r8, r9}
		stmia	r0!, {r1 - r9}		@ Save all into page 0 ram
		ldmfd	sp!, {r4 - r9, pc}^

/**************************************************************************
 * Prefetch abort handler
 */

#ifdef DEBUG_UNDEF
t:		.ascii "*** undef ***\r\n\0"
		.align
#endif

Lvector_prefetch:
		sub	lr, lr, #4
		tst	lr, #3
		bne	Lprefetch_not_user
		USER_SAVE_ALL
		teqp	pc, #0x00000003		@ NOT a problem - doesnt change mode
		bic	r0, lr, #0xfc000003	@ Address of abort
		mov	r1, #FAULT_CODE_PREFETCH|FAULT_CODE_USER @ Error code
		mov	r2, sp			@ Tasks registers
		bl	_do_PrefetchAbort
		teq	r0, #0			@ If non-zero, we believe this abort..
		bne	_ret_from_sys_call
#ifdef DEBUG_UNDEF
		adr	r0,t			@ Otherwise it was probably an undefined
		bl	_printk			@ instruction.  (I do wish that I had a
#endif
		ldr	lr, [sp,#S_PC]		@ program to test this on.  I think its
		b	_undefinstr		@ broken at the moment though!)

Lprefetch_not_user:
		SVC_SAVE_ALL			@ Prefetch aborts are definitely *not*
		mov	r0, sp			@ allowed in non-user modes.  We cant
		mov	r1, #BAD_PREFETCH	@ recover from this problem.
		and	r2, lr, #3
		b	_bad_mode		@ Doesnt return

/**************************************************************************
 * Undefined instruction handler
 */
LC8:
		.word	_last_task_used_math
		.word	_current
LC9:
		.word	_fp_enter
		.word	_fp_save
		.word	_fp_restore

Lundef_not_user:
		SVC_SAVE_ALL			@ Non-user mode
		bic	r0, lr, #0xfc000003
		and	r2, lr, #3
		sub	r0, r0, #4
		mov	r1, sp
		bl	_do_undefinstr
		SVC_RESTORE_ALL

		.global	_fpreturn
		.global	_fpundefinstr
Lvector_undefinstr:
		tst	lr,#3
		bne	Lundef_not_user
		USER_SAVE_ALL			@ USER mode undefined instructions
		teqp	pc, #0x08000003		@ MUST disable interrupts.
_undefinstr:
@
@ before calling FP, must call math_state_restore!!!
@  something along the lines of:
@
		adr	r3, LC8
		ldmia	r3, {r1, r2, r3}
		ldr	r1, [r1]		@ last task used math
		ldr	r2, [r2]		@ current task
		teq	r1, r2
		stmnefd	sp!, {ip, lr}
		blne	_math_state_restore
		ldmnefd	sp!, {ip, lr}

		ldr	r3, [pc, #LC9 - . - 8]
		ldr	pc, [r3]		@ Call FP module (when loaded)
_fpundefinstr:	mov	r1,sp			@ Called by FP module on undefined instr
		teqp	pc, #3			@ Enable interrupts
		mov	r2,#0
		mov	lr, pc
@ Change!!
		add	lr, lr, #_ret_from_sys_call - . - 4
		b	_do_undefinstr

/**************************************************************************
 * Address exception handler
 *
 * These aren't too critical. (they're not supposed to happen).
 */

/*
 * In order to debug the reason for address exceptions in non-user modes,
 * we have to obtain all the registers so that we can see what's going on.
 */
Laddrexcptn_illegal_mode:
		mov	r0, sp
		str	lr, [sp, #-4]!
		orr	r1, r2, #0x0c000000
		teqp	r1, #0			@ change into mode (wont be user mode)
		mov	r0, r0
		mov	r1, r8			@ Any register from r8 - r14 can be banked
		mov	r2, r9
		mov	r3, r10
		mov	r4, r11
		mov	r5, r12
		mov	r6, r13
		mov	r7, r14
		teqp	pc, #0x04000003		@ back to svc
		mov	r0, r0
		stmfd	sp!, {r1-r7}
		ldmia	r0, {r0-r7}
		stmfd	sp!, {r0-r7}
		mov	r0, sp
		mov	r1, #BAD_ADDREXCPTN
		b	_bad_mode

Laddrexcptn_not_user:
		SVC_SAVE_ALL
		and	r2, lr, #3
		teq	r2, #3
		bne	Laddrexcptn_illegal_mode
		teqp	pc, #0x00000003		@ NOT a problem - doesnt change mode
		bic	r0, lr, #0xfc000003
		mov	r1, sp
		orr	r2, r2, #0x400
		bl	_do_excpt
		ldmia	sp, {r0 - lr}		@ I cant remember the reason I changed this...
		add	sp, sp, #15*4
		movs	pc, lr

Lvector_addrexcptn:
		sub	lr, lr, #8
		tst	lr, #3
		bne	Laddrexcptn_not_user
		USER_SAVE_ALL
		teq	pc, #0x00000003
		bic	r0, lr, #0xfc000003	@ Point to instruction
		mov	r1, sp			@ Point to registers
		mov	r2, #0x400
		mov	lr, pc
@ Change!!
		add	lr, lr, #_ret_from_sys_call - . - 4
		b	_do_excpt

/**************************************************************************
 * Data abort handler code
 */

Lvector_data:	sub	lr, lr, #8		@ Correct lr
		tst	lr, #3
		bne	Ldata_not_user
		USER_SAVE_ALL
		teqp	pc, #0x00000003		@ NOT a problem - doesnt change mode
		bic	r0, lr, #0xfc000003
		mov	r2, #FAULT_CODE_USER
		mov	lr, pc
		add	lr, lr, #_ret_from_sys_call - . - 4
		b	Ldata_do

Ldata_not_user:
		SVC_SAVE_ALL
		and	r2, lr, #3
		teq	r2, #3
		bne	Ldata_illegal_mode
		tst	lr, #0x08000000
		teqeqp	pc, #0x00000003		@ NOT a problem - doesnt change mode
		bic	r0, lr, #0xfc000003
		mov	r2, #0
		bl	Ldata_do
		SVC_RESTORE_ALL

Ldata_illegal_mode:
		mov	r0, sp
		mov	r1, #BAD_DATA
		b	_bad_mode

/**************************************************************************
 *
 * All exits to user mode from the kernel go through this code.
 */

LC6:
		.word	_intr_count		@ -8
		.word	_bh_mask		@ -4
		.word	_bh_active		@ -0
		.word	_need_resched		@ +4
		.word	_current		@ +8
		.word	_init_task		@ +12

Lreschedule:	bl	_schedule
_fpreturn:
_ret_from_sys_call:
		adr	r4, LC6
		ldmia	r4, {r4, r6, r7, r9}
		ldr	r5, [r4]
		teq	r5, #0
		bne	Lret_no_check
Lrecheck_bh:	ldr	r0, [r6]
		ldr	r1, [r7]
		tst	r0, r1
		bne	Lhandle_bottom_half
		ldr	r0, [sp, #S_PC]		@ Get old PC
		tst	r0, #3
		bne	Lret_no_check
		ldr	r0, [r9]
		teq	r0, #0
		bne	Lreschedule

		ldr	r2, [pc, #LC6 - . + 8]
		ldr	r2, [r2]
		ldr	r1, [pc, #LC6 - . + 12]
		teq	r2, r1
		beq	Lret_no_check

		ldr	r1, [r2, #SIGNAL]
		ldr	r0, [r2, #BLOCKED]
		bics	r1, r1, r0
		movne	r1, sp
		blne	_do_signal
Lret_no_check:	USER_RESTORE_ALL

Lhandle_bottom_half:
		add	r0, r5, #1
		str	r0, [r4]
		mov	r8, pc
		teqp	pc, #0x00000003
		bl	_do_bottom_half
		teqp	r8, #0
		str	r5, [r4]
		b	Lrecheck_bh

/**************************************************************************
 * SWI handler
 *
 *
 * We now handle sys-call tracing, and the errno in the task structure.
 * Still have a problem with >4 arguments for functions.  Theres only
 * a couple of functions in the code that have 5 arguments, so Im not
 * too worried.
 */

#include "calls.S"

LC1:		.word	_current

Lvector_swi:	USER_SAVE_ALL
		bic	lr, lr, #0xfc000003	@ get pc value from link register
		ldr	r6, [lr, #-4]		@ get swi instruction
		teqp	pc, #0x00000003		@ enable irqs
		bic	r6, r6, #0xff000000	@ mask off swi op code
		eor	r6, r6, #OS_NUMBER << 20 @ check OS number
Lretry:		cmp	r6, #NR_SYSCALLS	@ check upper syscall limit
		bcs	Lswi_bad_call
		ldr	r5, [pc, #LC1 - . - 8]
		ldr	r5, [r5]
		mov	ip, #0			@ zero errno
		str	ip, [r5, #ERRNO]

		ldr	ip, [r5, #FLAGS]	@ check for syscall tracing
		tst	ip, #PF_TRACESYS
		bne	Ltrace_this_syscall

		adr	ip, _sys_call_table
		mov	r9, sp			@ hack for old routines needing '*regs'
		str	r4, [sp, #-4]!		@ new-style: (r0 = arg1, r4 = arg5)
		mov	lr, pc
		ldr	pc, [ip, r6, lsl #2]	@ call sys routine (r0, r1, r2, r3, r4)
		add	sp, sp, #4

		ldr	ip, [r5, #ERRNO]	@ check errno
		rsbs	ip, ip, #0
		movne	r0, ip
		str	r0, [sp, #0]		@ returned r0
		b	_ret_from_sys_call

Ltrace_this_syscall:
		ldr	r7, [sp, #S_IP]
		mov	r0, #0
		str	r0, [sp, #S_IP]
		bl	_syscall_trace		@ trace entry [IP must = 0]
		str	r7, [sp, #S_IP]
		ldmia	sp, {r0 - r3}		@ have to reload r0 - r3

		adr	ip, _sys_call_table
		mov	r9, sp			@ hack for routines needing '*regs'
		str	r4, [sp, #-4]!
		mov	lr, pc
		ldr	pc, [ip, r6, lsl #2]	@ call sys routine (r0, r1, r2, r3, r4)
		add	sp, sp, #4

		ldr	ip, [r5, #ERRNO]
		rsbs	ip, ip, #0
		movne	r0, ip
		str	r0, [sp, #0]		@ returned r0

		mov	r0, #1
		str	r0, [sp, #S_IP]
		bl	_syscall_trace		@ trace exit [IP must != 0]
		str	r7, [sp, #S_IP]
		b	_ret_from_sys_call


Lswi_bad_call:	tst	r6, #0x00f00000
		bne	Lbad
		cmp	r6, #(KSWI_SYS_BASE - KSWI_BASE)@ check for arm private syscalls
		bcs	Larm_sys_call
		bl	_sys_ni_syscall
		str	r0, [sp, #0]		@ returned r0
		b	_ret_from_sys_call

Lbad:		eor	r0, r6, #OS_NUMBER << 20 @ check OS number
		bl	_deferred
		ldmfd	sp, {r0 - r3}
		b	Lretry

Larm_sys_call:	bic	r0, r6, #0x000f0000
		mov	r1, sp
		bl	_arm_syscall
		b	_ret_from_sys_call

@ r0 = syscall number
@ r1 = syscall r0
@ r5 = syscall r4
@ ip = syscall table
_sys_syscall:	mov	r6, r0
		eor	r6, r6, #OS_NUMBER << 20
		cmp	r6, #NR_SYSCALLS		@ check range
		movgt	r0, #-ENOSYS
		movgt	pc, lr
		add	sp, sp, #4			@ take of the save of our r4
		ldmib	sp, {r0 - r4}			@ get our args
		str	r4, [sp, #-4]!			@ Put our arg on the stack
		ldr	pc, [ip, r6, lsl #2]

		.global	_sys_call_table
_sys_call_table:
#include "calls.S"

/******************************************************************************
 *
 * Data abort handler code.  This handles both exceptions from user and SVC
 *   modes, computes the address range of the problem, and does any
 *   correction that is required.  It then calls the kernel data abort routine.
 *
 * This is where I wish that the ARM would tell you which address aborted.
 */

Ldata_do:	mov	r3, sp
		ldr	r4, [r0]		@ Get instruction
		tst	r4, #1 << 20		@ Check to see if it is a write instruction
		orreq	r2, r2, #FAULT_CODE_WRITE @ Indicate write instruction
		mov	r1, r4, lsr #22		@ Now branch to the relevent processing routine
		and	r1, r1, #15 << 2
		add	pc, pc, r1
		movs	pc, lr
		b	Ldata_unknown
		b	Ldata_unknown
		b	Ldata_unknown
		b	Ldata_unknown
		b	Ldata_ldrstr_post	@ ldr	rd, [rn], #m
		b	Ldata_ldrstr_numindex	@ ldr	rd, [rn, #m]	@ RegVal
		b	Ldata_ldrstr_post	@ ldr	rd, [rn], rm
		b	Ldata_ldrstr_regindex	@ ldr	rd, [rn, rm]
		b	Ldata_ldmstm		@ ldm*a	rn, <rlist>
		b	Ldata_ldmstm		@ ldm*b	rn, <rlist>
		b	Ldata_unknown
		b	Ldata_unknown
		b	Ldata_ldrstr_post	@ ldc	rd, [rn], #m	@ Same as ldr	rd, [rn], #m
		b	Ldata_ldcstc_pre	@ ldc	rd, [rn, #m]
		b	Ldata_unknown
Ldata_unknown:	@ Part of jumptable
		ldr	r3, [sp, #15 * 4]
		str	r3, [sp, #-4]!
		mov	r1, r1, lsr #2
		mov	r2, r0
		mov	r3, r4
		adr	r0, Ltt
		bl	_printk
Llpxx:		b	Llpxx

Ltt:		.ascii	"Unknown data abort code %d [pc=%p, *pc=%p]\nLR=%p\0"
		.align

Ldata_ldrstr_post:
		mov	r0, r4, lsr #14		@ Get Rn
		and	r0, r0, #15 << 2	@ Mask out reg.
		teq	r0, #15 << 2
		ldr	r0, [r3, r0]		@ Get register
		biceq	r0, r0, #0xfc000003
		mov	r1, r0
		tst	r4, #1 << 21
		orrne	r2, r2, #FAULT_CODE_USER
#ifdef FAULT_CODE_LDRSTRPOST
		orr	r2, r2, #FAULT_CODE_LDRSTRPOST
#endif
		b	_do_DataAbort

Ldata_ldrstr_numindex:
		mov	r0, r4, lsr #14		@ Get Rn
		and	r0, r0, #15 << 2	@ Mask out reg.
		teq	r0, #15 << 2
		ldr	r0, [r3, r0]		@ Get register
		biceq	r0, r0, #0xfc000003
		mov	r1, r4, lsl #20
		tst	r4, #1 << 23
		addne	r0, r0, r1, lsr #20
		subeq	r0, r0, r1, lsr #20
		mov	r1, r0
#ifdef FAULT_CODE_LDRSTRPRE
		orr	r2, r2, #FAULT_CODE_LDRSTRPRE
#endif
		b	_do_DataAbort

Ldata_ldrstr_regindex:
		mov	r0, r4, lsr #14		@ Get Rn
		and	r0, r0, #15 << 2	@ Mask out reg.
		teq	r0, #15 << 2
		ldr	r0, [r3, r0]		@ Get register
		biceq	r0, r0, #0xfc000003
		and	r7, r4, #15
		teq	r7, #15			@ Check for PC
		ldr	r7, [r3, r7, lsl #2]	@ Get Rm
		biceq	r7, r7, #0xfc000003
		and	r8, r4, #0x60		@ Get shift types
		mov	r9, r4, lsr #7		@ Get shift amount
		and	r9, r9, #31
		teq	r8, #0
		moveq	r7, r7, lsl r9
		teq	r8, #0x20		@ LSR shift
		moveq	r7, r7, lsr r9
		teq	r8, #0x40		@ ASR shift
		moveq	r7, r7, asr r9
		teq	r8, #0x60		@ ROR shift
		moveq	r7, r7, ror r9
		tst	r4, #1 << 23
		addne	r0, r0, r7
		subeq	r0, r0, r7		@ Apply correction
		mov	r1, r0
#ifdef FAULT_CODE_LDRSTRREG
		orr	r2, r2, #FAULT_CODE_LDRSTRREG
#endif
		b	_do_DataAbort

Ldata_ldmstm:
		mov	r7, #0x11
		orr	r7, r7, r7, lsl #8
		and	r0, r4, r7
		and	r1, r4, r7, lsl #1
		add	r0, r0, r1, lsr #1
		and	r1, r4, r7, lsl #2
		add	r0, r0, r1, lsr #2
		and	r1, r4, r7, lsl #3
		add	r0, r0, r1, lsr #3
		add	r0, r0, r0, lsr #8
		add	r0, r0, r0, lsr #4
		and	r7, r0, #15		@ r7 = no. of registers to transfer.
		mov	r5, r4, lsr #14		@ Get Rn
		and	r5, r5, #15 << 2
		ldr	r0, [r3, r5]		@ Get reg
		eor	r6, r4, r4, lsl #2
		tst	r6, #1 << 23		@ Check inc/dec ^ writeback
		rsbeq	r7, r7, #0
		add	r7, r0, r7, lsl #2	@ Do correction (signed)
		subne	r1, r7, #1
		subeq	r1, r0, #1
		moveq	r0, r7
		tst	r4, #1 << 21		@ Check writeback
		strne	r7, [r3, r5]
		eor	r6, r4, r4, lsl #1
		tst	r6, #1 << 24		@ Check Pre/Post ^ inc/dec
		addeq	r0, r0, #4
		addeq	r1, r1, #4
		teq	r5, #15*4		@ CHECK FOR PC
		biceq	r1, r1, #0xfc000003
		biceq	r0, r0, #0xfc000003
#ifdef FAULT_CODE_LDMSTM
		orr	r2, r2, #FAULT_CODE_LDMSTM
#endif
		b	_do_DataAbort

Ldata_ldcstc_pre:
		mov	r0, r4, lsr #14		@ Get Rn
		and	r0, r0, #15 << 2	@ Mask out reg.
		teq	r0, #15 << 2
		ldr	r0, [r3, r0]		@ Get register
		biceq	r0, r0, #0xfc000003
		mov	r1, r4, lsl #24		@ Get offset
		tst	r4, #1 << 23
		addne	r0, r0, r1, lsr #24
		subeq	r0, r0, r1, lsr #24
		mov	r1, r0
#ifdef FAULT_CODE_LDCSTC
		orr	r2, r2, #FAULT_CODE_LDCSTC
#endif
		b	_do_DataAbort

		.global	_fpe_save
_fpe_save:	ldr	r1, [pc, #LC9 - . - 4]
		ldr	pc, [r1]

		.global	_fpe_restore
_fpe_restore:	ldr	r1, [pc, #LC9 - . - 0]
		ldr	pc, [r1]

_fpnull:	mov	pc, lr

		.data

		.global	_fp_enter
		.global	_fp_save
		.global	_fp_restore
_fp_enter:	.word	_fpundefinstr
_fp_save:	.word	_fpnull
_fp_restore:	.word	_fpnull

Lirq_temp:	.word	0
