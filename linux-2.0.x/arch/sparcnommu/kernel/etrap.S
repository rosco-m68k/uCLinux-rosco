/* $Id: etrap.S,v 1.18 1996/04/25 06:08:35 davem Exp $
 * etrap.S: Sparc trap window preparation for entry into the
 *          Linux kernel.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <asm/cprefix.h>
#include <asm/head.h>
#include <asm/asi.h>
#include <asm/contregs.h>
#include <asm/page.h>
#include <asm/psr.h>
#include <asm/ptrace.h>
#include <asm/winmacro.h>

/* Registers to not touch at all. */
#define t_psr        l0 /* Set by caller */
#define t_pc         l1 /* Set by caller */
#define t_npc        l2 /* Set by caller */
#define t_wim        l3 /* Set by caller */
#define t_twinmask   l4 /* Set at beginning of this entry routine. */
#define t_kstack     l5 /* Set right before pt_regs frame is built */
#define t_retpc      l6 /* If you change this, change winmacro.h header file */
#define t_systable   l7 /* Never touch this, could be the syscall table ptr. */
#define curptr       g4 /* Set after pt_regs frame is built */

	.text
	.align 4

	/* At trap time, interrupts and all generic traps do the
	 * following:
	 *
	 * rd	%psr, %l0
	 * b	some_handler
	 * rd	%wim, %l3
	 * nop
	 *
	 * Then 'some_handler' if it needs a trap frame (ie. it has
	 * to call c-code and the trap cannot be handled in-window)
	 * then it does the SAVE_ALL macro in entry.S which does
	 *
	 * sethi	%hi(trap_setup), %l4
	 * jmpl		%l4 + %lo(trap_setup), %l6
	 * mov		1, %l4
	 */

	/* 2 3 4  window number
	 * -----
	 * O T S  mnemonic
	 *
	 * O == Current window before trap
	 * T == Window entered when trap occurred
	 * S == Window we will need to save if (1<<T) == %wim
	 *
	 * Before execution gets here, it must be guaranteed that
	 * %l0 contains trap time %psr, %l1 and %l2 contain the
	 * trap pc and npc, and %l3 contains the trap time %wim.
	 */

	.globl	trap_setup
trap_setup:
	/* Calculate mask of trap window.  See if from user
	 * or kernel and branch conditionally.
	 */

	mov	1, %t_twinmask
	sll	%t_twinmask, %t_psr, %t_twinmask ! t_twinmask = (1 << psr)
	andcc	%t_psr, PSR_PS, %g0		 ! fromsupv_p = (psr & PSR_PS)
	be	trap_setup_from_user		 ! nope, from user mode
	 nop

	/* From kernel, allocate more kernel stack and
	 * build a pt_regs trap frame.
	 */
	sub	%fp, (REGWIN_SZ + TRACEREG_SZ), %t_kstack
	STORE_PT_ALL(t_kstack, t_psr, t_pc, t_npc, g2)

	/* See if we are in the trap window. */
	andcc	%t_twinmask, %t_wim, %g0
	be	1f
	 nop

	b	trap_setup_kernel_spill		! in trap window, clean up
	 nop

	/* Trap from kernel with a window available.
	 * Just do it...
	 */
1:
	jmpl	%t_retpc + 0x8, %g0	! return to caller
	 mov	%t_kstack, %sp		! jump onto new stack

trap_setup_kernel_spill:
	LOAD_CURRENT(curptr, g1)
	ld	[%curptr + THREAD_UMASK], %g1
	orcc	%g0, %g1, %g0
	bne	trap_setup_user_spill	! there are some user windows, yuck
	 nop

		/* Spill from kernel, but only kernel windows, adjust
		 * %wim and go.
		 */
		srl	%t_wim, 0x1, %g2	! begin computation of new %wim
		sll	%t_wim, 0x7, %t_wim
		or	%t_wim, %g2, %g2
		and	%g2, 0xff, %g2

	save	%g0, %g0, %g0

	/* Set new %wim value */
	wr	%g2, 0x0, %wim
	WRITE_PAUSE

	/* Save the kernel window onto the corresponding stack. */
	STORE_WINDOW(sp)

	restore	%g0, %g0, %g0

	jmpl	%t_retpc + 0x8, %g0	! return to caller
	 mov	%t_kstack, %sp		! and onto new kernel stack

trap_setup_from_user:
	/* We can't use %curptr yet. */
	LOAD_CURRENT(t_kstack, t_twinmask)
	mov	1, %t_twinmask
	ld	[%t_kstack + TASK_SAVED_KSTACK], %t_kstack
	sll	%t_twinmask, %t_psr, %t_twinmask ! t_twinmask = (1 << psr)

	/* Build pt_regs frame. */
	STORE_PT_ALL(t_kstack, t_psr, t_pc, t_npc, g2)

	/* Clear current->tss.w_saved */
	LOAD_CURRENT(curptr, g1)
	st	%g0, [%curptr + THREAD_W_SAVED]

	/* See if we are in the trap window. */
	andcc	%t_twinmask, %t_wim, %g0
	bne	trap_setup_user_spill		! yep we are
	 orn	%g0, %t_twinmask, %g1		! negate trap win mask into %g1

		/* Trap from user, but not into the invalid window.
		 * Calculate new umask.  The way this works is,
		 * any window from the %wim at trap time until
		 * the window right before the one we are in now,
		 * is a user window.  A diagram:
		 *
		 *      7 6 5 4 3 2 1 0    window number
		 *      ---------------
		 *        I     L T        mnemonic
		 *
		 * Window 'I' is the invalid window in our example,
		 * window 'L' is the window the user was in when
		 * the trap occurred, window T is the trap window
		 * we are in now.  So therefore, windows 5, 4 and
		 * 3 are user windows.  The following sequence
		 * computes the user winmask to represent this.
		 */
		subcc	%t_wim, %t_twinmask, %g2
		bneg,a	1f
		 sub	%g2, 0x1, %g2
1:
		andn	%g2, %t_twinmask, %g2
		and	%g2, 0xff, %g2
		st	%g2, [%curptr + THREAD_UMASK]	! store new umask

		jmpl	%t_retpc + 0x8, %g0		! return to caller
		 mov	%t_kstack, %sp			! and onto kernel stack

trap_setup_user_spill:
		/* A spill occurred from either kernel or user mode
		 * and there exist some user windows to deal with.
		 * A mask of the currently valid user windows
		 * is in %g1 upon entry to here.
		 */

		and	%g1, 0xff, %g1
		srl	%t_wim, 0x1, %g2	! compute new %wim
		sll	%t_wim, 0x7, %t_wim
		or	%t_wim, %g2, %g2	! %g2 is new %wim
		and	%g2, 0xff, %g2
		andn	%g1, %g2, %g1		! clear this bit in %g1
		st	%g1, [%curptr + THREAD_UMASK]

		save	%g0, %g0, %g0

	wr	%g2, 0x0, %wim
	WRITE_PAUSE
	andcc	%sp, 0x7, %g0

trap_setup_good_ustack:
	STORE_WINDOW(sp)

trap_setup_finish_up:
	restore %g0, %g0, %g0

	jmpl	%t_retpc + 0x8, %g0
	 mov	%t_kstack, %sp

