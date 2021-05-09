/*
 * linux/include/asm-arm/semaphore.h
 *
 * 
 */
#ifndef __ASM_ARM_SEMAPHORE_H
#define __ASM_ARM_SEMAPHORE_H

#include <linux/linkage.h>

struct semaphore {
	int count;
	int waking;
	int lock;
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 0, NULL })

extern void __down (struct semaphore *sem);
extern void __up (struct semaphore *sem);
extern int __down_interruptible (struct semaphore *sem);

/* Primitives to spin on a lock.  Needed only for SMP version.
 */
extern inline void get_buzz_lock (int *lock_ptr)
{
#ifdef __SMP__
	while (xchg(lock_ptr,1) != 0);
#endif
}

extern inline void give_buzz_lock (int *lock_ptr)
{
#ifdef __SMP__
	*lock_ptr = 0;
#endif
}

#ifdef __arm__
#include <asm/proc/semaphore.h>
#endif

#endif
