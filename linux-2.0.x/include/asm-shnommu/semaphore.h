/*
 * Modification History
 *              29 Aug 2002: down macro is modified
 *                   jsr SYMBOL_NAME_STR is changed with 3 instructions
 *                   as it is not supported in SH2
 */
#ifndef _SH7615_SEMAPHORE_H
#define _SH7615_SEMAPHORE_H

#include <linux/linkage.h>

/*
 * SMP- and interrupt-safe semaphores..
 *
 * (C) Copyright 1996 Linus Torvalds
 *
 * SH7615 version  
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
 * routine that actually waits. See arch/sh/lib/semaphore.S
 */


/*Posh2 - Modified as per SH2 instruction set*/
 
 
extern inline void down(struct semaphore * sem)
{
	
	
	register struct semaphore *sem1 __asm__ ("r4") = sem; //STORE THE ADDRESS OF SEM IN SEM1 

	__asm__ __volatile__( 
		"stc sr,r0 \n\t"	// TRANSFER THE CONTENTS OF SR TO SP - 4 
		"mov.l r0, @-r15 \n\t"
 
		"or #0xf0,r0 \n\t"	// DISABLE I (INTERRUPT) BITS (Bits 4 to 7) IN SR REGISTER 
		"ldc  r0,sr \n\t" 

		"mov.l @%0,r0\n\t"	// MOVE FIRST FOUR BYTES IN THE ADDRESS IN SEM1 TO RO REG 
		"dt r0\n\t"		// DECREMENT THE CONTENTS OF REG RO THIS ONE ie COUNT ?1 
 
		"mov.l r0,@%0\n\t"	// COUNT IS AGAIN RETRIEVED TO ADDRESS REFERRED IN SEM1 
		"ldc.l @r15+,sr\n\t"	// SR IS POPPED FROM STACK 
		"mov #0x00,r2\n\t"	// SET 0 IN R2 REG 
 
		"cmp/gt r0,r2\n\t"	// IF R2 > R0 T BIT IS SET.ie IF COUNT IS LESS THAN 0
					// T BIT IS SET.ie RESOURCE IS NOT FREE 

		"bf 2f\n\t"		//BRANCH TO LABEL 1: IF T BIT IS 0 ie RESOURCE IS FREE 
		                        // pos2- changed from 1f -2f to add another label
		"nop \n\t"

		"mov.l 1f,r7 \n\t"	// JUMPS TO A SUBROUTINE WHICH TELLS RESOURCE IS NOT FREE
		"sts.l	pr,@-r15 \n\t"
		"jsr @r7 \n\t" 
		"nop \n\t" 
		"lds.l	@r15+,pr \n\t"
		"bra 2f \n\t"
                "nop  \n\t"
		".align  4 \n\t" 

		"1: .long __down_failed \n\t"
		"2: \n\t" 
		
		: /* no outputs */ 
		: "r" (sem1) 
		: "r0","r7","r2","memory");
}


/*
 * This version waits in interruptible state so that the waiting
 * process can be killed.  The down_failed_interruptible routine
 * returns negative for signalled and zero for semaphore acquired.
 */


/*Posh2 - Modified as per SH2 instruction set  check the output constraint?*/
 

