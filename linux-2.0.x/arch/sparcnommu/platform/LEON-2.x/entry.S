/* $Id: entry.S,v 1.93 1996/04/25 06:08:32 davem Exp $
 * arch/sparc/kernel/entry.S:  Sparc trap low-level entry points.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <linux/config.h>
#include <linux/errno.h>

#include <asm/head.h>
#include <asm/asi.h>
#include <asm/kgdb.h>
#include <asm/contregs.h>
#include <asm/ptrace.h>
#include <asm/psr.h>
#include <asm/cprefix.h>
#include <asm/page.h>
#include <asm/winmacro.h>
#include <asm/signal.h>

#include <asm/asmmacro.h>

#define NR_SYSCALLS 255      /* Each OS is different... */

/* First, KGDB low level things.  This is a rewrite
 * of the routines found in the sparc-stub.c asm() statement
 * from the gdb distribution.  This is also dual-purpose
 * as a software trap for userlevel programs.
 */
	.data
	.align	4

in_trap_handler:
	.word	0

	.text
	.align	4
	
/* For now all IRQ's not registered get sent here. handler_irq() will
 * see if a routine is registered to handle this interrupt and if not
 * it will say so on the console.
 */

	.align	4
	.globl	real_irq_entry
real_irq_entry:
	SAVE_ALL
	ENTER_IRQ
	/* start atomic operation with respect to software interrupts */
	sethi	%hi(C_LABEL(intr_count)), %l4
	ld	[%l4 + %lo(C_LABEL(intr_count))], %l5
	add	%l5, 0x1, %l5
	st	%l5, [%l4 + %lo(C_LABEL(intr_count))]

	/* Enable traps w/IRQs off, so we can call c-code properly.
	 * Note how we are increasing PIL so we need to do two writes
	 * to work around a MicroSPARC bug of sorts.
	 */
	or	%l0, PSR_PIL, %l4

	wr	%l4, 0x0, %psr
	WRITE_PAUSE
	wr	%l4, PSR_ET, %psr
	WRITE_PAUSE

	mov	%l7, %o0		! irq level
	call	C_LABEL(process_int)
	 add	%sp, REGWIN_SZ, %o1	! pt_regs ptr

rie_checkbh:
	sethi	%hi(C_LABEL(intr_count)), %l4
	ld	[%l4 + %lo(C_LABEL(intr_count))], %l5
	subcc	%l5, 0x1, %l5
	bne	2f	/* IRQ within IRQ, get out of here... */
	 nop

	sethi	%hi(C_LABEL(bh_active)), %l3
	ld	[%l3 + %lo(C_LABEL(bh_active))], %g2
	sethi	%hi(C_LABEL(bh_mask)), %l3
	ld	[%l3 + %lo(C_LABEL(bh_mask))], %g3
	andcc	%g2, %g3, %g0
	be	2f
	 nop

	call	C_LABEL(do_bottom_half)	
	 nop

	/* Try again... */
	b	rie_checkbh
	 nop
	
2:
	st	%l5, [%l4 + %lo(C_LABEL(intr_count))]

	LEAVE_IRQ
	RESTORE_ALL

	/* This routine handles illegal instructions and privileged
	 * instruction attempts from user code.
	 */
	.align	4
	.globl	bad_instruction
bad_instruction:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(do_illegal_instruction)
	 mov	%l0, %o3

	RESTORE_ALL

	.align	4
	.globl	priv_instruction
priv_instruction:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(do_priv_instruction)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles unaligned data accesses.
	 */
	.align	4
	.globl	mna_handler
mna_handler:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(do_memaccess_unaligned)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles floating point disabled traps. */
	.align	4
	.globl	fpd_trap_handler
fpd_trap_handler:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(do_fpd_trap)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles Floating Point Exceptions. */
	.align	4
	.globl	fpe_trap_handler
fpe_trap_handler:
	set	fpsave_magic, %l5
	cmp	%l1, %l5
	be	1f
	 sethi	%hi(C_LABEL(fpsave)), %l5
	or	%l5, %lo(C_LABEL(fpsave)), %l5
	cmp	%l1, %l5
	bne	2f
	 sethi	%hi(fpsave_catch2), %l5
	or	%l5, %lo(fpsave_catch2), %l5
	wr	%l0, 0x0, %psr
	WRITE_PAUSE
	jmp	%l5
	 rett	%l5 + 4
