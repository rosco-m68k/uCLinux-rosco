/*
 *  linux/arch/shnommu/kernel/signal.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
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
	unsigned long newmask = regs->r3;

	current->blocked = newmask & _BLOCKABLE;
	regs->r0 = -EINTR;
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
	//usp= usp & 0xFFFFE000;  /*8k kernel stack*/
	usp = current_set[0]->kernel_stack_page;
	/* get stack frame pointer */
	sw = (struct switch_stack *) &__unused;
	regs = (struct pt_regs *) (sw + 1);
	/*posh2 uses ptrefs for this*/
	regs =(struct pt_regs *)  __unused;

	/* get previous context (including pointer to possible extra junk) */
        if (verify_area(VERIFY_READ, (void *)usp, sizeof(context)))
                goto badframe;
		
	fp=usp + 24;
	memcpy_fromfs(&context,(void *)(usp + 24), sizeof(context));

	fp = usp + sizeof (context);

	/* restore signal mask */
	current->blocked = context.sc_mask & _BLOCKABLE;

	/* restore passed registers */
	regs->r0	      = context.r0;
	regs->r1	      = context.r1;
	regs->r2	      = context.r2;
	regs->r3	      = context.r3;
	regs->r4	      = context.r4;
	regs->r5	      = context.r5;
	regs->r6	      = context.r6;
	regs->r7	      = context.r7;
	regs->r8	      = context.r8;
	regs->r9	      = context.r9;
	regs->r10	      = context.r10;
	regs->r11	      = context.r11;
	regs->r12	      = context.r12;
	regs->r13	      = context.r13;
	regs->r14	      = context.r14;
	regs->sr	      = context.sr;
	regs->pc	      = context.pc;
	regs->vec	      = context.vec;
	regs->pr	      = context.pr;
	regs->orig_r0	      = -1;	/* disable syscall checks */
	
	
	wrusp(context.sc_usp);

	if (context.sc_usp != fp+fsize) {
#ifdef DEBUG
                printk("Stack off by %d\n",context.sc_usp - (fp+fsize));
#endif
		current->flags &= ~PF_ONSIGSTK;
	}

	return regs->r0;
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
 *	 |     r0      (first saved reg)
 *	 |     r1
 *	 |     r2
 *	 |     r3
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
	unsigned long *frame, *tframe,tusp;
#define fsize 0

/*	tusp = (unsigned long )rdusp();
	tusp = tusp &  0xFFFFE000;*/
	tusp =current_set[0]->kernel_stack_page;
	frame =(unsigned long *) tusp;
	if (!(current->flags & PF_ONSIGSTK) && (sa->sa_flags & SA_STACK)) {
		frame = (unsigned long *)sa->sa_restorer;
		current->flags |= PF_ONSIGSTK;
	}
#ifdef DEBUG
printk("setup_frame: usp %.8x\n",frame);
#endif
	/*frame += UFRAME_SIZE(fsize);*/
#ifdef DEBUG
printk("setup_frame: frame %.8x\n",frame);
#endif
	if (verify_area(VERIFY_WRITE,frame,UFRAME_SIZE(fsize)*4))
		do_exit(SIGSEGV);
	if (fsize) {
		memcpy_tofs (frame + UFRAME_SIZE(0), regs + 1, fsize);
	/*Posh2 commented	regs->stkadj = fsize;*/
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
	tframe++;
	/* "scp" parameter.  points to sigcontext */
	put_user((ulong)(frame+6), tframe); tframe++; 

/* set up the return code... */
//	put_user(0x7f14e077,tframe); tframe++; /* addaw #20,sp */  /*add #20,r15 ,mov #119,r0,*/
//	put_user(0xC3200009,tframe); tframe++; /* moveq #119,d0; trap #0 */ /* trapa  #32,nop*/
	put_user(0xe077C320,tframe); tframe++; /* addaw #20,sp */  /*mov #119,r0,trapa #32*/
	put_user(0xe00090009,tframe); tframe++; /* addaw #20,sp */  /*nop,nop*/

/* setup and copy the sigcontext structure */
	context.sc_mask       = oldmask;
	context.sc_usp	      = rdusp();
	context.r0	      = regs->r0;
	context.r1	      = regs->r1;
	context.r2	      = regs->r2;
	context.r3	      = regs->r3;
	context.r4	      = regs->r4;
	context.r5	      = regs->r5;
	context.r6	      = regs->r6;
	context.r7	      = regs->r7;
	context.r8	      = regs->r8;
	context.r9	      = regs->r9;
	context.r10	      = regs->r10;
	context.r11	      = regs->r11;
	context.r12	      = regs->r12;
	context.r13	      = regs->r13;
	context.r14	      = regs->r14;
	context.sr	      = regs->sr;
	context.pc	      = regs->pc;
	context.vec	      = regs->vec;
	context.pr	      = regs->pr;
	context.orig_r0	      = regs->orig_r0;




	memcpy_tofs (tframe, &context, sizeof(context));
#ifdef DEBUG
printk("setup_frame: top tframe %.8x\n",tframe + sizeof(context));
#endif

	/* Set up registers for signal handler */
	/*wrusp ((unsigned long) frame);*/
	regs->pc = (unsigned long) sa->sa_handler;
	regs->pr = (unsigned long)  (frame + 4);
	regs->r4 = (unsigned long) signr;

	/* Prepare to skip over the extra stuff in the exception frame.  */
/*posh2 commented if (regs->stkadj) {
		struct pt_regs *tregs =
			(struct pt_regs *)((ulong)regs + regs->stkadj);*/
         
#ifdef DEBUG
	/*	printk("Performing stackadjust=%04x\n", regs->stkadj);*/
#endif

		/* This must be copied with decreasing addresses to
                   handle overlaps.  */
		/*tregs->pc = regs->pc;
		tregs->sr = regs->sr;
	}*/
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
	if (regs->orig_r0 >= 0) {
		/* If so, check system call restarting.. */
		switch (regs->r0) {
			case -ERESTARTNOHAND:
				regs->r0 = -EINTR;
				break;

			case -ERESTARTSYS:
				if (!(sa->sa_flags & SA_RESTART)) {
					regs->r0 = -EINTR;
					break;
				}
		/* fallthrough */
			case -ERESTARTNOINTR:
				regs->r0 = regs->orig_r0;
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
			if (regs->orig_r0 >= 0) {
				/* Restart the system call */
				if (regs->r0 == -ERESTARTNOHAND ||
				    regs->r0 == -ERESTARTSYS ||
				    regs->r0 == -ERESTARTNOINTR) {
					regs->r0 = regs->orig_r0;
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
	if (regs->orig_r0 >= 0) {
		/* Restart the system call - no handlers present */
		if (regs->r0 == -ERESTARTNOHAND ||
		    regs->r0 == -ERESTARTSYS ||
		    regs->r0 == -ERESTARTNOINTR) {
			regs->r0 = regs->orig_r0;
			regs->pc -= 2;		}
	}

	/* If we are about to discard some frame stuff we must copy
	   over the remaining frame. */
	/*Posh2 commentedif (regs->stkadj) {
		struct pt_regs *tregs =
		  (struct pt_regs *) ((ulong) regs + regs->stkadj);*/
	
#if defined(DEBUG) || 1
	printk("!!!!!!!!!!!!!! stkadj !!!\n");
#endif
		/* This must be copied with decreasing addresses to
		   handle overlaps.  */
	/*	tregs->pc = regs->pc;
		tregs->sr = regs->sr;
	}*/
	return 0;
}
