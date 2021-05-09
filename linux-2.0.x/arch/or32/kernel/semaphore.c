/*
 *   FILE: semaphore.c
 * AUTHOR: kma@cse.ogi.edu
 *  DESCR: interrupt-safe i960 semaphore implementation; isn't ready for SMP
 */

#include <asm/semaphore.h>
#include <linux/sched.h>

extern int __down_common(struct semaphore* sem, int intrflag)
{
	long flags;
	int retval=0;

	save_flags(flags);
	cli();
	
	if (--sem->count < 0) {
		if (intrflag) {
			interruptible_sleep_on(&sem->wait);
			if (current->signal & ~current->blocked) {
				retval = -1;
			}
		}
		else
			sleep_on(&sem->wait);
	}
	restore_flags(flags);
	return retval;
}

void down(struct semaphore * sem)
{
	__down_common(sem, 0);
}


/*
 * This version waits in interruptible state so that the waiting
 * process can be killed.  The down_failed_interruptible routine
 * returns negative for signalled and zero for semaphore acquired.
 */
extern int down_interruptible(struct semaphore * sem)
{
	return __down_common(sem, 1); 
	return 0;
}


/*
 * Primitives to spin on a lock.  Needed only for SMP.
 */
extern void get_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
        while (xchg(lock_ptr,1) != 0) ;
#endif
}

extern void give_buzz_lock(int *lock_ptr)
{
#ifdef __SMP__
        *lock_ptr = 0 ;
#endif
}


/*
 * We wake people up only if the semaphore was negative (== somebody was
 * waiting on it).
 */
extern void up(struct semaphore * sem)
{
	long flags;
	save_flags(flags);
	cli();

	if (sem->count++ < 0)
		wake_up(&sem->wait);
	
	restore_flags(flags);
}

