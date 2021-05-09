	.global _stext
	.global _start
	.global __bss_start
	.global _ramend
	.global splash_bits

#define IMR 0xFFFFF304
#undef INIT_LCD

	.data
splash_bits:
#include "penguin.rh"
	.text
_start:
_stext:

	nop
	/* Init like the MOT ADS board.  Check this first */
	moveb	#0x00,   0xfffffb0b	/* Watchdog off */
	moveb	#0x1c,   0xfffff000	/* SCR */

	movew   #0x2400, 0xfffff200	/* PLLCR */
	movew   #0x0123, 0xfffff202	/* PLLFSR */

	moveb	#0x00,   0xfffff40b	/* enable chip select */
	moveb	#0x00,   0xfffff423	/* enable /DWE */
	moveb	#0x08,   0xfffffd0d	/* disable hardmap */
	moveb	#0x07,   0xfffffd0e	/* level 7 interrupt clear */

	movew	#0x8600, 0xfffff100	/* FLASH at 0x10c00000 */
	movew	#0x0187, 0xfffff110	/* 1Meg, 16bit, enable, 0ws */

	movew	#0x8f00, 0xfffffc00	/* DRAM configuration */
	movew	#0x9667, 0xfffffc02	/* DRAM control */
	movew	#0x0000, 0xfffff106	/* DRAM at 0x00000000 */
	movew	#0x068f, 0xfffff116	/* 8Meg, 16bit, enable, 0ws */

	moveb	#0x40,   0xfffff300	/* IVR */
	movel	#0x007FFFFF, %d0	/* IMR */
	movel	%d0,     0xfffff304

	moveb	#0x08,   0xfffff907	/* Ignore CTS */
	movew	#0x010b, 0xfffff902	/* BAUD to 9600 */
	movew	#0xe100, 0xfffff900	/* enable */

	moveb   #0,      0xfffffA27	/* LCKCON - LCD is off */
#ifdef INIT_LCD
	movel	#splash_bits, 0xfffffA00 /* LSSA */
	moveb	#0x28,   0xfffffA05	/* LVPW */
	movew	#0x280,  0xFFFFFa08	/* LXMAX */
	movew	#0x1df,  0xFFFFFa0a	/* LYMAX */
	moveb	#0,      0xfffffa29	/* LBAR */
	moveb	#0,      0xfffffa25	/* LPXCD */
	moveb	#0x08,   0xFFFFFa20	/* LPICF */
	moveb	#0x01,   0xFFFFFA21	/* -ve pol */
	moveb	#0x81,   0xfffffA27	/* LCKCON */
	movew	#0xff00, 0xfffff412	/* LCD pins */
#endif
	moveal	#_ramend - 0x10, %sp	/* stack setup */
	movew	#0x2700, %sr

	moveq	#13, %d7		/* '\r' */
	jsr putc

	moveq	#10, %d7		/* '\n' */
	jsr putc

	moveq	#65, %d7		/* 'A' */
	jsr putc

	movew	#16384, %d0  /* PLL settle wait loop */
L0:
	subw	#1, %d0
	bne	L0

	moveq	#66, %d7
	jsr	putc

#if 1
	/* Copy data segment from ROM to RAM */
	moveal	#__data_rom_start, %a0
	moveal	#__data_start, %a1
	moveal	#__data_end, %a2
	moveq	#67, %d7
	jsr	putc

	/* Copy %a0 to %a1 until %a1 == %a2 */
LD1:
	movel	%a0@+, %d0
	movel	%d0, %a1@+
	cmpal	%a1, %a2
	bhi	LD1
#endif

	moveq	#68, %d7
	jsr	putc

	moveal	#__bss_start, %a0
	moveal	#end, %a1

	/* Copy 0 to %a0 until %a0 == %a1 */
L1:
	movel	#0, %a0@+
	cmpal	%a0, %a1
	bhi	L1

	moveq	#69, %d7
	jsr	putc

	pea	0
	pea	env
	pea	%sp@(4)
	pea	0

	moveq	#70, %d7
	jsr	putc

lp:
	jsr	start_kernel
        jmp lp
_exit:

	jmp	_exit


putc:
	moveb	%d7,0xfffff907
pclp:
	movew	0xfffff906, %d7
	andw	#0x2000, %d7
	beq	pclp
	rts

	.data
env:
	.long	0
	.text
