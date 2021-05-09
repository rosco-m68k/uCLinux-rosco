/*
 * include/asm-arm/processor.h
 *
 * Copyright (C) 1995 Russell King
 */

#ifndef __ASM_ARM_PROCESSOR_H
#define __ASM_ARM_PROCESSOR_H

struct fp_hard_struct {
	unsigned int save[140/4];		/* as yet undefined */
};

struct fp_soft_struct {
	unsigned int save[140/4];		/* undefined information */
};

union fp_state {
	struct fp_hard_struct	hard;
	struct fp_soft_struct	soft;
};

#define DECLARE_THREAD_STRUCT							\
struct thread_struct {								\
	unsigned long	fs;			/* simulated fs		*/	\
	unsigned long	address;		/* Address of fault	*/	\
	unsigned long	trap_no;		/* Trap number		*/	\
	unsigned long	error_code;		/* Error code of trap	*/	\
	union fp_state	fpstate;		/* FPE save state	*/	\
	EXTRA_THREAD_STRUCT							\
}

#include <asm/arch/processor.h>
#include <asm/proc/processor.h>

#define INIT_TSS  {			\
	0,				\
	0,				\
	0,				\
	0,				\
	{ { { 0, }, }, },		\
	EXTRA_THREAD_STRUCT_INIT	\
}

#define MAX_USER_ADDR		TASK_SIZE
#define MMAP_SEARCH_START	(TASK_SIZE/3)

#endif /* __ASM_ARM_PROCESSOR_H */
