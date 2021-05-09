#ifndef _M68K_SYSTEM_H
#define _M68K_SYSTEM_H

#include <linux/config.h> /* get configuration macros */
#include <linux/linkage.h>
#include <asm/segment.h>

#ifdef CONFIG_M68328
#include <asm/MC68328.h>
#endif

#ifdef CONFIG_M68360
#include <asm/m68360.h>
#endif

#ifdef CONFIG_M68EZ328
#include <asm/MC68EZ328.h>
#endif

extern inline unsigned long rdusp(void) {
#ifdef CONFIG_COLDFIRE
	extern unsigned int	sw_usp;
	return(sw_usp);
#else
  	unsigned long usp;
	__asm__ __volatile__("move %/usp,%0"
			     : "=a" (usp));
	return usp;
#endif
}

extern inline void wrusp(unsigned long usp) {
#ifdef CONFIG_COLDFIRE
	extern unsigned int	sw_usp;
	sw_usp = usp;
#else
	__asm__ __volatile__("move %0,%/usp"
			     :
			     : "a" (usp));
#endif
}

extern inline unsigned long rda5(void) {
  	unsigned long a5;

	__asm__ __volatile__("movel %/a5,%0"
			     : "=a" (a5));
	return a5;
}

extern inline void wra5(unsigned long a5) {
	__asm__ __volatile__("movel %0,%/a5"
			     :
			     : "a" (a5));
}

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
#define switch_to(prev, next) {						\
  __asm__ __volatile__(	"movel	%3, %%d2\n\t"				\
  			"movel	%2, %%d1\n\t"				\
			"movel	%0, %%d0\n\t"				\
			"movel	%1, %%a1\n\t"				\
			"movel	%%d0, %%a0\n\t"				\
			"jbsr " SYMBOL_NAME_STR(resume) "\n\t" 		\
		       :						\
		       : "a" (prev),					\
			 "a" (next),					\
			 "i" ((int)&((struct task_struct *)0)->tss),	\
		         "g" ((char)((prev)->mm == (next)->mm)) 	\
		       : "cc", "d0", "d1", "d2", "d3", "d4", "d5", "a0", "a1");\
}

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((volatile struct __xchg_dummy *)(x))

#ifdef CONFIG_COLDFIRE
#define sti() __asm__ __volatile__ ( \
	"move %/sr,%%d0\n\t" \
	"andi.l #0xf8ff,%%d0\n\t" \
	"move %%d0,%/sr\n" \
	: /* no outputs */ \
	: \
        : "cc", "%d0", "memory")
#define cli() __asm__ __volatile__ ( \
	"move %/sr,%%d0\n\t" \
	"ori.l  #0x0700,%%d0\n\t" \
	"move %%d0,%/sr\n" \
	: /* no inputs */ \
	: \
	: "cc", "%d0", "memory")
#else
#if defined(CONFIG_ATARI) && !defined(CONFIG_AMIGA) && !defined(CONFIG_MAC)
/* block out HSYNC on the atari */
#define sti() __asm__ __volatile__ ("andiw #0xfbff,%/sr": : : "memory")
#else /* portable version */
#define sti() __asm__ __volatile__ ("andiw #0xf8ff,%/sr": : : "memory")
#endif /* machine compilation types */ 
#define cli() __asm__ __volatile__ ("oriw  #0x0700,%/sr": : : "memory")
#endif

#define nop() __asm__ __volatile__ ("nop"::)
#define mb()  __asm__ __volatile__ (""   : : :"memory")

#define save_flags(x) \
__asm__ __volatile__("movew %/sr,%0":"=d" (x) : /* no input */ :"memory")

#define restore_flags(x) \
__asm__ __volatile__("movew %0,%/sr": /* no outputs */ :"d" (x) : "memory")

#define iret() __asm__ __volatile__ ("rte": : :"memory", "sp", "cc")

