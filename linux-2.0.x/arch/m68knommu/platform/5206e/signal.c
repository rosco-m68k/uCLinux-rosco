/*
 *  linux/arch/m68knommu/platform/5206e/signal.c
 *
 *  Copyright (C) 2001  Lineo, Inc.  (www.lineo.com) 
 *  Copyright (C) 2000  Lineo, Inc.  (www.lineo.com) 
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@lineo.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *
 *  Based on:
 *
 *  linux/arch/m68k/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

/*
 * Linux/m68k support by Hamish Macdonald
 *
 * 68000 and CPU32 support D. Jeff Dionne
 * 68060 fixes by Jesper Skov
 * 5307  fixes by David W. Miller
 * ColdFire support by Greg Ungerer (gerg@snapgear.com)
 */

/*
 * ++roman (07/09/96): implemented signal stacks (specially for tosemu on
 * Atari :-) Current limitation: Only one sigstack can be active at one time.
 * If a second signal with SA_STACK set arrives while working on a sigstack,
 * SA_STACK is ignored. This behaviour avoids lots of trouble with nested
 * signal handlers!
 */

#include <linux/config.h>

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>

#include <asm/setup.h>
#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/traps.h>

/*
#define DEBUG
*/

#define offsetof(type, member)  ((size_t)(&((type *)0)->member))

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options);
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs *regs);

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int do_sigsuspend(struct pt_regs *regs)
{
	unsigned long oldmask = current->blocked;
	unsigned long newmask = regs->d3;

	current->blocked = newmask & _BLOCKABLE;
	regs->d0 = -EINTR;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(oldmask, regs))
			return -EINTR;
	}
}

asmlinkage int do_sigreturn(unsigned long __unused)
{
	struct sigcontext_struct context;
	struct pt_regs *regs;
	struct switch_stack *sw;
	int fsize = 0;
	unsigned long fp;
	unsigned long usp = rdusp();

#ifdef DEBUG
	printk("sys_sigreturn, usp=%08x\n", (unsigned) usp);
#endif

	/* get stack frame pointer */
	sw = (struct switch_stack *) &__unused;
	regs = (struct pt_regs *) (sw + 1);

	/* get previous context (including pointer to possible extra junk) */
        if (verify_area(VERIFY_READ, (void *)usp, sizeof(context)))
                goto badframe;

	memcpy_fromfs(&context,(void *)usp, sizeof(context));

	fp = usp + sizeof (context);

	/* restore signal mask */
	current->blocked = context.sc_mask & _BLOCKABLE;

	/* restore passed registers */
	regs->d0 = context.sc_d0;
	regs->d1 = context.sc_d1;
	regs->a0 = context.sc_a0;
	regs->a1 = context.sc_a1;
	regs->sr = (regs->sr & 0xff00) | (context.sc_sr & 0xff);
	regs->pc = context.sc_pc;
	regs->orig_d0 = -1;		/* disable syscall checks */
	wrusp(context.sc_usp);
	if (context.sc_usp != fp+fsize) {
#ifdef DEBUG
                printk("Stack off by %d\n",context.sc_usp - (fp+fsize));
#endif
		current->flags &= ~PF_ONSIGSTK;
	}

	return regs->d0;
badframe:
        do_exit(SIGSEGV);
}


/* This snippet of code is used to return from a user land signal processor.
 * All such returns are via here.  This code used to live on the stack
 * but that causes major problems when fast interrupts are enabled.
 * We need the end of this routine for the memory protection code.
 */
asmlinkage void mcf_signal_return_odd(void);
asmlinkage void mcf_signal_return_even(void);
asm(	".globl mcf_signal_return\n\t"
	".globl mcf_signal_return_end\n"

	"mcf_signal_return:\n"
	"mcf_signal_return_odd:\n"
	"	movq	#18, %d0\n"
	"	jbra	1f\n"

	"mcf_signal_return_even:\n"
	"	moveq	#16, %d0\n"
	"1:	move.l	%sp@(12), %a5\n"
	"	adda.l	%d0, %sp\n"
	"	moveq	#119, %d0\n"
	"	trap	#0\n"
	"mcf_signal_return_end:"
);

