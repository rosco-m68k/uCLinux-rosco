/*
 * include/asm-i960/processor.h
 *
 * Copyright (C) 1999 Keith Adams
 */

#ifndef __ASM_I960_PROCESSOR_H
#define __ASM_I960_PROCESSOR_H

#include <asm/segment.h>

/*
 * Bus types
 */
#define EISA_bus__is_a_macro	1
#define EISA_bus 0
#define MCA_bus__is_a_macro	1
#define MCA_bus 0

/*
 * The i960 can't do write protection
 */
#define wp_works_ok__is_a_macro	1
#define wp_works_ok 0

/* MAX floating point unit state size (FSAVE/FRESTORE) */
#define FPSTATESIZE   0

struct thread_struct {
	unsigned long  pfp;		/* saved pfp of thread */
};

/* XXX: do something with this */
#define INIT_MMAP { &init_mm, 0, 0x40000000, __pgprot(_PAGE_PRESENT|_PAGE_ACCESSED), VM_READ | VM_WRITE | VM_EXEC }

#define INIT_TSS  { \
	(long) init_kernel_stack, 0, \
	0 \
}

unsigned long alloc_kernel_stack(void);
void free_kernel_stack(unsigned long ksp);

/*
 * Do necessary setup to start up a newly executed thread.
 */
struct i960_frame {
	unsigned long	pfp;
	unsigned long	sp;
	unsigned long	rip;
};

extern void start_thread(struct pt_regs * regs,
			 unsigned long ip, 
			 unsigned long sp);
/* TODO: tracing stuff */
static inline void tron(void)
{
}

static inline void troncof(void)
{
}

static inline void troff(void)
{
}

/*
 * Return saved PC of a blocked thread.
 */
extern inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	struct i960_frame* frame = (struct i960_frame*)t->pfp;
	return frame->rip;
}

#endif
