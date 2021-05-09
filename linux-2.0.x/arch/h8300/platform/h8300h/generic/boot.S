/* Hitachi H8/300H Startup */

	
#define ASSEMBLY

#include <linux/config.h>
#include <linux/linkage.h>

	.global SYMBOL_NAME(_stext)
	.global SYMBOL_NAME(_start)
	.global SYMBOL_NAME(_command_line)
	
	.h8300h
	.section .text
	.file	"boot.S"

.global	SYMBOL_NAME(_start)
	
SYMBOL_NAME_LABEL(_start)
SYMBOL_NAME_LABEL(_stext)
	mov.l	#SYMBOL_NAME(_ramend),sp
	ldc	#0x80,ccr
	mov.l	#SYMBOL_NAME(__bss_start),er5
	mov.l	er5,er6
	inc.l	#1,er6
	mov.l	#SYMBOL_NAME(__bss_end),er4
	sub.l	er5,er4
	sub.w	r0,r0
	mov.b	r0l,@er5
1:	
	eepmov.w
	mov.w	e0,e0
	beq	1f
	dec.w	#1,e0
	bra	1b
1:	
	mov.l	#SYMBOL_NAME(COMMAND_START),er5
	mov.l	#SYMBOL_NAME(_command_line),er6
	mov.w	#512,r4
	eepmov.w
	mov.l	#SYMBOL_NAME(_begin_data),er5
	mov.l	#SYMBOL_NAME(__data_start),er6
	mov.l	#SYMBOL_NAME(_edata),er4
	sub.l	er6,er4		
	;eepmov.w		

	ldc	#0x90,ccr
	jsr	@SYMBOL_NAME(start_kernel)
_exit:

	jmp	_exit

	rts

	.section .bss
SYMBOL_NAME_LABEL(_command_line)
	.space	512
