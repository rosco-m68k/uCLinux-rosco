/*
 *  linux/arch/i960/kernel/ptrace.c
 *
 *  Copyright (C) 1998  Keith Adams	<kma@cse.ogi.edu>
 * 			Oregon Graduate Institute
 *
 *  Based on:
 *
 *  linux/arch/m68k/kernel/ptrace
 *
 *  Copyright (C) 1994 by Hamish Macdonald
 *  Taken from linux/kernel/ptrace.c and modified for M680x0.
 *  linux/kernel/ptrace.c is by Ross Biro 1/23/92, edited by Linus Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of
 * this archive for more details.
 */

/*
 * XXX: unimplemented
 */ 
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>

#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ptrace.h>
#include <linux/user.h>

#include <asm/segment.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>

/* change a pid into a task struct. */
static inline struct task_struct * get_task(int pid)
{
	int i;

	for (i = 1; i < NR_TASKS; i++) {
		if (task[i] != NULL && (task[i]->pid == pid))
			return task[i];
	}
	return NULL;
}

/*
 * Get contents of register REGNO in task TASK.
 */
static inline long get_reg(struct task_struct *task, int regno)
{
	return -1;
}

/*
 * Write contents of register REGNO in task TASK.
 */
static inline int put_reg(struct task_struct *task, int regno,
			  unsigned long data)
{
	return -1;
}

inline
static unsigned long get_long(struct task_struct * tsk, 
	struct vm_area_struct * vma, unsigned long addr)
{
	return *(unsigned long*)addr;
}

inline
static void put_long(struct task_struct * tsk, struct vm_area_struct * vma, unsigned long addr,
	unsigned long data)
{
	*(unsigned long*)addr = data;
}


inline
static int read_long(struct task_struct * tsk, unsigned long addr,
	unsigned long * result)
{
	*result = *(unsigned long *)addr;
	return 0;
}

inline
static int write_long(struct task_struct * tsk, unsigned long addr,
	unsigned long data)
{
	*(unsigned long *)addr = data;
	return 0;
}

asmlinkage int sys_ptrace(long request, long pid, long addr, long data)
{
	return -1;
}

asmlinkage void syscall_trace(void)
{
}
