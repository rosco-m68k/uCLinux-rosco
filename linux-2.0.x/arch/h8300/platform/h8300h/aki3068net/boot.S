/* Hitachi H8/300H Startup                       */
/* Supported Akizuki H8/3068 Netwotk micom board */
	
#define ASSEMBLY

#include <linux/config.h>
#include <linux/linkage.h>

/* Internal Peripheral Registers */
/* Bus Controler */
#define DRCRA  0xFEE026
#define DRCRB  0xFEE027
#define RTMCSR 0xFEE028
#define RTCOR  0xFEE02A
/* I/O Port */
#define P1DDR  0xFEE000
#define P2DDR  0xFEE001
#define P5DDR  0xFEE004
#define P8DDR  0xFEE007
	
	.global SYMBOL_NAME(_stext)
	.global SYMBOL_NAME(_start)
	.global SYMBOL_NAME(_command_line)
	
	.h8300h
	.section .text
	.file	"boot.S"

.global	SYMBOL_NAME(_start)
	
SYMBOL_NAME_LABEL(_start)
SYMBOL_NAME_LABEL(_stext)
	mov.l	#SYMBOL_NAME(COMMAND_START)-2,sp
	ldc	#0x80,ccr
#ifdef CONFIG_ROMKERNEL
/* Internal Peripheral Initialize */
	mov.l	#init_regs,er0
	mov.w	#8,e1
1:
	mov.l	@er0+,er2
	mov.w	@er0+,r1
	mov.b	r1l,@er2
	dec.w	#1,e1
	bne	1b

/* DRAM Setup Delay */
	mov.b	0,r0l
1:
	dec.b	r0l
	bne	1b

/* .data Copy */
	mov.l	#SYMBOL_NAME(__rom_data),er5
	mov.l	#SYMBOL_NAME(__data_start),er6
	mov.l	#SYMBOL_NAME(_edata),er4
	sub.l	er6,er4		
1:	
	eepmov.w
	dec.w	#1,e4
	bpl	1b
#endif

/* .bss zero clear */
	mov.l	#SYMBOL_NAME(__bss_start),er5
	mov.l	er5,er6
	inc.l	#1,er6
	mov.l	#SYMBOL_NAME(__bss_end),er4
	sub.l	er5,er4
	sub.w	r0,r0
	mov.b	r0l,@er5
1:	
	eepmov.w
	dec.w	#1,e0
	bpl	1b

#ifdef CONFIG_RAMKERNEL
/* RAM Interrupt Vector Table Setup */
	mov.l	#SYMBOL_NAME(_ram_vector_image),er5
	mov.l	#SYMBOL_NAME(_ram_vector),er6
	mov.w	#0x100,r4
	eepmov.w
#endif

/* Kernel Command line Copy */
	mov.l	#SYMBOL_NAME(COMMAND_START),er5
	mov.l	#SYMBOL_NAME(_command_line),er6
	mov.w	#512,r4
	eepmov.w

/* Kernel Startup */
	mov.l	#SYMBOL_NAME(_ramend),sp
	ldc	#0x90,ccr
	jsr	@SYMBOL_NAME(start_kernel)
_exit:

	jmp	_exit

	rts

/* Internal Peripheral initialize data */
init_regs:
	.long	RTCOR
	.word	5		; 1.5[ms] reflesh cycle
	.long	RTMCSR
	.word	0x30		; clk x 2048
	.long	DRCRB
	.word	0x90
	.long	DRCRA
	.word	0x22
	.long	P1DDR
	.word	0xff
	.long	P2DDR
	.word	0xff
	.long	P5DDR
	.word	0x01
	.long	P8DDR
	.word	0x0c
init_regs_end:

#ifdef CONFIG_RAMKERNEL
	.section .bootvec
	jmp	@SYMBOL_NAME(_start)
#endif

	.section .bss
SYMBOL_NAME_LABEL(_command_line)
	.space	512
