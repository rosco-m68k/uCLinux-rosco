/*  $Id: signal.c,v 1.1 2002/08/01 23:25:18 mdurrant Exp $
 *  linux/arch/sparc/kernel/signal.c
 *
 *  Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <linux/mm.h>

#include <asm/segment.h>
#include <asm/bitops.h>
#include <asm/ptrace.h>

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

asmlinkage int sys_waitpid(pid_t pid, unsigned long *stat_addr, int options);
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs * regs);

/*
 * atomically swap in the new signal mask, and wait for a signal.
 * This is really tricky on the Sparc, watch out...
 */

asmlinkage void do_sigsuspend (struct pt_regs *regs)
{
	unsigned long mask;
	unsigned long set;

	set = regs->u_regs [UREG_I0];
	mask = current->blocked;
	current->blocked = set & _BLOCKABLE;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(mask,regs)){
			regs->psr |= PSR_C;
			regs->u_regs [UREG_I0] = -EINTR;
			return;
		}
	}
}

/*
 * Set up a signal frame... Make the stack look the way SunOS
 * expects it to look which is basically:
 *
 * ---------------------------------- <-- %sp at signal time
 * Return code instructions
 * Struct sigcontext
 * Signal address
 * Ptr to sigcontext area above
 * Signal code
 * The signal number itself
 * One register window
 * ---------------------------------- <-- New %sp
 */
struct signal_sframe {
	struct reg_window sig_window;
	int sig_num;
	int sig_code;
	struct sigcontext_struct *sig_scptr;
	int sig_address;
	struct sigcontext_struct sig_context;
	int retcode[2];
};
/* To align the structure properly. */
#define SF_ALIGNEDSZ  (((sizeof(struct signal_sframe) + 3) & (~3)))

asmlinkage void do_sigreturn(struct pt_regs *regs)
{
	struct signal_sframe *fp = (struct signal_sframe *) regs->u_regs[UREG_FP];
	struct sigcontext_struct *scptr = &fp->sig_context;

	/* Check sanity of the user arg. */
	if(verify_area(VERIFY_READ, scptr, sizeof(struct sigcontext_struct)) ||
	   ((((unsigned long) scptr)) & 0x3)) {
		printk("%s [%d]: do_sigreturn, scptr is invalid at pc<%08lx> scptr<%p>\n",
		       current->comm, current->pid, regs->pc, scptr);
		do_exit(SIGSEGV);
	}

	current->blocked = scptr->sigc_mask & _BLOCKABLE;
	current->tss.sstk_info.cur_status = (scptr->sigc_onstack & 1);
	regs->pc = scptr->sigc_pc;
	/* wentao: restore the globals and reg window */
	memcpy(regs->u_regs, scptr->sigc_g, 32); 
	memcpy((void*)(scptr->sigc_sp), (void*)(regs->u_regs[UREG_FP]), 
	       sizeof(struct reg_window));

	regs->u_regs[UREG_FP] = scptr->sigc_sp;
	regs->u_regs[UREG_I0] = scptr->sigc_o0;
	regs->u_regs[UREG_I7] = scptr->sigc_o7;
	//regs->u_regs[UREG_G1] = scptr->sigc_g1;

	/* User can only change condition codes in %psr. */
	regs->psr &= (~PSR_ICC);
	regs->psr |= (scptr->sigc_psr & PSR_ICC);
	regs->psr &= (~PSR_C);		/* disable syscall checks */
}

