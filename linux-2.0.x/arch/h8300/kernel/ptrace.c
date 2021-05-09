/*
 *  linux/arch/h8300/kernel/ptrace.c
 *
 *  Yoshinori Sato <qzb04471@nifty.ne.jp>
 *
 *  Based on:
 *
 *  linux/arch/m68knommu/kernel/ptrace.c
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
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
#define SR_MASK 0x006f

/* Find the stack offset for a register, relative to tss.esp0. */
#define PT_REG(reg)	((long)&((struct pt_regs *)0)->reg)
#define SW_REG(reg)	((long)&((struct switch_stack *)0)->reg \
			 - sizeof(struct switch_stack))
/* Mapping from PT_xxx to the stack offset at which the register is
   saved.  Notice that usp has no stack-slot and needs to be treated
   specially (see get_reg/put_reg below). */
static const int pt_regs_offset[] = {
	PT_REG(er1), PT_REG(er2), PT_REG(er3), PT_REG(er4),
	PT_REG(er5), SW_REG(er6), PT_REG(er0), PT_REG(orig_er0),
	PT_REG(ccr), PT_REG(pc)
};

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

	if (regno == PT_CCR)
		return *(unsigned short *)(task->tss.esp0 + pt_regs_offset[regno]);
	if (regno == PT_USP)
		addr = &task->tss.usp;
	else if (regno < sizeof(pt_regs_offset)/sizeof(pt_regs_offset[0]))
		addr = (unsigned long *)(task->tss.esp0 + pt_regs_offset[regno]);
	else
		return 0;
	return *addr;
}

/*
 * Write contents of register REGNO in task TASK.
 */
