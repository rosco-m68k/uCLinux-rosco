/*
  GDB Display Message
*/

#include <linux/linkage.h>

	.h8300h
	.section .text

.global SYMBOL_NAME(gdb_output)

/* void gdb_output(const char *msg) */
SYMBOL_NAME_LABEL(gdb_output)
1:	sub.l	er2,er2
	mov.b	@er0+,r2l
	beq	1f
	jsr	@0xc4
	bra	1b
1:	rts