/*
 * Set up a signal frame...
 *
 * This routine is somewhat complicated by the fact that if the
 * kernel may be entered by an exception other than a system call;
 * e.g. a bus error or other "bad" exception.  If this is the case,
 * then *all* the context on the kernel stack frame must be saved.
 *
 * For a large number of exceptions, the stack frame format is the same
 * as that which will be created when the process traps back to the kernel
 * when finished executing the signal handler.	In this case, nothing
 * must be done.  This is exception frame format "0".  For exception frame
 * formats "2", "9", "A" and "B", the extra information on the frame must
 * be saved.  This information is saved on the user stack and restored
 * when the signal handler is returned.
 *
 * The format of the user stack when executing the signal handler is:
 *
 *     usp ->  00: RETADDR (points to mcf_signal_return)
 *	       01: signum  (parm #1)
 *	       02: sigcode (parm #2 ; vector number)
 *	       03: scp     (parm #3 ; sigcontext pointer, pointer to #1 below)
 *             04: a5      saved value of a5 for user app
		 *             05: code0   (move pc@(-6), a5);  restore original a5
		 *	       06: code1   (addaw #3x,sp) ; pop parms and code off stack
		 *	       07: code2   (moveq #119,d0; trap #0) ; sigreturn syscall
		 *             08: slot1   (spare slots for ill-timed exception frame)
		 *             09: slot2
	 *     #1|     10: oldmask
 *     #1|     05: oldmask
 *	 |     06: old usp
 *	 |     07: d0      (first saved reg)
 *	 |     08: d1
 *	 |     09: a0
 *	 |     10: a1
 *	 |     11: sr      (saved status register)
 *	 |     12: pc      (old pc; one to return to)
 * ----->|     13: forvec  (format and vector word of old supervisor stck frame)
 *	 |     14: floating point context
 *
 * These are optionally followed by some extra stuff, depending on the
 * stack frame interrupted. This is 1 longword for format "2", 3
 * longwords for format "9", 6 longwords for format "A", and 21
 * longwords for format "B".
 *
 * everything after ------> is not present if the processor does not push a vector
 * format.  Only 68000 and the new 68000 modular core are like this
 */

#define UFRAME_SIZE(fs) (sizeof(struct sigcontext_struct)/4 + 5 + fs/4)

static void setup_frame (struct sigaction * sa, struct pt_regs *regs,
			 int signr, unsigned long oldmask)
{
	struct sigcontext_struct context;
	unsigned long *frame, *tframe;
	int stackOdd;
#define fsize 0

	frame = (unsigned long *)rdusp();
	stackOdd = !!(frame[-2] & 0x20000000);
	if (!(current->flags & PF_ONSIGSTK) && (sa->sa_flags & SA_STACK)) {
		frame = (unsigned long *)sa->sa_restorer;
		current->flags |= PF_ONSIGSTK;
	}
#ifdef DEBUG
printk("setup_frame: usp %.8x\n",frame);
#endif
	frame -= UFRAME_SIZE(fsize);
#ifdef DEBUG
printk("setup_frame: frame %.8x\n",frame);
#endif
	if (verify_area(VERIFY_WRITE,frame,UFRAME_SIZE(fsize)*4))
		do_exit(SIGSEGV);
	if (fsize) {
		memcpy_tofs (frame + UFRAME_SIZE(0), regs + 1, fsize);
		regs->stkadj = fsize;
	}

/* set up the "normal" stack seen by the signal handler */
	tframe = frame;

	/* return address points to code on stack */
	if (stackOdd)
		put_user((ulong)(&mcf_signal_return_odd), tframe);
	else
		put_user((ulong)(&mcf_signal_return_even), tframe);
	tframe++;
	if (current->exec_domain && current->exec_domain->signal_invmap)
	    put_user(current->exec_domain->signal_invmap[signr], tframe);
	else
	    put_user(signr, tframe);
	tframe++;
	put_user((ulong)0, tframe); tframe++;
	/* "scp" parameter.  points to sigcontext */
	put_user((ulong)(frame+10), tframe); tframe++;

	/* set up the return code... */
	/* Digestable ColdFire code :-) */

	/* put a5 on the stack first */
	put_user((ulong)rda5(), tframe); tframe++;

/* setup and copy the sigcontext structure */
	context.sc_mask       = oldmask;
	context.sc_usp	      = rdusp();
	context.sc_d0	      = regs->d0;
	context.sc_d1	      = regs->d1;
	context.sc_a0	      = regs->a0;
	context.sc_a1	      = regs->a1;
	context.sc_sr	      = regs->sr;
	context.sc_pc	      = regs->pc;

	memcpy_tofs (tframe, &context, sizeof(context));
#ifdef DEBUG
printk("setup_frame: top tframe %.8x\n",tframe + sizeof(context));
#endif

	/* Set up registers for signal handler */
	wrusp ((unsigned long) frame);
	regs->pc = (unsigned long) sa->sa_handler;

	/* Prepare to skip over the extra stuff in the exception frame.  */
	if (regs->stkadj) {
		struct pt_regs *tregs =
			(struct pt_regs *)((ulong)regs + regs->stkadj);

#ifdef DEBUG
		printk("Performing stackadjust=%04x\n", regs->stkadj);
#endif

		/* This must be copied with decreasing addresses to
                   handle overlaps.  */
		tregs->pc = regs->pc;
		tregs->sr = regs->sr;
	}
}

/*
 * OK, we're invoking a handler
 */	
