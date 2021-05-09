/*
 *  Mar 2, 2001 - Ken Hill
 *     Implement Nios architecture specifics
 *
 *  $Id: process.c,v 1.1 2002/08/01 23:25:18 mdurrant Exp $
 *  linux/arch/sparc/kernel/process.c
 *
 *  Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

/*
 * This file handles the architecture-dependent parts of process handling..
 */

#define __KERNEL_SYSCALLS__
#include <stdarg.h>

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/ldt.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/segment.h>
#include <asm/system.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/delay.h>
#include <asm/processor.h>
#include <asm/psr.h>

int active_ds = USER_DS;

/*
 * the idle loop on a Nios... ;)
 */
asmlinkage int sys_idle(void)
{
#ifdef	DEBUG
	printk("Arrived at sys_idle\n");
#endif
	if (current->pid != 0)
		return -EPERM;

	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
		schedule();
	}
	return 0;
}

extern char saved_command_line[];

void hard_reset_now(void)
{
#if 0
	panic("Reboot failed!");
#else
	asm("trap 0");				/* this traps back to monitor */
#endif
}

void show_regwindow(struct reg_window *rw)
{
	printk("l0:%08lx l1:%08lx l2:%08lx l3:%08lx l4:%08lx l5:%08lx l6:%08lx l7:%08lx\n",
	       rw->locals[0], rw->locals[1], rw->locals[2], rw->locals[3],
	       rw->locals[4], rw->locals[5], rw->locals[6], rw->locals[7]);
	printk("i0:%08lx i1:%08lx i2:%08lx i3:%08lx i4:%08lx i5:%08lx i6:%08lx i7:%08lx\n",
	       rw->ins[0], rw->ins[1], rw->ins[2], rw->ins[3],
	       rw->ins[4], rw->ins[5], rw->ins[6], rw->ins[7]);
}

void show_regs(struct pt_regs * regs)
{
        printk("PSR: %08lx PC: %08lx\n", regs->psr, regs->pc);
	printk("%%g0: %08lx %%g1: %08lx %%g2: %08lx %%g3: %08lx\n",
	       regs->u_regs[0], regs->u_regs[1], regs->u_regs[2],
	       regs->u_regs[3]);
	printk("%%g4: %08lx %%g5: %08lx %%g6: %08lx %%g7: %08lx\n",
	       regs->u_regs[4], regs->u_regs[5], regs->u_regs[6],
	       regs->u_regs[7]);
	printk("%%o0: %08lx %%o1: %08lx %%o2: %08lx %%o3: %08lx\n",
	       regs->u_regs[8], regs->u_regs[9], regs->u_regs[10],
	       regs->u_regs[11]);
	printk("%%o4: %08lx %%o5: %08lx %%sp: %08lx %%ret_pc: %08lx\n",
	       regs->u_regs[12], regs->u_regs[13], regs->u_regs[14],
	       regs->u_regs[15]);
}

/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
	flush_user_windows();

	if(last_task_used_math == current) {
		/* Keep process from leaving FPU in a bogon state. */
		last_task_used_math = NULL;
		current->tss.flags &= ~NIOS_FLAG_COPROC;
	}
}

/*
 * Free old dead task when we know it can never be on the cpu again.
 */
void release_thread(struct task_struct *dead_task)
{
}

void flush_thread(void)
{
	/* Make sure old user windows don't get in the way. */
	flush_user_windows();
	current->tss.sig_address = 0;
	current->tss.sig_desc = 0;
	current->tss.sstk_info.cur_status = 0;
	current->tss.sstk_info.the_stack = 0;

	if(last_task_used_math == current) {
		last_task_used_math = NULL;
		current->tss.flags &= ~NIOS_FLAG_COPROC;
	}

	/* Now, this task is no longer a kernel thread. */
	current->tss.flags &= ~NIOS_FLAG_KTHREAD;
}

/*
 * Copy a Nios thread.  The fork() return value conventions:
 * 
 * Parent -->  %o0 == childs  pid, %o1 == 0
 * Child  -->  %o0 == parents pid, %o1 == 1
 *
 * NOTE: We have a separate fork kpsr because
 *       the parent could change this value between
 *       sys_fork invocation and when we reach here
 *       if the parent should sleep while trying to
 *       allocate the task_struct and kernel stack in
 *       do_fork().
 */
extern void ret_sys_call(void);

void copy_thread(int nr, unsigned long clone_flags, unsigned long sp,
		 struct task_struct *p, struct pt_regs *regs)
{
	struct pt_regs *childregs;
	struct reg_window *old_stack, *new_stack;
	unsigned long stack_offset;

	if(last_task_used_math == current) {
		current->tss.flags |= NIOS_FLAG_COPROC;
	}

	/* Calculate offset to stack_frame & pt_regs */
	stack_offset = (PAGE_SIZE - TRACEREG_SZ);

