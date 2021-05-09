/*****************************************************************************/
/* 
 *  ints.c v1.0 <2003-07-28 17:50:24 gc>
 * 
 *  linux/arch/m68knommu/platform/68000/ints.c
 *
 *  uClinux version 2.0.x MC68000 interrupt handling
 *
 *  Author:     Guido Classen (classeng@clagi.de)
 *
 *  This program is free software;  you can redistribute it and/or modify it
 *  under the  terms of the GNU  General Public License as  published by the
 *  Free Software Foundation.  See the file COPYING in the main directory of
 *  this archive for more details.
 *
 *  This program  is distributed  in the  hope that it  will be  useful, but
 *  WITHOUT   ANY   WARRANTY;  without   even   the   implied  warranty   of
 *  MERCHANTABILITY  or  FITNESS FOR  A  PARTICULAR  PURPOSE.   See the  GNU
 *  General Public License for more details.
 *
 *  Thanks to:
 *    inital code taken from 68328/int.c
 *
 *    Copyright 1996 Roman Zippel
 *    Copyright 1999 D. Jeff Dionne <jeff@uclinux.org>
 *
 *  Change history:
 *       2002-05-15 G. Classen: initial version for MC68000
 *
 */
 /****************************************************************************/


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/setup.h>

#define INTERNAL_IRQS (32)

/* assembler routines */
asmlinkage void system_call(void);
asmlinkage void buserr(void);
asmlinkage void trap(void);
asmlinkage void trap3(void);
asmlinkage void trap4(void);
asmlinkage void trap5(void);
asmlinkage void trap6(void);
asmlinkage void trap7(void);
asmlinkage void trap8(void);
asmlinkage void trap9(void);
asmlinkage void trap10(void);
asmlinkage void trap11(void);
asmlinkage void trap12(void);
asmlinkage void trap13(void);
asmlinkage void trap14(void);
asmlinkage void trap15(void);
asmlinkage void trap33(void);
asmlinkage void trap34(void);
asmlinkage void trap35(void);
asmlinkage void trap36(void);
asmlinkage void trap37(void);
asmlinkage void trap38(void);
asmlinkage void trap39(void);
asmlinkage void trap40(void);
asmlinkage void trap41(void);
asmlinkage void trap42(void);
asmlinkage void trap43(void);
asmlinkage void trap44(void);
asmlinkage void trap45(void);
asmlinkage void trap46(void);
asmlinkage void trap47(void);
asmlinkage void bad_interrupt(void);
asmlinkage void inthandler(void);
asmlinkage void inthandler1(void);
asmlinkage void inthandler2(void);
asmlinkage void inthandler3(void);
asmlinkage void inthandler4(void);
asmlinkage void inthandler5(void);
asmlinkage void inthandler6(void);
asmlinkage void inthandler7(void);

extern void *_ramvec[];

/* irq node variables for the 32 (potential) on chip sources */
static irq_node_t *int_irq_list[INTERNAL_IRQS];

static int int_irq_count[INTERNAL_IRQS];
static short int_irq_ablecount[INTERNAL_IRQS];

static void int_badint(int irq, void *dev_id, struct pt_regs *fp)
{
	num_spurious += 1;
}

/*
 * This function should be called during kernel startup to initialize
 * the IRQ handling routines.
 */

static void M68000_init_IRQ(void)
{
	int i;

#if 1
        for (i=2; i<=0xff; ++i) {
          _ramvec[i] = bad_interrupt;
        }
#endif
	/* set up the vectors */
	_ramvec[VEC_BUSERR]     = buserr;
	_ramvec[VEC_ADDRERR]    = trap3;
	_ramvec[VEC_ILLEGAL]    = trap4;
	_ramvec[VEC_ZERODIV]    = trap5;
	_ramvec[VEC_CHK]        = trap6;
	_ramvec[VEC_TRAP]       = trap7;
	_ramvec[VEC_PRIV]       = trap8;
	_ramvec[VEC_TRACE]      = trap9;
	_ramvec[VEC_LINE10]     = trap10;
	_ramvec[VEC_LINE11]     = trap11;
	_ramvec[VEC_RESV1]      = trap12;
	_ramvec[VEC_COPROC]     = trap13;
	_ramvec[VEC_FORMAT]     = trap14;
	_ramvec[VEC_UNINT]      = trap15;
        
	_ramvec[VEC_INT1]       = inthandler1;
	_ramvec[VEC_INT2]       = inthandler2;
	_ramvec[VEC_INT3]       = inthandler3;
	_ramvec[VEC_INT4]       = inthandler4;
	_ramvec[VEC_INT5]       = inthandler5;
	_ramvec[VEC_INT6]       = inthandler6;
	_ramvec[VEC_INT7]       = inthandler7;
        
	_ramvec[VEC_SYS]        = system_call;

 
	/* initialize handlers */
	for (i = 0; i < INTERNAL_IRQS; i++) {
		int_irq_list[i] = NULL;

		int_irq_ablecount[i] = 0;
		int_irq_count[i] = 0;
	}
	/* turn off all interrupts */
}

