#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch/irq.h>



/* The trio AIC need to recive EOI command at the end of interrupt handler,
   These command is compossed from previous IRQ nr and previous IRQ priority.
   So we maintain this value in the trio_irq_prority variable.

   The trio_irq_prtable contains the priority<<5 for each IRQ
*/   

volatile unsigned char trio_irq_priority = 0; /* Current IRQ number and priority */
unsigned char trio_irq_prtable[16] = {
	7 << 5,  /* FIQ */
	0 << 5,  /* WDIRQ */
	0 << 5,  /* SWIRQ */
	2 << 5, /* UAIRQ */
	0 << 5,		/* TC0 (Timer 0 used for DRAM refresh */
	0 << 5, /* TC1IRQ */
	1 << 5,		/* TC2IRQ */
	0 << 5,  /* PIOAIRQ */
	0 << 5,  /* LCDIRQ not used */
	5 << 5, /* SPIIRQ */
	4 << 5, /* IRQ0, (ETHERNET? )*/
	0 << 5,  /* IRQ1 */
	7 << 5,  /* OAKAIRQ */
	6 << 5,  /* OAKBIRQ */
	3 << 5, /* UBIRQ */
	5 << 5 /* PIOBIRQ */
};

#define LevelSensitive              (0<<5)
#define EdgeTriggered               (1<<5)
#define LowLevel                    (0<<5)
#define NegativeEdge                (1<<5)
#define HighLevel                   (2<<5)
#define PositiveEdge                (3<<5)

unsigned char trio_irq_type[16] = {
	EdgeTriggered,  /* FIQ */
	EdgeTriggered,  /* WDIRQ */
	EdgeTriggered,  /* SWIRQ */
	EdgeTriggered, /* UAIRQ */
	EdgeTriggered,		/* TC0 (Timer 0 used for DRAM refresh */
	EdgeTriggered, /* TC1IRQ */
	EdgeTriggered,		/* TC2IRQ */
	EdgeTriggered,  /* PIOAIRQ */
	EdgeTriggered,  /* LCDIRQ not used */
	EdgeTriggered, /* SPIIRQ */
	EdgeTriggered, /* IRQ0, (ETHERNET? )*/
	EdgeTriggered,  /* IRQ1 */
	LevelSensitive,  /* OAKAIRQ */
	LevelSensitive,  /* OAKBIRQ */
	EdgeTriggered, /* UBIRQ */
	EdgeTriggered /* PIOBIRQ */
};




void do_IRQ(int irq, struct pt_regs * regs);


inline void irq_ack(int priority)
{
	outl(priority, AIC_EOICR);
}
	
asmlinkage int probe_IRQ_interrupt(int irq, struct pt_regs * regs)
{
	mask_irq(irq);
	irq_ack(trio_irq_priority);
	return 0;
}

asmlinkage int bad_IRQ_interrupt(int irqn, struct pt_regs * regs)
{
	printk("bad interrupt %d recieved!\n", irqn);
	irq_ack(trio_irq_priority);
	return 0;
}

asmlinkage int IRQ_interrupt(int irq, struct pt_regs * regs)
{
	register unsigned long flags;
	register unsigned long saved_count;
	register unsigned char spr = trio_irq_priority;

	trio_irq_priority = ((unsigned char)irq) | trio_irq_prtable[irq];


	saved_count = intr_count;
	intr_count = saved_count + 1;

	save_flags(flags);
	sti();
	do_IRQ(irq, regs);
	restore_flags(flags);
	intr_count = saved_count;
	trio_irq_priority = spr;
	irq_ack(spr);
	return 0;
}


asmlinkage int timer_IRQ_interrupt(int irq, struct pt_regs * regs)
{
	register unsigned long flags;
	register unsigned long saved_count;
	register unsigned char spr = trio_irq_priority;

	trio_irq_priority = ((unsigned char)irq) | trio_irq_prtable[irq];



	saved_count = intr_count;
	intr_count = saved_count + 1;

	save_flags(flags);
	do_IRQ(irq, regs);
	restore_flags(flags);
	intr_count = saved_count;
	trio_irq_priority = spr;
	irq_ack(spr);
	return 0;
}


asmlinkage int fast_IRQ_interrupt(int irq, struct pt_regs * regs)
{
	register unsigned long saved_count;

	saved_count = intr_count;
	intr_count = saved_count + 1;

	do_IRQ(irq, regs);
	cli();
	intr_count = saved_count;
	inl(AIC_FVR);
	return 1;
}

void trio_init_aic()
{
	int irqno;

	// Disable all interrupts
	outl(0xFFFFFFFF, AIC_IDCR);
	
	// Clear all interrupts
	outl(0xFFFFFFFF, AIC_ICCR);

	for ( irqno = 0 ; irqno < 16 ; irqno++ )
	{
		outl(irqno, AIC_EOICR);
	}
	

	
	for ( irqno = 0 ; irqno < 16 ; irqno++ )
	{
		outl((trio_irq_prtable[irqno] >> 5)  |  trio_irq_type[irqno] , AIC_SMR(irqno));
	}
}
