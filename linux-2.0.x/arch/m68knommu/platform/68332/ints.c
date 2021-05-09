/*
 * linux/arch/m68knommu/platform/68332/ints.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2000 Michael Leslie <mleslie@lineo.com>
 * Copyright (c) 1996 Roman Zippel
 * Copyright (c) 1999 D. Jeff Dionne <jeff@uclinux.org>
 *
 * Cloned from linux/arch/m68knommu/platform/68360/quicc_ints.c
 * Gerold Boehler <gboehler@mail.austria.at> 
 *
 */
 
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/errno.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/setup.h>
#include <asm/MC68332.h>

/* assembler routines */
asmlinkage void system_call(void);
asmlinkage void buserr(void);
asmlinkage void trap(void);
asmlinkage void bad_interrupt(void);
asmlinkage void inthandler(void);

extern void *_ramvec[];

static irq_node_t *int_irq_list[NR_IRQS];

volatile unsigned int num_spurious;

#define NUM_IRQ_NODES 100
static irq_node_t nodes[NUM_IRQ_NODES];

static int int_irq_count[NR_IRQS];
static short int_irq_ablecount[NR_IRQS];

/*
 * This function should be called during kernel startup to initialize
 * IRQ handling routines.
 */

void init_IRQ(void)
{
        int i;

	/* set up the vectors */
#ifdef CONFIG_RAM_05MB
	_ramvec[0] = (void *)0x180000;
#else
	_ramvec[0] = (void *)0x200000;
#endif

#ifdef CONFIG_RAMKERNEL
	_ramvec[1] = (void *)0x100400;
#else
	_ramvec[1] = (void *)0x000400;
#endif
	_ramvec[VEC_BUSERR] = buserr;

	for(i = 3; i < 16; i++)
		_ramvec[i] = trap;

	_ramvec[24] = bad_interrupt;
	_ramvec[25] = inthandler;
	_ramvec[26] = inthandler;
	_ramvec[27] = inthandler;
	_ramvec[28] = inthandler;
	_ramvec[29] = inthandler;
	_ramvec[30] = inthandler;
	_ramvec[31] = inthandler;
	_ramvec[VEC_SYS] = system_call;

	for(i = 33; i < 48; i++)
		_ramvec[i] = trap;

	for ( i = 64; i < NR_IRQS; i++)
		_ramvec[i] = inthandler;	
	
        for (i = 0; i < NR_IRQS; i++) {
                int_irq_list[i] = NULL;
                int_irq_ablecount[i] = 0;
                int_irq_count[i] = 0;
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

void insert_irq(irq_node_t **list, irq_node_t *node)
{
	unsigned long flags;
	irq_node_t *cur;

	if (!node->dev_id)
		printk("%s: Warning: dev_id of %s is zero\n",
		       __FUNCTION__, node->devname);

	save_flags(flags);
	cli();

	cur = *list;

	while (cur) {
		list = &cur->next;
		cur = cur->next;
	}

	node->next = cur;
	*list = node;

	restore_flags(flags);
}

void delete_irq(irq_node_t **list, void *dev_id)
{
	unsigned long flags;
	irq_node_t *node;

	save_flags(flags);
	cli();

	for (node = *list; node; list = &node->next, node = *list) {
		if (node->dev_id == dev_id) {
			*list = node->next;
			/* Mark it as free. */
			node->handler = NULL;
			restore_flags(flags);
			return;
		}
	}
	restore_flags(flags);
	printk ("%s: tried to remove invalid irq\n", __FUNCTION__);
}


int request_irq(unsigned int irq,
                void (*handler)(int, void *, struct pt_regs *),
                unsigned long flags, const char *devname, void *dev_id)
{

	irq = IRQ_IDX(irq);	

        if (irq >= NR_IRQS ) {
                printk ("%s: Unknown IRQ %d from %s\n", __FUNCTION__, irq,
                                devname);
                return -ENXIO;
        }

	if (!int_irq_list[irq]) {
                int_irq_list[irq] = new_irq_node();
                int_irq_list[irq]->flags   = IRQ_FLG_STD;
	}

	if (!(int_irq_list[irq]->flags & IRQ_FLG_STD)) {
                if (int_irq_list[irq]->flags & IRQ_FLG_LOCK) {
                        printk("%s: IRQ %d from %s is not replaceable\n",
                                        __FUNCTION__, irq,
                                        int_irq_list[irq]->devname);
                        return -EBUSY;
                }
                if (flags & IRQ_FLG_REPLACE) {
                        printk("%s: %s can't replace IRQ %d from %s\n",
                                        __FUNCTION__, devname, irq,
                                        int_irq_list[irq]->devname);
                        return -EBUSY;
                }
        }
        int_irq_list[irq]->handler = handler;
        int_irq_list[irq]->flags   = flags;
        int_irq_list[irq]->dev_id  = dev_id;
        int_irq_list[irq]->devname = devname;

        return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
	irq = IRQ_IDX(irq);
	
	if (irq >= NR_IRQS)
        {
		printk ("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_list[irq]->dev_id != dev_id)
                printk("%s: removing probably wrong IRQ %d from %s\n",
                                __FUNCTION__, irq, int_irq_list[irq]->devname);
	int_irq_list[irq]->handler = NULL;
	int_irq_list[irq]->flags   = IRQ_FLG_STD;
	int_irq_list[irq]->dev_id  = NULL;
	int_irq_list[irq]->devname = NULL;

}

/*
 * Enable/disable a particular machine specific interrupt source.
 * Note that this may affect other interrupts in case of a shared interrupt.
 * This function should only be called for a _very_ short time to change some
 * internal data, that may not be changed by the interrupt at the same time.
 * int_(enable|disable)_irq calls may also be nested.
 */

void enable_irq(unsigned int irq)
{
	irq = IRQ_IDX(irq);

	if (irq >= NR_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (--int_irq_ablecount[irq])
		return;
}

void disable_irq(unsigned int irq)
{

	irq = IRQ_IDX(irq);

	if (irq >= NR_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_ablecount[irq]++)
		return;
}

/* The 68k family did not have a good way to determine the source
 * of interrupts until later in the family.  The EC000 core does
 * not provide the vector number on the stack, we vector everything
 * into one vector and look in the blasted mask register...
 * This code is designed to be fast, almost constant time, not clean!
 */
inline int process_int(int vec, struct pt_regs *fp)
{
        int irq;
       
        irq = vec;

        if (int_irq_list[irq] && int_irq_list[irq]->handler)
        {
                int_irq_list[irq]->handler(irq | IRQ_MACHSPEC,
                                int_irq_list[irq]->dev_id, fp);
                int_irq_count[irq]++;
                return 0;
        }
	else {
		printk("unable to handle interrupt %d!\n", irq);
		return -1;
	}
}

int get_irq_list(char *buf)
{
	int i, len = 0;
	irq_node_t *node;

	len += sprintf(buf+len, "Internal 68332 interrupts\n");

	for (i = 0; i < NR_IRQS; i++) {
		if (!(node = int_irq_list[i]))
			continue;
		if (!(node->handler))
			continue;

		len += sprintf(buf+len, " %2d: %10u    %s\n", i,
		               int_irq_count[i], int_irq_list[i]->devname);
	}
	return len;
}

void config_mwi_irq(void)
{
	mach_default_handler = NULL;
	mach_init_IRQ        = init_IRQ;
	mach_request_irq     = request_irq;
	mach_free_irq        = (void*)free_irq;
	mach_enable_irq      = enable_irq;
	mach_disable_irq     = disable_irq;
	mach_get_irq_list    = get_irq_list;
	mach_process_int     = process_int;
}
