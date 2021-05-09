/*
 * linux/arch/$(ARCH)/platform/$(PLATFORM)/traps.c -- general exception handling code
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
 *
 * Modified by Anurag 12/8/02
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/errno.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/page.h>
#include <asm/machdep.h>

/*
 * posh2 - FOr the time being defining these variables
 * need to remove this when linker script is working
 * properly
 */

 
unsigned int sw_usp=0;
unsigned int sw_ksp=0;

/* table for system interrupt handlers */
static irq_handler_t irq_list[SYS_IRQS];

/*static const char *default_names[SYS_IRQS] = {
	"spurious int", "int1 handler", "int2 handler", "int3 handler",
	"int4 handler", "int5 handler", "int6 handler", "int7 handler"
};
*/

/* The number of spurious interrupts */
volatile unsigned int num_spurious;

/*Posh2 Same as NR_IRQs 59*/
#define NUM_IRQ_NODES  59

static irq_node_t nodes[NUM_IRQ_NODES];


/* Posh 2 the IVT in the case of SH2 will be defined in 
 * entry.s */

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
	unsigned char  i;

	for (i = 0; i < SYS_IRQS; i++) {
		irq_list[i].handler = NULL;
		irq_list[i].flags   = IRQ_FLG_STD;
		irq_list[i].dev_id  = NULL;
		irq_list[i].devname = NULL;
	}

	for (i = 0; i < NUM_IRQ_NODES; i++)
		nodes[i].handler = NULL;

      //mach_init_IRQ ();
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

/* posh2 Removed the mach specific request_irq */

/*	if (irq & IRQ_MACHSPEC)
	  return mach_request_irq(IRQ_IDX(irq), handler, flags, devname, dev_id);
	if (irq < IRQ1 || irq > IRQ7) {
		printk("%s: Incorrect IRQ %d from %s\n", __FUNCTION__, irq, devname);
		return -ENXIO;
	}

	if (!(irq_list[irq].flags & IRQ_FLG_STD)) {
		if (irq_list[irq].flags & IRQ_FLG_LOCK) {
			printk("%s: IRQ %d from %s is not replaceable\n",
			       __FUNCTION__, irq, irq_list[irq].devname);
			return -EBUSY;
		}
		if (flags & IRQ_FLG_REPLACE) {
			printk("%s: %s can't replace IRQ %d from %s\n",
			       __FUNCTION__, devname, irq, irq_list[irq].devname);
			return -EBUSY;
		}
	}
*/
}

void free_irq(unsigned int irq, void *dev_id)
{


/* Posh2 removed mach dependaant IRQ */

/*	if (irq & IRQ_MACHSPEC) {
		mach_free_irq(IRQ_IDX(irq), dev_id);
		return;
	}

	if (irq < IRQ1 || irq > IRQ7) {
		printk("%s: Incorrect IRQ %d\n", __FUNCTION__, irq);
		return;
	} */

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
	
/* Posh removed machine specfic portion*/
/*	if ((irq & IRQ_MACHSPEC) && mach_enable_irq)
		mach_enable_irq(IRQ_IDX(irq));
*/
}

void disable_irq(unsigned int irq)
{
/*	if ((irq & IRQ_MACHSPEC) && mach_disable_irq)
		mach_disable_irq(IRQ_IDX(irq));
*/
}

asmlinkage void process_int(unsigned long vec, struct pt_regs *fp)
{
	/* give the machine specific code a crack at it first */
/*Posh2 Removed machine specific portion */
/*	if (mach_process_int)
		if (!mach_process_int(vec, fp))
			return;

	if (vec < VEC_SPUR || vec > VEC_INT7)
		panic("No interrupt handler for vector %ld\n", vec);
	vec -= VEC_SPUR;
*/


	kstat.interrupts[vec]++;
	if (irq_list[vec].handler)
		irq_list[vec].handler(vec, irq_list[vec].dev_id, fp);
	else
		panic("No interrupt handler for autovector %ld\n", vec);
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
/*Posh2 removed Mach specific part*/
/*	if (mach_get_irq_list)
		len += mach_get_irq_list(buf+len);
*/
	return len;

}

/*
 *	Generic dumping code. Used for panic and debug.
 */

void dump(struct pt_regs *fp)
{

	unsigned long	*sp;
	unsigned char	*tp;
	int		i;

	printk("\n\rCURRENT PROCESS:");
/* Posh2 if required change this function later */
#if 0
{
	extern int	swt_lastjiffies, swt_reference;
	printk("WATCHDOG: jiffies=%d lastjiffies=%d [%d] reference=%d\n",
		jiffies, swt_lastjiffies, (swt_lastjiffies - jiffies),
		swt_reference);
}
#endif
	printk("COMM=%s PID=%d\r\n", current->comm, current->pid);
/*	if (current->mm) {
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
	}*/

	printk("\r\nPC: %08lx", fp->pc);
	printk("\r\nSR: %08lx     PR: %08lx", (long) fp->sr, (long) fp->pr);
	printk("\r\nPC: %08lx" , (long) fp->pc);
	printk("\r\nUSP: %08x", current->tss.usp);
	printk("\r\nKSP: %08x", current->tss.ksp);
	printk("\r\nSTACKPAGE: %08x",(long) current->kernel_stack_page);
	printk("\r\nSYSTEMUPTIME %08x ms", jiffies*10);
	
	 printk("\r\n TEST PURPOSE : NOT RECOVERING FROM TRAP");
	 cli();
	 while (1)
	;
	

	/*printk("r0: %08lx    r1: %08lx    r2: %08lx    r3: %08lx\n",
		fp->r0, fp->r1, fp->r2, fp->r3);
	printk("r4: %08lx    r5: %08lx    r6: %08lx    r7: %08lx\n",
		fp->r4, fp->r5, fp->r6, fp->r7);
	printk("\nUSP: %08x   TRAPFRAME: %08x\n",
		rdusp(), (unsigned int) fp);

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
	printk("\n");

	printk("\nUSER STACK:");
	tp = (unsigned char *) (rdusp() - 0x10);
	for (sp = (unsigned long *) tp, i = 0; (i < 0x80); i += 4) {
		if ((i % 0x10) == 0)
			printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n\n");*/
	
	
}
