/*
 * arch/microblaze/kernel/ptrace.c -- `ptrace' system call
 *
 *  Copyright (C) 2002,03  NEC Electronics Corporation
 *  Copyright (C) 2002,03  Miles Bader <miles@gnu.org>
 *
 * Derived from arch/mips/kernel/ptrace.c:
 *
 *  Copyright (C) 1992 Ross Biro
 *  Copyright (C) Linus Torvalds
 *  Copyright (C) 1994, 95, 96, 97, 98, 2000 Ralf Baechle
 *  Copyright (C) 1996 David S. Miller
 *  Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 *  Copyright (C) 1999 MIPS Technologies, Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of this
 * archive for more details.
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/ptrace.h>

#include <asm/errno.h>
#include <asm/ptrace.h>
#include <asm/processor.h>
#include <asm/uaccess.h>

/* Returns the address where the register at REG_OFFS in P is stashed away.  */
static microblaze_reg_t *reg_save_addr (unsigned reg_offs, struct task_struct *t)
{
	struct pt_regs *regs;

	/* Three basic cases:

	   (1) A register normally saved before calling the scheduler, is
	       available in the kernel entry pt_regs structure at the top
	       of the kernel stack.  The kernel trap/irq exit path takes
	       care to save/restore almost all registers for ptrace'd
	       processes.

	   (2) A call-clobbered register, where the process P entered the
	       kernel via [syscall] trap, is not stored anywhere; that's
	       OK, because such registers are not expected to be preserved
	       when the trap returns anyway (so we don't actually bother to
	       test for this case).

	   (3) A few registers not used at all by the kernel, and so
	       normally never saved except by context-switches, are in the
	       context switch state.  */

	/* Register saved during kernel entry (or not available).  */
	regs = task_regs (t);

	return (microblaze_reg_t *)((char *)regs + reg_offs);
}

/* Makes sure hardware single-stepping is (globally) enabled.
   Returns true if successful.  */
static inline int enable_single_stepping (void)
{
	static int enabled = 1;	/* Remember whether we already did it.  */
	return enabled;
	if (! enabled) {
		/* Turn on the SE (`single-step enable') bit, 0x100, in the
		   DIR (`debug information register').  This may fail if a
		   processor doesn't support it or something.  We also try
		   to clear bit 0x40 (`INI'), which is necessary to use the
		   debug stuff on the microblazee2; on the microblazee, clearing 0x40
		   shouldn't cause any problem.  */
		microblaze_reg_t dir = set_dir (0x100, 0x40);
		/* Make sure it really got set.  */
		if (dir & 0x100)
			enabled = 1;
	}
	return enabled;
}

/* Try to set CHILD's single-step flag to VAL.  Returns true if successful.  */
static int set_single_step (struct task_struct *t, int val)
{
	microblaze_reg_t *sstep_addr = reg_save_addr(PT_SINGLESTEP, t);
	if (val) {
		/* Make sure single-stepping is enabled.  */
		if (! enable_single_stepping ())
			return 0;
		/* Set T's single-step flag.  */
		*sstep_addr = 1;
	} else
		*sstep_addr = 0;
	return 1;
}

