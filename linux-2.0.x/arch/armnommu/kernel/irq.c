/*
 *  linux/arch/arm/kernel/irq.c
 *
 *  Copyright (C) 1992 Linus Torvalds
 *  Modifications for ARM processor Copyright (C) 1995, 1996 Russell King.
 *
 * This file contains the code used by various IRQ handling routines:
 * asking for different IRQ's should be done through these routines
 * instead of just grabbing them. Thus setups with different IRQ numbers
 * shouldn't result in any weird surprises, and installing new handlers
 * should be easier.
 */

/*
 * IRQ's are in fact implemented a bit like signal handlers for the kernel.
 * Naturally it's not a 1:1 relation, but there are similarities.
 */
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/kernel_stat.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/malloc.h>
#include <linux/random.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/irq.h>

#include <asm/arch/irq.h>

void disable_irq(unsigned int irq_nr)
{
	unsigned long flags;

#ifdef cliIF
	save_flags(flags);
	cliIF();
#else
	save_flags_cli (flags);
#endif
	mask_irq(irq_nr);
	restore_flags(flags);
}

void enable_irq(unsigned int irq_nr)
{
	unsigned long flags;

#ifdef cliIF
	save_flags (flags);
	cliIF();
#else
	save_flags_cli (flags);
#endif
	unmask_irq(irq_nr);
	restore_flags(flags);
}

BUILD_IRQ(FIRST,0,0x01)
BUILD_IRQ(FIRST,1,0x02)
BUILD_IRQ(FIRST,2,0x04)
BUILD_IRQ(FIRST,3,0x08)
BUILD_IRQ(FIRST,4,0x10)
BUILD_IRQ(FIRST,5,0x20)
BUILD_IRQ(FIRST,6,0x40)
BUILD_IRQ(FIRST,7,0x80)
BUILD_IRQ(SECOND,8,0x01)
BUILD_IRQ(SECOND,9,0x02)
BUILD_IRQ(SECOND,10,0x04)
BUILD_IRQ(SECOND,11,0x08)
BUILD_IRQ(SECOND,12,0x10)
BUILD_IRQ(SECOND,13,0x20)
BUILD_IRQ(SECOND,14,0x40)
BUILD_IRQ(SECOND,15,0x80)
BUILD_IRQ(SECOND,16,0x01)
BUILD_IRQ(SECOND,17,0x02)
BUILD_IRQ(SECOND,18,0x04)
BUILD_IRQ(SECOND,19,0x08)
BUILD_IRQ(SECOND,20,0x10)
BUILD_IRQ(SECOND,21,0x20)
BUILD_IRQ(SECOND,22,0x40)
BUILD_IRQ(SECOND,23,0x80)
BUILD_IRQ(SECOND,24,0x10)
BUILD_IRQ(SECOND,25,0x20)
BUILD_IRQ(SECOND,26,0x40)
BUILD_IRQ(SECOND,27,0x80)
BUILD_IRQ(SECOND,28,0x10)
BUILD_IRQ(SECOND,29,0x20)
BUILD_IRQ(SECOND,30,0x40)
BUILD_IRQ(SECOND,31,0x80)

/*
 * Pointers to the low-level handlers: first the general ones, then the
 * fast ones, then the bad ones.
 */
static void (* const interrupt[32])(void) = {
    IRQ_INTERRUPT( 0), IRQ_INTERRUPT( 1), IRQ_INTERRUPT( 2), IRQ_INTERRUPT( 3),
    IRQ_INTERRUPT( 4), IRQ_INTERRUPT( 5), IRQ_INTERRUPT( 6), IRQ_INTERRUPT( 7),
    IRQ_INTERRUPT( 8), IRQ_INTERRUPT( 9), IRQ_INTERRUPT(10), IRQ_INTERRUPT(11),
    IRQ_INTERRUPT(12), IRQ_INTERRUPT(13), IRQ_INTERRUPT(14), IRQ_INTERRUPT(15),
    IRQ_INTERRUPT(16), IRQ_INTERRUPT(17), IRQ_INTERRUPT(18), IRQ_INTERRUPT(19),
    IRQ_INTERRUPT(20), IRQ_INTERRUPT(21), IRQ_INTERRUPT(22), IRQ_INTERRUPT(23),
    IRQ_INTERRUPT(24), IRQ_INTERRUPT(25), IRQ_INTERRUPT(26), IRQ_INTERRUPT(27),
    IRQ_INTERRUPT(28), IRQ_INTERRUPT(29), IRQ_INTERRUPT(30), IRQ_INTERRUPT(31)
};

