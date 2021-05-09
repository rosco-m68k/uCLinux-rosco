/*
 *  linux/arch/i960/kernel/process.c
 *
 *  Copyright (C) 1999	Keith Adams	<kma@cse.ogi.edu>
 *  			Oregon Graduate Institute
 *  
 *  Based on:
 *
 *  linux/arch/m68k/kernel/process.c
 *
 *  Copyright (C) 1995  Hamish Macdonald
 *
 */

/*
 * This file handles the architecture-dependent parts of process handling..
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>
#include <linux/user.h>
#include <linux/a.out.h>

#include <asm/segment.h>
#include <asm/system.h>
#include <asm/traps.h>
#include <asm/machdep.h>
#include <asm/setup.h>
#include <asm/dprintk.h>

/* The following option permits to be able to run with older application's crt0
 * that expected a pointer to argc to be passed in g13 upon startup.
 *
 * It currently has to be defined for things to work, but it should be
 * possible to set things up on the stack so that argc, argv and envp
 * can be directly accessed on the launched application's stack at
 * known locations.
 */
#define REMAIN_COMPATIBLE_WITH_OLD_LOADER 1

extern void stack_trace(void);
extern void stack_trace_from(unsigned long sp);

asmlinkage void ret_from_exception(void);

/*
 * The idle loop on an i960
 */
asmlinkage int sys_idle(void)
{
	if (current->pid != 0)
		return -EPERM;

	sti();
	/* endless idle loop with no priority at all */
	current->counter = -100;
	for (;;) {
		__asm__ ("halt 2");
		schedule();
	}
}

static unsigned long
stack_align(unsigned long sp)
{
#define STACK_ALIGN	16	/* must be power of 2 */
	if ((unsigned long) sp & (STACK_ALIGN-1)) {
		(unsigned long) sp += STACK_ALIGN;
		(unsigned long) sp &= ~(STACK_ALIGN-1);
	}
#undef STACK_ALIGN
	return sp;
}

#ifdef CONFIG_MON960
#include <asm/mon960.h>
#endif

void hard_reset_now(void)
{
#ifdef CONFIG_MON960
	cli();
	mon960_exit(0);
#else
	printk("ermm, I don't know how to reset. Sorry.\n");
#endif
}

void show_regs(struct pt_regs * regs)
{
	int ii;
	printk("\n");
	printk("PC:\t%08lx\tAC:\t%08lx\n",
	       regs->pc, regs->ac);
	printk("pfp:\t%08lx\tg0:\t%08lx\n",
	       regs->lregs[0], regs->gregs[0]);
	printk("sp:\t%08lx\tg1:\t%08lx\n",
	       regs->lregs[1], regs->gregs[1]);
	printk("rip:\t%08lx\tg2:\t%08lx\n",
	       regs->lregs[2], regs->gregs[2]);
	for (ii=3; ii < 16; ii++)
		printk("r%d:\t%08lx\tg%d\t%08lx\n", ii,
		       regs->lregs[ii], ii, regs->gregs[ii]);
}

void hex_dump(unsigned long start, int len)
{
	char* c = (char*) start;
	int	ii;
	
#define CHARS_IN_LINE 16
	for (ii=0; ii < len; ii++) {
		if (! (ii & (CHARS_IN_LINE-1))) {
			printk("\n%8x: ", start + ii);
		}
		printk("%2x ", c[ii]);
	}
	printk("\n");
}

/*
 * Free current thread data structures etc..
 */
void exit_thread(void)
{
}

void flush_thread(void)
{
}

void release_thread(struct task_struct *dead_task)
{
}
void unused_function()
{
#warning Get rid of this!
}

/***************************************************************************
 *
 * Because of the way the task switchs are implemented (changing the pfp),
 * the function doing the switch needs to be called with a call,
 * and cannot be a branched to as a leaf procedure.
 *
 * If everything is done within the single switch_to function,
 * An optimizing compiler will turn switch_to into a leaf procedure, which
 * means that the context switch will not be completed on the return from
 * switch_to, but on the return from schedule().  Which causes problems
 * with, amongst other things, sleeping processes.
 *
 * This is we force switch_to to call an unused_function() in order to make sure
 * that the compiler cannot play tricks on us.
 *
 * This is ugly and should be correctly fixed by writing the code that
 * does the switch directly in assembler.
 *
 ***************************************************************************/

void switch_to(struct task_struct* prev, struct task_struct* next)
{
	if (next == prev)
		return;

#if 0
	dprintk("*** switch_to: new pfp: 0x%8x\n", next->tss.pfp);
	dprintk("*** switch_to: old stack:\n");
	stack_trace();
	dprintk("*** switch_to: new stack:\n");
	stack_trace_from(next->tss.pfp);
#endif
	current_set[smp_processor_id()] = next;

    unused_function(); /* Prevents this function to be a leaf procedure... Yuk! */

    __asm__ __volatile__
		("st	pfp, (%1)\n\t"		/* save current pfp */
		 "flushreg\n\t"	
		 "mov	%2, pfp\n\t"
		 "flushreg\n\t"
		 : "=m" (prev->tss.pfp)
		 :"r" (&prev->tss.pfp), "r" (next->tss.pfp));
}

/*
 * Called from the vfork path. We need to copy enough of the caller's kernel and
 * user stacks to get the child out of the kernel, and back to the level where
 * vfork was called. This is, I admit, somewhat heinous, and probably a longer
 * code path than one would like.
 */
void copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
		 struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs		*childregs;
	extern void 		syscall_return(void);
	unsigned long		new_ustack;

	if (!regs)
		return;		/* kernel_thread hack */

	asm("flushreg");
	/*
	 * Setting up child's ustack: allocate it just below caller's ustack,
	 * copy in caller's ustack, and relocate sp's. When returning to the
	 * child, we skip the syscall_frame and go straight back to the caller
	 * frame.
	 * 
	 * low addresses
	 *  	 _______________
	 *  	| caller_frame	|
	 *  	|_______________|
	 *  	| syscall_frame |
	 *  	|_______________|
	 *  	| child_caller_frame
	 *  	|_______________|
	 *  	...
	 *  	 _______________
	 *  	| regs		|  (top o' kernel stack)
	 *  	...
	 *  
	 * high addresses
	 */
	{
		struct i960_frame	*syscall_frame, *caller_frame;
		struct i960_frame	*child_caller_frame;
		unsigned long		frame_len, offset;

		syscall_frame = (struct i960_frame*)
			(regs->lregs[PT_PFP] & ~0xf);
		caller_frame = (struct i960_frame*)syscall_frame->pfp;
		dprintk("new_ustack: 0x%8x\n", syscall_frame->sp);
		new_ustack = stack_align(syscall_frame->sp);
		dprintk("new_ustack: 0x%8x\n", new_ustack);
		child_caller_frame = (struct i960_frame*) new_ustack;
		frame_len = caller_frame->sp - (unsigned long)(caller_frame);
		dprintk("frame len: 0x%8x\n", frame_len);
		memcpy((void*)new_ustack, caller_frame, frame_len);

		offset = new_ustack - (unsigned long)caller_frame;
		child_caller_frame->sp += offset;
		/*
		 * XXX: you should never return from the function that called
		 * vfork; right now, you'll just walk off with a bad stack,
		 * but eventually we might want to catch this case and force
		 * an exit.
		 */
		child_caller_frame->pfp = 0;
	}

	/* 
	 * We build a near-copy of the top frame of the caller's syscall stack
	 * in the child's kstack; the child's sp points to the child stack. Its
	 * pfp heads to the frame in the child's ustack that we created above.
	 */
	
	childregs = (struct pt_regs*)p->kernel_stack_page;
	*childregs = *regs;		/* bitwise copy */
	
	childregs->lregs[PT_PFP] = new_ustack | 2;	/* return to user */
	childregs->lregs[PT_SP] = (unsigned long)(childregs + 1);
	childregs->gregs[0] = 0;	/* Child gets a retval of zero */
	childregs->gregs[15] = (unsigned long) &childregs->lregs[0];
	
	/* we reload the pfp from the thread struct in switch_to */
	p->tss.pfp = (unsigned long)&childregs->lregs;
#ifdef DEBUG
	dprintk("child stack:\n");
	stack_trace_from(p->tss.pfp);
#endif
}

/* Fill in the fpu structure for a core dump.  */

int dump_fpu (struct pt_regs *regs, struct user_m68kfp_struct *fpu)
{
  return 0;
}

/*
 * fill in the user structure for a core dump..
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
	/* XXX: write this */
}

/*
 * start a thread
 *
 * For a picture of how the stack has been built with the environment and parameters,
 * check create_flat_tables_stack_grows_up() in fs/binfmt_flat.c
 *
 */
void start_thread(struct pt_regs * regs, unsigned long ip, unsigned long sp)
{
	struct i960_frame* frame;
#ifdef REMAIN_COMPATIBLE_WITH_OLD_LOADER
    unsigned long location_of_argc = sp - 3*sizeof(unsigned long); /* go backwards envp, argv and argc. */
#endif
    
	/* align the stack */
	sp = stack_align(sp);
	
	/*
	 * build a little stack frame at sp; start running
	 */
	frame = (struct i960_frame*) sp;
	frame->sp = ((unsigned long)sp) + 16*4;
	frame->rip = ip;
	frame->pfp = 0;
#ifdef DEBUG
	dprintk("before:\n");
	stack_trace();
#endif
	regs->lregs[PT_PFP] = (sp | 0x7);	/* returning from interrupt */
	regs->pc = (1<<13);		/* bit 13: interrupted mode */
	regs->ac = (1<<15) | (1<<12);	/* bits 15 and 12: precise faults, no overflow interrupts */
	
	/*
	 * g12 contains the "data" bias. the code expects text and data to
	 * be allocated contiguously, and g12 points to code. Since data/bss
	 * might be allocated separately from text, we point g12 at 
	 * data_start - text_len
     *
     * This g12 register is normally only used when user applications are compiled with
     * ctools' gcc960 with the -mpid (position independent data) switch.
     *
     * With GNU's gcc, this could be used by the -fpic option, but this option doesn't do anything on
     * the i960 as of right now.
	 */
	regs->gregs[12] = current->mm->start_data - 
		(current->mm->end_code - current->mm->start_code);
#ifdef REMAIN_COMPATIBLE_WITH_OLD_LOADER
    regs->gregs[13] = location_of_argc;
#endif
	regs->gregs[14] = 0;
	/* make memory coherent; the stack cache flush might be gratuitous */
	asm("flushreg
	    dcctl	3, g0, g0
	    icctl	2, g0, g0");
#ifdef DEBUG
	dprintk("set data seg to: 0x%8x\n", regs->gregs[12]);
	dprintk("set start to %p\n", ip);
	stack_trace();
#endif
}

