#ifndef _H8300_SYSTEM_H
#define _H8300_SYSTEM_H

#include <linux/config.h> /* get configuration macros */
#include <linux/linkage.h>
#include <asm/segment.h>

/*
 * switch_to(n) should switch tasks to task ptr, first checking that
 * ptr isn't the current task, in which case it does nothing.  This
 * also clears the TS-flag if the task we switched to has used the
 * math co-processor latest.
 */
/*
 * switch_to() saves the extra registers, that are not saved
 * automatically by SAVE_SWITCH_STACK in resume(), ie. d0-d5 and
 * a0-a1. Some of these are used by schedule() and its predecessors
 * and so we might get see unexpected behaviors when a task returns
 * with unexpected register values.
 *
 * syscall stores these registers itself and none of them are used
 * by syscall after the function in the syscall has been called.
 *
 * Beware that resume now expects *next to be in d1 and the offset of
 * tss to be in a1. This saves a few instructions as we no longer have
 * to push them onto the stack and read them back right after.
 *
 * 02/17/96 - Jes Sorensen (jds@kom.auc.dk)
 *
 * Changed 96/09/19 by Andreas Schwab
 * pass prev in a0, next in a1, offset of tss in d1, and whether
 * the mm structures are shared in d2 (to avoid atc flushing).
 */
asmlinkage void resume(void);
#define switch_to(prev,next) { \
  register void *_prev __asm__ ("er0") = (prev); \
  register void *_next __asm__ ("er2") = (next); \
  register int _tssoff __asm__ ("er1") = (int)&((struct task_struct *)0)->tss; \
  register char _shared __asm__ ("er3") = ((prev)->mm == (next)->mm); \
  __asm__ __volatile__("jsr @" SYMBOL_NAME_STR(resume) "\n\t" \
		       : : "r" (_prev), "r" (_next), "r" (_tssoff), \
		           "r" (_shared) \
		       : "er0", "er1", "er2", "er3", "er4", "er5", "er6"); \
}

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((volatile struct __xchg_dummy *)(x))

#define sti() __asm__ __volatile__ ( \
	"andc #0x7f,ccr\n\t" \
	: /* no outputs */ \
	: \
        : "memory")
#define cli() __asm__ __volatile__ ( \
	"orc #0x80,ccr\n\t" \
	: /* no inputs */ \
	: \
	:  "memory")
#define nop() __asm__ __volatile__ ("nop"::)
#define mb()  __asm__ __volatile__ (""   : : :"memory")

#define save_flags(x) \
__asm__ __volatile__("sub.l er0,er0\n\tstc ccr,r0l\n\tmov.l er0,%0" \
                     :"=r" (x) : /* no input */ :"er0","memory")

#define restore_flags(x) \
__asm__ __volatile__("mov.l %0,er0\n\tldc r0l,ccr": /* no outputs */ :"r" (x) : "er0","memory")

#define iret() __asm__ __volatile__ ("rte": : :"memory", "sp", "cc")

static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
  unsigned long tmp, flags;

  save_flags(flags);
  cli();

  switch (size) {
  case 1:
    __asm__ __volatile__
    ("mov.b %2,%0\n\t"
     "mov.b %1,%2"
    : "=&d" (tmp) : "d" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 2:
    __asm__ __volatile__
    ("mov.w %2,%0\n\t"
     "mov.w %1,%2"
    : "=&d" (tmp) : "d" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 4:
    __asm__ __volatile__
    ("mov.l %2,%0\n\t"
     "mov.l %1,%2"
    : "=&r" (tmp) : "r" (x), "m" (*__xg(ptr)) : "memory");
    break;
  }
  restore_flags(flags);
  return tmp;
}

extern unsigned int rdusp(void);
extern void wrusp(unsigned int usp);

#define HARD_RESET_NOW() ({		\
        asm("				\
	ldc #0x80, ccr;			\
        mov.l @0x00, er0;		\
        jmp @er0;			\
        ");				\
})

#endif /* _H8300_SYSTEM_H */