1:	
	sethi	%hi(fpsave_catch), %l5
	or	%l5, %lo(fpsave_catch), %l5
	wr	%l0, 0x0, %psr
	WRITE_PAUSE
	jmp	%l5
	 rett	%l5 + 4

2:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(do_fpe_trap)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles Register Access Exceptions. */
	.align	4
	.globl	do_reg_access
do_reg_access:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(handle_reg_access)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles Co-Processor Disabled Exceptions. */
	.align	4
	.globl	do_cp_disabled
do_cp_disabled:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(handle_cp_disabled)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles Unimplemented FLUSH Exceptions. */
	.align	4
	.globl	do_bad_flush
do_bad_flush:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(handle_bad_flush)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles Co-Processor Exceptions. */
	.align	4
	.globl	do_cp_exception
do_cp_exception:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(handle_cp_exception)
	 mov	%l0, %o3

	RESTORE_ALL

	/* This routine handles Hardware Divide By Zero Exceptions. */
	.align	4
	.globl	do_hw_divzero
do_hw_divzero:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr		! re-enable traps
	WRITE_PAUSE

	add	%sp, REGWIN_SZ, %o0
	mov	%l1, %o1
	mov	%l2, %o2
	call	C_LABEL(handle_hw_divzero)
	 mov	%l0, %o3

	RESTORE_ALL

	.align	4
	.globl	do_flush_windows
do_flush_windows:
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr
	WRITE_PAUSE

	andcc	%l0, PSR_PS, %g0
	bne	dfw_kernel
	 nop

	call	C_LABEL(flush_user_windows)
	 nop

	/* Advance over the trap instruction. */
	ld	[%sp + REGWIN_SZ + PT_NPC], %l1
	add	%l1, 0x4, %l2
	st	%l1, [%sp + REGWIN_SZ + PT_PC]
	st	%l2, [%sp + REGWIN_SZ + PT_NPC]

	RESTORE_ALL

	/* We get these for debugging routines using __builtin_return_address() */
dfw_kernel:
	FLUSH_ALL_KERNEL_WINDOWS

	/* Advance over the trap instruction. */
	ld	[%sp + REGWIN_SZ + PT_NPC], %l1
	add	%l1, 0x4, %l2
	st	%l1, [%sp + REGWIN_SZ + PT_PC]
	st	%l2, [%sp + REGWIN_SZ + PT_NPC]

	RESTORE_ALL

	/* The getcc software trap.  The user wants the condition codes from
	 * the %psr in register %g1.
	 */

	.align	4
	.globl	getcc_trap_handler
getcc_trap_handler:
	srl	%l0, 20, %g1	! give user
	and	%g1, 0xf, %g1	! only ICC bits in %psr
	jmp	%l2		! advance over trap instruction
	rett	%l2 + 0x4	! like this...

	/* The setcc software trap.  The user has condition codes in %g1
	 * that it would like placed in the %psr.  Be careful not to flip
	 * any unintentional bits!
	 */

	.align	4
	.globl	setcc_trap_handler
setcc_trap_handler:
	sll	%g1, 0x14, %l4
	set	PSR_ICC, %l5
	andn	%l0, %l5, %l0	! clear ICC bits in current %psr
	and	%l4, %l5, %l4	! clear non-ICC bits in user value
	or	%l4, %l0, %l4	! or them in... mix mix mix

	wr	%l4, 0x0, %psr	! set new %psr
	WRITE_PAUSE		! TI scumbags...

	jmp	%l2		! advance over trap instruction
	rett	%l2 + 0x4	! like this...

	/* Note how we really return to ret_syscall because we share the
	 * register window with our caller.
	 */

	.align 4
	.globl	C_LABEL(sys_ptrace)
C_LABEL(sys_ptrace):
	call	C_LABEL(do_ptrace)
	 add	%sp, REGWIN_SZ, %o0

	LOAD_CURRENT(l4, l5)
	ld	[%l4 + 0x14], %l5
	andcc	%l5, 0x20, %g0
	be	1f
	 nop

	call	C_LABEL(syscall_trace)
	 nop

