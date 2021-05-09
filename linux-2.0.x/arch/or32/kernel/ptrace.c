/*
 *  linux/arch/m68knommu/kernel/ptrace.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
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

#include <stddef.h>
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

/*
 * does not yet catch signals sent when the child dies.
 * in exit.c or in signal.c.
 */

/* determines which bits in the SR the user has access to. */
/* 1 = access 0 = no access */
#define SR_MASK 0x001f

/* sets the trace bits. */
#define TRACE_BITS 0x8000

/* Mapping from PT_xxx to the stack offset at which the register is
   saved.  Notice that usp has no stack-slot and needs to be treated
   specially (see get_reg/put_reg below). */
static int regoff[] = {0,0,0,0,0,0};

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
	unsigned long *addr;
/* SIMON */
#if 0
	if (regno == PT_USP)
		addr = &task->tss.usp;
	else if (regno < sizeof(regoff)/sizeof(regoff[0]))
		addr = (unsigned long *)(task->tss.esp0 + regoff[regno]);
	else
		return 0;
#endif
	return *addr;
}

/*
 * Write contents of register REGNO in task TASK.
 */
static inline int put_reg(struct task_struct *task, int regno,
			  unsigned long data)
{
/* SIMON */
#if 0
	unsigned long *addr;
	if (regno == PT_USP)
		addr = &task->tss.usp;
	else if (regno < sizeof(regoff)/sizeof(regoff[0]))
		addr = (unsigned long *) (task->tss.esp0 + regoff[regno]);
	else
		return -1;
	*addr = data;
#endif
	return 0;
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
/* SIMON */
#if 0
	struct task_struct *child;
	struct user * dummy;
	dummy = NULL;

	if (request == PTRACE_TRACEME) {
		/* are we already being traced? */
		if (current->flags & PF_PTRACED)
			return -EPERM;
		/* set the ptrace bit in the process flags. */
		current->flags |= PF_PTRACED;
		return 0;
	}
	if (pid == 1)		/* you may not mess with init */
		return -EPERM;
	if (!(child = get_task(pid)))
		return -ESRCH;
	if (request == PTRACE_ATTACH) {
		if (child == current)
			return -EPERM;
		if ((!child->dumpable ||
		    (current->uid != child->euid) ||
		    (current->uid != child->suid) ||
		    (current->uid != child->uid) ||
	 	    (current->gid != child->egid) ||
		    (current->gid != child->sgid) ||
	 	    (current->gid != child->gid)) && !suser())
			return -EPERM;
		/* the same process cannot be attached many times */
		if (child->flags & PF_PTRACED)
			return -EPERM;
		child->flags |= PF_PTRACED;
		if (child->p_pptr != current) {
			REMOVE_LINKS(child);
			child->p_pptr = current;
			SET_LINKS(child);
		}
		send_sig(SIGSTOP, child, 1);
		return 0;
	}
	if (!(child->flags & PF_PTRACED))
		return -ESRCH;
	if (child->state != TASK_STOPPED) {
		if (request != PTRACE_KILL)
			return -ESRCH;
	}
	if (child->p_pptr != current)
		return -ESRCH;

	switch (request) {
	/* when I and D space are separate, these will need to be fixed. */
		case PTRACE_PEEKTEXT: /* read word at location addr. */ 
		case PTRACE_PEEKDATA: {
			unsigned long tmp;
			int res;

			res = read_long(child, addr, &tmp);
			if (res < 0)
				return res;
			res = verify_area(VERIFY_WRITE, (void *) data, sizeof(long));
			if (!res)
				put_user(tmp, (unsigned long *) data);
			return res;
		}

	/* read the word at location addr in the USER area. */
		case PTRACE_PEEKUSR: {
			unsigned long tmp;
			int res;
			
			if ((addr & 3) || addr < 0 || addr >= sizeof(struct user))
				return -EIO;
			
			res = verify_area(VERIFY_WRITE, (void *) data,
					  sizeof(long));
			if (res)
				return res;
			tmp = 0;  /* Default return condition */
			addr = addr >> 2; /* temporary hack. */
			if (addr < 19) {
				tmp = get_reg(child, addr);
				if (addr == PT_SR)
					tmp >>= 16;
			}
			else if (addr >= 21 && addr < 49)
				tmp = child->tss.fp[addr - 21];
			else if (addr == 49)
				tmp = child->mm->start_code;
			else if (addr == 50)
				tmp = child->mm->start_data;
			else
				return -EIO;
			put_user(tmp,(unsigned long *) data);
			return 0;
		}

      /* when I and D space are separate, this will have to be fixed. */
		case PTRACE_POKETEXT: /* write the word at location addr. */
		case PTRACE_POKEDATA:
			return write_long(child,addr,data);

		case PTRACE_POKEUSR: /* write the word at location addr in the USER area */
			if ((addr & 3) || addr < 0 || addr >= sizeof(struct user))
				return -EIO;

			addr = addr >> 2; /* temporary hack. */
			    
			if (addr == PT_ORIG_D0)
				return -EIO;
			if (addr == PT_SR) {
				data &= SR_MASK;
				data <<= 16;
				data |= get_reg(child, PT_SR) & ~(SR_MASK << 16);
			}
			if (addr < 19) {
				if (put_reg(child, addr, data))
					return -EIO;
				return 0;
			}
			if (addr >= 21 && addr < 48)
			{
				child->tss.fp[addr - 21] = data;
				return 0;
			}
			return -EIO;

		case PTRACE_SYSCALL: /* continue and stop at next (return from) syscall */
		case PTRACE_CONT: { /* restart after signal. */
			long tmp;

			if ((unsigned long) data >= NSIG)
				return -EIO;
			if (request == PTRACE_SYSCALL)
				child->flags |= PF_TRACESYS;
			else
				child->flags &= ~PF_TRACESYS;
			child->exit_code = data;
			wake_up_process(child);
			/* make sure the single step bit is not set. */
			tmp = get_reg(child, PT_SR) & ~(TRACE_BITS << 16);
			put_reg(child, PT_SR, tmp);
			return 0;
		}

/*
 * make the child exit.  Best I can do is send it a sigkill. 
 * perhaps it should be put in the status that it wants to 
 * exit.
 */
		case PTRACE_KILL: {
			long tmp;

			if (child->state == TASK_ZOMBIE) /* already dead */
				return 0;
			wake_up_process(child);
			child->exit_code = SIGKILL;
	/* make sure the single step bit is not set. */
			tmp = get_reg(child, PT_SR) & ~(TRACE_BITS << 16);
			put_reg(child, PT_SR, tmp);
			return 0;
		}

		case PTRACE_SINGLESTEP: {  /* set the trap flag. */
			long tmp;

			if ((unsigned long) data >= NSIG)
				return -EIO;
			child->flags &= ~PF_TRACESYS;
			tmp = get_reg(child, PT_SR) | (TRACE_BITS << 16);
			put_reg(child, PT_SR, tmp);

			wake_up_process(child);
			child->exit_code = data;
	/* give it a chance to run. */
			return 0;
		}

		case PTRACE_DETACH: { /* detach a process that was attached. */
			long tmp;

			if ((unsigned long) data >= NSIG)
				return -EIO;
			child->flags &= ~(PF_PTRACED|PF_TRACESYS);
			wake_up_process(child);
			child->exit_code = data;
			REMOVE_LINKS(child);
			child->p_pptr = child->p_opptr;
			SET_LINKS(child);
			/* make sure the single step bit is not set. */
			tmp = get_reg(child, PT_SR) & ~(TRACE_BITS << 16);
			put_reg(child, PT_SR, tmp);
			return 0;
		}

		default:
			return -EIO;
	}
#endif
	return 0;
}

asmlinkage void syscall_trace(void)
{
	if ((current->flags & (PF_PTRACED|PF_TRACESYS))
			!= (PF_PTRACED|PF_TRACESYS))
		return;
	current->exit_code = SIGTRAP;
	current->state = TASK_STOPPED;
	notify_parent(current, SIGCHLD);
	schedule();
	/*
	 * this isn't the same as continuing with a signal, but it will do
	 * for normal use.  strace only continues with a signal if the
	 * stopping signal is not SIGTRAP.  -brl
	 */
	if (current->exit_code)
		current->signal |= (1 << (current->exit_code - 1));
	current->exit_code = 0;
	return;
}
