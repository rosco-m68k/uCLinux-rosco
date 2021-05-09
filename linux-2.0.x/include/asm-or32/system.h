#ifndef _OR32_SYSTEM_H
#define _OR32_SYSTEM_H

#include <linux/config.h> /* get configuration macros */
#include <linux/linkage.h>
#include <asm/segment.h>

#include <asm/processor.h>
#include <asm/board.h>

extern int abs(int);

extern void __save_flags(unsigned long *flags);
extern void __restore_flags(unsigned long flags);
extern void sti(void);
extern void cli(void);
extern void mtspr(unsigned long add, unsigned long val);
extern unsigned long mfspr(unsigned long add);
extern void ic_enable(void);
extern void ic_disable(void);
extern void ic_invalidate(void);
extern void dc_enable(void);
extern void dc_disable(void);
extern void dc_line_invalidate(unsigned long);
extern void _print(const char *fmt, ...);
extern void _switch(struct thread_struct *, struct thread_struct *);

#define SYS_TICK_PER  0x10000
#define BIGALLOCS     1

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((volatile struct __xchg_dummy *)(x))

#define nop() __asm__ __volatile__ ("l.nop"::)
#define mb()  __asm__ __volatile__ (""   : : :"memory")

#define save_flags(x) __save_flags((unsigned long *)&(x))
#define restore_flags(x) __restore_flags((x))

static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
  unsigned long tmp, flags;

  save_flags(flags);
  cli();

  switch (size) {
  case 1:
    __asm__ __volatile__
    ("l.lbz %0,%2\n\t"
     "l.sb %2,%1"
    : "=&r" (tmp) : "r" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 2:
    __asm__ __volatile__
    ("l.lhz %0,%2\n\t"
     "l.sh %2,%1"
    : "=&r" (tmp) : "r" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 4:
    __asm__ __volatile__
    ("l.lwz %0,%2\n\t"
     "l.sw %2,%1"
    : "=&r" (tmp) : "r" (x), "m" (*__xg(ptr)) : "memory");
    break;
  }

  restore_flags(flags);
  return tmp;
}

#define HARD_RESET_NOW() ({		\
        cli();				\
})

#endif /* _OR32_SYSTEM_H */