static void (* const fast_interrupt[32])(void) = {
    FAST_INTERRUPT( 0), FAST_INTERRUPT( 1), FAST_INTERRUPT( 2), FAST_INTERRUPT( 3),
    FAST_INTERRUPT( 4), FAST_INTERRUPT( 5), FAST_INTERRUPT( 6), FAST_INTERRUPT( 7),
    FAST_INTERRUPT( 8), FAST_INTERRUPT( 9), FAST_INTERRUPT(10), FAST_INTERRUPT(11),
    FAST_INTERRUPT(12), FAST_INTERRUPT(13), FAST_INTERRUPT(14), FAST_INTERRUPT(15),
    FAST_INTERRUPT(16), FAST_INTERRUPT(17), FAST_INTERRUPT(18), FAST_INTERRUPT(19),
    FAST_INTERRUPT(20), FAST_INTERRUPT(21), FAST_INTERRUPT(22), FAST_INTERRUPT(23),
    FAST_INTERRUPT(24), FAST_INTERRUPT(25), FAST_INTERRUPT(26), FAST_INTERRUPT(27),
    FAST_INTERRUPT(28), FAST_INTERRUPT(29), FAST_INTERRUPT(30), FAST_INTERRUPT(31)
};

static void (* const bad_interrupt[32])(void) = {
    BAD_INTERRUPT( 0), BAD_INTERRUPT( 1), BAD_INTERRUPT( 2), BAD_INTERRUPT( 3),
    BAD_INTERRUPT( 4), BAD_INTERRUPT( 5), BAD_INTERRUPT( 6), BAD_INTERRUPT( 7),
    BAD_INTERRUPT( 8), BAD_INTERRUPT( 9), BAD_INTERRUPT(10), BAD_INTERRUPT(11),
    BAD_INTERRUPT(12), BAD_INTERRUPT(13), BAD_INTERRUPT(14), BAD_INTERRUPT(15),
    BAD_INTERRUPT(16), BAD_INTERRUPT(17), BAD_INTERRUPT(18), BAD_INTERRUPT(19),
    BAD_INTERRUPT(20), BAD_INTERRUPT(21), BAD_INTERRUPT(22), BAD_INTERRUPT(23),
    BAD_INTERRUPT(24), BAD_INTERRUPT(25), BAD_INTERRUPT(26), BAD_INTERRUPT(27),
    BAD_INTERRUPT(28), BAD_INTERRUPT(29), BAD_INTERRUPT(30), BAD_INTERRUPT(31)
};

static void (* const probe_interrupt[32])(void) = {
    PROBE_INTERRUPT( 0), PROBE_INTERRUPT( 1), PROBE_INTERRUPT( 2), PROBE_INTERRUPT( 3),
    PROBE_INTERRUPT( 4), PROBE_INTERRUPT( 5), PROBE_INTERRUPT( 6), PROBE_INTERRUPT( 7),
    PROBE_INTERRUPT( 8), PROBE_INTERRUPT( 9), PROBE_INTERRUPT(10), PROBE_INTERRUPT(11),
    PROBE_INTERRUPT(12), PROBE_INTERRUPT(13), PROBE_INTERRUPT(14), PROBE_INTERRUPT(15),
    PROBE_INTERRUPT(16), PROBE_INTERRUPT(17), PROBE_INTERRUPT(18), PROBE_INTERRUPT(19),
    PROBE_INTERRUPT(20), PROBE_INTERRUPT(21), PROBE_INTERRUPT(22), PROBE_INTERRUPT(23),
    PROBE_INTERRUPT(24), PROBE_INTERRUPT(25), PROBE_INTERRUPT(26), PROBE_INTERRUPT(27),
    PROBE_INTERRUPT(28), PROBE_INTERRUPT(29), PROBE_INTERRUPT(30), PROBE_INTERRUPT(31)
};

