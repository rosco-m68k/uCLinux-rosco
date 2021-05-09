#ifndef _M68K_SEMAPHORE_H
#define _M68K_SEMAPHORE_H

#include <linux/linkage.h>

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 *
 * m68k version by Andreas Schwab
 */

struct semaphore {
	int count;
	int waking;
	int lock;			/* to make waking testing atomic */
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 0, NULL })

asmlinkage void __down_failed(void /* special register calling convention */);
asmlinkage int  __down_failed_interruptible(void);  /* params in registers */
asmlinkage void __up_wakeup(void /* special register calling convention */);

extern void __down(struct semaphore * sem);
extern void __up(struct semaphore * sem);

/*
 * This is ugly, but we want the default case to fall through.
 * "down_failed" is a special asm handler that calls the C
 * routine that actually waits. See arch/m68k/lib/semaphore.S
 */
extern inline void down(struct semaphore * sem)
{
	__asm__ __volatile__(
		"| atomic down operation\n\t"
		"movel	%0, %%a1\n\t"
		"lea	%%pc@(1f), %%a0\n\t"
		"subql	#1, %%a1@\n\t"
		"jmi " SYMBOL_NAME_STR(__down_failed) "\n"
		"1:"
		: /* no outputs */
		: "g" (sem)
		: "cc", "%a0", "%a1", "memory");
}


/*
 * This version waits in interruptible state so that the waiting
 * process can be killed.  The down_failed_interruptible routine
 * returns negative for signalled and zero for semaphore acquired.
 */
extern inline int down_interruptible(struct semaphore * sem)
{
	int ret;
	__asm__ __volatile__(
		"| atomic down operation\n\t"
		"movel	%1, %%a1\n\t"
		"lea	%%pc@(1f), %%a0\n\t"
		"subql	#1, %%a1@\n\t"
		"jmi " SYMBOL_NAME_STR(__down_failed_interruptible) "\n\t"
		"clrl	%%d0\n"
		"1: movel	%%d0, %0\n"
		: "=d" (ret)
		: "g" (sem)
		: "cc", "%d0", "%a0", "%a1", "memory");
	return(ret);
}


/*
 * Primitives to spin on a lock.  Needed only for SMP version ... so
 * somebody should start playing with one of those Sony NeWS stations.
 */
extern inline void get_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
        while (xchg(lock_ptr,1) != 0) ;
#endif
}

extern inline void give_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
        *lock_ptr = 0 ;
#endif
}


/*
 * Note! This is subtle. We jump to wake people up only if
 * the semaphore was negative (== somebody was waiting on it).
 * The default case (no contention) will result in NO
 * jumps for both down() and up().
 */
extern inline void up(struct semaphore * sem)
{
	__asm__ __volatile__(
		"| atomic up operation\n\t"
		"movel	%0, %%a1\n\t"
		"lea	%%pc@(1f), %%a0\n\t"
		"addql	#1, %%a1@\n\t"
		"jle " SYMBOL_NAME_STR(__up_wakeup) "\n"
		"1:"
		: /* no outputs */
		: "g" (sem)
		: "cc", "%a0", "%a1", "memory");
}

#endif
