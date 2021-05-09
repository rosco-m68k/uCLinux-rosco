/*
 * linux/include/asm-arm/atomic.h
 *
 * Copyright (c) 1996 Russell King.
 *
 * Changelog:
 *  27-06-1996	RMK	Created
 *  13-04-1997	RMK	Made functions atomic
 */
#ifndef __ASM_ARM_ATOMIC_H
#define __ASM_ARM_ATOMIC_H

#include <asm/system.h>

typedef int atomic_t;

extern __inline__ void atomic_add(atomic_t i, atomic_t *v)
{
	unsigned long flags;

	save_flags_cli (flags);
	*v += i;
	restore_flags (flags);
}

extern __inline__ void atomic_sub(atomic_t i, atomic_t *v)
{
	unsigned long flags;

	save_flags_cli (flags);
	*v -= i;
	restore_flags (flags);
}

extern __inline__ void atomic_inc(atomic_t *v)
{
	unsigned long flags;

	save_flags_cli (flags);
	*v += 1;
	restore_flags (flags);
}

extern __inline__ void atomic_dec(atomic_t *v)
{
	unsigned long flags;

	save_flags_cli (flags);
	*v -= 1;
	restore_flags (flags);
}

extern __inline__ int atomic_dec_and_test(atomic_t *v)
{
	unsigned long flags;
	int result;

	save_flags_cli (flags);
	*v -= 1;
	result = (*v == 0);
	restore_flags (flags);

	return result;
}

#endif

