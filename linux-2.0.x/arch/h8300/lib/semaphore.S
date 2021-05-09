/*
 *  linux/arch/h8300/lib/semaphore.S
 *
 *  Copyright (C) 1996  Linus Torvalds
 *
 *  m68k version by Andreas Schwab
 *
 *  MAR/1999 -- modified to support ColdFire (gerg@moreton.com.au)
 *
 *  H8/300H by Yoshinori Sato
 *
 */

#include <linux/linkage.h>
#include <linux/config.h>

	.h8300h

	.section .text
	.align	1

.globl	SYMBOL_NAME(__down_failed)
.globl	SYMBOL_NAME(__down_failed_interruptible)
.globl	SYMBOL_NAME(__up_wakeup)
	
/*
 * "down_failed" is called with the eventual return address
 * in %a0, and the address of the semaphore in %a1. We need
 * to increment the number of waiters on the semaphore,
 * call "__down()", and then eventually return to try again.
 */

SYMBOL_NAME_LABEL(__down_failed)
	mov.l er0,@-sp
	mov.l er2,@-sp
	mov.l er1,@-sp
	jsr SYMBOL_NAME(__down)
	mov.l @sp+,er1
	mov.l @sp+,er2
	mov.l @sp+,er0
	rts

SYMBOL_NAME_LABEL(__down_failed_interruptible)
	mov.l er0,@-sp
	mov.l er2,@-sp
	mov.l er1,@-sp
	jsr SYMBOL_NAME(__down_interruptible)
	mov.l @sp+,er1
	mov.l @sp+,er2
	mov.l @sp+,er0
	rts

SYMBOL_NAME_LABEL(__up_wakeup)
	mov.l er0,@-sp
	mov.l er2,@-sp
	mov.l er1,@-sp
	jsr SYMBOL_NAME(__up)
	mov.l @sp+,er1
	mov.l @sp+,er2
	mov.l @sp+,er0
	rts
