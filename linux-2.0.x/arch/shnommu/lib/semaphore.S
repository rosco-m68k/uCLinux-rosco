/*
 *  Copyright (C) 1996  Linus Torvalds
 *  m68k version by Andreas Schwab
 *
 *  MAR/1999 -- modified to support ColdFire (gerg@moreton.com.au)
 *  Modified by Anurag for SH2 platform  on 9/8/02
 */
	.file "semaphore.S"

#include <linux/linkage.h>
#include <linux/config.h>

	.section .text
	.align  4	

.globl	SYMBOL_NAME(__down_failed)
.globl	SYMBOL_NAME(__down_failed_interruptible)
.globl	SYMBOL_NAME(__up_wakeup)


/*
 * "down_failed" is called with the eventual return address
 * in %a0, and the address of the semaphore in %a1. We need
 * to increment the number of waiters on the semaphore,
 * call "__down()", and then eventually return to try again.
 */

/*Posh  we have to verify the registers that have pushed to the stack*/


ENTRY(__down_failed)
	
	sts.l pr,@-r15
	mov.l  label_down,r7
	jsr  @r7
	nop
	lds.l @r15+,pr
	rts
	nop


ENTRY(__down_failed_interruptible)

	mov.l r0,@-r15
	mov.l r2,@-r15
	mov.l r1,@-r15
/*	mov.l label_dwnintr,r7
	jsr  @r7*/
	nop
	mov.l @r15+,r1
	mov.l @r15+,r2
	mov.l @r15+,r0
	rts

SYMBOL_NAME_LABEL(__up_wakeup)
	sts.l pr,@-r15
	mov.l label_up,r7
	jsr  @r7
	nop
	lds.l @r15+,pr
	rts
	nop
	
    ALIGN
label_down:
	.long  SYMBOL_NAME(___down)
/* label_dwnintr:
	.long	SYMBOL_NAME(__down_interruptible)*/
 label_up:
	.long	SYMBOL_NAME(___up)
