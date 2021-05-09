/*
 * linux/arch/h8300/boot/traps.c -- general exception handling code
 *
 * Cloned from Linux/m68k.
 *
 * No original Copyright holder listed,
 * Probabily original (C) Roman Zippel (assigned DJD, 1999)
 *
 * Copyright 1999-2000 D. Jeff Dionne, <jeff@rt-control.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/errno.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/page.h>

/* table for system interrupt handlers */
static irq_handler_t irq_list[SYS_IRQS];

/* The number of spurious interrupts */
volatile unsigned int num_spurious;

#define NUM_IRQ_NODES 64
static irq_node_t nodes[NUM_IRQ_NODES];

extern int ptrace_cancel_bpt(struct task_struct *child);

void trap_init (void)
{
}

static inline void console_verbose(void)
{
	extern int console_loglevel;
	console_loglevel = 15;
}

/*
 * void init_IRQ(void)
 *
 * Parameters:	None
 *
 * Returns:	Nothing
 *
 * This function should be called during kernel startup to initialize
 * the IRQ handling routines.
 */

void init_IRQ(void)
{
	int i;

	for (i = 0; i < SYS_IRQS; i++) {
		irq_list[i].handler = NULL;
		irq_list[i].flags   = IRQ_FLG_STD;
		irq_list[i].devname = NULL;
		irq_list[i].dev_id  = NULL;
	}

	for (i = 0; i < NUM_IRQ_NODES; i++)
		nodes[i].handler = NULL;
        
}

irq_node_t *new_irq_node(void)
{
	irq_node_t *node;
	short i;

	for (node = nodes, i = NUM_IRQ_NODES-1; i >= 0; node++, i--)
		if (!node->handler)
			return node;

	printk ("new_irq_node: out of nodes\n");
	return NULL;
}

int request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
                unsigned long flags, const char *devname, void *dev_id)
{
	irq_list[irq].handler = handler;
	irq_list[irq].flags   = flags;
	irq_list[irq].devname = devname;
	irq_list[irq].dev_id  = dev_id;
	return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
	if (irq_list[irq].dev_id != dev_id)
		printk("%s: Removing probably wrong IRQ %d from %s\n",
		       __FUNCTION__, irq, irq_list[irq].devname);

	irq_list[irq].handler = NULL;
	irq_list[irq].flags   = IRQ_FLG_STD;
	irq_list[irq].dev_id  = NULL;
	irq_list[irq].devname = NULL;
}

/*
 * Do we need these probe functions on the m68k?
 */
unsigned long probe_irq_on (void)
{
	return 0;
}

int probe_irq_off (unsigned long irqs)
{
	return 0;
}

void enable_irq(unsigned int irq)
{
}

void disable_irq(unsigned int irq)
{
}

asmlinkage void process_int(unsigned long vec, struct pt_regs *fp)
{
	kstat.interrupts[vec]++;
	if (irq_list[vec].handler)
		irq_list[vec].handler(vec, irq_list[vec].dev_id, fp);
	else
		panic("No interrupt handler for %ld\n", vec);
}

int get_irq_list(char *buf)
{
	int i, len = 0;

	/* autovector interrupts */
	for (i = 0; i < SYS_IRQS; i++) {
		if (irq_list[i].handler) {
			len += sprintf(buf+len, "auto %2d: %10u ", i,
			               i ? kstat.interrupts[i] : num_spurious);
			if (irq_list[i].flags & IRQ_FLG_LOCK)
				len += sprintf(buf+len, "L ");
			else
				len += sprintf(buf+len, "  ");
			len += sprintf(buf+len, "%s\n", irq_list[i].devname);
		}
	}
	return 0;
}

/*
 *	Generic dumping code. Used for panic and debug.
 */

void dump(struct pt_regs *fp)
{
	unsigned long	*sp;
	unsigned char	*tp;
	int		i;

	printk("\nCURRENT PROCESS:\n\n");
#if 0
{
	extern int	swt_lastjiffies, swt_reference;
	printk("WATCHDOG: jiffies=%d lastjiffies=%d [%d] reference=%d\n",
		jiffies, swt_lastjiffies, (swt_lastjiffies - jiffies),
		swt_reference);
}
#endif
	printk("COMM=%s PID=%d\n", current->comm, current->pid);
	if (current->mm) {
		printk("TEXT=%08x-%08x DATA=%08x-%08x BSS=%08x-%08x\n",
			(int) current->mm->start_code,
			(int) current->mm->end_code,
			(int) current->mm->start_data,
			(int) current->mm->end_data,
			(int) current->mm->end_data,
			(int) current->mm->brk);
		printk("USER-STACK=%08x  KERNEL-STACK=%08x\n\n",
			(int) current->mm->start_stack,
			(int) current->kernel_stack_page);
	}

	printk("PC: %08lx\n", (long)fp->pc);
	printk("CCR: %08lx   SP: %08lx\n", fp->ccr, (long) fp);
	printk("ER0: %08lx  ER1: %08lx   ER2: %08lx   ER3: %08lx\n",
		fp->er0, fp->er1, fp->er2, fp->er3);
	printk("ER4: %08lx  ER5: %08lx",
		fp->er4, fp->er5);
	printk("\nCODE:");
	tp = ((unsigned char *) fp->pc) - 0x20;
	for (sp = (unsigned long *) tp, i = 0; (i < 0x40);  i += 4) {
		if ((i % 0x10) == 0)
			printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n");

	printk("\nKERNEL STACK:");
	tp = ((unsigned char *) fp) - 0x40;
	for (sp = (unsigned long *) tp, i = 0; (i < 0xc0); i += 4) {
		if ((i % 0x10) == 0)
			printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n");
	if (STACK_MAGIC != *(unsigned long *)current->kernel_stack_page)
                printk("(Possibly corrupted stack page??)\n");

	printk("\n\n");
}

asmlinkage void set_esp0 (unsigned long ssp)
{
  current->tss.esp0 = ssp;
}

void die_if_kernel (char *str, struct pt_regs *fp, int nr)
{
	if (!(fp->ccr & PS_S))
		return;

	console_verbose();
	dump(fp);

	do_exit(SIGSEGV);
}

asmlinkage void trace_trap(unsigned long bp)
{
	if (current->debugreg[0] == bp ||
            current->debugreg[1] == bp) {
	        ptrace_cancel_bpt(current);
		force_sig(SIGTRAP,current);
	} else
	        force_sig(SIGILL,current);
}
