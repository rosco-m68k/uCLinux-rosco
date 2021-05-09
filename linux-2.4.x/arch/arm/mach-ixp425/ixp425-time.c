/*
 * arch/arm/mach-ixp425/ixp425-time.c
 *
 * Timer tick for IXP425 based sytems. We use OS timer1 on the CPU for
 * the timer tick and the timestamp counter to account for missed jiffies.
 *
 * Author:  Peter Barry
 * Copyright:   (C) 2001 Intel Corporation.
 * 		(C) 2002-2003 MontaVista Software, Inc.
 *
 * Maintainer: Deepak Saxena <dsaxena@mvista.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/smp.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <linux/timex.h>
#include <asm/hardware.h>

extern int setup_arm_irq(int, struct irqaction *);
static unsigned volatile last_jiffy_time;

#define	USEC_PER_SEC		1000000
#define CLOCK_TICKS_PER_USEC	(CLOCK_TICK_RATE / USEC_PER_SEC)

/* IRQs are disabled before entering here from do_gettimeofday() */
static unsigned long ixp425_gettimeoffset(void)
{
	u32 elapsed, usec, curr, reload;
	volatile u32 stat1, stat2;

	elapsed = *IXP425_OSTS - last_jiffy_time;

	return elapsed / CLOCK_TICKS_PER_USEC;
}

static void ixp425_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long flags;

	/* Clear Pending Interrupt by writing '1' to it */
	*IXP425_OSST = IXP425_OSST_TIMER_1_PEND;

	/*
	 * Catch up with the real idea of time
	 */
	while((*IXP425_OSTS - last_jiffy_time) > LATCH) {
		do_timer(regs);
		last_jiffy_time += LATCH;
	}		
}

extern unsigned long (*gettimeoffset)(void);

static struct irqaction timer_irq = {
	name: "Timer Tick",
	flags: SA_INTERRUPT
};

void __init setup_timer(void)
{
	gettimeoffset = ixp425_gettimeoffset;
	timer_irq.handler = ixp425_timer_interrupt;

	/* Clear Pending Interrupt by writing '1' to it */
	*IXP425_OSST = IXP425_OSST_TIMER_1_PEND;

	/* Setup the Timer counter value */
	*IXP425_OSRT1 = (LATCH & ~IXP425_OST_RELOAD_MASK) | IXP425_OST_ENABLE;

	/* Reset time-stamp counter */
	*IXP425_OSTS = 0;
	last_jiffy_time = 0;

	/* Connect the interrupt handler and enable the interrupt */
	setup_arm_irq(IRQ_IXP425_TIMER1, &timer_irq);
}


