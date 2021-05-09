#ifndef _I960_SEMAPHORE_H
#define _I960_SEMAPHORE_H

/*
 * interrupt-safe (not SMP-safe) semaphores..
 *
 * (C) Copyright 1999		Keith Adams <kma@cse.ogi.edu>
 * 				Oregon Graduate Institute
 * 				
 * Based on include/asm-i386/semaphore.h:
 * (C) Copyright 1996 Linus Torvalds
 *
 */
#include <asm/atomic.h>

struct semaphore {
	atomic_t count;
	atomic_t waking;
	int lock;	/* we don't actually use this; sched.c needs it, tho */
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 0, NULL })

extern void __down(struct semaphore * sem);
extern void __up(struct semaphore * sem);

extern void down(struct semaphore * sem);
extern int down_interruptible(struct semaphore * sem);
extern void up(struct semaphore * sem);

extern void get_buzz_lock(int *lock_ptr);
extern void give_buzz_lock(int *lock_ptr);

#endif
