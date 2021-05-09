/*
 * Copyright (C) 1999		Keith Adams <kma@cse.ogi.edu>
 *				Oregon Graduate Institute
 */

#include <linux/linkage.h>

/*
 * 64-bit math routines that gcc needs but doesn't have.
 */

/*
 * 64-bit addition.
 *
 * registers:
 * g0 = low word of arg 1, g1 = hi word of arg 1
 * g2 = low word of arg 2, g3 = hi word of arg 2
 * it wants result in g0-g1
 */
#define AC_OVERFLOW_BIT	8
	.globl	SYMBOL_NAME(__adddi3)
SYMBOL_NAME_LABEL(__adddi3)
	cmpo	1, 0
	addc	g0, g2, g0
	addc	g1, g3, g1
	ret

/*
 * 64-bit subtraction: a-b = c
 *
 * registers in:
 * g0-g1 = a; g2-g3 = b
 * registers out:
 * g0-g1 = c
 */
	.globl	SYMBOL_NAME(__subdi3)
SYMBOL_NAME_LABEL(__subdi3)
	cmpo	1, 0
	subc	g2, g0, g0
	subc	g3, g1, g1
	ret
