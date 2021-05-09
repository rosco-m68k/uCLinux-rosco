/*
 *  linux/arch/h8300/kernel/process.c
 *
 * Yoshinori Sato <qzb04471@nifty.ne.jp>
 *
 *  Based on:
 *
 *  linux/arch/m68knommu/kernel/process.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
 *
 *  linux/arch/m68k/kernel/process.c
 *
 *  Copyright (C) 1995  Hamish Macdonald
 *
 *  68060 fixes by Jesper Skov
 */

/*
 * This file handles the architecture-dependent parts of process handling..
 */

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/segment.h>
#include <asm/system.h>
#include <asm/traps.h>
#include <asm/setup.h>

asmlinkage void ret_from_exception(void);

/*
 * The idle loop on an H8/300H..
 */
asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;


	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
		/* asm("sleep\n\t"); */
		schedule();
	}
}

void hard_reset_now(void)
{
	HARD_RESET_NOW(); 
}

void show_regs(struct pt_regs * regs)
{
	printk("\n");
	printk("PC: %08lx  Status: %02x\n",
	       regs->pc, regs->ccr);
	printk("ORIG_ER0: %08lx ER0: %08lx ER1: %08lx\n",
	       regs->orig_er0, regs->er0, regs->er1);
	printk("ER2: %08lx ER3: %08lx  ER4: %08lx\n",
	       regs->er2, regs->er3, regs->er4);
	printk("ER5: %08lx ",
	       regs->er5);
	if (!(regs->ccr & 0x10))
		printk("USP: %08lx\n", rdusp());
}

/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
}

void flush_thread(void)
{
	int i;

	for (i = 0; i < 8; i++)
		current->debugreg[i] = 0;
}

/*
 * "m68k_fork()".. By the time we get here, the
 * non-volatile registers have also been saved on the
 * stack. We do some ugly pointer stuff here.. (see
 * also copy_thread)
 */

asmlinkage int h8300_fork(struct pt_regs *regs)
{
	return do_fork(SIGCHLD|CLONE_WAIT, rdusp(), regs);
}

asmlinkage int h8300_clone(struct pt_regs *regs)
{
	unsigned long clone_flags;
	unsigned long newsp;

	/* syscall2 puts clone_flags in d1 and usp in d2 */
	clone_flags = regs->er1;
	newsp = regs->er2;
	if (!newsp)
	  newsp  = rdusp();
	return do_fork(clone_flags, newsp, regs);
}

void release_thread(struct task_struct *dead_task)
{
}

void copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
		 struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs * childregs;
	struct switch_stack * childstack, *stack;
	unsigned long stack_offset;
	char *retp;

	stack_offset = PAGE_SIZE - sizeof(struct pt_regs);
	childregs = (struct pt_regs *) (p->kernel_stack_page + stack_offset);

	*childregs = *regs;
	childregs->er0 = 0;

	retp = (unsigned char *) regs;
	stack = (struct switch_stack *) (retp-sizeof(struct switch_stack)-8);

	childstack = ((struct switch_stack *) childregs) - 1;
	*childstack = *stack;
	childstack->retpc = (unsigned long) ret_from_exception;
	p->tss.usp = usp;
	p->tss.ksp = (unsigned long)childstack;
}

/* Fill in the fpu structure for a core dump.  */
#if 0
int dump_fpu (struct pt_regs *regs, struct user_m68kfp_struct *fpu)
{
  return 0;
}
#endif
/*
 * fill in the user structure for a core dump..
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
/* changed the size calculations - should hopefully work better. lbt */
	dump->magic = CMAGIC;
	dump->start_code = 0;
	dump->start_stack = rdusp() & ~(PAGE_SIZE - 1);
	dump->u_tsize = ((unsigned long) current->mm->end_code) >> PAGE_SHIFT;
	dump->u_dsize = ((unsigned long) (current->mm->brk +
					  (PAGE_SIZE-1))) >> PAGE_SHIFT;
	dump->u_dsize -= dump->u_tsize;
	dump->u_ssize = 0;


	/* FIXME */
	/*if (dump->start_stack < TASK_SIZE)
		dump->u_ssize = ((unsigned long) (TASK_SIZE - dump->start_stack)) >> PAGE_SHIFT;*/

	dump->u_ar0 = (struct pt_regs *)(((int)(&dump->regs)) -((int)(dump)));
	dump->regs = *regs;
	dump->regs2 = ((struct switch_stack *)regs)[-1];
}

/*
 * sys_execve() executes a new program.
 */
asmlinkage int sys_execve(char *name, char **argv, char **envp,int dummy,...)
{
	int error;
	char * filename,* sp;
	struct pt_regs *regs;
        sp = (char *) &dummy;
	sp+=8;
	regs=(struct pt_regs *)sp;
	error = getname(name, &filename);
	if (error)
		return error;
	error = do_execve(filename, argv, envp, regs);
	putname(filename);
	return error;
}