#ifndef CONFIG_RMW_INSNS
static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
  unsigned long tmp, flags;

  save_flags(flags);
  cli();

  switch (size) {
  case 1:
    __asm__ __volatile__
    ("moveb %2,%0\n\t"
     "moveb %1,%2"
    : "=&d" (tmp) : "d" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 2:
    __asm__ __volatile__
    ("movew %2,%0\n\t"
     "movew %1,%2"
    : "=&d" (tmp) : "d" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 4:
    __asm__ __volatile__
    ("movel %2,%0\n\t"
     "movel %1,%2"
    : "=&d" (tmp) : "d" (x), "m" (*__xg(ptr)) : "memory");
    break;
  }
  restore_flags(flags);
  return tmp;
}
#else
static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
	    case 1:
		__asm__ __volatile__
			("moveb %2,%0\n\t"
			 "1:\n\t"
			 "casb %0,%1,%2\n\t"
			 "jne 1b"
			 : "=&d" (x) : "d" (x), "m" (*__xg(ptr)) : "memory");
		break;
	    case 2:
		__asm__ __volatile__
			("movew %2,%0\n\t"
			 "1:\n\t"
			 "casw %0,%1,%2\n\t"
			 "jne 1b"
			 : "=&d" (x) : "d" (x), "m" (*__xg(ptr)) : "memory");
		break;
	    case 4:
		__asm__ __volatile__
			("movel %2,%0\n\t"
			 "1:\n\t"
			 "casl %0,%1,%2\n\t"
			 "jne 1b"
			 : "=&d" (x) : "d" (x), "m" (*__xg(ptr)) : "memory");
		break;
	}
	return x;
}
#endif

#if defined (CONFIG_M68332) || defined (CONFIG_CPU32)
#define HARD_RESET_NOW() ({		\
        cli();				\
        asm("				\
	movew   #0x0000, 0xfffa6a;	\
        reset;				\
        /*movew #0x1557, 0xfffa44;*/	\
        /*movew #0x0155, 0xfffa46;*/	\
        moveal #0, %a0;			\
        movec %a0, %vbr;		\
        moveal 0, %sp;			\
        moveal 4, %a0;			\
        jmp (%a0);			\
        ");				\
})
#endif

#if defined( CONFIG_M68328 ) || defined( CONFIG_M68EZ328 )
#define HARD_RESET_NOW() ({		\
        cli();				\
        asm("				\
        moveal #0x10c00000, %a0;	\
        moveb #0, 0xFFFFF300;		\
        moveal 0(%a0), %sp;		\
        moveal 4(%a0), %a0;		\
        jmp (%a0);			\
        ");				\
})
#endif

#if defined(CONFIG_M68360)
/* Stop the processor doing anything */
/* TODO: enable the watchdog and kick it in the timer ISR. */
#define HARD_RESET_NOW() ({             \
        cli();                          \
        while(1);                       \
        })
#endif

#ifdef CONFIG_COLDFIRE
#if defined(CONFIG_FLASH_SNAPGEAR) || defined(CONFIG_HW_FEITH)
#ifdef CONFIG_M5272
/*
 *	Need to account for broken early mask of 5272 silicon. So don't
 *	jump through the original start address. Jump strait into the
 *	known start of the FLASH code.
 */
#define HARD_RESET_NOW() ({		\
        asm("				\
	movew #0x2700, %sr;		\
        jmp 0xf0000400;			\
        ");				\
})
#else
#define HARD_RESET_NOW() ({		\
        asm("				\
	movew #0x2700, %sr;		\
	moveal #0x10000044, %a0;	\
	movel #0xffffffff, (%a0);	\
	moveal #0x10000001, %a0;	\
	moveb #0x00, (%a0);		\
        moveal #0xf0000004, %a0;	\
        moveal (%a0), %a0;		\
        jmp (%a0);			\
        ");				\
})
#endif	/* CONFIG_M5272 */

#elif defined(CONFIG_COBRA5272)
/* 
 * HARD_RESET_NOW() for senTec COBRA5272:
 *
 * ATTENTION!
 * This seems to be a dirty workaround for the reboot!
 * We currently just jump to the startcode of colilo.
 * 
 * If you are using dBUG, you will have to ajust the jmp
 * destination. Look for the address of _asm_startmeup, and
 * put it in the jump statementi!
 * 
 */
#define HARD_RESET_NOW() ({      \
   asm("           \
         jmp 0xffe00400;             \
       ");          \
})
/* end HARDRESET_NOW() for senTec COBRA5272. */
#else
#define HARD_RESET_NOW() ({		\
        asm("				\
	movew #0x2700, %sr;		\
        moveal #0x4, %a0;		\
        moveal (%a0), %a0;		\
        jmp (%a0);			\
        ");				\
})
#endif	/* CONFIG_FLASH_SNAPGEAR */

#endif	/* CONFIG_COLDFIRE */

#ifdef CONFIG_M68000
/* 2002-05-14 gc: */
#define HARD_RESET_NOW() ({		\
        asm volatile("	                \
	movew #0x2700, %sr;		\
        moveal #0x4, %a0;		\
        moveal (%a0), %a0;		\
        jmp (%a0);			\
        ");				\
})
#endif /* CONFIG_M68000 */

#endif /* _M68K_SYSTEM_H */
