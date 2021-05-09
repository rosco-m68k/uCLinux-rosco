/*
 *  linux/arch/arm/kernel/signal.c
 *
 *  Copyright (C) 1995, 1996 Russell King
 */

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

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

#define SWI_SYS_SIGRETURN 0xef900077

asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options);
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs * regs);
extern int ptrace_cancel_bpt (struct task_struct *);
extern int ptrace_set_bpt (struct task_struct *);

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int sys_sigsuspend(int restart, unsigned long oldmask, unsigned long set)
{
    unsigned long mask;
    struct pt_regs * regs;
    __asm__("mov\t%0,r9\n\t": "=r" (regs));

    mask = current->blocked;
    current->blocked = set & _BLOCKABLE;
    regs->ARM_r0 = -EINTR;

    while (1) {
	current->state = TASK_INTERRUPTIBLE;
	schedule();
	if (do_signal(mask,regs))
	    return regs->ARM_r0;
    }
}

/*
 * This sets regs->esp even though we don't actually use sigstacks yet..
 */
asmlinkage int sys_sigreturn(unsigned long __unused)
{
    struct pt_regs *regs;
    struct sigcontext_struct context;
    void *frame;
  	
    __asm__ ("mov\t%0, r9\n\t": "=r" (regs));

    frame = (void *)regs->ARM_sp;

    if (verify_area(VERIFY_READ,frame,sizeof(context))) {
	printk("*** Bad stack frame\n");
	goto badframe;
    }

    memcpy_fromfs(&context,frame,sizeof(context));

    if (context.magic!=0x4B534154) {
	printk("Bad stack identifier\n");
	goto badframe;
    }

    *regs = context.reg;

    current->blocked = context.oldmask & _BLOCKABLE;

    if (!valid_user_regs(regs))
	goto badframe;

    /* send SIGTRAP if we're single-stepping */
    if (ptrace_cancel_bpt (current))
    	send_sig (SIGTRAP, current, 1);

    return regs->ARM_r0;
badframe:
    do_exit(SIGSEGV);
    return 0;
}

/*
 * Set up a signal frame...
 */
static void setup_frame(struct sigaction * sa, unsigned long ** fp, unsigned long eip,
	struct pt_regs * regs, int signr, unsigned long oldmask)
{
    unsigned long * frame;
    struct sigcontext_struct context;
	
    frame = (unsigned long *)(((int)(*fp)) - sizeof(context) - 4);

    if (verify_area (VERIFY_WRITE, frame, sizeof(context)))
	do_exit (SIGSEGV);

    context.magic	= 0x4B534154; /* Signature */
    context.reg		= *regs;
    context.reg.ARM_sp	= (unsigned long)*fp;
    context.reg.ARM_pc	= eip;

    context.trap_no	= current->tss.trap_no;
    context.error_code	= current->tss.error_code;
    context.oldmask	= oldmask;

    memcpy_tofs (frame, &context, sizeof(context));
    put_user (SWI_SYS_SIGRETURN, frame + sizeof (context)/4);
    /*
     * Ensure that the SWI instruction is flushed out of D-cache
     */
    __flush_entry_to_ram(frame + sizeof (context)/4);

    *fp = frame;
    if (current->exec_domain && current->exec_domain->signal_invmap)
	regs->ARM_r0 = current->exec_domain->signal_invmap[signr];
    else
	regs->ARM_r0 = signr;
    regs->ARM_lr = ((unsigned long)frame) + sizeof (context);
    if (!valid_user_regs(&context.reg))
	goto badframe;

    return;

badframe:
    do_exit(SIGSEGV);
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
	unsigned long orig_signal;
	unsigned long mask = ~current->blocked;
	unsigned long handler_signal = 0;
	unsigned long *frame = NULL;
	unsigned long eip = 0;
	unsigned long signr;
	struct sigaction * sa;
	unsigned long *pcinstr = (unsigned long *)(instruction_pointer(regs)-4);
	int swiinstr = 0, single_stepping;

	single_stepping = ptrace_cancel_bpt (current);

	if (verify_area(VERIFY_READ, pcinstr, 4) == 0 &&
	    (get_user(pcinstr) & 0x0f000000) == 0x0f000000)
		swiinstr = 1;

	while ((signr = current->signal & mask)) {
		orig_signal = current->signal;
		eip = signr;
		for (signr = 0; signr < 32; signr++)

		if (eip & (1 << signr)) {
			current->signal &= ~(1 << signr);
			break;
		}
		sa = current->sig->action + signr;
		signr++;

		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;
			notify_parent(current, SIGCHLD);
			schedule();
			single_stepping |= ptrace_cancel_bpt (current);

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

			case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (is_orphaned_pgrp (current->pgrp))
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
				single_stepping |= ptrace_cancel_bpt (current);
				continue;

			case SIGBUS:  case SIGQUIT: case SIGILL:
			case SIGTRAP: case SIGIOT:  case SIGFPE:
			case SIGSEGV:
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
		if (swiinstr && (regs->ARM_r0 == -ERESTARTNOHAND ||
			    (regs->ARM_r0 == -ERESTARTSYS && !(sa->sa_flags & SA_RESTART))))
			regs->ARM_r0 = -EINTR;
		handler_signal |= 1 << (signr-1);
		mask &= ~sa->sa_mask;
	}

	if (swiinstr &&
	    (regs->ARM_r0 == -ERESTARTNOHAND ||
	     regs->ARM_r0 == -ERESTARTSYS ||
	     regs->ARM_r0 == -ERESTARTNOINTR)) {
		regs->ARM_r0 = regs->ARM_ORIG_r0;
		regs->ARM_pc -= 4;
	}

	if (!handler_signal) {		/* no handler will be called - return 0 */
		if (single_stepping)
			ptrace_set_bpt (current);
		return 0;
	}

	eip = regs->ARM_pc;
	frame = (unsigned long *) regs->ARM_sp;
	signr = 1;
	sa = current->sig->action;
	for (mask = 1 ; mask ; sa++,signr++,mask += mask) {
		if (mask > handler_signal)
			break;
		if (!(mask & handler_signal))
			continue;
		setup_frame(sa,&frame,eip,regs,signr,oldmask);
		eip = (unsigned long) sa->sa_handler;
		if (sa->sa_flags & SA_ONESHOT)
			sa->sa_handler = NULL;
/* force a supervisor-mode page-in of the signal handler to reduce races */
			current->blocked |= sa->sa_mask;
			oldmask |= sa->sa_mask;
	}
	regs->ARM_sp = (unsigned long) frame;
	regs->ARM_pc = eip;		/* "return" to the first handler */
	current->tss.trap_no = current->tss.error_code = 0;
	if (single_stepping)
	    	ptrace_set_bpt (current);
	return 1;
}
