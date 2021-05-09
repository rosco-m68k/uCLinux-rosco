/*
 * Modification History : 
 *
 * 13 Aug 2002 : __delay
 *
 *     posh2 :
 *     This function is changed with SH instructions.
 *
 *     udelay is taken from H8 code
 *
 *     There was a function muldiv which was commented , is removed. 
 *
 */
#ifndef _SH7615_DELAY_H
#define _SH7615__DELAY_H

extern unsigned long loops_per_sec;
#include <linux/kernel.h>
#include <linux/config.h>

/*
 * Copyright (C) 1994 Hamish Macdonald
 *
 * Delay routines, using a pre-computed "loops_per_second" value.
 */

extern __inline__ void __delay(unsigned long loops)
{
        __asm__ __volatile__ ("mov %0,r0 \n\t"
			      "1: dt r0 \n\t"
			       "bf 1b \n\t"
			       ::"r" (loops):"r0");

	/*
	 * while( loops--)
	 * This is what it does
	 *
	 * loops     ->Ro
	 * DT   ( Ro = Ro- 1) if RO == 0 T Bit is set
	 * BF    ( Branch if T bit is set. I
	 */
}



/*
 * posh2 - This fucntuion is taken from H8 code
 * Use only for very small delays ( < 1 msec).  Should probably use a
 * lookup table, really, as the multiplications take much too long with
 * short delays.  This is a "reasonable" implementation, though (and the
 * first constant multiplications gets optimized away if the delay is
 * a constant)  
 */

extern __inline__ void udelay(unsigned long usecs)
{
	/* Sigh */
	__delay(usecs);
}

#define muldiv(a, b, c)    (((a)*(b))/(c))


#endif /* defined(_SH7615_DELAY_H) */
