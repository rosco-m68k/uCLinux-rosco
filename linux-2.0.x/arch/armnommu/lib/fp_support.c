/*
 * linux/arch/arm/lib/fp_support.c
 *
 * Copyright (C) 1995, 1996 Russell King
 */

#include <linux/sched.h>

extern void (*fp_save)(struct fp_soft_struct *);
#if 0
__asm__(
	".stabs \"fp_printk\", 11, 0, 0, 0\n\t"
	".stabs \"printk\", 1, 0, 0, 0\n\t"
	".stabs \"fp_current\", 11, 0, 0, 0\n\t"
	".stabs \"current_set\", 1, 0, 0, 0\n\t"
	".stabs \"fp_send_sig\", 11, 0, 0, 0\n\t"
	".stabs \"send_sig\", 1, 0, 0, 0\n\t");
#endif

void fp_setup(void)
{
	struct task_struct **p;

	p = task;
	do {
		if(*p)
			fp_save(&(*p)->tss.fpstate.soft);
	}
	while (++p < task + NR_TASKS);
}
