/*
 *  linux/arch/i960/kernel/traps.c
 *
 *  Copyright (C) 1999		Keith Adams <kma@cse.ogi.edu>
 * 				Oregon Graduate Institute
 *
 *  Based on:
 *
 *  linux/arch/m68k/kernel/traps.c
 *
 *  Copyright (C) 1993, 1994 by Hamish Macdonald
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

/*
 * Sets up all exception vectors
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/a.out.h>
#include <linux/user.h>
#include <linux/string.h>
#include <linux/linkage.h>

#include <asm/setup.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/traps.h>
#include <asm/pgtable.h>
#include <asm/machdep.h>
#include <asm/i960jx.h>
#include <asm/mon960.h>

/* set fault handlers in fault table, syscall table */
void trap_init (void)
{
	int	ii;
	unsigned long** prcb = (unsigned long**) get_prcbptr();
	unsigned long* syscall_tab = prcb[5];
	unsigned long* fault_tab = prcb[0];
	extern void fault(void);

	/* faults use syscall table entry 1 (0 is for syscalls) */
	syscall_tab[13] = ((unsigned long) fault) | 0x2;
	for (ii=0; ii < 20; ii += 2) {
               /* leave trace trap to mon960; don't mess with reserved
                * fields. */
		if (ii == 2 || ii == 8 || ii == 12 || ii == 16 || ii == 18)
			continue;

		fault_tab[ii] = (1 << 2) | 2;	/* use syscall #1 */
		fault_tab[ii+1] = 0x27f;	/* magic number */
	}
}

void cfault(unsigned long* fault_rec, struct pt_regs* regs)
{
	int	ii;
	unsigned long wd = fault_rec[2];
	unsigned char type = (wd >> 16);
	unsigned char subtype = (unsigned char)(wd);
	char* nm = "<no fault>";
	int sig = 0;

	switch(type) {
		case 3:
			/* arithmetic fault */
			nm = "arithmetic error";
			sig = SIGFPE;
			break;
		case 5:
			/* constraint error? */
			nm = "constraint error";
			break;
		case 2:
			switch(subtype) {
				case 1:
					nm = "illegal instruction";
					sig = SIGILL;
					break;
				case 2:
					nm = "accessed bad address";
					sig = SIGSEGV;
					break;
				case 3:
					nm = "unaligned";
					sig = SIGBUS;
					break;
				case 4:
					nm = "invalid operand";
					sig = SIGILL;
					break;
			}
			break;
		case 7:
			nm = "protection fault";
			sig = SIGILL;
			break;
		case 1:
			nm = "trace fault";
			/* bugger: we need to do better tracing */
			break;

		case 0xa:
			nm = "type.mismatch";
			sig = SIGILL;
			break;
	}

	if (user_mode(regs)) {
		extern void leave_kernel(struct pt_regs* regs);
		if (sig) {
			current-> signal |= (1 << (sig -1));
		}
		leave_kernel(regs);
		return;
	}

	cli();
	/* Amuse the user. */
	printk("\n"
"              \\|/ ____ \\|/\n"
"              \"@'/ ,. \\`@\"\n"
"              /_| \\__/ |_\\\n"
"                 \\__U_/\n");

	printk("kernel fault: %s\n", nm);
	for (ii=0; ii < 4; ii++)
		printk("fault_rec: 0x%8x\n", fault_rec[ii]);

	show_regs(regs);
	stack_trace();
	
#ifdef CONFIG_MON960
	system_break();
#else
	for (;;) ;
#endif
}

void die_if_kernel(char* msg, struct pt_regs* regs, long err)
{
	if (user_mode(regs))
		return;

	/* Amuse the user. */
	printk(
"              \\|/ ____ \\|/\n"
"              \"@'/ ,. \\`@\"\n"
"              /_| \\__/ |_\\\n"
"                 \\__U_/\n");

	printk("%s: %04lx\n", msg, err & 0xffff);
	show_regs(regs);
	stack_trace();
	
	cli();
	for (;;) ;
}

/*
 * Print a quick n' dirty stack trace
 */
struct frameo {
	struct frameo*	pfp;
	unsigned long	sp;
	void*		rip;
};

void stack_trace(void)
{
	unsigned long fp;
	
	__asm__ __volatile__ ("flushreg; mov	pfp, %0" : "=r"(fp));
	stack_trace_from(fp);
}

void stack_trace_from(unsigned long ulfp)
{
	int	ii;
	long	flags;
	struct frameo* fp = (struct frameo*) ulfp;
	
	save_flags(flags);
	cli();
	printk("trace: fp == %p\n", fp);
	for (ii=1; fp && (ii <  50); ii++, fp=fp->pfp) {
		unsigned long type = ((unsigned long)fp) & 0xf;
		fp = ((unsigned long)fp) & ~0xf;
		
		switch(type) {
			case	1:
				printk("<f>");
				break;
				
			case 2:
			case 3:
				printk("<s>");
				break;
			case 7:
				printk("<i>");
		}
		printk("\t%8x", fp->rip);
		if (! (ii & 0x3))
			printk("\n");
	}
	
	if (!fp) {
		printk("<bottom of stack>\n");
	}
	
	printk("\n");
	restore_flags(flags);
}

void ckpt(int num, unsigned long val)
{
	
	printk("--------reached ckpt: %d\tval:0x%8x\n", num, val);
#if 0
	struct pt_regs* regs;
	asm("flushreg; subo 16, fp, %0" : "=r"(regs));

	show_regs(regs);
#endif
	printk("current stack trace:\n");
	stack_trace();
	printk("new stack strace:\n");
	stack_trace_from(val);
	printk("offset of ksp: 0x%8x\n", 
	       &(((struct task_struct*)0)->kernel_stack_page));
	printk("current ksp: 0x%8x\n", current->kernel_stack_page);
}

void system_break(void)
{
	long	flags;

	save_flags(flags);
	cli();
	mon_entry();
	restore_flags(flags);
}