static void handle_signal(unsigned long signr, struct sigaction *sa,
			  unsigned long oldmask, struct pt_regs *regs)
{
#ifdef DEBUG
	printk("Entering handle_signal, signr=%d\n", signr);
#endif
	/* are we from a system call? */
	if (regs->orig_d0 >= 0) {
		/* If so, check system call restarting.. */
		switch (regs->d0) {
			case -ERESTARTNOHAND:
				regs->d0 = -EINTR;
				break;

			case -ERESTARTSYS:
				if (!(sa->sa_flags & SA_RESTART)) {
					regs->d0 = -EINTR;
					break;
				}
		/* fallthrough */
			case -ERESTARTNOINTR:
				regs->d0 = regs->orig_d0;
				regs->pc -= 2;
		}
	}

	/* set up the stack frame */
	setup_frame(sa, regs, signr, oldmask);

	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;
	if (!(sa->sa_flags & SA_NOMASK))
		current->blocked |= (sa->sa_mask | _S(signr)) & _BLOCKABLE;
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals
 * that the kernel can handle, and then we build all the user-level signal
 * handling stack-frames in one go after that.
 */
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs *regs)
{
	unsigned long mask = ~current->blocked;
	unsigned long signr;
	struct sigaction * sa;

	current->tss.esp0 = (unsigned long) regs;

	/* If the process is traced, all signals are passed to the debugger. */
	if (current->flags & PF_PTRACED)
		mask = ~0UL;
	while ((signr = current->signal & mask) != 0) {
                signr = ffz(~signr);
                clear_bit(signr, &current->signal);
		sa = current->sig->action + signr;
#ifdef DEBUG
		printk("Signal %d, Handler %d: ", signr, sa->sa_handler);
#endif
		signr++;

		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;

			/* Did we come from a system call? */
			if (regs->orig_d0 >= 0) {
				/* Restart the system call */
				if (regs->d0 == -ERESTARTNOHAND ||
				    regs->d0 == -ERESTARTSYS ||
				    regs->d0 == -ERESTARTNOINTR) {
					regs->d0 = regs->orig_d0;
					regs->pc -= 2;
				}
			}
			notify_parent(current, SIGCHLD);
			schedule();
			if (!(signr = current->exit_code)) {
			    discard_frame:
			    continue;
			}
			current->exit_code = 0;
			if (signr == SIGSTOP)
				goto discard_frame;
			if (_S(signr) & current->blocked) {
				current->signal |= _S(signr);
				mask &= ~_S(signr);
				continue;
			}
			sa = current->sig->action + signr - 1;
		}
		if (sa->sa_handler == SIG_IGN) {
#ifdef DEBUG
			printk("Ignoring.\n");
#endif
			if (signr != SIGCHLD)
				continue;
			/* check for SIGCHLD: it's special */
			while (sys_waitpid(-1,NULL,WNOHANG) > 0)
				/* nothing */;
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
#ifdef DEBUG
			printk("Default.\n");
#endif
			if (current->pid == 1)
				continue;
			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (is_orphaned_pgrp(current->pgrp))
					continue;
			case SIGSTOP:
				if (current->flags & PF_PTRACED)
					continue;
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa_flags & 
				      SA_NOCLDSTOP))
					notify_parent(current, SIGCHLD);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGIOT: case SIGFPE: case SIGSEGV:
				if (current->binfmt && current->binfmt->core_dump) {
				    if (current->binfmt->core_dump(signr, regs))
					signr |= 0x80;
				}
				/* fall through */
			default:
				current->signal |= _S(signr & 0x7f);
				current->flags |= PF_SIGNALED;
				do_exit(signr);
			}
		}
		handle_signal(signr, sa, oldmask, regs);
		return 1;
	}

	/* Did we come from a system call? */
	if (regs->orig_d0 >= 0) {
		/* Restart the system call - no handlers present */
		if (regs->d0 == -ERESTARTNOHAND ||
		    regs->d0 == -ERESTARTSYS ||
		    regs->d0 == -ERESTARTNOINTR) {
			regs->d0 = regs->orig_d0;
			regs->pc -= 2;
		}
	}

	/* If we are about to discard some frame stuff we must copy
	   over the remaining frame. */
	if (regs->stkadj) {
		struct pt_regs *tregs =
		  (struct pt_regs *) ((ulong) regs + regs->stkadj);
#ifdef DEBUG
	printk("!!!!!!!!!!!!!! stkadj !!!\n");
#endif
		/* This must be copied with decreasing addresses to
		   handle overlaps.  */
		tregs->pc = regs->pc;
		tregs->sr = regs->sr;
	}
	return 0;
}


/*
 *	return the correct value for a5/start_data,  mainly for signal
 *	handlers to ensure they get the correct data segment.
 */

unsigned long
get_pic_a5()
{
#ifdef DEBUG
  	unsigned long a5;

	__asm__ __volatile__("movel %/a5,%0"
			     : "=a" (a5));
	if (current->mm->start_data != a5)
		printk("broken a5 0x%x != 0x%x\n", current->mm->start_data, a5);
#endif
	return(current->mm->start_data);
}