static inline int put_reg(struct task_struct *task, int regno,
			  unsigned long data)
{
	unsigned long *addr;

	if (regno == PT_CCR) {
		addr = (unsigned long *)task->tss.usp;
		*addr &= 0xffffff;
		*addr |= data << 24;
		return 0;
	}
	if (regno == PT_PC) {
		addr = (unsigned long *)task->tss.usp;
		*addr |= data & 0xffffff;
		return 0;
	}
	else if (regno == PT_USP)
		addr = &task->tss.usp;
	else if (regno < sizeof(pt_regs_offset)/sizeof(pt_regs_offset[0]))
		addr = (unsigned long *) (task->tss.esp0 + pt_regs_offset[regno]);
	else
		return -1;
	*addr = data;
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

#define ENABLE_PTRACE

#ifdef ENABLE_PTRACE
int ptrace_cancel_bpt(struct task_struct *child)
{
        int i,r=0;

	for(i=0; i<4; i++) {
	        if (child->debugreg[i]) {
		        if (child->debugreg[i]!=-1)
		                put_fs_word(child->debugreg[i+4],child->debugreg[i]);
			r = 1;
			child->debugreg[i] = 0;
		}
	}
	return r;
}

const static unsigned char opcode0[]={
  0x04,0x02,0x04,0x02,0x04,0x02,0x04,0x02,  /* 0x58 */
  0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,  /* 0x60 */
  0x02,0x02,0x11,0x11,0x02,0x02,0x04,0x04,  /* 0x68 */
  0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,  /* 0x70 */
  0x08,0x04,0x06,0x04,0x04,0x04,0x04,0x04}; /* 0x78 */

static int table_parser01(unsigned char *pc);
static int table_parser02(unsigned char *pc);
static int table_parser100(unsigned char *pc);
static int table_parser101(unsigned char *pc);

const static int (*parsers[])(unsigned char *pc)={table_parser01,table_parser02};

static int insn_length(unsigned char *pc)
{
  if (*pc == 0x01)
    return table_parser01(pc+1);
  if (*pc < 0x58 || *pc>=0x80) 
    return 2;
  else
    if (opcode0[*pc-0x58]<0x10)
      return opcode0[*pc-0x58];
    else
      return (*parsers[opcode0[*pc-0x58]-0x10])(pc+1);
}

static int table_parser01(unsigned char *pc)
{
  const unsigned char codelen[]={0x10,0x00,0x00,0x00,0x11,0x00,0x00,0x00,
                                 0x02,0x00,0x00,0x00,0x04,0x04,0x00,0x04};
  const static int (*parsers[])(unsigned char *)={table_parser100,table_parser101};
  unsigned char second_index;
  second_index = (*pc) >> 4;
  if (codelen[second_index]<0x10)
    return codelen[second_index];
  else
    return parsers[codelen[second_index]-0x10](pc);
}

static int table_parser02(unsigned char *pc)
{
  return (*pc & 0x20)?0x06:0x04;
}

static int table_parser100(unsigned char *pc)
{
  return (*(pc+2) & 0x02)?0x08:0x06;
}

static int table_parser101(unsigned char *pc)
{
  return (*(pc+2) & 0x02)?0x08:0x06;
}

#define BREAK_INST 0x5730 /* TRAPA #3 */

int ptrace_set_bpt(struct task_struct *child)
{
        unsigned long pc,next;
	unsigned short insn;
	pc = get_reg(child,PT_PC);
	next = insn_length((unsigned char *)pc) + pc;
	insn = get_fs_word(pc);
	if (insn == 0x5470) {
	        /* rts */ 
	        unsigned long sp;
		sp = get_reg(child,PT_USP);
		next = get_fs_long(sp);
	} else if ((insn & 0xfb00) != 0x5800) {
	        /* jmp / jsr */
	        int regs;
		const short reg_tbl[]={PT_ER0,PT_ER1,PT_ER2,PT_ER3,
                                       PT_ER4,PT_ER5,PT_ER6,PT_USP};
	        switch(insn & 0xfb00) {
		        case 0x5900:
			       regs = (insn & 0x0070) >> 8;
                               next = get_reg(child,reg_tbl[regs]);
			       break;
		        case 0x5a00:
			       next = get_fs_long(pc+2) & 0x00ffffff;
			       break;
		        case 0x5b00:
			       /* unneccessary? */
			       next = *(unsigned long *)(insn & 0xff);
                               break;
		}
	} else if (((insn & 0xf000) == 0x4000) || ((insn &0xff00) == 0x5500)) { 
	        /* b**:8 */
	        unsigned long dsp;
		dsp = (long)(insn && 0xff)+pc+2;
		child->debugreg[1] = dsp;
		child->debugreg[5] = get_fs_word(dsp);
		put_fs_word(dsp,BREAK_INST);
	} else if (((insn & 0xff00) == 0x5800) || ((insn &0xff00) == 0x5c00)) { 
	        /* b**:16 */
	        unsigned long dsp;
		dsp = (long)get_fs_word(pc+2)+pc+4;
		child->debugreg[1] = dsp;
		child->debugreg[5] = get_fs_word(dsp);
		put_fs_word(dsp,BREAK_INST);
	}
	child->debugreg[0] = next;
	child->debugreg[4] = get_fs_word(next);
	put_fs_word(BREAK_INST,next);
	return 0;
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
				put_fs_long(tmp, (unsigned long *) data);
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
			if (addr < 10)
				tmp = get_reg(child, addr);
			else
				return -EIO;
			put_fs_long(tmp,(unsigned long *) data);
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
			    
			if (addr == PT_ORIG_ER0)
				return -EIO;
			if (addr == PT_CCR) {
				data &= SR_MASK;
			}
			if (addr < 10) {
				if (put_reg(child, addr, data))
					return -EIO;
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
			ptrace_cancel_bpt(child);
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
			ptrace_cancel_bpt(child);
			return 0;
		}

		case PTRACE_SINGLESTEP: {  /* set the trap flag. */
			long tmp;

			if ((unsigned long) data >= NSIG)
				return -EIO;
			child->flags &= ~PF_TRACESYS;
			child->debugreg[0]=-1;
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
			ptrace_cancel_bpt(child);
			return 0;
		}

		default:
			return -EIO;
	}
}
#else
int ptrace_cancel_bpt(struct task_struct *child)
{
        return 0;
}

int ptrace_set_bpt(struct task_struct *child)
{
        return 0;
}

asmlinkage int sys_ptrace(long request, long pid, long addr, long data)
{
        return -EIO;
}
#endif

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
