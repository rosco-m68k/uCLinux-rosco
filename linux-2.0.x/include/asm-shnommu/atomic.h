/*
 * Modification history
 *
 * 28 Aug - changed the lines for or 0xfo with SR. 
 *
 * changes in the following macro. 
 *          atomic_sub
 *          atomic_inc
 *          atomic_dec
 *          atomic_dec_and_test
 *         ldcc @r15,sr  is changed to ldc.l @r15+, sr
 *
 */
#ifndef __ARCH_SH7615_ATOMIC__
#define __ARCH_SH7615_ATOMIC__

#include <linux/config.h>

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

/*
 * We do not have SMP SH2 systems, so we don't have to deal with that.
 */

typedef int atomic_t;

/* Posh2
 *  Modified taking H8 as reference 
 * output constraint of i changed ?
 */
static __inline__ void atomic_add(atomic_t i, atomic_t *v)
{
	__asm__ __volatile__(	"stc.l sr,@-r15 \n\t" // push the contents of sr reg in stack
				"stc sr,r0 \n\t"
				"or  #0xf0,r0 \n\t"
				"ldc  r0,sr \n\t"
				"mov  %0,r0 \n\t"	// move v(holds address) to r0
				"mov  r0,r2 \n\t"
				"mov.l @r0,r0 \n\t"	// move contents of v to r0
				"mov  %1,r1 \n\t"	// move i to r1 reg
				"add  r1,r0 \n\t"	// adds
				"mov.l r0,@r2 \n\t"	// move the result to v	
				"ldc.l @r15+,sr \n\t"	// pop back the sr reg
				::"r"(v),"r" (i):"r0","r1","r2");
			
}

/* Posh2
 * Modified taking H8 as reference 
 * output constraint of i changed ?
 */
static __inline__ void atomic_sub(atomic_t i, atomic_t *v)
{
		__asm__ __volatile__(	"stc.l sr,@-r15 \n\t" 	// push the contents of sr reg in stack
				        "stc  sr, r0 \n\t" 	// moving sr -> r0
				        "or #0xf0,r0 \n\t" 	// Disable interrupt bits in sr register
				        "ldc r0, sr   \n\t" 	// moving back to sr
					"mov %0,r0 \n\t"	// move v (holds address)to r0
					"mov r0,r2 \n\t"
					"mov.l	@r0,r0 \n\t"	// move contents of v to r0
					"mov %1,r1 \n\t"	// move i to r1 reg
					"sub r1,r0 \n\t"	// subtract
					"mov.l r0,@r2 \n\t"	// move the result to v
					"ldc.l @r15+,sr \n\t"	// pop back the sr reg
					::"r"(v),"r" (i):"r0","r1","r2");
	
}

/* Posh2
 * Modified taking H8 as reference 
 */
static __inline__ void atomic_inc(atomic_t *v)
{
	/*
	 * posh2 - modified the sr or operation with the 3 instructions for sr
	 */
	__asm__ __volatile__(	"stc.l sr,@-r15 \n\t" 	// push the contents of sr reg in stack
				"stc  sr, r0 \n\t" 	// moving sr -> r0
				"or #0xf0,r0 \n\t" 	// Disable interrupt bits in sr register
				"ldc r0, sr   \n\t" 	// moving back to sr
				 "mov %0,r0 \n\t"	// move v (holds address) to r0
				 "mov r0,r1 \n\t"
				 "mov.l @r0,r0 \n\t"	// move contents of v to r0
				 "add #0x01,r0 \n\t"	// subtract 1 from r0 reg
				 "mov.l r0,@r1 \n\t"	// move the result to v
				 "ldc.l @r15+,sr \n\t"	// pop back the sr reg
				::"r"(v):"r0","r1");
}

/* Posh2
 * Modified taking H8 as reference 
 */
static __inline__ void atomic_dec(atomic_t *v)
{
	/*
	 * posh2 - modified the sr or operation with the 3 instructions for sr
	 */
		__asm__ __volatile__(	"stc.l sr,@-r15 \n\t" 	// push the contents of sr reg in stack
				        "stc  sr, r0 \n\t" 	// moving sr -> r0
					"or #0xf0,r0 \n\t" 	// Disable interrupt bits in sr register
					"ldc r0, sr   \n\t" 	// moving back to sr
					"mov %0,r0 \n\t"	// move v(holds address) to r0
					"mov r0,r1 \n\t"
					"mov.l @r0,r0 \n\t"	// move contents of v to r0
					"dt r0 \n\t"		// subtract 1 from r0 reg
					"mov.l r0,@r1 \n\t"	// move the result to v
					"ldc.l @r15+,sr \n\t"	// pop back the sr reg
					::"r"(v):"r0","r1");
}

/* Posh2
 * Modified taking H8 as reference 
 */
static __inline__ int atomic_dec_and_test(atomic_t *v)
{
	int c;
	/*
	 * posh2 - modified the sr or operation with the 3 instructions for sr
	 */
	__asm__ __volatile__(	"stc.l sr,@-r15 \n\t" 	// push the contents of sr reg in stack
				 "stc  sr, r0 \n\t" 	// moving sr -> r0
				 "or #0xf0,r0 \n\t" 	// Disable interrupt bits in sr register
				 "ldc r0, sr   \n\t" 	// moving back to sr
				"mov %1,%0 \n\t"      	// move v (holds address) to c
				"mov %0,r0 \n\t"      	// mov c to r0 reg
				"mov.l @r0,r0\n\t"
				"dt  r0\n\t"        	// subtract 1 from r0 reg
				"mov %1,r1\n\t"
				"mov.l r0,@r1 \n\t" 	// move the result to c
				 "mov r0,%0 \n\t"
				"ldc.l @r15+,sr \n\t" 	// pop back the sr reg
				:"=r"(c):"r"(v):"r0","r1");
	return c ==0;
}

#endif /* __ARCH_SH7615_ATOMIC __ */
