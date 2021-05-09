#ifndef _ASM_OR32_PROCESSOR_H
#define _ASM_OR32_PROCESSOR_H

#include <linux/config.h>
#include <asm/segment.h>
#include <asm/ptrace.h>
#include <asm/spr_defs.h>
#include <asm/board.h>

/*
 * Bus types
 */
#define EISA_bus__is_a_macro	1
#define EISA_bus 0
#define MCA_bus__is_a_macro	1
#define MCA_bus 0

/*
 * The or32 has no problems with write protection
 */
#define wp_works_ok__is_a_macro	1
#define wp_works_ok 1

/* MAX floating point unit state size (FSAVE/FRESTORE) */
#define FPSTATESIZE   (216/sizeof(unsigned char))

struct thread_struct {
	unsigned long  pc;		/* PC when last entered system */
	unsigned long  sr;		/* saved status register */
	unsigned long  ksp;		/* kernel stack pointer */
	unsigned long  usp;		/* user stack pointer */
	unsigned long  *regs;		/* pointer to saved register state */
	unsigned short fs;		/* saved fs (sfc, dfc) */
	unsigned long  crp[2];		/* cpu root pointer */
	unsigned long  esp0;		/* points to SR of stack frame */
	unsigned long  fp[8*3];
	unsigned long  fpcntl[3];	/* fp control regs */
	unsigned char  fpstate[FPSTATESIZE];  /* floating point state */
};

#define INIT_MMAP { &init_mm, 0, 0x40000000, __pgprot(_PAGE_PRESENT|_PAGE_ACCESSED), VM_READ | VM_WRITE | VM_EXEC }

#define INIT_TSS  { 	0x100,        \
			((DCACHE << SPR_SR_DCE_BIT) | \
       (ICACHE << SPR_SR_ICE_BIT) | \
			 (DMMU   << SPR_SR_DME_BIT) | \
       (IMMU   << SPR_SR_IME_BIT) | \
        SPR_SR_IEE                | \
        SPR_SR_TEE                | \
        SPR_SR_SM),                 \
			sizeof(init_kernel_stack) + (long) init_kernel_stack, \
}

#define SR_USER                     \
			((DCACHE << SPR_SR_DCE_BIT) | \
       (ICACHE << SPR_SR_ICE_BIT) | \
			 (DMMU   << SPR_SR_DME_BIT) | \
       (IMMU   << SPR_SR_IME_BIT) | \
        SPR_SR_IEE                | \
        SPR_SR_TEE)

#define alloc_kernel_stack()    __get_free_page(GFP_KERNEL)
#define free_kernel_stack(page) free_page((page))

/*
 * Do necessary setup to start up a newly executed thread.
 */
static inline void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long a5, unsigned long usp)
{
#ifndef NO_MM
	unsigned long nilstate = 0;
#endif

	/* reads from user space */
	set_fs(USER_DS);

	regs->pc = pc;
	regs->sp = usp;
	regs->sr = SR_USER;
}

/*
 * Return saved PC of a blocked thread.
 */
extern inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	return t->pc;
}

#endif
