/*
 * include/asm-arm/arch-ebsa/irq.h
 *
 * Copyright (C) 1996 Russell King
 */

#define IRQ_MCLR	((volatile unsigned char *)0xf3000000)
#define IRQ_MSET	((volatile unsigned char *)0xf2c00000)
#define IRQ_MASK	((volatile unsigned char *)0xf2c00000)
 
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

#define IRQ_INTERRUPT(n)	IRQ_interrupt##n
#define FAST_INTERRUPT(n)	fast_IRQ_interrupt
#define BAD_INTERRUPT(n)	bad_IRQ_interrupt
#define PROBE_INTERRUPT(n)	probe_IRQ_interrupt

static __inline__ void mask_irq(unsigned int irq)
{
	if (irq < 8)
		*IRQ_MCLR = 1 << irq;
}

static __inline__ void unmask_irq(unsigned int irq)
{
	if (irq < 8)
		*IRQ_MSET = 1 << irq;
}
 
static __inline__ unsigned long get_enabled_irqs(void)
{
	return 0;
}

static __inline__ void irq_init_irq(void)
{
	unsigned long flags;

	save_flags_cli (flags);
	*IRQ_MCLR = 0xff;
	*IRQ_MSET = 0x55;
	*IRQ_MSET = 0x00;
	if (*IRQ_MASK != 0x55)
		while (1);
	*IRQ_MCLR = 0xff;		/* clear all interrupt enables */
	restore_flags (flags);
}
