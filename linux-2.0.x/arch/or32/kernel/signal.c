/*
 *  linux/arch/ppc/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Adapted for PowerPC by Gary Thomas
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
asmlinkage int sys_sigsuspend(unsigned long set, int p2, int p3, int p4, int p6, int p7, struct pt_regs *regs)
{
	unsigned long mask;
 
        mask = current->blocked;
        current->blocked = set & _BLOCKABLE;
        regs->gprs[1] = -EINTR;
#if 0
printk("Task: %x[%d] - SIGSUSPEND at %x, Mask: %x\n", current, current->pid, regs->nip,
set);
#endif
        while (1) {
                current->state = TASK_INTERRUPTIBLE;
                schedule();
                if (do_signal(mask,regs))
                        return -EINTR;
        }
}

/*
 * This sets regs->esp even though we don't actually use sigstacks yet..
 */
asmlinkage int sys_sigreturn(struct pt_regs *regs)
{
	struct sigcontext_struct *sc;
	struct pt_regs *int_regs;
	int signo;
	sc = (struct sigcontext_struct *)regs->sp;
	current->blocked = sc->oldmask & _BLOCKABLE;
	int_regs = sc->regs;
	signo = sc->signal;
	sc++;  /* Pop signal 'context' */
	if (sc == (struct sigcontext_struct *)(int_regs))
	{ /* Last stacked signal */
		int i;
#if 1
		memcpy(regs, int_regs, sizeof(*regs));
#else
		/* Don't mess up 'my' stack frame */
		for(i = 0; i < 30; i++)
			if(i != 7)
				regs->gprs[i] = int_regs->gprs[i];
		regs->sp = int_regs->sp;
#endif		
		if ((int)regs->orig_gpr3 >= 0 &&
		    ((int)regs->result == -ERESTARTNOHAND ||
		     (int)regs->result == -ERESTARTSYS ||
		     (int)regs->result == -ERESTARTNOINTR))
		{
			regs->gprs[1] = regs->orig_gpr3;
			regs->pc -= 8; /* Back up & retry system call */
			regs->result = 0;
		}
		return (regs->result);
	} else
	{ /* More signals to go */
		regs->sp = (unsigned long)sc;
		regs->gprs[1] = sc->signal; 	/* r3 */
		regs->gprs[2] = (unsigned long)sc->regs;	/* r4 */
		regs->gprs[7] = (unsigned long)((sc->regs)+1); 	/* r9 - link register */
		regs->pc = sc->handler;
		return (sc->signal);
	}
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals that
 * the kernel can handle, and then we build all the user-level signal handling
 * stack-frames in one go after that.
 */
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs * regs)
{
	unsigned long mask = ~current->blocked;
	unsigned long handler_signal = 0;
	unsigned long *frame = NULL;
	unsigned long *trampoline;
	unsigned long *regs_ptr;
	unsigned long nip = 0;
	unsigned long signr;
	int bitno;
	struct sigcontext_struct *sc;
	struct sigaction * sa;
	unsigned long flags;

	save_flags(flags);
        cli(); 

	while ((signr = current->signal & mask)) {
#if 0
		signr = ffz(~signr);  /* Compute bit # */
#else
		for (bitno = 0;  bitno < 32;  bitno++)
		{
			if (signr & (1<<bitno)) break;
		}
		signr = bitno;
#endif
		current->signal &= ~(1<<signr);  /* Clear bit */
		sa = current->sig->action + signr;
		signr++;
		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;
			notify_parent(current, SIGCHLD);
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
			/* check for SIGCHLD: it's special */
			while (sys_waitpid(-1,NULL,WNOHANG) > 0)
				/* nothing */;
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
			if (current->pid == 1)
				continue;
			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
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
				do_exit(signr);
			}
		}
		/*
		 * OK, we're invoking a handler
		 */
		if ((int)regs->orig_gpr3 >= 0) {
			if ((int)regs->result == -ERESTARTNOHAND ||
			   ((int)regs->result == -ERESTARTSYS && !(sa->sa_flags & SA_RESTART)))
				(int)regs->result = -EINTR;
		}
		handler_signal |= 1 << (signr-1);
		mask &= ~sa->sa_mask;
	}
	if (!handler_signal)		/* no handler will be called - return 0 */
	{
		restore_flags(flags);
		return 0;
	}
	nip = regs->pc;
	frame = (unsigned long *) regs->sp;
	/* Build trampoline code on stack */
	frame -= 3;
	trampoline = frame;
	trampoline[0] = 0x9d607777;  /* l.addi r11,r0,0x7777 */
	trampoline[1] = 0x20000001;  /* l.sys  1 */
	trampoline[2] = 0x15000000;  /* l.nop  0 */
	frame -= sizeof(*regs) / sizeof(long);
	regs_ptr = frame;
	memcpy(regs_ptr, regs, sizeof(*regs));
	signr = 1;
	sa = current->sig->action;
	for (mask = 1 ; mask ; sa++,signr++,mask += mask) {
		if (mask > handler_signal)
			break;
		if (!(mask & handler_signal))
			continue;
		frame -= sizeof(struct sigcontext_struct) / sizeof(long);
		sc = (struct sigcontext_struct *)frame;
		nip = (unsigned long) sa->sa_handler;
#if 0 /* Old compiler */		
		nip = *(unsigned long *)nip;
#endif		
		if (sa->sa_flags & SA_ONESHOT)
			sa->sa_handler = NULL;
		sc->handler = nip;
		sc->oldmask = current->blocked;
		sc->regs = (struct pt_regs *)regs_ptr;
		sc->signal = signr;
		current->blocked |= sa->sa_mask;
		regs->gprs[1] = signr; /* r3 = signr */
		regs->gprs[2] = (unsigned long)regs_ptr; /* r4 = regs_ptr */
	}
	regs->gprs[7] = (unsigned long)trampoline; /* r9 = trampoline (r9 is link register) */
	regs->pc = nip;
	regs->sp = (unsigned long)sc;
	/* The DATA cache must be flushed here to insure coherency */
	/* between the DATA & INSTRUCTION caches.  Since we just */
	/* created an instruction stream using the DATA [cache] space */
	/* and since the instruction cache will not look in the DATA */
	/* cache for new data, we have to force the data to go on to */
	/* memory and flush the instruction cache to force it to look */
	/* there.  The following function performs this magic */
#if ICACHE
	ic_invalidate();
#endif
	restore_flags(flags);
	return 1;
}