1:
	RESTORE_ALL

	.align	4
	.globl	C_LABEL(sys_execve)
C_LABEL(sys_execve):
	mov	%o7, %l5
	call	C_LABEL(sparc_execve)
	 add	%sp, REGWIN_SZ, %o0		! pt_regs *regs arg

	jmp	%l5 + 0x8
	 nop

	.globl	C_LABEL(sys_execv)
C_LABEL(sys_execv):
	st	%g0, [%sp + REGWIN_SZ + PT_I2]

	call	C_LABEL(sparc_execve)
	 add	%sp, REGWIN_SZ, %o0

	b	C_LABEL(ret_sys_call)
	 nop

	.align	4
	.globl	C_LABEL(sys_pipe)
C_LABEL(sys_pipe):
	mov	%o7, %l5

	call	C_LABEL(sparc_pipe)
	 add	%sp, REGWIN_SZ, %o0		! pt_regs *regs arg

	jmp	%l5 + 0x8
	 nop

	.align	4
	.globl	C_LABEL(sys_sigpause)
C_LABEL(sys_sigpause):
	ld	[%sp + REGWIN_SZ + PT_I0], %o0
	call	C_LABEL(do_sigpause)
	 add	%sp, REGWIN_SZ, %o1

	LOAD_CURRENT(l4, l5)
	ld	[%l4 + 0x14], %l5
	andcc	%l5, 0x20, %g0
	be	1f
	 nop

	call	C_LABEL(syscall_trace)
	 nop

1:
	/* We are returning to a signal handler. */
	RESTORE_ALL

	.align	4
	.globl	C_LABEL(sys_sigsuspend)
C_LABEL(sys_sigsuspend):
	call	C_LABEL(do_sigsuspend)
	 add	%sp, REGWIN_SZ, %o0

	LOAD_CURRENT(l4, l5)
	ld	[%l4 + 0x14], %l5
	andcc	%l5, 0x20, %g0
	be	1f
	 nop

	call	C_LABEL(syscall_trace)
	 nop

1:
	/* We are returning to a signal handler. */
	RESTORE_ALL

	.align	4
	.globl	C_LABEL(sys_sigreturn)
C_LABEL(sys_sigreturn):
	call	C_LABEL(do_sigreturn)
	 add	%sp, REGWIN_SZ, %o0

	LOAD_CURRENT(l4, l5)
	ld	[%l4 + 0x14], %l5
	andcc	%l5, 0x20, %g0
	be	1f
	 nop

	call	C_LABEL(syscall_trace)
	 nop

1:
	/* We don't want to muck with user registers like a
	 * normal syscall, just return.
	 */
	RESTORE_ALL

	/* Now that we have a real sys_clone, sys_fork() is
	 * implemented in terms of it.  Our _real_ implementation
	 * of SunOS vfork() will use sys_clone() instead.
	 */
	.align	4
	.globl	C_LABEL(sys_fork), C_LABEL(sys_vfork)
C_LABEL(sys_vfork):
C_LABEL(sys_fork):
	mov	%o7, %l5

	/* Save the kernel state as of now. */
	FLUSH_ALL_KERNEL_WINDOWS;
	STORE_WINDOW(sp)
	LOAD_CURRENT(g6, g5)
	rd	%psr, %g4
	rd	%wim, %g5
	std	%g4, [%g6 + THREAD_FORK_KPSR]

	mov	SIGCHLD, %o0			! arg0:	clone flags
	ld	[%sp + REGWIN_SZ + PT_FP], %o1	! arg1:	usp
	call	C_LABEL(do_fork)
	 add	%sp, REGWIN_SZ, %o2		! arg2:	pt_regs ptr

	jmp	%l5 + 0x8
	 nop
	
	/* Whee, kernel threads! */
	.globl	C_LABEL(sys_clone)
