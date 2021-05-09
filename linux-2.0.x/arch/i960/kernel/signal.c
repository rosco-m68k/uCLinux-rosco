/*
 *  linux/arch/i960/kernel/signal.c
 *
 *  Copyright (C) 1999  Keith Adams	<kma@cse.ogi.edu>
 * 			Oregon Graduate Institute
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


#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

int do_signal(struct pt_regs *regs);

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int do_sigsuspend(struct pt_regs* regs)
{
	int err;
	sigset_t* set = (sigset_t*) regs->gregs[0];
	unsigned long mask;

	if ((err=verify_area(VERIFY_READ, set, 4)))
		return -EINTR;

	mask = current->blocked;
	current->blocked = *set & _BLOCKABLE;
	regs->gregs[0] = -EINTR;
	for (;;) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(regs))
			return -EINTR;
	}
}

/*
 * We don't ever need this on i960; the function return path lets setup_frame
 * return straight to the faulting instruction.
 */
asmlinkage int do_sigreturn(unsigned long __unused)
{
	printk("killing %d for trying to sigreturn\n");
	do_exit(SIGSEGV);
	return -1;
}

#define STACK_ALIGN 16
static inline unsigned long stack_align(unsigned long sp)
{
	int lobits  = sp & (STACK_ALIGN -1);

	if (sp & lobits)
		sp += STACK_ALIGN - lobits;
	return sp;
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals
 * that the kernel can handle, and then we build all the user-level signal
 * handling stack-frames in one go after that.
 * 
 * XXX: todo: sigcontext stuff.
 */

asmlinkage int do_signal(struct pt_regs *regs)
{
	unsigned long mask = ~current->blocked;
	unsigned long handler_signal = 0;
	unsigned long signr;
	struct sigaction * sa;

	while ((signr = current->signal & mask)) {
		signr = ffz(~signr);
		clear_bit(signr, &current->signal);
		sa = current->sig->action + signr;
		signr++;
		
		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;
			notify_parent(current, signr);
			schedule();
			
			if (!(signr = current->exit_code))
				continue;
			current->exit_code = 0;
			if (signr == SIGSTOP)
				continue;
			if (_S(signr) & current->blocked) {
				current->signal |= _S(signr);
				continue;
			}
			sa = current->sig->action + signr - 1;
		}
		
		if (sa->sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			/* SIGCHLD is special */
			while (sys_waitpid(-1,0,WNOHANG) > 0)
				;
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
			if (current->pid == 1)
				continue;
			switch(signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				      continue;
			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (current->flags & PF_PTRACED)
					continue;
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa_flags & 
						SA_NOCLDSTOP))
					notify_parent(current, signr);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGIOT: case SIGFPE: case SIGSEGV: case SIGBUS:
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
		
		/*
		 * OK, we're invoking a handler
		 */
#ifdef DEBUG
		printk("user-specified handler for signal: %d\n", signr);
#endif
		handler_signal |= 1 << (signr-1);
		mask &= ~sa->sa_mask;
	}
	
	if (! handler_signal)
		return 0;
	
	/*
	 * We chain user frames for the various handlers together. Since we use
	 * local regs to pass parameters, all RIPs go to the asm routine
	 * signal_head found in entry.S; if we were an MMU-full system, we would
	 * have to copy signal_head out to the user process's stack.
	 */
	{
		extern void signal_head(void);
		struct frame {
			unsigned long pfp;
			unsigned long sp;
			unsigned long rip;
			unsigned long* args[13];
		} *userframe, *newframe;
		unsigned long type;

#ifdef DEBUG
		stack_trace();
#endif
		asm("flushreg");
		type = regs->lregs[PT_PFP] & 0x7;
#ifdef DEBUG
		printk("pfp, type: 0x%8x, 0x%8x\n", regs->lregs[PT_PFP], type);
#endif
		userframe = (struct frame*)
			(((unsigned long) regs->lregs[PT_PFP]) & ~0x7);
#ifdef DEBUG
		printk("userframe: 0x%8x\n", userframe);
		stack_trace_from((unsigned long) userframe);
#endif

		for (sa=current->sig->action, signr=1, mask=1; 
		     mask; 
		     sa++, signr++, mask<<=1) {
			if (mask > handler_signal)
				break;
			if ((!(mask & handler_signal)) || (!sa->sa_handler))
				continue;
#ifdef DEBUG
			printk("userframe->rip: 0x%8x\n", userframe->rip);
#endif
			newframe = stack_align(userframe->sp);
			newframe->rip = (unsigned long) signal_head;
			newframe->args[0] = (unsigned long) sa->sa_handler;
			newframe->args[1] = signr;
#ifdef DEBUG
			printk("r3,r4: 0x%8x, %d\n", sa->sa_handler, signr);
#endif
			newframe->pfp = (unsigned long) userframe;
			newframe->sp = (unsigned long)(newframe) + 64;
			userframe = newframe;
		}
		
#ifdef DEBUG
		printk("done; look:\n");
		stack_trace();
#endif
		regs->lregs[PT_PFP] = (unsigned long)userframe | type;
		asm("flushreg");
		
		/* 
		 * appropriate syscalls should return EINTR
		 */
		if ((type & 6) == 2 &&
		    (regs->gregs[0] == -ERESTARTNOHAND ||
		     regs->gregs[0] == -ERESTARTSYS ||
		     regs->gregs[0] == -ERESTARTNOINTR))
			regs->gregs[0] = -EINTR;
	}

	return 1;
}
