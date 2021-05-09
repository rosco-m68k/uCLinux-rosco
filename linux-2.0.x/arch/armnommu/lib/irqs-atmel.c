
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch/irq.h>

/* The trio AIC need to recive EOI command at the end of interrupt handler,
   These command is compossed from previous IRQ nr and previous IRQ priority.
   So we maintain this value in the atmel_irq_prority variable.

   The atmel_irq_prtable contains the priority<<5 for each IRQ
*/   

/* Current IRQ number and priority */
volatile unsigned char atmel_irq_priority = 0;

unsigned char atmel_irq_prtable[32] = {
  7 << 5, /* #00 FIQ */
  0 << 5, /* #01 SW  */
  3 << 5, /* #02 US0 */
  2 << 5, /* #03 US1 */
  1 << 5, /* #04 TC0 */
  1 << 5, /* #05 TC1 */
  1 << 5, /* #06 TC2 */
  5 << 5, /* #07 WD  */
  4 << 5, /* #08 PIO */
  0 << 5, /* #09 */ 
  0 << 5, /* #10 */ 
  0 << 5, /* #11 */ 
  0 << 5, /* #12 */ 
  0 << 5, /* #13 */ 
  0 << 5, /* #14 */ 
  0 << 5, /* #15 */ 
  6 << 5, /* #16 IRQ0 */ 
  6 << 5, /* #17 IRQ1 */ 
  6 << 5, /* #18 IRQ2 */ 
  0 << 5, /* #19 */ 
  0 << 5, /* #20 */ 
  0 << 5, /* #21 */ 
  0 << 5, /* #22 */ 
  0 << 5, /* #23 */ 
  0 << 5, /* #24 */ 
  0 << 5, /* #25 */ 
  0 << 5, /* #26 */ 
  0 << 5, /* #27 */ 
  0 << 5, /* #28 */ 
  0 << 5, /* #29 */ 
  0 << 5, /* #30 */ 
  0 << 5  /* #31 */ 
};

#define LevelSensitive (0<<5)
#define EdgeTriggered  (1<<5)
#define LowLevel       (0<<5)
#define NegativeEdge   (1<<5)
#define HighLevel      (2<<5)
#define PositiveEdge   (3<<5)

unsigned char atmel_irq_type[32] = {
  EdgeTriggered, /* #00 FIQ */
  EdgeTriggered, /* #01 SW  */
  EdgeTriggered, /* #02 US0 */
  EdgeTriggered, /* #03 US1 */
  EdgeTriggered, /* #04 TC0 */
  EdgeTriggered, /* #05 TC1 */
  EdgeTriggered, /* #06 TC2 */
  EdgeTriggered, /* #07 WD  */
  EdgeTriggered, /* #08 PIO */
  EdgeTriggered, /* #09 */ 
  EdgeTriggered, /* #10 */ 
  EdgeTriggered, /* #11 */ 
  EdgeTriggered, /* #12 */ 
  EdgeTriggered, /* #13 */ 
  EdgeTriggered, /* #14 */ 
  EdgeTriggered, /* #15 */ 
  EdgeTriggered, /* #16 IRQ0 */ 
  PositiveEdge,  /* #17 IRQ1 */ 
  EdgeTriggered, /* #18 IRQ2 */ 
  EdgeTriggered, /* #19 */ 
  EdgeTriggered, /* #20 */ 
  EdgeTriggered, /* #21 */ 
  EdgeTriggered, /* #22 */ 
  EdgeTriggered, /* #23 */ 
  EdgeTriggered, /* #24 */ 
  EdgeTriggered, /* #25 */ 
  EdgeTriggered, /* #26 */ 
  EdgeTriggered, /* #27 */ 
  EdgeTriggered, /* #28 */ 
  EdgeTriggered, /* #29 */ 
  EdgeTriggered, /* #30 */ 
  EdgeTriggered  /* #31 */ 
};

void do_IRQ(int irq, struct pt_regs * regs);


inline void irq_ack(int priority)
{
  outl(priority, AIC_EOICR);
}
  
asmlinkage int probe_IRQ_interrupt(int irq, struct pt_regs * regs)
{
  mask_irq(irq);
  irq_ack(atmel_irq_priority);
  return 0;
}

asmlinkage int bad_IRQ_interrupt(int irqn, struct pt_regs * regs)
{
  printk("bad interrupt %d recieved!\n", irqn);
  irq_ack(atmel_irq_priority);
  return 0;
}

asmlinkage int IRQ_interrupt(int irq, struct pt_regs * regs)
{
  register unsigned long flags;
  register unsigned long saved_count;
  register unsigned char spr = atmel_irq_priority;

  atmel_irq_priority = ((unsigned char)irq) | atmel_irq_prtable[irq];

  saved_count = intr_count;
  intr_count = saved_count + 1;

  save_flags(flags);
  sti();
  do_IRQ(irq, regs);
  restore_flags(flags);
  intr_count = saved_count;
  atmel_irq_priority = spr;
  irq_ack(spr);
  return 0;
}

asmlinkage int timer_IRQ_interrupt(int irq, struct pt_regs * regs)
{
  register unsigned long flags;
  register unsigned long saved_count;
  register unsigned char spr = atmel_irq_priority;

//printk("timer_IRQ_interrupt\n");
  atmel_irq_priority = ((unsigned char)irq) | atmel_irq_prtable[irq];

  saved_count = intr_count;
  intr_count = saved_count + 1;

  save_flags(flags);
  do_IRQ(irq, regs);
  restore_flags(flags);
  intr_count = saved_count;
  atmel_irq_priority = spr;
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

extern void vector_IRQ(void);

void aic_atmel_init()
{
  int irqno;

  // Disable all interrupts
  outl(0xFFFFFFFF, AIC_IDCR);
  
  // Clear all interrupts
  outl(0xFFFFFFFF, AIC_ICCR);

  for ( irqno = 0 ; irqno < 32 ; irqno++ ) {
    outl(irqno, AIC_EOICR);
  }
  
  for ( irqno = 0 ; irqno < 32 ; irqno++ ) {
    outl((atmel_irq_prtable[irqno] >> 5) | atmel_irq_type[irqno],
	 AIC_SMR(irqno));
  }

  for ( irqno = 0 ; irqno < 32 ; irqno++ ) {
    outl(0, AIC_SVR(irqno));
  }
}








