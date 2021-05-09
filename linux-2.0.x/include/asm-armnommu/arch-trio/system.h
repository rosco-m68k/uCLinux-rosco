/*
 * linux/include/asm-arm/arch-trio/system.h
 *
 * Copyright (c) 1996 Russell King
 */
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <asm/hardware.h>
#include <asm/io.h>

extern __inline__ void arch_hard_reset (void)
{
	outl((1 << 11) | 1, SIAP_RST); 

	while (1);
}

#endif
