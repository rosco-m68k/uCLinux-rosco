/*
 *
 * File Name : asm-shnommu/system.h
 * 
 * Description : This file contains the system.h utilities
 * 
 * Modification History : 
 *
 *   11 Aug 2002 : Changed the macro for switch_to
 *                 for working in SH2
 *
 *   11 Aug 2002 : Chganed the macro xchg
 *   12 Aug 2002 : Changed the macro for HARD_RESET_NOW
 *   
 *   12 Aug 2002 : void wrusp(unsigned long usp) ;
 *                 inline unsigned long rda5(void) ;
 *                 inline void wra5(unsigned long a5) ;
 *          
 *                 Lines for these macros are removed.
 *                 These macros are not required for SH2
 *                 ( Assumption Because this is not seen in H8)
 *    28 Aug 2002: HARD_RESET_NOW macro has to be modified
 *                 in the proper way. Keeping it as blank
 *                 for the time being.
 *    28 Aug 2002 :  IN xchg macro- in case 4
 *      mov %1,r7 \n\t"  posh2 - changing mov.l move 
 *      mov  @r7,%2 \n\t"  posh2 changing mov.l move *
 *      
 *   29 Aug - Commented out th esource lines for bsr
 *      posh2  __asm__ __volatile__("mov.l 1f, r1 \n\t" \
 * 		       "jsr @r1 \n\t" \
 *		        "nop \n\t" \
 *			"1: .long resume \n\t" \
 */

#ifndef _SH7615__SYSTEM_H
#define _SH7615__SYSTEM_H

#include <linux/config.h> /* get configuration macros */
#include <linux/linkage.h>
#include <asm/segment.h>
#include <asm/SH7615.h>

extern inline unsigned long rdusp(void) {
/*
 * posh2 commenting the unnecssary lines. Dont know what it does
 */
	extern unsigned int	sw_usp;
	return(sw_usp);
}

extern inline void wrusp(unsigned long usp) ;
extern inline unsigned long rda5(void) ;
extern inline void wra5(unsigned long a5) ;

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
extern asmlinkage void resume(void);

/*
 * posh2 - jsr  SYMBOL_NAME_STR is changed with  
 * __asm__ __volatile__("mov.l 1f, r1 \n\t" \
 *		       "jsr @r1 \n\t" \
 *		        "nop \n\t" \
 *			"1: .long resume \n\t" \
 */
#define switch_to(prev,next) { \
  register void *_prev __asm__ ("r4") = (prev); \
  register void *_next __asm__ ("r2") = (next); \
  register int _tssoff __asm__ ("r1") = (int)&((struct task_struct *)0)->tss; \
  register char _shared __asm__ ("r3") = ((prev)->mm == (next)->mm); \
  __asm__ __volatile__( "mov.l	r0,@-r15 \n\t" \
   			"mov.l	r1,@-r15 \n\t" \
			"mov.l	r2,@-r15 \n\t" \
   			"mov.l	r3,@-r15 \n\t" \
			"mov.l	r4,@-r15 \n\t" \
   			"mov.l	r5,@-r15 \n\t" \
			"mov.l	r6,@-r15 \n\t" \
   			"mov.l	r7,@-r15 \n\t" \
			"mov.l	r8,@-r15 \n\t" \
   			"mov.l	r9,@-r15 \n\t" \
			"mov.l	r10,@-r15 \n\t" \
   			"mov.l	r11,@-r15 \n\t" \
			"mov.l	r12,@-r15 \n\t" \
   			"mov.l	r13,@-r15 \n\t" \
			"mov.l	r14,@-r15 \n\t" \
			"stc.l	sr,@-r15 \n\t" \
   			"mov.l 1f, r13 \n\t" \
			"jsr @r13 \n\t" \
		        "nop \n\t" \
			"ldc.l	@r15+,sr \n\t" \
			"mov.l	@r15+,r14 \n\t" \
			"mov.l	@r15+,r13 \n\t" \
			"mov.l	@r15+,r12\n\t" \
			"mov.l	@r15+,r11 \n\t" \
			"mov.l	@r15+,r10 \n\t" \
			"mov.l	@r15+,r9\n\t" \
			"mov.l	@r15+,r8 \n\t" \
			"mov.l	@r15+,r7\n\t" \
			"mov.l	@r15+,r6\n\t" \
			"mov.l	@r15+,r5 \n\t" \
			"mov.l	@r15+,r4\n\t" \
			"mov.l	@r15+,r3\n\t" \
			"mov.l	@r15+,r2\n\t" \
			"mov.l	@r15+,r1 \n\t" \
			"mov.l	@r15+,r0\n\t" \
			"bra  2f \n\t" \
			"nop \n\t" \
			" .balign 4 \n\t" \
			"1: \n\t" \
			" .long " "resume \n\t" \
			"2: \n\t" \
			" nop \n\t" \
		       : : "r" (_prev), "r" (_next), "r" (_tssoff), \
		           "r" (_shared) \
		       :  "r1", "r2", "r3", "r4","r13"); \
		      }

