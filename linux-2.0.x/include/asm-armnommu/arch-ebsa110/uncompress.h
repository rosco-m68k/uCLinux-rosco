/*
 * linux/include/asm-arm/arch-ebsa/uncompress.h
 *
 * Copyright (C) 1996 Russell King
 */

/*
 * This does not append a newline
 */
static void puts(const char *s)
{
	__asm__ __volatile__("
	ldrb	%0, [%1], #1
	teq	%0, #0
	beq	3f
1:	strb	%0, [%2]
2:	ldrb	%0, [%2, #0x14]
	and	%0, %0, #0x60
	teq	%0, #0x60
	bne	2b
	teq	%0, #'\n'
	moveq	%0, #'\r'
	beq	1b
	ldrb	%0, [%1], #1
	teq	%0, #0
	bne	1b
3:	" : : "r" (0), "r" (s), "r" (0xf0000be0) : "cc");
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