static inline void
setup_frame(struct sigaction *sa, struct sigcontext_struct **fp,
	    unsigned long pc, struct pt_regs *regs,
	    int signr, unsigned long oldmask)
{
	struct signal_sframe *sframep;
	struct sigcontext_struct *sc;
	int old_status = current->tss.sstk_info.cur_status;

	sframep = (struct signal_sframe *) *fp;
	sframep = (struct signal_sframe *) (((unsigned long) sframep)-SF_ALIGNEDSZ);
	sc = &sframep->sig_context;
	if(verify_area(VERIFY_WRITE, sframep, sizeof(*sframep)) ||
	   (((unsigned long) sframep) & 3)) {
#if 0 /* fills up the console logs... */
		printk("%s [%d]: User has trashed signal stack\n",
		       current->comm, current->pid);
		printk("Sigstack ptr %p handler at pc<%08lx> for sig<%d>\n",
		       sframep, pc, signr);
#endif
		/* Don't change signal code and address, so that
		 * post mortem debuggers can have a look.
		 */
		current->sig->action[SIGILL-1].sa_handler = SIG_DFL;
		current->blocked &= ~(1<<(SIGILL-1));
		send_sig(SIGILL,current,1);
		return;
	}
	*fp = (struct sigcontext_struct *) sframep;

	sc->sigc_onstack = old_status;
	sc->sigc_mask = oldmask;
	sc->sigc_sp = regs->u_regs[UREG_FP];
	sc->sigc_pc = pc;
	sc->sigc_psr = regs->psr;
	//sc->sigc_g1 = regs->u_regs[UREG_G1];
	sc->sigc_o0 = regs->u_regs[UREG_I0];
	sc->sigc_o7 = regs->u_regs[UREG_I7];
	
	/* wentao: backup the globals */ 
	memcpy(sc->sigc_g, regs->u_regs, 32);

	memcpy(sframep, (char *)regs->u_regs[UREG_FP],
	       sizeof(struct reg_window));

	sframep->retcode[0] = 0x36e19803;	/* pfx 119, movi %g1,119 */
	sframep->retcode[1] = 0x793f;		/* trap 63 ; syscall */
	regs->u_regs[UREG_I7] = ((unsigned long) &sframep->retcode) >> 1;	/* Set new return addr */
	
	sframep->sig_num = signr;
	if(signr == SIGSEGV ||
	   signr == SIGILL ||
	   signr == SIGFPE ||
	   signr == SIGBUS) {
		sframep->sig_code = current->tss.sig_desc;
		sframep->sig_address = current->tss.sig_address;
	} else {
		sframep->sig_code = 0;
		sframep->sig_address = 0;
	}
	sframep->sig_scptr = sc;
	regs->u_regs[UREG_FP] = (unsigned long) *fp;
	regs->u_regs[UREG_I0] = signr;
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
	struct sigcontext_struct *frame = NULL;
	unsigned long pc = 0;
	unsigned long signr;
	struct sigaction *sa;

	while ((signr = current->signal & mask) != 0) {
		signr = ffz(~signr);
		clear_bit(signr, &current->signal);
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
		if(sa->sa_handler == SIG_IGN) {
			if(signr != SIGCHLD)
				continue;
			while(sys_waitpid(-1,NULL,WNOHANG) > 0);
			continue;
		}
		if(sa->sa_handler == SIG_DFL) {
			if(current->pid == 1)
				continue;
			switch(signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if(!(current->p_pptr->sig->action[SIGCHLD-1].sa_flags &
				     SA_NOCLDSTOP))
					notify_parent(current,  SIGCHLD);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGABRT: case SIGFPE: case SIGSEGV:
				if(current->binfmt && current->binfmt->core_dump) {
					if(current->binfmt->core_dump(signr, regs))
						signr |= 0x80;
				}
				/* fall through */
			default:
				current->signal |= _S(signr & 0x7f);
				current->flags |= PF_SIGNALED;
				do_exit(signr);
			}
		}
		/* OK, we're invoking a handler. */
		if(regs->psr & PSR_C) {
			if(regs->u_regs[UREG_I0] == -ERESTARTNOHAND ||
			   (regs->u_regs[UREG_I0] == -ERESTARTSYS && !(sa->sa_flags & SA_RESTART)))
				regs->u_regs[UREG_I0] = -EINTR;
		}
		handler_signal |= 1 << (signr - 1);
		mask &= ~sa->sa_mask;
	}
	if((regs->psr & PSR_C) &&
	   (regs->u_regs[UREG_I0] == -ERESTARTNOHAND ||
	    regs->u_regs[UREG_I0] == -ERESTARTSYS ||
	    regs->u_regs[UREG_I0] == -ERESTARTNOINTR)) {
		/* replay the system call when we are done */
		regs->u_regs[UREG_I0] = regs->spare;
		regs->pc -= 1;
	}
	if(!handler_signal)
		return 0;
	pc = regs->pc;
	frame = (struct sigcontext_struct *) regs->u_regs[UREG_FP];
	signr = 1;
	sa = current->sig->action;
	for(mask = 1; mask; sa++, signr++, mask += mask) {
		if(mask > handler_signal)
			break;
		if(!(mask & handler_signal))
			continue;
		setup_frame(sa, &frame, pc, regs, signr, oldmask);
		pc = (unsigned long) sa->sa_handler;
		if(sa->sa_flags & SA_ONESHOT)
			sa->sa_handler = NULL;
		current->blocked |= sa->sa_mask;
		oldmask |= sa->sa_mask;
	}
	regs->pc = pc;
	return 1;
}
