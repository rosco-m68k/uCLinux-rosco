/*
 * uclinux/include/asm-armnommu/arch-atmel/time.h
 *
 */
#ifndef __ASM_ARCH_TIME_H
#define __ASM_ARCH_TIME_H

extern __inline__ unsigned long gettimeoffset (void)
{
  struct atmel_timers* tt = (struct atmel_timers*) (TIMER_BASE);
  struct atmel_timer_channel* tc = &tt->chans[KERNEL_TIMER].ch;

  return tc->cv * (1000*1000)/(ARM_CLK/128);
}

/*
 * No need to reset the timer at every irq
 */
extern __inline__ int reset_timer()
{
  struct atmel_timers* tt = (struct atmel_timers*) (TIMER_BASE);
  volatile struct  atmel_timer_channel* tc = &tt->chans[KERNEL_TIMER].ch;
  unsigned long v = tc->sr;

  return (1);
  
}

/*
 * Updating of the RTC.  We don't currently write the time to the
 * CMOS clock.
 */
#define update_rtc() {}

#endif
