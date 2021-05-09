/*
 *  linux/arch/arm/kernel/process.c
 *
 *  Copyright (C) 1996 Russell King - Converted to ARM.
 *  Origional Copyright (C) 1995  Linus Torvalds
 */

/*
 * This file handles the architecture-dependent parts of process handling..
 */

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
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/io.h>

extern void fpe_save(struct fp_soft_struct *);
extern char *processor_modes[];

asmlinkage void ret_from_sys_call(void) __asm__("_ret_from_sys_call");

static int hlt_counter = 0;

void disable_hlt(void)
{
	hlt_counter++;
}

void enable_hlt(void)
{
	hlt_counter--;
}

/*
 * The idle loop on an arm..
 */
asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;

	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
		if (!hlt_counter && !need_resched)
			proc_idle ();
		schedule();
	}
}

void reboot_setup(char *str, int *ints)
{
}

/*
 * This routine reboots the machine by resetting the expansion cards via
 * their loaders, turning off the processor cache (if ARM3), copying the
 * first instruction of the ROM to 0, and executing it there.
 */
void hard_reset_now(void)
{
	proc_hard_reset ();
	arch_hard_reset ();
}

void show_regs(struct pt_regs * regs)
{
	unsigned long flags;

	flags = condition_codes(regs);

	printk("\n"
		"pc : [<%08lx>]\n"
		"lr : [<%08lx>]\n"
		"sp : %08lx  ip : %08lx  fp : %08lx\n",
		instruction_pointer(regs),
		regs->ARM_lr,
		regs->ARM_sp,
		regs->ARM_ip,
		regs->ARM_fp);
	printk( "r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10,
		regs->ARM_r9,
		regs->ARM_r8);
	printk( "r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7,
		regs->ARM_r6,
		regs->ARM_r5,
		regs->ARM_r4);
	printk( "r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3,
		regs->ARM_r2,
		regs->ARM_r1,
		regs->ARM_r0);
	printk("Flags: %c%c%c%c",
		flags & CC_N_BIT ? 'N' : 'n',
		flags & CC_Z_BIT ? 'Z' : 'z',
		flags & CC_C_BIT ? 'C' : 'c',
		flags & CC_V_BIT ? 'V' : 'v');
	printk("  IRQs %s  FIQs %s  Mode %s\n",
		interrupts_enabled(regs) ? "on" : "off",
		fast_interrupts_enabled(regs) ? "on" : "off",
		processor_modes[processor_mode(regs)]);
}

/*
 * Free current thread data structures etc..
 */
void exit_thread (void)
{
	if (last_task_used_math == current)
    		last_task_used_math = NULL;
}

void flush_thread(void)
{
	int i;

	for (i = 0; i < 8; i++)
		current->debugreg[i] = 0;
	if (last_task_used_math == current)
		last_task_used_math = NULL;
	current->tss.fs = USER_DS;
}

void release_thread(struct task_struct *dead_task)
{
}

void copy_thread(int nr, unsigned long clone_flags, unsigned long esp,
	struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs * childregs;
	struct context_save_struct * save;

	childregs = ((struct pt_regs *)(p->kernel_stack_page + 4096)) - 1;
	*childregs = *regs;
	childregs->ARM_r0 = 0;
	if (esp) childregs->ARM_sp = esp;

	save = ((struct context_save_struct *)(childregs)) - 1;
	copy_thread_css (save);
	p->tss.save = save;
	/*
	 * Save current math state in p->tss.fpe_save if not already there.
	 */
	if (last_task_used_math == current)
		fpe_save (&p->tss.fpstate.soft);
}

/*
 * fill in the fpe structure for a core dump...
 */
int dump_fpu (struct pt_regs *regs, struct user_fp *fp)
{
	int fpvalid = 0;

	if (current->used_math) {
		if (last_task_used_math == current)
			fpe_save (&current->tss.fpstate.soft);

		memcpy (fp, &current->tss.fpstate.soft, sizeof (fp));
	}

	return fpvalid;
}

/*
 * fill in the user structure for a core dump..
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
	int i;

	dump->magic = CMAGIC;
	dump->start_code = current->mm->start_code;
	dump->start_stack = regs->ARM_sp & ~(PAGE_SIZE - 1);

	dump->u_tsize = (current->mm->end_code - current->mm->start_code) >> PAGE_SHIFT;
	dump->u_dsize = (current->mm->brk - current->mm->start_data + PAGE_SIZE - 1) >> PAGE_SHIFT;
	dump->u_ssize = 0;

	for (i = 0; i < 8; i++)
		dump->u_debugreg[i] = current->debugreg[i];  

	if (dump->start_stack < 0x04000000)
		dump->u_ssize = (0x04000000 - dump->start_stack) >> PAGE_SHIFT;

	dump->regs = *regs;
	dump->u_fpvalid = dump_fpu (regs, &dump->u_fp);
}

asmlinkage int sys_fork(void)
{
	struct pt_regs *regs;
	__asm__("mov\t%0, r9\n\t": "=r" (regs) );

	return do_fork(SIGCHLD|CLONE_WAIT, regs->ARM_sp, regs);
}

asmlinkage int sys_clone(unsigned long clone_flags, unsigned long newsp)
{
	struct pt_regs *regs;
	__asm__("mov\t%0, r9\n\t":"=r" (regs) );
	if (!newsp)
		newsp = regs->ARM_sp;

	return do_fork(clone_flags, newsp, regs);
}

/*
 * sys_execve() executes a new program.
 */

asmlinkage int sys_execve(char *filenamei, char **argv, char **envp)
{
	int error;
	char * filename;
	struct pt_regs *regs;
	/*
	 * I hate this type of thing.  Oh well.
	 * I can't guarantee any way of getting the registers efficiently
	 */
	__asm__("mov\t%0, r9\n\t": "=r" (regs));
	error = getname(filenamei, &filename);
	if (error)
		return error;
	error = do_execve(filename, argv, envp, regs);
	putname(filename);
	/* if execve is suceeding, we really want to return argc to the exec'd prcess */
	if (0 == error) return regs->ARM_r0;
	
	return error;
}