C_LABEL(sys_clone):
	mov	%o7, %l5
	/* Save the kernel state as of now. */
	FLUSH_ALL_KERNEL_WINDOWS;
	STORE_WINDOW(sp)
	LOAD_CURRENT(g6, g5)
	rd	%psr, %g4
	rd	%wim, %g5
	std	%g4, [%g6 + THREAD_FORK_KPSR]

	ldd	[%sp + REGWIN_SZ + PT_I0], %o0	! arg0,1: flags,usp
	cmp	%o1, 0x0			! Is new_usp NULL?
	be,a	1f
	 ld	[%sp + REGWIN_SZ + PT_FP], %o1	! yes, use current usp
1:
	add	%sp, REGWIN_SZ, %o2		! arg2:	pt_regs ptr
	call	C_LABEL(do_fork)
#if 1
	 sub	%sp, 0x10, %sp			! clobber
	add	%sp, 0x10, %sp
#else
	 nop
#endif
	jmp	%l5 + 0x8
	 nop

	/* Linux native and SunOS system calls enter here... */
	.align	4
	.globl	linux_sparc_syscall
linux_sparc_syscall:
	/* While we are here trying to optimize our lives
	 * away, handle the easy bogus cases like a
	 * ni_syscall or sysnum > NR_SYSCALLS etc.
	 * In the cases where we cannot optimize the
	 * call inline we don't really lose anything
	 * performance wise because we are doing here
	 * things which we did anyway in the original
	 * routine.  The only added complexity is a
	 * bit test, compare, and branch to decide
	 * if we need to save process state or not.
	 */

	/* XXX TODO: When we have ptrace working test
	 * XXX       test for PF_TRACESYS in task flags.
	 */

	/* Direct access to user regs, must faster. */
	cmp	%g1, NR_SYSCALLS
	blu,a	1f
	 sll	%g1, 2, %l4

	set	C_LABEL(sys_ni_syscall), %l7
	b	syscall_is_too_hard
	 nop

1:
	ld	[%l7 + %l4], %l7

	/* If bit-1 is set, this is a "fast" syscall.
	 * This is the _complete_ overhead of this optimization,
	 * and we save ourselves a load, so it evens out to nothing.
	 */
	andcc	%l7, 0x1, %g0
	be	syscall_is_too_hard
	 andn	%l7, 0x1, %l7

	jmpl	%l7, %g0
	 nop

	.globl	syscall_is_too_hard
syscall_is_too_hard:
	rd	%wim, %l3
	SAVE_ALL
	ENTER_SYSCALL

	wr	%l0, PSR_ET, %psr
	WRITE_PAUSE

	LOAD_CURRENT(l4, l5)
	ld	[%l4 + 0x14], %l5
	andcc	%l5, 0x20, %g0
	be	2f
	 nop

	call	C_LABEL(syscall_trace)
	 nop

2:
	ldd	[%sp + REGWIN_SZ + PT_I0], %o0
	st	%o0, [%sp + REGWIN_SZ + PT_G0]	! for restarting syscalls
	ldd	[%sp + REGWIN_SZ + PT_I2], %o2
	call	%l7
	 ldd	[%sp + REGWIN_SZ + PT_I4], %o4

	st	%o0, [%sp + REGWIN_SZ + PT_I0]

	.globl	C_LABEL(ret_sys_call)
C_LABEL(ret_sys_call):
	ld	[%sp + REGWIN_SZ + PT_I0], %o0
	set	PSR_C, %l6
	cmp	%o0, -ENOIOCTLCMD
	bgeu	1f
	 ld	[%sp + REGWIN_SZ + PT_PSR], %l5

	/* System call success, clear Carry condition code. */		
	andn	%l5, %l6, %l5
	b	2f
	 st	%l5, [%sp + REGWIN_SZ + PT_PSR]	

1:
	/* System call failure, set Carry condition code.
	 * Also, get abs(errno) to return to the process.
	 */
	sub	%g0, %o0, %o0
	st	%o0, [%sp + REGWIN_SZ + PT_I0]
	or	%l5, %l6, %l5
	st	%l5, [%sp + REGWIN_SZ + PT_PSR]

2:
	LOAD_CURRENT(l4, l5)
	ld	[%l4 + 0x14], %l5
	andcc	%l5, 0x20, %g0
	be	3f
	 nop

	call	C_LABEL(syscall_trace)
	 nop

	/* Advance the pc and npc over the trap instruction. */
