/*
 * linux/include/asm-arm/arch-ebsa/system.h
 *
 * Copyright (c) 1996 Russell King.
 */
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

extern __inline__ void arch_hard_reset (void)
{
	/*
	 * loop endlessly
	 */
	while (1);
}

#endif