	if(regs->psr & PSR_SUPERVISOR)
		stack_offset -= REGWIN_SZ;
	childregs = ((struct pt_regs *) (p->kernel_stack_page + stack_offset));
	*childregs = *regs;
	new_stack = (((struct reg_window *) childregs) - 1);
	old_stack = (((struct reg_window *) regs) - 1);
	*new_stack = *old_stack;
	p->tss.ksp = p->saved_kernel_stack = (unsigned long) new_stack;
	p->tss.kpc = (((unsigned long) ret_sys_call));
#if 1
	/* Start the new process with a fresh register window */
	p->tss.kpsr = (current->tss.fork_kpsr & ~PSR_CWP) | ((get_hi_limit() - 1) << 4);
#else
	p->tss.kpsr = current->tss.fork_kpsr;
#endif

	/* If we decide to split the register file up for multiple
	   task usage we will need this. Single register file usage
	   for now so do not use.

	p->tss.kwvalid = current->tss.fork_kwvalid;
	*/

	p->tss.kregs = childregs;
	childregs->u_regs[UREG_FP] = sp;
#if 1
	/* Start the new process with a fresh register window */
	childregs->psr = (childregs->psr & ~PSR_CWP) | (get_hi_limit() << 4);
#endif

	if(regs->psr & PSR_SUPERVISOR) {
		stack_offset += TRACEREG_SZ;
		childregs->u_regs[UREG_FP] = p->kernel_stack_page + stack_offset;
		new_stack->ins[6]=childregs->u_regs[UREG_FP];
		memcpy(childregs->u_regs[UREG_FP], regs->u_regs[UREG_FP], REGWIN_SZ);
		p->tss.flags |= NIOS_FLAG_KTHREAD;
	} else
		p->tss.flags &= ~NIOS_FLAG_KTHREAD;

	/* Set the return value for the child. */
//vic	childregs->u_regs[UREG_I0] = current->pid;
	childregs->u_regs[UREG_I0] = 0;
	childregs->u_regs[UREG_I1] = 1;

	/* Set the return value for the parent. */
	regs->u_regs[UREG_I1] = 0;
}

/*
 * fill in the user structure for a core dump..
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
//vic	unsigned long first_stack_page;
//vic
//vic	dump->magic = SUNOS_CORE_MAGIC;
//vic	dump->len = sizeof(struct user);
//vic	dump->regs.psr = regs->psr;
//vic	dump->regs.pc = regs->pc;
//vic	dump->regs.npc = regs->npc;
//vic	dump->regs.y = regs->y;
//vic	/* fuck me plenty */
//vic	memcpy(&dump->regs.regs[0], &regs->u_regs[1], (sizeof(unsigned long) * 15));
//vic	dump->uexec = current->tss.core_exec;
//vic	dump->u_tsize = (((unsigned long) current->mm->end_code) -
//vic		((unsigned long) current->mm->start_code)) & ~(PAGE_SIZE - 1);
//vic	dump->u_dsize = ((unsigned long) (current->mm->brk + (PAGE_SIZE-1)));
//vic	dump->u_dsize -= dump->u_tsize;
//vic	dump->u_dsize &= ~(PAGE_SIZE - 1);
//vic	first_stack_page = (regs->u_regs[UREG_FP] & ~(PAGE_SIZE - 1));
//vic	dump->u_ssize = (TASK_SIZE - first_stack_page) & ~(PAGE_SIZE - 1);
//vic	memcpy(&dump->fpu.fpstatus.fregs.regs[0], &current->tss.float_regs[0], (sizeof(unsigned long) * 32));
//vic	dump->fpu.fpstatus.fsr = current->tss.fsr;
//vic	dump->fpu.fpstatus.flags = dump->fpu.fpstatus.extra = 0;
//vic	dump->fpu.fpstatus.fpq_count = current->tss.fpqdepth;
//vic	memcpy(&dump->fpu.fpstatus.fpq[0], &current->tss.fpqueue[0],
//vic	       ((sizeof(unsigned long) * 2) * 16));
//vic	dump->sigcode = current->tss.sig_desc;
}

/*
 * fill in the fpu structure for a core dump.
 */
int dump_fpu (void *fpu_structure)
{
	/* Currently we report that we couldn't dump the fpu structure */
	return 0;
}

/*
 * nios_execve() executes a new program after the asm stub has set
 * things up for us.  This should basically do what I want it to.
 */
asmlinkage int nios_execve(struct pt_regs *regs)
{
	int error;
	char *filename;

	flush_user_windows();
	error = getname((char *) regs->u_regs[UREG_I0], &filename);
	if(error)
		return error;
	error = do_execve(filename, (char **) regs->u_regs[UREG_I1],
			  (char **) regs->u_regs[UREG_I2], regs);
	putname(filename);
	return error;
}