3:
	ld	[%sp + REGWIN_SZ + PT_NPC], %l1	/* pc  = npc   */
	add	%l1, 0x4, %l2			/* npc = npc+4 */
	st	%l1, [%sp + REGWIN_SZ + PT_PC]
	st	%l2, [%sp + REGWIN_SZ + PT_NPC]

	RESTORE_ALL

/* Saving and restoring the FPU state is best done from lowlevel code.
 *
 * void fpsave(unsigned long *fpregs, unsigned long *fsr,
 *             void *fpqueue, unsigned long *fpqdepth)
 */

	.globl	C_LABEL(fpsave)
C_LABEL(fpsave):
	st	%fsr, [%o1]	! this can trap on us if fpu is in bogon state
	ld	[%o1], %g1
	set	0x2000, %g4
	andcc	%g1, %g4, %g0
	be	2f
	 mov	0, %g2

	/* We have an fpqueue to save. */
1:
	std	%fq, [%o2]
fpsave_magic:
	st	%fsr, [%o1]
	ld	[%o1], %g3
	andcc	%g3, %g4, %g0
	add	%g2, 1, %g2
	bne	1b
	 add	%o2, 8, %o2

2:
	st	%g2, [%o3]

	std	%f0, [%o0 + 0x00]
	std	%f2, [%o0 + 0x08]
	std	%f4, [%o0 + 0x10]
	std	%f6, [%o0 + 0x18]
	std	%f8, [%o0 + 0x20]
	std	%f10, [%o0 + 0x28]
	std	%f12, [%o0 + 0x30]
	std	%f14, [%o0 + 0x38]
	std	%f16, [%o0 + 0x40]
	std	%f18, [%o0 + 0x48]
	std	%f20, [%o0 + 0x50]
	std	%f22, [%o0 + 0x58]
	std	%f24, [%o0 + 0x60]
	std	%f26, [%o0 + 0x68]
	std	%f28, [%o0 + 0x70]
	retl
	 std	%f30, [%o0 + 0x78]

	/* Thanks for Theo Deraadt and the authors of the Sprite/netbsd/openbsd
	 * code for pointing out this possible deadlock, while we save state
	 * above we could trap on the fsr store so our low level fpu trap
	 * code has to know how to deal with this.
	 */
fpsave_catch:
	b	fpsave_magic + 4
	 st	%fsr, [%o1]

fpsave_catch2:
	b	C_LABEL(fpsave) + 4
	 st	%fsr, [%o1]

	/* void fpload(unsigned long *fpregs, unsigned long *fsr); */

	.globl	C_LABEL(fpload)
C_LABEL(fpload):
	ldd	[%o0 + 0x00], %f0
	ldd	[%o0 + 0x08], %f2
	ldd	[%o0 + 0x10], %f4
	ldd	[%o0 + 0x18], %f6
	ldd	[%o0 + 0x20], %f8
	ldd	[%o0 + 0x28], %f10
	ldd	[%o0 + 0x30], %f12
	ldd	[%o0 + 0x38], %f14
	ldd	[%o0 + 0x40], %f16
	ldd	[%o0 + 0x48], %f18
	ldd	[%o0 + 0x50], %f20
	ldd	[%o0 + 0x58], %f22
	ldd	[%o0 + 0x60], %f24
	ldd	[%o0 + 0x68], %f26
	ldd	[%o0 + 0x70], %f28
	ldd	[%o0 + 0x78], %f30
	ld	[%o1], %fsr
	retl
	 nop

	.globl	C_LABEL(udelay)
C_LABEL(udelay):
	save	%sp, -REGWIN_SZ, %sp
	mov	%i0, %o0
	sethi	%hi(0x10c6), %o1
	call	.umul
	 or	%o1, %lo(0x10c6), %o1
	sethi	%hi(C_LABEL(loops_per_sec)), %o3
	call	.umul
	 ld	[%o3 + %lo(C_LABEL(loops_per_sec))], %o1

	cmp	%o1, 0x0
1:
	bne	1b
	 subcc	%o1, 1, %o1
	
	ret
	restore

/* End of entry.S */