/*
 * Initial irq handlers.
 */

static void no_action (int irq, void *dev_id, struct pt_regs *regs)
{
}

struct irqaction *irq_action[NR_IRQS];

unsigned long validirqs[4] = {
#ifdef	CONFIG_ARCH_NETARM
	0xffffffff,
#else
	0x003fffff,
#endif
	0x000001ff,
	0x000000ff,
	0x00000000
};

void (*irqjump[NR_IRQS])(void) = {
    PROBE_INTERRUPT( 0), PROBE_INTERRUPT( 1), PROBE_INTERRUPT( 2), PROBE_INTERRUPT( 3),
    PROBE_INTERRUPT( 4), PROBE_INTERRUPT( 5), PROBE_INTERRUPT( 6), PROBE_INTERRUPT( 7),
    PROBE_INTERRUPT( 8), PROBE_INTERRUPT( 9), PROBE_INTERRUPT(10), PROBE_INTERRUPT(11),
    PROBE_INTERRUPT(12), PROBE_INTERRUPT(13), PROBE_INTERRUPT(14), PROBE_INTERRUPT(15),
    PROBE_INTERRUPT(16), PROBE_INTERRUPT(17), PROBE_INTERRUPT(18), PROBE_INTERRUPT(19),
    PROBE_INTERRUPT(20), PROBE_INTERRUPT(21), PROBE_INTERRUPT(22), PROBE_INTERRUPT(23),
    PROBE_INTERRUPT(24), PROBE_INTERRUPT(25), PROBE_INTERRUPT(26), PROBE_INTERRUPT(27),
    PROBE_INTERRUPT(28), PROBE_INTERRUPT(29), PROBE_INTERRUPT(30), PROBE_INTERRUPT(31)
};

int get_irq_list(char *buf)
{
	int i, len = 0;
	struct irqaction * action;

	for (i = 0 ; i < NR_IRQS ; i++) {
	    	action = irq_action[i];
		if (!action)
			continue;
		len += sprintf(buf+len, "%2d: %10u %c %s",
				i, kstat.interrupts[i],
				(action->flags & SA_INTERRUPT) ? '+' : ' ',
				action->name);
		for (action = action->next; action; action = action->next) {
			len += sprintf(buf+len, ",%s %s",
			    (action->flags & SA_INTERRUPT) ? " +" : "",
			    action->name);
		}
		len += sprintf(buf+len, "\n");
	}
	return len;
}

/*
 * do_IRQ handles IRQ's that have been installed without the
 * SA_INTERRUPT flag: it uses the full signal-handling return
 * and runs with other interrupts enabled. All relatively slow
 * IRQ's should use this format: notably the keyboard/timer
 * routines.
 */
asmlinkage void do_IRQ(int irq, struct pt_regs * regs)
{
	struct irqaction * action = *(irq + irq_action);
	int do_random = 0;

	kstat.interrupts[irq]++;

	while (action) {
	        do_random |= action->flags;
		action->handler(irq, action->dev_id, regs);
		action = action->next;
	}
	if (do_random & SA_SAMPLE_RANDOM)
		add_interrupt_randomness(irq);
}

/*
 * do_fast_IRQ handles IRQ's that don't need the fancy interrupt return
 * stuff - the handler is also running with interrupts disabled unless
 * it explicitly enables them later.
 */
asmlinkage void do_fast_IRQ(int irq)
{
	struct irqaction * action = *(irq + irq_action);
	int do_random = 0;

	kstat.interrupts[irq]++;

	while (action) {
		do_random |= action->flags;
		action->handler(irq, action->dev_id, NULL);
		action = action->next;
	}
	if (do_random & SA_SAMPLE_RANDOM)
		add_interrupt_randomness(irq);
}

#define SA_PROBE SA_ONESHOT

