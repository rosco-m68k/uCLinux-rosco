/*
 * linux/include/asm-arm/proc-arm2/processor.h
 *
 * Copyright (c) 1996 Russell King.
 *
 * Changelog:
 *  27-06-1996	RMK	Created
 *  10-10-1996	RMK	Brought up to date with SA110
 *  26-09-1996	RMK	Added 'EXTRA_THREAD_STRUCT*'
 *  28-09-1996	RMK	Moved start_thread into the processor dependencies
 */
#ifndef __ASM_PROC_PROCESSOR_H
#define __ASM_PROC_PROCESSOR_H

#ifdef __KERNEL__

#include <asm/assembler.h>
#include <linux/string.h>

#ifndef GFP_KERNEL
#define GFP_KERNEL 0x03
#endif
void * kmalloc(unsigned int size, int priority);
void kfree(void * obj);

#define KERNEL_STACK_SIZE 4096

extern __inline__ unsigned long alloc_kernel_stack(void)
{
    void *stk;

    stk = kmalloc (KERNEL_STACK_SIZE, GFP_KERNEL);
    if (stk)
	memzero (stk, KERNEL_STACK_SIZE);
    return (unsigned long)stk;
}

extern __inline__ void free_kernel_stack(unsigned long stk)
{
    if (stk)
	kfree((void *)stk);
}

/*
 * on arm2,3 wp does not work
 */
#define wp_works_ok 0
#define wp_works_ok__is_a_macro /* for versions in ksyms.c */

struct context_save_struct {
	unsigned long r4;
	unsigned long r5;
	unsigned long r6;
	unsigned long r7;
	unsigned long r8;
	unsigned long r9;
	unsigned long fp;
	unsigned long pc;
};

#define EXTRA_THREAD_STRUCT			\
	struct context_save_struct *save;	\
	unsigned long	memmap;			\
	unsigned long	memcmap[256];

#define EXTRA_THREAD_STRUCT_INIT		\
	0,					\
	(unsigned long) swapper_pg_dir,		\
	{ 0, }

DECLARE_THREAD_STRUCT;

/*
 * Return saved PC of a blocked thread.
 */
extern __inline__ unsigned long thread_saved_pc (struct thread_struct *t)
{
	if (t->save)
		return t->save->pc & PCMASK;
	else
		return 0;
}

extern __inline__ unsigned long get_css_fp (struct thread_struct *t)
{
	if (t->save)
		return t->save->fp;
	else
		return 0;
}

asmlinkage void ret_from_sys_call(void) __asm__("_ret_from_sys_call");

extern __inline__ void copy_thread_css (struct context_save_struct *save)
{
	save->r4 =
	save->r5 =
	save->r6 =
	save->r7 =
	save->r8 =
	save->r9 =
	save->fp = 0;
	save->pc = ((unsigned long)ret_from_sys_call) | SVC26_MODE;
}

/*
 * A hack to get round the problem of declaring current &
 * flush_tlb_mm
 */
#define start_thread(regs,pc,sp) start_thread_setup(regs, pc, sp); flush_tlb_mm(current->mm)

extern __inline__ void start_thread_setup(struct pt_regs * regs, unsigned long pc, unsigned long sp)
{
	unsigned long *stack = (unsigned long *)sp;

	/* Initialise all registers to zero */
	memzero(regs->uregs, sizeof (regs->uregs));
	regs->ARM_pc = pc;		/* pc */
	regs->ARM_sp = sp;		/* sp */
	regs->ARM_r2 = stack[2];	/* r2 (envp) */
	regs->ARM_r1 = stack[1];	/* r1 (argv) */
	regs->ARM_r0 = stack[0];	/* r0 (argc) */
}

#endif

#endif
