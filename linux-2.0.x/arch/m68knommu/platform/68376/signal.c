/*
 *  linux/arch/m68knommu/kernel/signal.c
 *
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
 */

/*
 * ++roman (07/09/96): implemented signal stacks (specially for tosemu on
 * Atari :-) Current limitation: Only one sigstack can be active at one time.
 * If a second signal with SA_STACK set arrives while working on a sigstack,
 * SA_STACK is ignored. This behaviour avoids lots of trouble with nested
 * signal handlers!
 */

#include <linux/config.h>

#ifdef CONFIG_M68328
#define NO_FORMAT_VEC
#endif

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

#ifndef NO_FORMAT_VEC
static const int extra_sizes[16] = {
  0,
  -1, /* sizeof(((struct frame *)0)->un.fmt1), */
  sizeof(((struct frame *)0)->un.fmt2),
  sizeof(((struct frame *)0)->un.fmt3),
  sizeof(((struct frame *)0)->un.fmt4),
  -1, /* sizeof(((struct frame *)0)->un.fmt5), */
  -1, /* sizeof(((struct frame *)0)->un.fmt6), */
  sizeof(((struct frame *)0)->un.fmt7),
  -1, /* sizeof(((struct frame *)0)->un.fmt8), */
  sizeof(((struct frame *)0)->un.fmt9),
  sizeof(((struct frame *)0)->un.fmta),
  sizeof(((struct frame *)0)->un.fmtb),
  sizeof(((struct frame *)0)->un.fmtc),
  -1, /* sizeof(((struct frame *)0)->un.fmtd), */
  -1, /* sizeof(((struct frame *)0)->un.fmte), */
  -1, /* sizeof(((struct frame *)0)->un.fmtf), */
};
#endif

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
#ifndef NO_FORMAT_VEC
	int formatvec = 0;
#endif
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
#ifndef NO_FORMAT_VEC
	formatvec = context.sc_formatvec;
	regs->format = formatvec >> 12;
	regs->vector = formatvec & 0xfff;

	fsize = extra_sizes[regs->format];
	if (fsize < 0) {
		/*
		 * user process trying to return with weird frame format
		 */
#ifdef DEBUG
		printk("user process returning with weird frame format\n");
#endif
		goto badframe;
	}
#endif /* !NO_FORMAT_VEC */
	if (context.sc_usp != fp+fsize) {
#ifdef DEBUG
                printk("Stack off by %d\n",context.sc_usp - (fp+fsize));
#endif
		current->flags &= ~PF_ONSIGSTK;
	}
#ifndef NO_FORMAT_VEC	
	/* OK.	Make room on the supervisor stack for the extra junk,
	 * if necessary.
	 */

	if (fsize) {
		if (verify_area(VERIFY_READ, (void *)fp, fsize))
                        goto badframe;

#define frame_offset (sizeof(struct pt_regs)+sizeof(struct switch_stack))
		__asm__ __volatile__
			("movel %0,%/a0\n\t"
			 "subl %1,%/a0\n\t"     /* make room on stack */
			 "movel %/a0,%/sp\n\t"  /* set stack pointer */
			 /* move switch_stack and pt_regs */
			 "1: movel %0@+,%/a0@+\n\t"
			 "   dbra %2,1b\n\t"
			 "lea %/sp@(%c3),%/a0\n\t" /* add offset of fmt stuff */
			 "lsrl  #2,%1\n\t"
			 "subql #1,%1\n\t"
			 "2: movesl %4@+,%2\n\t"
			 "   movel %2,%/a0@+\n\t"
			 "   dbra %1,2b\n\t"
			 "bral " SYMBOL_NAME_STR(ret_from_signal)
			 : /* no outputs, it doesn't ever return */
			 : "a" (sw), "d" (fsize), "d" (frame_offset/4-1),
			   "n" (frame_offset), "a" (fp)
			 : "a0");
#undef frame_offset
		goto badframe;
		/* NOTREACHED */
	}
#endif /* NO_FORMAT_VEC */

	return regs->d0;
