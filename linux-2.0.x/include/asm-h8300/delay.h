#ifndef _H8300_DELAY_H
#define _H8300_DELAY_H

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
	__asm__ __volatile__ ("mov.l %0,er0\n\t1: dec.l #1,er0\n\tbpl 1b"
		::"r" (loops):"er0");
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
	/* Sigh */
	__delay(usecs);
}

#endif /* _H8300_DELAY_H */
