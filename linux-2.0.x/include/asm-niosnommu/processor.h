/*
 * include/asm-niosnommu/processor.h
 *
 * Copyright (C) 2001  Ken Hill (khill@microtronix.com)    
 *                     Vic Phillips (vic@microtronix.com)
 *
 * hacked from:
 *
 * include/asm-sparc/processor.h
 *
 * Copyright (C) 1994 David S. Miller (davem@caip.rutgers.edu)
 */

#ifndef __ASM_NIOS_PROCESSOR_H
#define __ASM_NIOS_PROCESSOR_H

#include <linux/a.out.h>

#include <asm/psr.h>
#include <asm/ptrace.h>
#include <asm/signal.h>
#include <asm/segment.h>

/*
 * Bus types
 */
#define EISA_bus 0
#define EISA_bus__is_a_macro /* for versions in ksyms.c */
#define MCA_bus 0
#define MCA_bus__is_a_macro /* for versions in ksyms.c */

/*
 * The nios has no problems with write protection
 */
#define wp_works_ok 1
#define wp_works_ok__is_a_macro /* for versions in ksyms.c */

/* Whee, this is STACK_TOP and the lowest kernel address too... */
#if 0
#define KERNBASE        0x00000000  /* First address the kernel will eventually be */
#define TASK_SIZE	(KERNBASE)
#define MAX_USER_ADDR	TASK_SIZE
#define MMAP_SEARCH_START (TASK_SIZE/3)
#endif

/* The Nios processor specific thread struct. */
struct thread_struct {
	unsigned long spare;
	struct pt_regs *kregs;

	/* For signal handling */
	unsigned long sig_address;
	unsigned long sig_desc;

	/* Context switch saved kernel state. */
	unsigned long ksp;
	unsigned long kpc;
	unsigned long kpsr;
	unsigned long kwvalid;

	/* Special child fork kpsr/kwim values. */
	unsigned long fork_kpsr;
	unsigned long fork_kwvalid;

	/* A place to store user windows and stack pointers
	 * when the stack needs inspection.
	 *
	 * KH - I don't think the nios port requires this
	 */
#define NSWINS 8
	struct reg_window spare_reg_window[NSWINS];
	unsigned long spare_rwbuf_stkptrs[NSWINS];
	unsigned long spare_w_saved;

	/* The Nios does not have a floating point processor these
	 * are acting as place holders to prevent C-structures and
	 * assembler constants from getting out of step.
	 */
	unsigned long   spare_float_regs[64];
	unsigned long   spare_fsr;
	unsigned long   spare_fpqdepth;
	struct spare_fpq {
		unsigned long *insn_addr;
		unsigned long insn;
	} spare_fpqueue[16];
	struct sigstack sstk_info;

	/* Flags are defined below */

	unsigned long flags;
	int current_ds;
	struct exec core_exec;     /* just what it says. */
};

#define NIOS_FLAG_KTHREAD	0x00000001	/* task is a kernel thread */
#define NIOS_FLAG_COPROC	0x00000002	/* Thread used coprocess */

#define INIT_MMAP { &init_mm, (0), (0), \
		    __pgprot(0x0) , VM_READ | VM_WRITE | VM_EXEC }

#define INIT_TSS  { \
/* uwinmask, kregs, sig_address, sig_desc, ksp, kpc, kpsr, kwvalid */ \
   0,        0,     0,           0,        0,   0,   0,    0, \
/* fork_kpsr, fork_kwvalid */ \
   0,         0, \
/* spare_reg_window */  \
{ { { 0, }, { 0, } }, }, \
/* spare_rwbuf_stkptrs */  \
{ 0, 0, 0, 0, 0, 0, 0, 0, }, \
/* spare_w_saved */ \
   0, \
/* Spare FPU regs */   \
                 { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, }, \
/* spare FPU status, spare FPU qdepth, FPU queue */ \
   0,                0,            { { 0, 0, }, }, \
/* sstk_info */ \
{ 0, 0, }, \
/* flags,              current_ds, */ \
   NIOS_FLAG_KTHREAD, USER_DS, \
/* core_exec */ \
{ 0, }, \
}

/* Return saved PC of a blocked thread. */
extern inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	return t->kregs->pc;
}

/*
 * Do necessary setup to start up a newly executed thread.
 */
extern inline void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long sp, unsigned long globals)
{
	unsigned long saved_psr = (get_hi_limit() << 4) | PSR_IPRI | PSR_IE;
	int i;

	for(i = 0; i < 16; i++) regs->u_regs[i] = 0;
	regs->pc = pc >> 1;
	regs->psr = saved_psr;
	regs->u_regs[UREG_FP] = (sp - REGWIN_SZ);
	regs->globals = globals;
}

#ifdef __KERNEL__
#define alloc_kernel_stack()    __get_free_page(GFP_KERNEL)
#define free_kernel_stack(page) free_page((page))
#endif

#endif /* __ASM_NIOS_PROCESSOR_H */
