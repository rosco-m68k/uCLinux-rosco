#ifndef _I960_DELAY_H
#define _I960_DELAY_H

extern unsigned long loops_per_sec;
#include <linux/kernel.h>
#include <linux/config.h>

/*
 * Copyright (C) 1995 Erik Walthinsen (omega@cse.ogi.edu)
 *
 * Delay routines, using a pre-computed "loops_per_second" value.
 */

extern __inline__ void __delay(unsigned long loops)
{
	while (loops--);
}

/*
 * Use only for very small delays ( < 1 msec).  Should probably use a
 * lookup table, really, as the multiplications take much too long with
 * short delays.  This is a "reasonable" implementation, though (and the
 * first constant multiplications gets optimized away if the delay is
 * a constant)  
 */
extern __inline__ void udelay(unsigned long usecs)
{
	usecs *= loops_per_sec;
	usecs /= 1000000;
	__delay(usecs);
}

#define muldiv(a, b, c)    (((a)*(b))/(c))

#endif /* defined(_I960_DELAY_H) */
