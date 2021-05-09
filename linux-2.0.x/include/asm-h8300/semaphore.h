#ifndef _H8300_SEMAPHORE_H
#define _H8300_SEMAPHORE_H

#include <linux/linkage.h>

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 *
 * H8/300H by Yoshinori Sato <qzb04471@nifty.ne.jp>
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
	register struct semaphore *sem1 __asm__ ("er1") = sem;
	__asm__ __volatile__(
		"stc ccr,@-sp\n\t"
		"orc #0x80,ccr\n\t"
		"mov.l @%0,er0\n\t"
		"dec.l #1,er0\n\t"
		"mov.l er0,@%0\n\t"
		"ldc @sp+,ccr\n\t"
		"mov.l er0,er0\n\t"
		"bpl 1f\n\t"
		"jsr " SYMBOL_NAME_STR(__down_failed) "\n"
		"1:"
		: /* no outputs */
		: "r" (sem1)
		: "er0", "memory");
}


/*
 * This version waits in interruptible state so that the waiting
 * process can be killed.  The down_failed_interruptible routine
 * returns negative for signalled and zero for semaphore acquired.
 */
extern inline int down_interruptible(struct semaphore * sem)
{
	register int ret __asm__ ("%er0");
	register struct semaphore *sem1 __asm__ ("%er1") = sem;
	__asm__ __volatile__(
		"stc ccr,@-sp\n\t"
		"orc #0x80,ccr\n\t"
		"mov.l @%0,er0\n\t"
		"dec.l #1,er0\n\t"
		"mov.l er0,@%0\n\t"
		"ldc @sp+,ccr\n\t"
		"mov.l er0,er0\n\t"
		"bpl 1f\n\t"
		"jsr " SYMBOL_NAME_STR(__down_failed_interruptible) "\n\t"
		"bra 2f\n"
		"1:\n\t"
		"sub.l %0,%0\n"
		"2:"
		: "=d" (ret)
		: "a" (sem1)
		: "%er0", "memory");
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
	register struct semaphore *sem1 __asm__ ("er1") = sem;
	__asm__ __volatile__(
		"stc ccr,@-sp\n\t"
		"orc #0x80,ccr\n\t"
		"mov.l @%0,er0\n\t"
		"inc.l #1,er0\n\t"
		"mov.l er0,@%0\n\t"
		"ldc @sp+,ccr\n\t"
		"mov.l er0,er0\n\t"
		"bmi 1f\n\t"
		"jsr " SYMBOL_NAME_STR(__up_wakeup) "\n"
		"1:"
		: /* no outputs */
		: "r" (sem1)
		: "er0", "memory");
}

#endif