int sys_ptrace(long request, long pid, long addr, long data)
{
	struct task_struct *child;
	int rval;

	lock_kernel();
	/* printk("sys_ptrace - PID:%u  ADDR:%08x  DATA:%08x\n",pid,addr,data); */

	if (request == PTRACE_TRACEME) {
		/* are we already being traced? */
		if (current->ptrace & PT_PTRACED) {
			rval = -EPERM;
			goto out;
		}
		/* set the ptrace bit in the process flags. */
		current->ptrace |= PT_PTRACED;
		rval = 0;
		goto out;
	}
	rval = -ESRCH;
	read_lock(&tasklist_lock);
	child = find_task_by_pid(pid);
	if (child)
		get_task_struct(child);
	read_unlock(&tasklist_lock);
	if (!child)
		goto out;

	rval = -EPERM;
	if (pid == 1)		/* you may not mess with init */
		goto out;

	if (request == PTRACE_ATTACH) {
		rval = ptrace_attach(child);
		goto out_tsk;
	}
	rval = -ESRCH;
	if (!(child->ptrace & PT_PTRACED))
		goto out_tsk;
	if (child->state != TASK_STOPPED) {
		if (request != PTRACE_KILL)
			goto out_tsk;
	}
	if (child->p_pptr != current)
		goto out_tsk;

	switch (request) {
		unsigned long val, copied;

	case PTRACE_PEEKTEXT: /* read word at location addr. */
	case PTRACE_PEEKDATA:
	/*	printk("PEEKTEXT/PEEKDATA at %08X\n",addr); */
		copied = access_process_vm(child, addr, &val, sizeof(val), 0);
		rval = -EIO;
		if (copied != sizeof(val))
			break;
		rval = put_user(val, (unsigned long *)data);
		goto out;

	case PTRACE_POKETEXT: /* write the word at location addr. */
	case PTRACE_POKEDATA:
		/* printk("POKETEXT/POKEDATA to %08X\n",addr); */
		rval = 0;
		if (access_process_vm(child, addr, &data, sizeof(data), 1)
		    == sizeof(data))
			break;
		rval = -EIO;
		goto out;

	/* Read/write the word at location ADDR in the registers.  */
	case PTRACE_PEEKUSR:
	case PTRACE_POKEUSR:
		rval = 0;
		if (addr >= PT_SIZE && request == PTRACE_PEEKUSR) {
			/* Special requests that don't actually correspond
			   to offsets in struct pt_regs.  */
			if (addr == PT_TEXT_ADDR)
			{
				val = child->mm->start_code;
			}
			else if (addr == PT_DATA_ADDR)
			{
				val = child->mm->start_data;
			}
			else if (addr == PT_TEXT_LEN)
			{
				val = child->mm->end_code
					- child->mm->start_code;
			}
			else if (addr == PT_DATA_LEN)
			{
				val = child->mm->end_data
					- child->mm->start_data;
			}
			else
			{
				rval = -EIO;
			}
		} else if (addr >= 0 && addr < PT_SIZE && (addr & 0x3) == 0) {
			microblaze_reg_t *reg_addr = reg_save_addr(addr, child);
			if (request == PTRACE_PEEKUSR)
			{
				val = *reg_addr;
			}
			else
			{
				*reg_addr = data;
			}
		} else
			rval = -EIO;

		if (rval == 0 && request == PTRACE_PEEKUSR)
		{
			rval = put_user (val, (unsigned long *)data);
		}
		goto out;

	/* Continue and stop at next (return from) syscall */
	case PTRACE_SYSCALL:
	/* Restart after a signal.  */
	case PTRACE_CONT:
		/* printk("PTRACE_CONT\n"); */
	/* Execute a single instruction. */
	case PTRACE_SINGLESTEP:
		/* printk("PTRACE_SINGLESTEP\n"); */
		rval = -EIO;
		if ((unsigned long) data > _NSIG)
			break;

		/* Turn CHILD's single-step flag on or off.  */
		/* printk("calling set_single_step\n"); */
		if (! set_single_step (child, request == PTRACE_SINGLESTEP))
			break;

		if (request == PTRACE_SYSCALL)
			child->ptrace |= PT_TRACESYS;
		else
			child->ptrace &= ~PT_TRACESYS;

		child->exit_code = data;
		/* printk("wakeup_process\n"); */
		wake_up_process(child);
		rval = 0;
		break;

	/*
	 * make the child exit.  Best I can do is send it a sigkill.
	 * perhaps it should be put in the status that it wants to
	 * exit.
	 */
	case PTRACE_KILL:
		/* printk("PTRACE_KILL\n"); */
		rval = 0;
		if (child->state == TASK_ZOMBIE)	/* already dead */
			break;
		child->exit_code = SIGKILL;
		wake_up_process(child);
		break;

	case PTRACE_DETACH: /* detach a process that was attached. */
		/* printk("PTRACE_DETACH\n"); */
		set_single_step (child, 0);  /* Clear single-step flag */
		rval = ptrace_detach(child, data);
		break;

	default:
		rval = -EIO;
		goto out;
	}

out_tsk:
	free_task_struct(child);
out:
	unlock_kernel();
	return rval;
}

asmlinkage void syscall_trace(void)
{
	if ((current->ptrace & (PT_PTRACED|PT_TRACESYS))
			!= (PT_PTRACED|PT_TRACESYS))
		return;
	/* The 0x80 provides a way for the tracing parent to distinguish
	   between a syscall stop and SIGTRAP delivery */
	current->exit_code = SIGTRAP | ((current->ptrace & PT_TRACESYSGOOD)
					? 0x80 : 0);
	current->state = TASK_STOPPED;
	notify_parent(current, SIGCHLD);
	schedule();
	/*
	 * this isn't the same as continuing with a signal, but it will do
	 * for normal use.  strace only continues with a signal if the
	 * stopping signal is not SIGTRAP.  -brl
	 */
	if (current->exit_code) {
		send_sig(current->exit_code, current, 1);
		current->exit_code = 0;
	}
}

void ptrace_disable (struct task_struct *child)
{
	/* nothing to do */
}