/*
 * This macro clears the interupt
 */
#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((volatile struct __xchg_dummy *)(x))


#define sti() __asm__ __volatile__ ( \
	"stc sr, r0\n\t" \
	"and #0x0f, r0\n\t" \
	"or  #0x10,r0 \n\t" \
	"ldc  r0, sr\n\t" \
	:  \
	: \
        : "r0")
	
#define usti() __asm__ __volatile__ ( \
	"stc sr, r0\n\t" \
	"and #0x0f, r0\n\t" \
	"ldc  r0, sr\n\t" \
	:  \
	: \
        : "r0")


/*
 * This macro enables the interupt
 */
#define cli() __asm__ __volatile__ ( \
	"stc sr,r0\n\t" \
	"or #0xf0,r0\n\t" \
	"ldc r0,sr\n\t"\
	: /* no outputs */ \
	: /* no inputs */ \
	: "r0")


#define nop() __asm__ __volatile__ ("nop"::)
#define mb()  __asm__ __volatile__ (""   : : :"memory")

#define save_flags(x) \
__asm__ __volatile__( \
	"stc sr,r7 \n\t" \
	"mov.l r7 ,%0 \n\t" \
	:"=m" (x) \
	: /* no input */ \
	:"r7","memory")

#define restore_flags(x) \
__asm__ __volatile__( \
	"mov.l %0,r7 \n\t" \
	"ldc	r7,sr \n\t" \
	: /* no outputs */ \
	:"m" (x) \
	: "r7","memory")

#define iret() __asm__ __volatile__ ("rte": : :"memory", "r15", "sr")


/*
 * posh2 -just for compilation, keepint a dummy function
 */
extern inline void wrusp(unsigned long usp) 
{
	extern unsigned long sw_usp;
	sw_usp = usp;

}


	 /* Posh2
	  Clears the interupt 
	 * We need to jump to the starting. 
	 * May have to replace 
	 * santhosh - need to find out what is  address need to be moved */


#define HARD_RESET_NOW()  	 
 

static __inline__ unsigned long xchg_u32(volatile int * m, unsigned long val)
{
        unsigned long  retval;

        retval = *m;
        *m = val;
        return retval;
}

static __inline__ unsigned long xchg_u8(volatile unsigned char * m, unsigned long val)
{
	unsigned long retval;

	retval = *m;
	*m = val & 0xff;
	return retval;
}

static __inline__ unsigned long xchg_u16(volatile unsigned short * m, unsigned long val)
{
	unsigned long retval;

	retval = *m;
	*m = val & 0xffff;
	return retval;
}
static __inline__ unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	unsigned long tmp, flags;

	save_flags(flags);
	cli();

	switch (size) {
	case 4:
		tmp= xchg_u32(ptr, x);
		break;
	case 2:
		tmp=xchg_u16(ptr,x);
		break;
	case 1:
		tmp= xchg_u8(ptr, x);
		break;
	}
	restore_flags(flags);
	return tmp;
}

extern __inline__ void toggle_led(void) {

	register unsigned long INT2_LED_REG  __asm__("r12")  = 0x08000004;
	register unsigned long INT2_INIT_LED1 __asm__("r13") = 0x0001;

	__asm__ __volatile__(   
				" mov     %0,r7 \n\t "      /* led adress*/
        			" mov.l   @r7,r8 \n\t"
        			" mov     %1,r4 \n\t"   /* xor value */
        			" xor     r4,r8 \n\t"
        			" mov     #0,r0 \n\t"
        			" mov.l   r0,@r7 \n\t"
      				" mov.l   r8,@r7 \n\t"

			     	: 
				: "r" (INT2_LED_REG),"r"(INT2_INIT_LED1)
				: "r7", "r8", "r0","r4");

}
	
#endif /* _SH7615_SYSTEM_H */
