/*
 * This file handles the architecture-dependent parts of process handling..
 * Based on m86k.
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
#include <asm/machdep.h>
#include <asm/board.h>

asmlinkage void ret_from_exception(void);

/*
 * The idle loop on an or32..
 */
asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;


	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
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
	printk("PC: %08lx  Status: %08lx\n",
	       regs->pc, regs->sr);
	printk("R0 : %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	0L,            regs->sp,      regs->gprs[0], regs->gprs[1], 
		regs->gprs[2], regs->gprs[3], regs->gprs[4], regs->gprs[5]);
	printk("R8 : %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	regs->gprs[6], regs->gprs[7], regs->gprs[8], regs->gprs[9], 
		regs->gprs[10], regs->gprs[11], regs->gprs[12], regs->gprs[13]);
	printk("R16: %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	regs->gprs[14], regs->gprs[15], regs->gprs[16], regs->gprs[17], 
		regs->gprs[18], regs->gprs[19], regs->gprs[20], regs->gprs[21]);
	printk("R24: %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx  %08lx\n",
	       	regs->gprs[22], regs->gprs[23], regs->gprs[24], regs->gprs[25], 
		regs->gprs[26], regs->gprs[27], regs->gprs[28], regs->gprs[29]);
}

/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
}

void flush_thread(void)
{
	set_fs(USER_DS);
	current->tss.fs = USER_DS;
}

asmlinkage int sys_fork(int p1, int p2, int p3, int p4, int p5, struct pt_regs *regs)
{
	return do_fork(SIGCHLD|CLONE_WAIT, regs->sp, regs);
}

asmlinkage int sys_clone(int p1, int p2, int p3, int p4, int p5, struct pt_regs *regs)
{
	unsigned long clone_flags = (unsigned long)p1;
	return do_fork(clone_flags, regs->sp, regs);
}

void release_thread(struct task_struct *dead_task)
{
}

void copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
		 struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs * childregs;

        /* Copy registers */
        childregs = ((struct pt_regs *) (p->kernel_stack_page + PAGE_SIZE)) - 1;
        *childregs = *regs;     /* STRUCT COPY */
        childregs->gprs[9] = 0x0;  /* Result from fork() */
        p->tss.ksp = childregs;

	/* If this is kernel thread, than we can use kernel stack also as 
	   "user" stack, if not, we use user stack from parent which should be a sleep
	   till we execute execve or exit (that is why sys_fork has CLONE_WAIT
	   flag allway set ) */
	if(regs->sr & SPR_SR_SM) {
		childregs->sp = (unsigned long)(childregs+1);
        	p->tss.usp = (unsigned long)(childregs+1);
	}
	else {
		childregs->sp = usp;
        	p->tss.usp = usp;
	}
}

/* Fill in the fpu structure for a core dump.  */

int dump_fpu (struct pt_regs *regs, struct user_or32fp_struct *fpu)
{
  return 0;
} 

void switch_to(struct task_struct *prev, struct task_struct *new) 
{
        struct pt_regs *regs;
        struct thread_struct *new_tss, *old_tss;
        long flags;

	save_flags(flags);
        cli();

        regs = (struct pt_regs *)new->tss.ksp;
        new_tss = &new->tss;
        old_tss = &current->tss;
        current_set[0] = new;   /* FIX ME! */
        _switch(old_tss, new_tss);
	restore_flags(flags);
} 

/*
 * fill in the user structure for a core dump..
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
/* changed the size calculations - should hopefully work better. lbt */
	dump->magic = CMAGIC;
	dump->start_code = 0;
	dump->start_stack = regs->sp & ~(PAGE_SIZE - 1);
	dump->u_tsize = ((unsigned long) current->mm->end_code) >> PAGE_SHIFT;
	dump->u_dsize = ((unsigned long) (current->mm->brk +
					  (PAGE_SIZE-1))) >> PAGE_SHIFT;
	dump->u_dsize -= dump->u_tsize;
	dump->u_ssize = 0;


	dump->u_ar0 = (struct pt_regs *)(((int)(&dump->regs)) -((int)(dump)));
	dump->regs = *regs;
	/* dump floating point stuff */
	dump->u_fpvalid = dump_fpu (regs, &dump->or32fp);
}

/*
 * sys_execve() executes a new program.
 */
asmlinkage int sys_execve(char *name, char **argv, char **envp,
		int dummy1, int dummy2, unsigned long sp)
{
	int error;
	char * filename;
	struct pt_regs *regs = (struct pt_regs *) sp;

_print("%s - %s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	error = getname(name, &filename);
_print("%s - %s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	if (error)
		return error;
_print("%s - %s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	error = do_execve(filename, argv, envp, regs);
_print("%s - %s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	putname(filename);
#if ICACHE
	ic_invalidate();
#endif
_print("%s - %s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return error;
}
