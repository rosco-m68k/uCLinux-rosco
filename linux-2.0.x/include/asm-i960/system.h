/*
 * Copyright (C) 1999		Keith Adams	<kma@cse.ogi.edu>
 * 				Oregon Graduate Institute
 */
#ifndef _I960_SYSTEM_H
#define _I960_SYSTEM_H

#include <linux/linkage.h>

asmlinkage void resume(void);
struct task_struct;
extern void switch_to(struct task_struct* prev, struct task_struct* next);


#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

#define cli() do { \
	int garbage; \
	__asm__ __volatile__("modpc	g0, %1, %1"	\
			     :"=r" (garbage)	\
			     : "0" (0x1f << 16)); \
} while (0)

#define sti() do { \
	int garbage;	\
	__asm__ __volatile__("modpc	g0, %1, %2"	\
			     : "=r" (garbage)	\
			     : "r" (0x1f << 16), "0" (0));	\
} while (0)

#define mb()  __asm__ __volatile__ (""   : : :"memory")

#define save_flags(flags) do { \
	__asm__ __volatile__ ("modpc	g0, 0, %0" :"=&r" (flags));	\
} while (0)

#define restore_flags(flags) do { \
	int	flagcpy=flags;	\
	__asm__ __volatile__ ("modpc	g0, %1, %2" \
			      : "=r"(flagcpy) \
			      : "r" (0xffffffff), "0" (flagcpy)); \
} while (0)

extern unsigned long atmod(volatile void* ptr, unsigned long mask, unsigned long val);

static inline unsigned long __xchg(unsigned long x, 
				   volatile void * ptr, int size)
{
	unsigned long mask, retval;
	int off;

	switch (size) {
		case 1:
			mask = 0xff;
			off = (long) ptr % 4;
			mask <<= off * 8;
			x <<= off * 8;
			retval = atmod(ptr, mask, x);
			break;

		case 2:
			mask = 0xffff;
			off = (long)ptr % 2;
			mask <<= off * 16;
			x <<= off * 16;
			retval = atmod(ptr, mask, x);	
			break;

		case 4:
			mask = 0xffffffff;
			retval = atmod(ptr, mask, x);
			break;
	}
	return retval;
}

/*  January 14, 2002    Fred Finster adding Hard Reset  */

#define SYSCALL(number)	\
asm("calls	%0; ret"	\
	: : "lI"(number): "g0");

/*
 unsigned long hard_mon_entry(void)
{
	SYSCALL(254);
}
*/

/* Stop the processor doing anything */
/* TODO: enable the watchdog and kick it in the timer ISR. */
#define HARD_RESET_NOW() ({             \
        cli();                          \
        while(1);                       \
        })

#endif /* _I960_SYSTEM_H */
