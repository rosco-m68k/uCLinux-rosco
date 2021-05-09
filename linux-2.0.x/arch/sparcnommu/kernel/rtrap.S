/* $Id: rtrap.S,v 1.27 1996/04/03 02:14:41 davem Exp $
 * rtrap.S: Return from Sparc trap low-level code.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <asm/cprefix.h>
#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/psr.h>
#include <asm/asi.h>
#include <asm/contregs.h>
#include <asm/winmacro.h>
#include <asm/asmmacro.h>

#define t_psr     l0
#define t_pc      l1
#define t_npc     l2
#define t_wim     l3
#define twin_tmp1 l4
#define twin_tmp2 l5
#define twin_tmp3 l6

	/* We need to check for a few things which are:
	 * 1) The need to call schedule() because this
	 *    processes quantum is up.
	 * 2) Pending signals for this process, if any
	 *    exist we need to call do_signal() to do
	 *    the needy.
	 *
	 * Else we just check if the rett would land us
	 * in an invalid window, if so we need to grab
	 * it off the user/kernel stack first.
	 */

	.globl	ret_trap_entry
ret_trap_entry:
	ld	[%sp + REGWIN_SZ + PT_PSR], %t_psr
	andcc	%t_psr, PSR_PS, %g0
	bne	ret_trap_kernel
	 nop

	sethi	%hi(C_LABEL(need_resched)), %twin_tmp1
	ld	[%twin_tmp1 + %lo(C_LABEL(need_resched))], %twin_tmp2

	cmp	%twin_tmp2, 0
	be	signal_p
	 nop

	call	C_LABEL(schedule)
	 nop

	/* Try to return again... We are a different process,
	 * most likely so load and then check if going back
	 * to user or kernel this time.
	 */
	b	ret_trap_entry
	 nop

signal_p:
	/* No signals for swapper. */
	LOAD_CURRENT(twin_tmp1, twin_tmp3)
	set	C_LABEL(init_task), %twin_tmp3
	cmp	%twin_tmp3, %twin_tmp1
	be	ret_trap_continue
	 nop

	ld	[%twin_tmp1 + TASK_SIGNAL], %twin_tmp2
	ld	[%twin_tmp1 + TASK_BLOCKED], %o0
	andncc	%twin_tmp2, %o0, %g0
	be	ret_trap_continue
	 nop

	 add	%sp, REGWIN_SZ, %o1	! pt_regs ptr
	call	C_LABEL(do_signal)
#if 1
	 sub	%sp, 0x10, %sp		! clobber
	add	%sp, 0x10, %sp
#else
	 nop
#endif
	/* Fall through... */
ret_trap_continue:
	ld	[%sp + REGWIN_SZ + PT_PSR], %t_psr
	wr	%t_psr, 0x0, %psr
	WRITE_PAUSE

	LOAD_CURRENT(twin_tmp2, twin_tmp1)
	ld	[%twin_tmp2 + THREAD_W_SAVED], %twin_tmp1
	orcc	%g0, %twin_tmp1, %g0
	be	ret_trap_nobufwins
	 nop

	wr	%t_psr, PSR_ET, %psr
	WRITE_PAUSE

	mov	1, %o1
	call	C_LABEL(try_to_clear_window_buffer)
	 add	%sp, REGWIN_SZ, %o0

	b	ret_trap_entry
	 nop

ret_trap_nobufwins:
	/* Load up the user's out registers so we can pull
	 * a window from the stack, if necessary.
	 */
	LOAD_PT_INS(sp)

	/* If there are already live user windows in the
	 * set we can return from trap safely.
	 */
	ld	[%twin_tmp2 + THREAD_UMASK], %twin_tmp1
	orcc	%g0, %twin_tmp1, %g0
	bne	ret_trap_userwins_ok
	 nop
	
		/* Calculate new %wim, we have to pull a register
		 * window from the users stack.
		 */
ret_trap_pull_one_window:
		rd	%wim, %t_wim
		sll	%t_wim, 0x1, %twin_tmp1
		srl	%t_wim, 0x7, %twin_tmp2
		or	%twin_tmp2, %twin_tmp1, %twin_tmp2
		and	%twin_tmp2, 0xff, %twin_tmp2

		wr	%twin_tmp2, 0x0, %wim
		WRITE_PAUSE
		andcc	%fp, 0x7, %g0	

		/* Restore first user register window now */
		restore	%g0, %g0, %g0
		LOAD_WINDOW(sp)
		save	%g0, %g0, %g0

ret_trap_userwins_ok:
	LOAD_PT_PRIV(sp, t_psr, t_pc, t_npc)
	or	%t_pc, %t_npc, %g2
	andcc	%g2, 0x3, %g0
	bne	ret_trap_unaligned_pc
	 nop

	LOAD_PT_YREG(sp, g1)
	LOAD_PT_GLOBALS(sp)

	LEAVE_SYSCALL

	wr	%t_psr, 0x0, %psr
	WRITE_PAUSE

	jmp	%t_pc
	rett	%t_npc
	
ret_trap_unaligned_pc:
	add	%sp, REGWIN_SZ, %o0
	ld	[%sp + REGWIN_SZ + PT_PC], %o1
	ld	[%sp + REGWIN_SZ + PT_NPC], %o2
	ld	[%sp + REGWIN_SZ + PT_PSR], %o3

	wr	%t_wim, 0x0, %wim		! or else...
	WRITE_PAUSE

	wr	%t_psr, PSR_ET, %psr
	WRITE_PAUSE

	call	C_LABEL(do_memaccess_unaligned)
	 nop

	b	ret_trap_entry			! maybe signal posted
	 nop

ret_trap_kernel:
	ld	[%sp + REGWIN_SZ + PT_PSR], %t_psr
	wr	%t_psr, 0x0, %psr
	WRITE_PAUSE

		/* Will the rett land us in the invalid window? */
		mov	2, %g1
		sll	%g1, %t_psr, %g1
		srl	%g1, 8, %g2
		or	%g1, %g2, %g1
		rd	%wim, %g2
		andcc	%g2, %g1, %g0
		be	1f		! Nope, just return from the trap
		 nop

		/* We have to grab a window before returning. */
		sll	%g2, 0x1, %g1
		srl	%g2, 7,  %g2
		or	%g1, %g2, %g1
		and	%g1, 0xff, %g1

	wr	%g1, 0x0, %wim
	WRITE_PAUSE

	restore	%g0, %g0, %g0
	LOAD_WINDOW(sp)
	save	%g0, %g0, %g0

	/* Reload the entire frame in case this is from a
	 * kernel system call or whatever...
	 */
1:
	LOAD_PT_ALL(sp, t_psr, t_pc, t_npc, g1)

	LEAVE_SYSCALL

	wr	%t_psr, 0x0, %psr
	WRITE_PAUSE

	jmp	%t_pc
	rett	%t_npc