int setup_arm_irq(int irq, struct irqaction * new)
{
	int shared = 0;
	struct irqaction *old, **p;
	unsigned long flags;

	p = irq_action + irq;

	if ((old = *p) != NULL) {
		/* Can't share interrupts unless both agree to */
		if (!(old->flags & new->flags & SA_SHIRQ))
			return -EBUSY;

		/* Can't share interrupts unless both are same type */
		if ((old->flags ^ new->flags) & SA_INTERRUPT)
			return -EBUSY;

		/* add new interrupt at end of irq queue */
		do {
			p = &old->next;
			old = *p;
		} while (old);
		shared = 1;
	}

	if (new->flags & SA_SAMPLE_RANDOM)
	        rand_initialize_irq(irq);

	save_flags_cli(flags);

	*p = new;

	if (!shared) {
		if (irq < 32) {
			if (!(new->flags & SA_PROBE)) { /* SA_ONESHOT is used by probing */
				if (new->flags & SA_INTERRUPT)
					irqjump[irq] = fast_interrupt[irq];
				else
					irqjump[irq] = interrupt[irq];
			} else
				irqjump[irq] = probe_interrupt[irq];
		}
		unmask_irq(irq);
	}
	restore_flags(flags);
	return 0;
}

/*
 * Using "struct sigaction" is slightly silly, but there
 * are historical reasons and it works well, so..
 */
int request_irq(unsigned int irq, void (*handler)(int, void *, struct pt_regs *),
		 unsigned long irq_flags, const char * devname, void *dev_id)
{
	unsigned long retval;
	struct irqaction *action;
        
	if (irq >= NR_IRQS || !(validirqs[irq >> 5] & (1 << (irq & 31))))
		return -EINVAL;
	if (!handler)
		return -EINVAL;

	action = (struct irqaction *)kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action)
		return -ENOMEM;

	action->handler = handler;
	action->flags = irq_flags;
	action->mask = 0;
	action->name = devname;
	action->next = NULL;
	action->dev_id = dev_id;

	retval = setup_arm_irq(irq, action);

	if (retval)
		kfree(action);
	return retval;
}

void free_irq(unsigned int irq, void *dev_id)
{
	struct irqaction * action, **p;
	unsigned long flags;

	if (irq >= NR_IRQS || !(validirqs[irq >> 5] & (1 << (irq & 31)))) {
		printk (KERN_ERR "Trying to free IRQ%d\n",irq);
#ifdef CONFIG_DEBUG_ERRORS
		__backtrace();
#endif
		return;
	}

	for (p = irq + irq_action; (action = *p) != NULL; p = &action->next) {
		if (action->dev_id != dev_id)
			continue;

	    	/* Found it - now free it */
		save_flags_cli (flags);
		*p = action->next;
		if (!irq_action[irq]) {
			mask_irq(irq);
			if (irq < 32)
				irqjump[irq] = bad_interrupt[irq];
		}
		restore_flags (flags);
		kfree(action);
		return;
	}
	printk(KERN_ERR "Trying to free free IRQ%d\n",irq);
#ifdef CONFIG_DEBUG_ERRORS
	__backtrace();
#endif
}

unsigned long probe_irq_on (void)
{
	unsigned int i, irqs = 0, irqmask;
	unsigned long delay;

	/* first snaffle up any unassigned irqs */
	for (i = 15; i > 0; i--) {
		if (!request_irq (i, no_action, SA_PROBE, "probe", NULL))
			irqs |= 1 << i;
	}

	/* wait for spurious interrupts to mask themselves out again */
	for (delay = jiffies + 2; delay > jiffies; ); /* min 10ms delay */

	/* now filter out any obviously spurious interrupts */
	irqmask = ~get_enabled_irqs();
	for (i = 15; i > 0; i--) {
		if (irqs & (1 << i) & irqmask) {
			irqs ^= 1 << i;
			free_irq (i, NULL);
		}
	}
	return irqs;
}

int probe_irq_off (unsigned long irqs)
{
	unsigned int i, irqmask;

	irqmask = ~get_enabled_irqs();

	for (i = 15; i > 0; i--) {
		if (irqs & (1 << i))
			free_irq (i, NULL);
	}

	irqs &= irqmask;
	if (!irqs)
		return 0;
	i = ffz (~irqs);
	if (irqs != (irqs & (1 << i)))
		i = -i;
	return i;
}

void init_IRQ(void)
{
	irq_init_irq();
}
