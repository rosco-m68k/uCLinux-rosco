/* ptrace.c */
/* By Ross Biro 1/23/92 */
/* edited by Linus Torvalds */
/* edited for ARM by Russell King */

#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/ptrace.h>
#include <linux/user.h>

#include <asm/segment.h>
#include <asm/pgtable.h>
#include <asm/system.h>

/*
 * does not yet catch signals sent when the child dies.
 * in exit.c or in signal.c.
 */

/*
 * Breakpoint SWI instruction: SWI &9F0001
 */
#define BREAKINST	0xef9f0001

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
 * this routine will get a word off of the processes privileged stack.
 * the offset is how far from the base addr as stored in the TSS.
 * this routine assumes that all the privileged stacks are in our
 * data space.
 */
static inline long get_stack_long(struct task_struct *task, int offset)
{
	unsigned char *stack;

	stack = (unsigned char *)(task->kernel_stack_page + 4096 - sizeof(struct pt_regs));
	stack += offset << 2;
	return *(unsigned long *)stack;
}

/*
 * this routine will put a word on the processes privileged stack.
 * the offset is how far from the base addr as stored in the TSS.
 * this routine assumes that all the privileged stacks are in our
 * data space.
 */
static inline long put_stack_long(struct task_struct *task, int offset,
	unsigned long data)
{
	unsigned char *stack;

	stack = (unsigned char *)(task->kernel_stack_page + 4096 - sizeof(struct pt_regs));
	stack += offset << 2;
	*(unsigned long *) stack = data;
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


/*
 * Get value of register `rn' (in the instruction)
 */
static unsigned long ptrace_getrn (struct task_struct *child, unsigned long insn)
{
	unsigned int reg = (insn >> 16) & 15;
	unsigned long val;

	if (reg == 15)
		val = pc_pointer (get_stack_long (child, reg));
	else
		val = get_stack_long (child, reg);

#if 0
printk ("r%02d=%08lX ", reg, val);
#endif
	return val;
}

/*
 * Get value of operand 2 (in an ALU instruction)
 */
static unsigned long ptrace_getaluop2 (struct task_struct *child, unsigned long insn)
{
	unsigned long val;
	int shift;
	int type;

#if 0
printk ("op2=");
#endif
	if (insn & 1 << 25) {
		val = insn & 255;
		shift = (insn >> 8) & 15;
		type = 3;
#if 0
printk ("(imm)");
#endif
	} else {
		val = get_stack_long (child, insn & 15);

		if (insn & (1 << 4))
			shift = (int)get_stack_long (child, (insn >> 8) & 15);
		else
			shift = (insn >> 7) & 31;

		type = (insn >> 5) & 3;
#if 0
printk ("(r%02ld)", insn & 15);
#endif
	}
#if 0
printk ("sh%dx%d", type, shift);
#endif
	switch (type) {
	case 0:	val <<= shift;	break;
	case 1:	val >>= shift;	break;
	case 2:
		val = (((signed long)val) >> shift);
		break;
	case 3:
		__asm__ __volatile__("mov %0, %0, ror %1" : "=r" (val) : "0" (val), "r" (shift));
		break;
	}
#if 0
printk ("=%08lX ", val);
#endif
	return val;
}

/*
 * Get value of operand 2 (in a LDR instruction)
 */
static unsigned long ptrace_getldrop2 (struct task_struct *child, unsigned long insn)
{
	unsigned long val;
	int shift;
	int type;

	val = get_stack_long (child, insn & 15);
	shift = (insn >> 7) & 31;
	type = (insn >> 5) & 3;

#if 0
printk ("op2=r%02ldsh%dx%d", insn & 15, shift, type);
#endif
	switch (type) {
	case 0:	val <<= shift;	break;
	case 1:	val >>= shift;	break;
	case 2:
		val = (((signed long)val) >> shift);
		break;
	case 3:
		__asm__ __volatile__("mov %0, %0, ror %1" : "=r" (val) : "0" (val), "r" (shift));
		break;
	}
#if 0
printk ("=%08lX ", val);
#endif
	return val;
}
#undef pc_pointer
#define pc_pointer(x) ((x) & 0x03fffffc)
int ptrace_set_bpt (struct task_struct *child)
{
	unsigned long insn, pc, alt;
	int i, nsaved = 0, res;

	pc = pc_pointer (get_stack_long (child, 15/*REG_PC*/));

	res = read_long (child, pc, &insn);
	if (res < 0)
		return res;

	child->debugreg[nsaved++] = alt = pc + 4;
#if 0
printk ("ptrace_set_bpt: insn=%08lX pc=%08lX ", insn, pc);
#endif
	switch (insn & 0x0e100000) {
	case 0x00000000:
	case 0x00100000:
	case 0x02000000:
	case 0x02100000: /* data processing */
#if 0
		printk ("data ");
#endif
		switch (insn & 0x01e0f000) {
		case 0x0000f000:
			alt = ptrace_getrn(child, insn) & ptrace_getaluop2(child, insn);
			break;
		case 0x0020f000:
			alt = ptrace_getrn(child, insn) ^ ptrace_getaluop2(child, insn);
			break;
		case 0x0040f000:
			alt = ptrace_getrn(child, insn) - ptrace_getaluop2(child, insn);
			break;
		case 0x0060f000:
			alt = ptrace_getaluop2(child, insn) - ptrace_getrn(child, insn);
			break;
		case 0x0080f000:
			alt = ptrace_getrn(child, insn) + ptrace_getaluop2(child, insn);
			break;
		case 0x00a0f000:
			alt = ptrace_getrn(child, insn) + ptrace_getaluop2(child, insn) +
				(get_stack_long (child, 16/*REG_PSR*/) & CC_C_BIT ? 1 : 0);
			break;
		case 0x00c0f000:
			alt = ptrace_getrn(child, insn) - ptrace_getaluop2(child, insn) +
				(get_stack_long (child, 16/*REG_PSR*/) & CC_C_BIT ? 1 : 0);
			break;
		case 0x00e0f000:
			alt = ptrace_getaluop2(child, insn) - ptrace_getrn(child, insn) +
				(get_stack_long (child, 16/*REG_PSR*/) & CC_C_BIT ? 1 : 0);
			break;
		case 0x0180f000:
			alt = ptrace_getrn(child, insn) | ptrace_getaluop2(child, insn);
			break;
		case 0x01a0f000:
			alt = ptrace_getaluop2(child, insn);
			break;
		case 0x01c0f000:
			alt = ptrace_getrn(child, insn) & ~ptrace_getaluop2(child, insn);
			break;
		case 0x01e0f000:
			alt = ~ptrace_getaluop2(child, insn);
			break;
		}
		break;

	case 0x04100000: /* ldr imm */
		if ((insn & 0xf000) == 0xf000) {
#if 0
printk ("ldrimm ");
#endif
			alt = ptrace_getrn(child, insn);
			if (insn & 1 << 24) {
				if (insn & 1 << 23)
					alt += insn & 0xfff;
				else
					alt -= insn & 0xfff;
			}
			if (read_long (child, alt, &alt) < 0)
				alt = pc + 4; /* not valid */
			else
				alt = pc_pointer (alt);
		}
		break;

	case 0x06100000: /* ldr */
		if ((insn & 0xf000) == 0xf000) {
#if 0
printk ("ldr ");
#endif
			alt = ptrace_getrn(child, insn);
			if (insn & 1 << 24) {
				if (insn & 1 << 23)
					alt += ptrace_getldrop2 (child, insn);
				else
					alt -= ptrace_getldrop2 (child, insn);
			}
			if (read_long (child, alt, &alt) < 0)
				alt = pc + 4; /* not valid */
			else
				alt = pc_pointer (alt);
		}
		break;

	case 0x08100000: /* ldm */
		if (insn & (1 << 15)) {
			unsigned long base;
			int nr_regs;
#if 0
printk ("ldm ");
#endif

			if (insn & (1 << 23)) {
				nr_regs = insn & 65535;

				nr_regs = (nr_regs & 0x5555) + ((nr_regs & 0xaaaa) >> 1);
				nr_regs = (nr_regs & 0x3333) + ((nr_regs & 0xcccc) >> 2);
				nr_regs = (nr_regs & 0x0707) + ((nr_regs & 0x7070) >> 4);
				nr_regs = (nr_regs & 0x000f) + ((nr_regs & 0x0f00) >> 8);
				nr_regs <<= 2;

				if (!(insn & (1 << 24)))
					nr_regs -= 4;
			} else {
				if (insn & (1 << 24))
					nr_regs = -4;
				else
					nr_regs = 0;
			}

			base = ptrace_getrn (child, insn);

			if (read_long (child, base + nr_regs, &alt) < 0)
				alt = pc + 4; /* not valid */
			else
				alt = pc_pointer (alt);
			break;
		}
		break;

	case 0x0a000000:
	case 0x0a100000: { /* bl or b */
		signed long displ;
#if 0
printk ("b/bl ");
#endif
		/* It's a branch/branch link: instead of trying to
		 * figure out whether the branch will be taken or not,
		 * we'll put a breakpoint at either location.  This is
		 * simpler, more reliable, and probably not a whole lot
		 * slower than the alternative approach of emulating the
		 * branch.
		 */
		displ = (insn & 0x00ffffff) << 8;
		displ = (displ >> 6) + 8;
		if (displ != 0 && displ != 4)
			alt = pc + displ;
	    }
	    break;
	}
#if 0
printk ("=%08lX\n", alt);
#endif
	if (alt != pc + 4)
		child->debugreg[nsaved++] = alt;

	for (i = 0; i < nsaved; i++) {
		res = read_long (child, child->debugreg[i], &insn);
		if (res >= 0) {
			child->debugreg[i + 2] = insn;
			res = write_long (child, child->debugreg[i], BREAKINST);
			__flush_entry_to_ram(child->debugreg[i]);
		}
		if (res < 0) {
			child->debugreg[4] = 0;
			return res;
		}
	}
	child->debugreg[4] = nsaved;
	return 0;
}

/* Ensure no single-step breakpoint is pending.  Returns non-zero
 * value if child was being single-stepped.
 */
int ptrace_cancel_bpt (struct task_struct *child)
{
	int i, nsaved = child->debugreg[4];

	child->debugreg[4] = 0;

	if (nsaved > 2) {
		printk ("ptrace_cancel_bpt: bogus nsaved: %d!\n", nsaved);
		nsaved = 2;
	}
	for (i = 0; i < nsaved; i++) {
		write_long (child, child->debugreg[i], child->debugreg[i + 2]);
		__flush_entry_to_ram(child->debugreg[i]);
	}
	return nsaved != 0;
}

asmlinkage int sys_ptrace(long request, long pid, long addr, long data)
{
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
		case PTRACE_PEEKTEXT:				/* read word at location addr. */
		case PTRACE_PEEKDATA: {
			unsigned long tmp;
			int res;

			res = read_long(child, addr, &tmp);
			if (res < 0)
				return res;
			res = verify_area(VERIFY_WRITE, (void *) data, sizeof(long));
			if (!res)
				put_fs_long(tmp,(unsigned long *) data);
			return res;
		}

		case PTRACE_PEEKUSR: {				/* read the word at location addr in the USER area. */
			unsigned long tmp;
			int res;

			if ((addr & 3) || addr < 0 || addr >= sizeof(struct user))
				return -EIO;

			res = verify_area(VERIFY_WRITE, (void *) data, sizeof(long));
			if (res)
				return res;

			tmp = 0;  /* Default return condition */

			if (addr < sizeof (struct pt_regs))
				tmp = get_stack_long(child, (int)addr >> 2);
			put_fs_long(tmp,(unsigned long *) data);
			return 0;
		}

		case PTRACE_POKETEXT:				/* write the word at location addr. */
		case PTRACE_POKEDATA:
			return write_long(child,addr,data);

		case PTRACE_POKEUSR:				/* write the word at location addr in the USER area */
			if ((addr & 3) || addr < 0 || addr >= sizeof(struct user))
				return -EIO;

			if (addr < sizeof (struct pt_regs))
				put_stack_long(child, (int)addr >> 2, data);
			else
				return -EIO;
			return 0;

		case PTRACE_SYSCALL:				/* continue and stop at next (return from) syscall */
		case PTRACE_CONT:				/* restart after signal. */
			if ((unsigned long) data > NSIG)
				return -EIO;
			if (request == PTRACE_SYSCALL)
				child->flags |= PF_TRACESYS;
			else
				child->flags &= ~PF_TRACESYS;
			child->exit_code = data;
			wake_up_process (child);
			/* make sure single-step breakpoint is gone. */
			ptrace_cancel_bpt (child);
			return 0;

		/* make the child exit.  Best I can do is send it a sigkill.
		 * perhaps it should be put in the status that it wants to
		 * exit.
		 */
		case PTRACE_KILL:
			if (child->state == TASK_ZOMBIE)	/* already dead */
				return 0;
			wake_up_process (child);
			child->exit_code = SIGKILL;
			ptrace_cancel_bpt (child);
			/* make sure single-step breakpoint is gone. */
			ptrace_cancel_bpt (child);
			return 0;

		case PTRACE_SINGLESTEP:				/* execute single instruction. */
			if ((unsigned long) data > NSIG)
				return -EIO;
			child->debugreg[4] = -1;
			child->flags &= ~PF_TRACESYS;
			wake_up_process(child);
			child->exit_code = data;
			/* give it a chance to run. */
			return 0;

		case PTRACE_DETACH:				/* detach a process that was attached. */
			if ((unsigned long) data > NSIG)
				return -EIO;
			child->flags &= ~(PF_PTRACED|PF_TRACESYS);
			wake_up_process (child);
			child->exit_code = data;
			REMOVE_LINKS(child);
			child->p_pptr = child->p_opptr;
			SET_LINKS(child);
			/* make sure single-step breakpoint is gone. */
			ptrace_cancel_bpt (child);
			return 0;

		default:
			return -EIO;
	}
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
}