void M68000_insert_irq(irq_node_t **list, irq_node_t *node)
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

void M68000_delete_irq(irq_node_t **list, void *dev_id)
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

static int 
M68000_request_irq(unsigned int irq, 
                   void (*handler)(int, void *, struct pt_regs *),
                   unsigned long flags, const char *devname, void *dev_id)
{
	if (irq >= INTERNAL_IRQS) {
		printk ("%s: Unknown IRQ %d from %s\n", 
                        __FUNCTION__, irq, devname);
		return -ENXIO;
	}

	if (!int_irq_list[irq]) {
		int_irq_list[irq] = new_irq_node();
		int_irq_list[irq]->flags   = IRQ_FLG_STD;
	}

	if (!(int_irq_list[irq]->flags & IRQ_FLG_STD)) {
		if (int_irq_list[irq]->flags & IRQ_FLG_LOCK) {
			printk("%s: IRQ %d from %s is not replaceable\n",
			       __FUNCTION__, irq, int_irq_list[irq]->devname);
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

static void M68000_free_irq(unsigned int irq, void *dev_id)
{
	if (irq >= INTERNAL_IRQS) {
		printk ("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_list[irq]->dev_id != dev_id)
		printk("%s: removing probably wrong IRQ %d from %s\n",
		       __FUNCTION__, irq, int_irq_list[irq]->devname);
	int_irq_list[irq]->handler = int_badint;
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

static void M68000_enable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (--int_irq_ablecount[irq])
		return;

	/* enable the interrupt */
}

void M68000_disable_irq(unsigned int irq)
{
	if (irq >= INTERNAL_IRQS) {
		printk("%s: Unknown IRQ %d\n", __FUNCTION__, irq);
		return;
	}

	if (int_irq_ablecount[irq]++)
		return;

	/* disable the interrupt */
}

/* The 68k family did not have a good way to determine the source
 * of interrupts until later in the family.  The 68000 core does
 * not provide the vector number on the stack, we use different 
 * entry points for the autovector interrupts in entry.S
 */
static int M68000_do_irq(int vec, struct pt_regs *fp)
{

    if (int_irq_list[vec] && int_irq_list[vec]->handler) {
        int_irq_list[vec]->handler(vec | IRQ_MACHSPEC, 
                                   int_irq_list[vec]->dev_id, fp);
        int_irq_count[vec]++;
    } else {
        printk("unregistered interrupt %d!\n", 
               vec);
    }
    return 0;
}

static int M68000_get_irq_list(char *buf)
{
    int i, len = 0;
    irq_node_t *node;

    len += sprintf(buf+len, "68000 autovector interrupts\n");

    for (i = 0; i < INTERNAL_IRQS; i++) {
        if (!(node = int_irq_list[i]))
            continue;
        if (!(node->handler))
            continue;

        len += sprintf(buf+len, " %2d: %10u    %s\n", i,
                       int_irq_count[i], int_irq_list[i]->devname);
    }
    return len;
}

void config_M68000_irq(void)
{
	mach_default_handler = NULL;
	mach_init_IRQ        = M68000_init_IRQ;
	mach_request_irq     = M68000_request_irq;
	mach_free_irq        = M68000_free_irq;
	mach_enable_irq      = M68000_enable_irq;
	mach_disable_irq     = M68000_disable_irq;
	mach_get_irq_list    = M68000_get_irq_list;
	/* mach_process_int     = M68000_do_irq; */
        mach_process_int     = NULL;
}

/*
 *Local Variables:
 * mode: c
 * c-indent-level: 8
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -8
 * c-argdecl-indent: 8
 * c-label-offset: -8
 * c-continued-statement-offset: 8
 * c-continued-brace-offset: 0
 * indent-tabs-mode: nil
 * c-file-style: "Linux"
 * fill-column: 76
 * tab-width: 8
 * time-stamp-pattern: "4/<%%>"
 * End:
 */
