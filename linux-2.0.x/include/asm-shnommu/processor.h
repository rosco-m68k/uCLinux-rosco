/*
 * File Name : processor.h
 *
 * Description : processor related macros
 *
 * Modification History
 *
 * 12 Aug : start_thread function is modified ( copied from H8 code)
 *                changed the ccr -> sr
 *
 *           thread_saved_pc function is modified. 
 *           filed a6 of struct switch_stack is modified to R6
 *         
 *           Following functions are removed
 *
 *           void tron()
 *           void troncof()
 *           void troff()
 *
 *           These functions are removed, I think it is
 *           not required in H8. 
 *
 *
 */

/*
 * include/asm-shnommu/processor.h
 *
 * Copyright (C) 1995 Hamish Macdonald
 */

#ifndef __ASM_SH7615__PROCESSOR_H
#define __ASM_SH7615__PROCESSOR_H

#include <linux/config.h>


#include <asm/segment.h>

/*
 * User space process size: 3.75GB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.
 */
/*#define TASK_SIZE	(0xF0000000UL)*/

/*
 * Bus types
 */
#define EISA_bus__is_a_macro	1
#define EISA_bus 0
#define MCA_bus__is_a_macro	1
#define MCA_bus 0

/*
 * The m68k has no problems with write protection
 */
#define wp_works_ok__is_a_macro	1
#define wp_works_ok 1

/* MAX floating point unit state size (FSAVE/FRESTORE) */
#define FPSTATESIZE   (216/sizeof(unsigned char))

/* 
 * if you change this structure, you must change the code and offsets
 * in m68k/machasm.S
 */
   
struct thread_struct {
	unsigned long  ksp;		/* kernel stack pointer */
	unsigned long  usp;		/* user stack pointer */
	unsigned long sr;		/* saved status register */
	unsigned long fs;		/* saved fs (sfc, dfc) */
	unsigned long  crp[2];		/* cpu root pointer */
	unsigned long  esp0;		/* points to SR of stack frame */
	unsigned long  fp[8*3];
	unsigned long  fpcntl[3];	/* fp control regs */
	unsigned char  fpstate[FPSTATESIZE];  /* floating point state */
};

#define INIT_MMAP { &init_mm, 0, 0x40000000, __pgprot(_PAGE_PRESENT|_PAGE_ACCESSED), VM_READ | VM_WRITE | VM_EXEC }

#define INIT_TSS  { \
	sizeof(init_kernel_stack) + (long) init_kernel_stack, 0, \
	PS_S, KERNEL_DS, \
	{0, 0}, 0, {0,}, {0, 0, 0}, {0,} \
}

/*#define alloc_kernel_stack()    __get_free_page(GFP_KERNEL)
#define free_kernel_stack(page) free_page((page))*/

extern void *kmalloc(size_t size, int priority);
extern void kfree(void *__ptr);

#define KERNEL_STACK_SIZE 	(24*1024)

extern __inline__ unsigned long alloc_kernel_stack(void)
{
    void *stk;

    stk = kmalloc (KERNEL_STACK_SIZE, 0x03); /*GFP_KERNEL - 0x03*/
           
    return (unsigned long)stk;
}

extern __inline__ void free_kernel_stack(unsigned long stk)
{
    if (stk)
	kfree((void *)stk);
}


/*
 * Do necessary setup to start up a newly executed thread.
 */

static inline void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long a5,
				unsigned long usp)
{
	/* reads from user space */
	set_fs(USER_DS);

	regs->sr &= ~0x10;
	/*
	 * Santhosh  Here the status register is updated, But dont know
	 * the correct value of it, For the time being keeping
	 * the value from M68k, Need to check it
	 */
	usp -= 4;
        *(long *)usp = pc;
	wrusp(usp);
}

/*
 * Return saved PC of a blocked thread.
 */
extern inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	/*
	 * the register value r6 is changed to r6
	 */
	unsigned long frame = ((struct switch_stack *)t->ksp)->r6;

	return ((unsigned long *)frame)[1];
}
#endif
