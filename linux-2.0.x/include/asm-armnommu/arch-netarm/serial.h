/*
 * linux/include/asm-arm/arch-a5k/serial.h
 *
 * Copyright (c) 1996 Russell King.
 *
 * Changelog:
 *  15-10-1996	RMK	Created
 */
#ifndef __ASM_ARCH_SERIAL_H
#define __ASM_ARCH_SERIAL_H

/*
 * This assumes you have a 1.8432 MHz clock for your UART.
 *
 * It'd be nice if someone built a serial card with a 24.576 MHz
 * clock, since the 16550A is capable of handling a top speed of 1.5
 * megabits/second; but this requires the faster clock.
 */
#define BASE_BAUD (1843200 / 16)

#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)

     /* UART CLK        PORT  IRQ     FLAGS        */
#define RS_UARTS \
	{ 0, BASE_BAUD, 0x3F8, 10, STD_COM_FLAGS },	/* ttyS0 */	\
	{ 0, BASE_BAUD, 0x2F8, 10, STD_COM_FLAGS }	/* ttyS1 */	


#endif
