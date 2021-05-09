/*
 * include/asm-armnommu/arch-netarm/irq.h
 *
 * Copyright (C) 2000 NETsilicon, Inc.
 * Copyright (C) 2000 WireSpeed Communications Corporation
 *
 * This software is copyrighted by WireSpeed. LICENSEE agrees that
 * it will not delete this copyright notice, trademarks or protective
 * notices from any copy made by LICENSEE.
 *
 * This software is provided "AS-IS" and any express or implied 
 * warranties or conditions, including but not limited to any
 * implied warranties of merchantability and fitness for a particular
 * purpose regarding this software. In no event shall WireSpeed
 * be liable for any indirect, consequential, or incidental damages,
 * loss of profits or revenue, loss of use or data, or interruption
 * of business, whether the alleged damages are labeled in contract,
 * tort, or indemnity.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * author(s) : Joe deBlaquiere
 */

#include	<asm/arch/netarm_registers.h>

#define BUILD_IRQ(s,n,m)

extern void IRQ_interrupt(void);
extern void timer_IRQ_interrupt(void);
extern void fast_IRQ_interrupt(void);
extern void bad_IRQ_interrupt(void);
extern void probe_IRQ_interrupt(void);

#define IRQ_interrupt0 IRQ_interrupt
#define IRQ_interrupt1 IRQ_interrupt
#define IRQ_interrupt2 IRQ_interrupt
#define IRQ_interrupt3 IRQ_interrupt
#define IRQ_interrupt4 timer_IRQ_interrupt
#define IRQ_interrupt5 IRQ_interrupt
#define IRQ_interrupt6 IRQ_interrupt
#define IRQ_interrupt7 IRQ_interrupt
#define IRQ_interrupt8 IRQ_interrupt
#define IRQ_interrupt9 IRQ_interrupt
#define IRQ_interrupt10 IRQ_interrupt
#define IRQ_interrupt11 IRQ_interrupt
#define IRQ_interrupt12 IRQ_interrupt
#define IRQ_interrupt13 IRQ_interrupt
#define IRQ_interrupt14 IRQ_interrupt
#define IRQ_interrupt15 IRQ_interrupt
#define IRQ_interrupt16 IRQ_interrupt
#define IRQ_interrupt17 IRQ_interrupt
#define IRQ_interrupt18 IRQ_interrupt
#define IRQ_interrupt19 IRQ_interrupt
#define IRQ_interrupt20 IRQ_interrupt
#define IRQ_interrupt21 IRQ_interrupt
#define IRQ_interrupt22 IRQ_interrupt
#define IRQ_interrupt23 IRQ_interrupt
#define IRQ_interrupt24 IRQ_interrupt
#define IRQ_interrupt25 IRQ_interrupt
#define IRQ_interrupt26 IRQ_interrupt
#define IRQ_interrupt27 IRQ_interrupt
#define IRQ_interrupt28 IRQ_interrupt
#define IRQ_interrupt29 IRQ_interrupt
#define IRQ_interrupt30 IRQ_interrupt
#define IRQ_interrupt31 IRQ_interrupt

#define IRQ_INTERRUPT(n)	IRQ_interrupt##n
#define FAST_INTERRUPT(n)	fast_IRQ_interrupt
#define BAD_INTERRUPT(n)	bad_IRQ_interrupt
#define PROBE_INTERRUPT(n)	probe_IRQ_interrupt

static __inline__ void mask_irq(unsigned int irq)
{
	volatile unsigned long int *mask = 
		(volatile unsigned long *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_INTR_ENABLE_CLR) ;
	
	if (32 > irq) *mask = ( 1 << irq );
}

static __inline__ void unmask_irq(unsigned int irq)
{
	volatile unsigned long int *set = 
		(volatile unsigned long *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_INTR_ENABLE_SET) ;
	
	if (32 > irq) *set = ( 1 << irq );
}
 
static __inline__ unsigned long get_enabled_irqs(void)
{
	volatile unsigned long int *ie = 
		(volatile unsigned long *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_INTR_ENABLE) ;
	
	return *ie ;
}

static __inline__ void irq_init_irq(void)
{
	unsigned long flags;
	volatile unsigned long int *mask = 
		(volatile unsigned long *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_INTR_ENABLE_CLR) ;
	volatile unsigned long int *set = 
		(volatile unsigned long *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_INTR_ENABLE_SET) ;
	volatile unsigned long int *ie = 
		(volatile unsigned long *)(NETARM_GEN_MODULE_BASE + 
		NETARM_GEN_INTR_ENABLE) ;

	save_flags_cli (flags);
	*mask = 0xffffffff;
	*set = 0x5aa55aa5;
	*set = 0x00000000;
	if (*ie != 0x5aa55aa5)
		 _netarm_led_FAIL();
	*mask = 0xffffffff;		/* clear all interrupt enables */
	restore_flags (flags);
}
