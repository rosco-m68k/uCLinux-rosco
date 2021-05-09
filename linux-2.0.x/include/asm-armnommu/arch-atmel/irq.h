/*
 * uclinux/include/asm-armnommu/arch-atmel/irq.h
 *
 */
#ifndef __ASM_ARCH_IRQ_H
#define __ASM_ARCH_IRQ_H

#define BUILD_IRQ(s,n,m)

struct pt_regs;

extern int IRQ_interrupt(int irq, struct pt_regs *regs);
extern int timer_IRQ_interrupt(int irq, struct pt_regs *regs);
extern int fast_IRQ_interrupt(int irq, struct pt_regs *regs);
extern int bad_IRQ_interrupt(int irq, struct pt_regs *regs);
extern int probe_IRQ_interrupt(int irq, struct pt_regs *regs);

#define IRQ_interrupt0  IRQ_interrupt
#define IRQ_interrupt1  IRQ_interrupt
#define IRQ_interrupt2  IRQ_interrupt
#define IRQ_interrupt3  IRQ_interrupt
#define IRQ_interrupt4  IRQ_interrupt
#define IRQ_interrupt5  IRQ_interrupt
#define IRQ_interrupt6  IRQ_interrupt
#define IRQ_interrupt7  IRQ_interrupt
#define IRQ_interrupt8  IRQ_interrupt
#define IRQ_interrupt9  IRQ_interrupt
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

#ifdef CONFIG_ARCH_ATMEL_EB55
#if (KERNEL_TIMER==0)
#   undef IRQ_interrupt6
#   define IRQ_interrupt6  timer_IRQ_interrupt
#elif (KERNEL_TIMER==1)
#   undef IRQ_interrupt7
#   define IRQ_interrupt7  timer_IRQ_interrupt
#elif (KERNEL_TIMER==2)
#   undef IRQ_interrupt8
#   define IRQ_interrupt8  timer_IRQ_interrupt
#else
#   error Wierd -- KERNEL_TIMER is not defined or something....
#endif
#else
#if (KERNEL_TIMER==0)
#   undef IRQ_interrupt4
#   define IRQ_interrupt4  timer_IRQ_interrupt
#elif (KERNEL_TIMER==1)
#   undef IRQ_interrupt5
#   define IRQ_interrupt5  timer_IRQ_interrupt
#elif (KERNEL_TIMER==2)
#   undef IRQ_interrupt6
#   define IRQ_interrupt6  timer_IRQ_interrupt
#else
#   error Wierd -- KERNEL_TIMER is not defined or something....
#endif
#endif

#define IRQ_INTERRUPT(n)   (void (*)(void))IRQ_interrupt##n
#define FAST_INTERRUPT(n)  (void (*)(void))fast_IRQ_interrupt
#define BAD_INTERRUPT(n)   (void (*)(void))bad_IRQ_interrupt
#define PROBE_INTERRUPT(n) (void (*)(void))probe_IRQ_interrupt

static __inline__ void mask_irq(unsigned int irq)
{
  unsigned long mask = 1 << (irq & 0x1f);
  outl(mask, AIC_IDCR);
  outl(0, AIC_SVR(irq));
} 

static __inline__ void unmask_irq(unsigned int irq)
{
  unsigned long mask = 1 << (irq & 0x1f);
  outl(mask, AIC_IECR);
  outl(irq, AIC_SVR(irq));
}

static __inline__ unsigned long get_enabled_irqs(void)
{
  return inl(AIC_IMR);
}

static __inline__ void irq_init_irq(void)
{
  extern void aic_atmel_init(void);
  aic_atmel_init();
}

#endif
