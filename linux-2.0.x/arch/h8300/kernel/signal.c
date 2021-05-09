/*
 *  linux/arch/h8300/kernel/signal.c
 *
 *  Yoshinori Sato <qzb04471@nifty.ne.jp>
 *
 *  Based on:
 *
 *  linux/arch/m68knommu/kernel/signal.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
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

extern int ptrace_cancel_bpt(struct task_struct *child);
extern int ptrace_set_bpt(struct task_struct *child);

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int do_sigsuspend(struct pt_regs *regs)
{
	unsigned long oldmask = current->blocked;
	unsigned long newmask = regs->er3;

	current->blocked = newmask & _BLOCKABLE;
	regs->er0 = -EINTR;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(oldmask, regs))
			return -EINTR;
	}
}

asmlinkage int do_sigreturn(struct switch_stack *sw)
{
	struct sigcontext_struct context;
	struct pt_regs *regs;
	int fsize = 0;
	unsigned long fp;
	unsigned long usp = rdusp()+10;

#ifdef DEBUG
	printk("sys_sigreturn, usp=%08x\n", (unsigned) usp);
#endif

	/* get stack frame pointer */
	regs = (struct pt_regs *) (sw + 1);

	/* get previous context (including pointer to possible extra junk) */
        if (verify_area(VERIFY_READ, (void *)usp, sizeof(context)))
                goto badframe;

	memcpy_fromfs(&context,(void *)usp, sizeof(context));

	fp = usp + sizeof (context);

	/* restore signal mask */
	current->blocked = context.sc_mask & _BLOCKABLE;

	/* restore passed registers */
	regs->er0 = context.sc_er0;
	regs->er1 = context.sc_er1;
	regs->er2 = context.sc_er2;
	regs->ccr = context.sc_sr;
	regs->pc = context.sc_pc;
	wrusp(context.sc_usp);

	if (context.sc_usp != fp+fsize) {
#ifdef DEBUG
                printk("Stack off by %d\n",context.sc_usp - (fp+fsize));
#endif
		current->flags &= ~PF_ONSIGSTK;
	}

        if (ptrace_cancel_bpt (current))
      	        send_sig (SIGTRAP, current, 1);

	return regs->er0;
badframe:
        do_exit(SIGSEGV);
}

/*
Pardon. It translates into English later.
Yoshinori Sato

シグナルハンドラから戻ってくるためのスタックフレームを作成。
作るのはユーザースタック。
カーネル側に作るのは気持ち悪い。

+--------------+ <- new usp
| sig_hdr_addr |
+--------------+
| sig_ret_addr ---+ このアドレスをさしておく
+--------------+<-+
| sig_ret      |    sub.l er0,er0; mov.b #119,r0l; trap #0;
+--------------+
|  sigcontext  |
+--------------+ <-old usp
| original     |
| stack frame  |
|              |
|              |

1.RESTORE_ALLのrteでsignal handler起動
2.signal handlerのrtsでsig_retに制御がまわる
3.SYSCALL(SIGRETURN)で戻ってこれる

*/
static void setup_frame (struct sigaction * sa, struct pt_regs *regs,
			 int signr, unsigned long oldmask)
{
	struct sigcontext_struct context;
	unsigned char *frame;
	const unsigned short sig_ret[3]={0x1a80,0xf877,0x5700}; 

	frame = (unsigned char  *)rdusp();
	if (!(current->flags & PF_ONSIGSTK) && (sa->sa_flags & SA_STACK)) {
		frame = (unsigned long *)sa->sa_restorer;
		current->flags |= PF_ONSIGSTK;
	}

	/* setup signalcontext */
	context.sc_mask       = oldmask;
	context.sc_usp	      = frame;
	context.sc_er0	      = regs->er0;
	context.sc_er1	      = regs->er1;
	context.sc_er2	      = regs->er2;
	context.sc_sr	      = regs->ccr;
	context.sc_pc	      = regs->pc;

#define FRAME_SIZE 4+4+6+sizeof(struct sigcontext_struct) 
        frame -= FRAME_SIZE;

	if (verify_area(VERIFY_WRITE,frame,FRAME_SIZE))
		do_exit(SIGSEGV);
/* set up the "normal" stack seen by the signal handler */
	wrusp ((unsigned long) frame);
	put_fs_long((unsigned long) sa->sa_handler,frame); frame+=4;
	/* return address points to code on stack */
	put_fs_long((unsigned long)(frame+4), frame); frame+=4;
        /* copy sigreturn code*/
	memcpy_tofs (frame, sig_ret, sizeof(sig_ret));
	frame+=sizeof(sig_ret);
	regs->er2 = (unsigned long)frame;
	memcpy_tofs (frame, &context, sizeof(context));
	if (current->exec_domain && current->exec_domain->signal_invmap)
	        regs->er0 = current->exec_domain->signal_invmap[signr];
	else
	        regs->er0 = signr;
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
	if (regs->orig_er0 >= 0) {
		/* If so, check system call restarting.. */
		switch (regs->er0) {
			case -ERESTARTNOHAND:
				regs->er0 = -EINTR;
				break;

			case -ERESTARTSYS:
				if (!(sa->sa_flags & SA_RESTART)) {
					regs->er0 = -EINTR;
					break;
				}
		/* fallthrough */
			case -ERESTARTNOINTR:
				regs->er0 = regs->orig_er0;
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
	unsigned long *pc;
	struct sigaction * sa;
	int single_stepping;

	current->tss.esp0 = (unsigned long) regs;
	pc = (unsigned long *)rdusp();
	regs->pc = *pc & 0xffffff;
	regs->ccr = *pc >> 24;

	single_stepping = ptrace_cancel_bpt(current);
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
			if (regs->orig_er0 >= 0) {
				/* Restart the system call */
				if (regs->er0 == -ERESTARTNOHAND ||
				    regs->er0 == -ERESTARTSYS ||
				    regs->er0 == -ERESTARTNOINTR) {
					regs->er0 = regs->orig_er0;
					*pc -= 2;
				}
			}
			notify_parent(current, SIGCHLD);
			schedule();
			single_stepping |= ptrace_cancel_bpt(current);
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
		if (single_stepping)
		        ptrace_set_bpt(current);
		return 1;
	}
	/* Did we come from a system call? */
	if (regs->orig_er0 >= 0) {
		/* Restart the system call - no handlers present */
		if (regs->er0 == -ERESTARTNOHAND ||
		    regs->er0 == -ERESTARTSYS ||
		    regs->er0 == -ERESTARTNOINTR) {
			regs->er0 = regs->orig_er0;
			*pc -= 2;
		}
	}
#if 0
	/* If we are about to discard some frame stuff we must copy
	   over the remaining frame. */
	if (regs->orig_er0) {
		struct pt_regs *tregs =
		  (struct pt_regs *) ((ulong) regs + regs->orig_er0);
#if defined(DEBUG) || 1
	printk("!!!!!!!!!!!!!! stkadj !!!\n");
#endif
		/* This must be copied with decreasing addresses to
		   handle overlaps.  */
		tregs->pc = regs->pc;
		tregs->ccr = regs->ccr;
	}
#endif
	if (single_stepping)
		ptrace_set_bpt(current);
	return 0;
}