extern inline int down_interruptible(struct semaphore * sem)
{
	
/*	register int retvar __asm__ ("r12"); */
	int retvar ; 

	register struct semaphore *sem1 __asm__ ("r0") = sem; //STORE THE ADDRESS OF SEM IN SEM1 
	/*retvar=0; */

	__asm__ __volatile__( 
		"stc sr,r3 \n\t"	// TRANSFER THE CONTENTS OF SR TO SP - 4 
		"mov.l r3, @-r15 \n\t" 

		"or #0xf0,r3 \n\t"	// DISABLE I (INTERRUPT) BITS IN SR REGISTER 
		"ldc  r3,sr \n\t" 
		"mov.l @%1,r3\n\t"	// MOVE FIRST FOUR BYTES IN THE ADDRESS IN SEM1 TO R3 REG 
		"dt r3\n\t"		// DECREMENT THE CONTENTS OF REG R3 THIS ONE ie COUNT ?1 
 
		"mov.l r3,@%1\n\t"	// COUNT IS AGAIN RETRIEVED TO ADDRESS REFERRED IN SEM1 
		"ldc.l @r15+,sr\n\t"	// SR IS POPPED FROM STACK 
		"mov #0x00,r4\n\t"	// SET 0 IN R4 REG 
 
		"cmp/ge r3,r4\n\t"	// IF R4 >=R3 T BIT IS SET .ie IF COUNT IS LESS THAN 0 T BIT IS SET.ie RESOURCE IS NOT FREE 

		"bf 1f\n\t"		//BRANCH TO LABEL 1: IF T BIT IS 0 ie RESOURCE IS FREE 
		"nop \n\t"

		"jsr " SYMBOL_NAME_STR(__down_failed_interruptible) "\n"// JUMPS TO A SUBROUTINE WHICH TELLS RESOURCE IS NOT FREE 
		"nop \n\t"
		"mov.l @%1,r3\n\t"	// MOVE FIRST FOUR BYTES (ie count )IN THE ADDRESS IN SEM1 TO R3 REG 
		"mov.l r3, %0\n\t"	//Move the count value to ret variable which will be negative if  resource is not free 
		"bra 2f \n\t"		// branch to label 2 without doing anything 
		"nop \n\t"

		"1:"			//resource is free 
		"mov #0x00,r4\n\t"	// move 0 to R4 reg 
		"mov.l r4, %0\n\t"	//move r4 to ret 
		"2: " 
		: "=m" ("retvar") 
		: "r" ("sem1") 
		:"r3", "r4"); 
	return retvar; 

}


/*
 * Primitives to spin on a lock.  Needed only for SMP version ... so
 * somebody should start playing with one of those Sony NeWS stations.
 *
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
 * jumps for both down() and up()*/

/*Posh2 - Modified as per SH2 instruction set*/
 
 
 
extern inline void up(struct semaphore * sem)
{
	register struct semaphore *sem1 __asm__ ("r4") = sem; //STORE THE ADDRESS OF SEM IN SEM1 

	__asm__ __volatile__( 
		"stc sr,r0 \n\t"	// TRANSFER THE CONTENTS OF SR TO SP - 4 
		"mov.l r0, @-r15 \n\t" 

		"or #0xf0,r0 \n\t"	// DISABLE I (INTERRUPT) BITS IN SR REGISTER 
		"ldc  r0,sr \n\t" 

		"mov.l @%0,r0\n\t"	// MOVE FIRST FOUR BYTES IN THE ADDRESS IN SEM1 TO RO REG 
		"add #0x01 r0\n\t"	// INCREMENT THE CONTENTS OF REG RO THIS ONE ie COUNT +1 
 
		"mov.l r0,@%0\n\t"	// COUNT IS AGAIN RETRIEVED TO ADDRESS REFERRED IN SEM1 
		"ldc.l @r15+,sr\n\t"	// SR IS POPPED FROM STACK 
		"mov #0x00,r2\n\t"	// SET 0 IN R2 REG 
 
		"cmp/gt r0,r2\n\t"	// IF R2 > R0 T BIT IS SET.ie IF COUNT IS LESS THAN 0 T BIT IS SET.
					// ie RESOURCE IS NOT FREE 

		"bt 2f\n\t"		// BRANCH TO LABEL 1: IF T BIT IS 1 ie RESOURCE IS NOT FREE 
		"nop \n\t"
		
		"mov.l 1f,r7 \n\t"	// JUMPS TO A SUBROUTINE WHEN  RESOURCE IS FREE
		"sts.l	pr,@-r15 \n\t"
		"jsr @r7 \n\t" 
		"nop \n\t" 
		"lds.l	@r15+,pr \n\t"
		"bra 2f \n\t"
		"nop   \n\t"
		".align	4 \n\t"
		"1: .long __up_wakeup \n\t"
		"2:" 
		: /* no outputs */ 
		: "r" (sem1) 
		: "r0","r7","r2","memory"); 

}

#endif
