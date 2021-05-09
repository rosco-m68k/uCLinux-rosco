/* atomic.h: 
 *
 * Copyright (C) 2001 Vic Phillips (vic@microtronix.com)
 *
 * Copyright (C) 1996 David S. Miller (davem@caip.rutgers.edu)
 */

#ifndef __ARCH_NIOS_ATOMIC__
#define __ARCH_NIOS_ATOMIC__

#include <asm/system.h>

typedef int atomic_t;

extern __inline__ void atomic_add(atomic_t i, atomic_t *v)
{
	_disable_interrupts();
	*v += i;
	_enable_interrupts();
}

extern __inline__ void atomic_sub(atomic_t i, atomic_t *v)
{
	_disable_interrupts();
	*v -= i;
	_enable_interrupts();
}

extern __inline__ int atomic_sub_and_test(atomic_t i, atomic_t *v)
{
	int result;
	_disable_interrupts();
	*v -= i;
	result = (*v == 0);
	_enable_interrupts();
	return result;
}

extern __inline__ void atomic_inc(atomic_t *v)
{
	_disable_interrupts();
	*v += 1;
	_enable_interrupts();
}

extern __inline__ void atomic_dec(atomic_t *v)
{
	int i = 1;					/* the compiler optimizes better this way */
	_disable_interrupts();
	*v -= i;
	_enable_interrupts();
}

extern __inline__ int atomic_dec_and_test(atomic_t *v)
{
	int result;
	int i = 1;					/* the compiler optimizes better this way */
	_disable_interrupts();
	*v -= i;
	result = (*v == 0);
	_enable_interrupts();
	return result;
}

extern __inline__ int atomic_inc_return(atomic_t *v)
{
	int result;
	_disable_interrupts();
	result = ++*v;
	_enable_interrupts();
	return result;
}

extern __inline__ int atomic_dec_return(atomic_t *v)
{
	int result;
	int i = 1;					/* the compiler optimizes better this way */
	_disable_interrupts();
	*v -= i;
	result = *v;
	_enable_interrupts();
	return result;
}


#endif /* !(__ARCH_NIOS_ATOMIC__) */
