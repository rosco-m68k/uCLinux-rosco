/*
 * uncompress.h 
 *
 *  Copyright (C) 2002 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _ARCH_UNCOMPRESS_H_
#define _ARCH_UNCOMPRESS_H_

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <linux/serial_reg.h>

static	volatile u32* UART_BASE;
#define TX_DONE (UART_LSR_TEMT|UART_LSR_THRE)

static __inline__ void putc(char c)
{
	/* Check THRE and TEMT bits before we transmit the character.
	 */
	while ((UART_BASE[UART_LSR] & TX_DONE) != TX_DONE); 
	*UART_BASE = c;
}

/*
 * This does not append a newline
 */
static void puts(const char *s)
{
	while (*s)
	{
		putc(*s);
		if (*s == '\n')
			putc('\r');
		s++;
	}
}

static __inline__ void arch_decomp_setup(void)
{
        unsigned long mach_type;

        asm("mov %0, r7" : "=r" (mach_type) ); 
	if(mach_type == MACH_TYPE_ADI_COYOTE)
		UART_BASE = IXP425_UART2_BASE_PHYS;
	else
		UART_BASE = IXP425_UART1_BASE_PHYS;
}

#define arch_decomp_wdog()

#endif
