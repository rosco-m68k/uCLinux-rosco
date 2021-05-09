#ifndef _OR32_SEMAPHORE_H
#define _OR32_SEMAPHORE_H

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 *
 */

struct semaphore {
	int count;
	int waking;
	int lock;			/* to make waking testing atomic */
	struct wait_queue * wait;
};

#define MUTEX ((struct semaphore) { 1, 0, 0, NULL })
#define MUTEX_LOCKED ((struct semaphore) { 0, 0, 0, NULL })

extern void down(struct semaphore * sem);
extern int down_interruptible(struct semaphore * sem);
extern void up(struct semaphore * sem);

extern void get_buzz_lock(int *lock_ptr);
extern void give_buzz_lock(int *lock_ptr);


#endif