badframe:
        do_exit(SIGSEGV);
}

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
 *     usp ->  RETADDR (points to code below)
 *	       signum  (parm #1)
 *	       sigcode (parm #2 ; vector number)
 *	       scp     (parm #3 ; sigcontext pointer, pointer to #1 below)
 *	       code1   (addaw #20,sp) ; pop parms and code off stack
 *	       code2   (moveq #119,d0; trap #0) ; sigreturn syscall
 *     #1|     oldmask
 *	 |     old usp
 *	 |     d0      (first saved reg)
 *	 |     d1
 *	 |     a0
 *	 |     a1
 *	 |     sr      (saved status register)
 *	 |     pc      (old pc; one to return to)
 * ----->|     forvec  (format and vector word of old supervisor stack frame)
 *	 |     floating point context
 *
 * These are optionally followed by some extra stuff, depending on the
 * stack frame interrupted. This is 1 longword for format "2", 3
 * longwords for format "9", 6 longwords for format "A", and 21
 * longwords for format "B".
 *
 * everything after ------> is not present if the processor does not push a vector
 * format.  Only 68000 and the new 68000 modular core are like this
 */

#define UFRAME_SIZE(fs) (sizeof(struct sigcontext_struct)/4 + 6 + fs/4)

static void setup_frame (struct sigaction * sa, struct pt_regs *regs,
			 int signr, unsigned long oldmask)
{
	struct sigcontext_struct context;
	unsigned long *frame, *tframe;
#ifndef NO_FORMAT_VEC
	int fsize = extra_sizes[regs->format];

	if (fsize < 0) {
#ifdef DEBUG
		printk ("setup_frame: Unknown frame format %#x\n",
			regs->format);
#endif
		do_exit(SIGSEGV);
	}
#else /* !NO_FORMAT_VEC */
#define fsize 0
#endif /* !NO_FORMAT_VEC */

	frame = (unsigned long *)rdusp();
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
	put_user((ulong)(frame+4), tframe); tframe++;
	if (current->exec_domain && current->exec_domain->signal_invmap)
	    put_user(current->exec_domain->signal_invmap[signr], tframe);
	else
	    put_user(signr, tframe);
	tframe++;
	/* dummy if NO_FORMAT_VEC */
#ifndef NO_FORMAT_VEC
	put_user(regs->vector, tframe);
#endif
	tframe++;
	/* "scp" parameter.  points to sigcontext */
	put_user((ulong)(frame+6), tframe); tframe++;

/* set up the return code... */
	put_user(0xdefc0014,tframe); tframe++; /* addaw #20,sp */
	put_user(0x70774e40,tframe); tframe++; /* moveq #119,d0; trap #0 */

/* setup and copy the sigcontext structure */
	context.sc_mask       = oldmask;
	context.sc_usp	      = rdusp();
	context.sc_d0	      = regs->d0;
	context.sc_d1	      = regs->d1;
	context.sc_a0	      = regs->a0;
	context.sc_a1	      = regs->a1;
	context.sc_sr	      = regs->sr;
	context.sc_pc	      = regs->pc;
#ifndef NO_FORMAT_VEC
	context.sc_formatvec  = (regs->format << 12 | regs->vector);
#endif

	memcpy_tofs (tframe, &context, sizeof(context));
#ifdef DEBUG
printk("setup_frame: top tframe %.8x\n",tframe + sizeof(context));
#endif

#ifndef NO_FORMAT_VEC
	/*
	 * no matter what frame format we were using before, we
	 * will do the "RTE" using a normal 4 word frame.
	 */
	regs->format = 0;
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
#ifndef NO_FORMAT_VEC
		tregs->vector = regs->vector;
		tregs->format = regs->format;
#endif
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
		signr++;

#ifdef DEBUG
		printk("Signal %d: ", sa->sa_handler);
#endif

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
#ifndef NO_FORMAT_VEC
			    /* Make sure that a faulted bus cycle
			       isn't restarted (only needed on the
			       68030).  */
			    if (regs->format == 10 || regs->format == 11) {
				regs->stkadj = extra_sizes[regs->format];
				regs->format = 0;
			    }
#endif /* !NO_FORMAT_VEC */
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
#if defined(DEBUG) || 1
	printk("!!!!!!!!!!!!!! stkadj !!!\n");
#endif
		/* This must be copied with decreasing addresses to
		   handle overlaps.  */
#ifndef NO_FORMAT_VEC
		tregs->vector = regs->vector;
		tregs->format = regs->format;
#endif
		tregs->pc = regs->pc;
		tregs->sr = regs->sr;
	}
	return 0;
}
