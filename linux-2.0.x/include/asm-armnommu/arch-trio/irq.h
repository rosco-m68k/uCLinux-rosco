/*
 * include/asm-arm/arch-trio/irq.h
 *
 * Copyright (C) 1996 Russell King
 * Copyright (C) 1999 APLIO SA
 *
 * Changelog:
 *   04-11-1999	VL	Created
 */

#define BUILD_IRQ(s,n,m)

struct pt_regs;

extern int IRQ_interrupt(int irq, struct pt_regs *regs);
extern int timer_IRQ_interrupt(int irq, struct pt_regs *regs);
extern int fast_IRQ_interrupt(int irq, struct pt_regs *regs);
extern int bad_IRQ_interrupt(int irq, struct pt_regs *regs);
extern int probe_IRQ_interrupt(int irq, struct pt_regs *regs);

#define IRQ_interrupt0 IRQ_interrupt
#define IRQ_interrupt1 IRQ_interrupt
#define IRQ_interrupt2 IRQ_interrupt
#define IRQ_interrupt3 IRQ_interrupt
#define IRQ_interrupt4 IRQ_interrupt
#define IRQ_interrupt5 timer_IRQ_interrupt
#define IRQ_interrupt4 IRQ_interrupt
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

#define IRQ_INTERRUPT(n)	(void (*)(void))IRQ_interrupt##n
#define FAST_INTERRUPT(n)	(void (*)(void))fast_IRQ_interrupt
#define BAD_INTERRUPT(n)	(void (*)(void))bad_IRQ_interrupt
#define PROBE_INTERRUPT(n)	(void (*)(void))probe_IRQ_interrupt



static __inline__ void mask_irq(unsigned int irq)
{
	unsigned long mask = 1 << (irq & 0xf);

	outl(mask, AIC_IDCR);
}	


static __inline__ void unmask_irq(unsigned int irq)
{
	unsigned long mask = 1 << (irq & 0xf);

	outl(mask, AIC_IECR);

}

static __inline__ unsigned long get_enabled_irqs(void)
{
	return inl(AIC_IMR);
}


static __inline__ void irq_init_irq(void)
{
	extern void trio_init_aic();

	trio_init_aic();
}
