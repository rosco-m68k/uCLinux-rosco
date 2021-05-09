	.global setjmp
	.global longjmp

	.align 4
setjmp:
	/* Save registers */
	stmia	r0, {v1-v6, sl, fp, sp, lr}

	mov r0,	#0
	mov	pc, lr

	.align 4
longjmp:
	mov	ip, r0
	movs	r0, r1		/* get the return value in place */
	moveq	r0, #1		/* can't let setjmp() return zero! */

	ldmia	ip, {v1-v6, sl, fp, sp, pc}
