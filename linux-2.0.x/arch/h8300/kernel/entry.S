/* -*- mode: asm -*-
 *
 *  linux/arch/h8300/kernel/entry.S
 *
 *  Yoshinori Sato <qzb04471@nifty.ne.jp>
 *
 * Based on:
 *
 *  linux/arch/m68knommu/platform/68EZ328/entry.S
 *
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *                      The Silver Hammer Group, Ltd.
 *
 * (The icky duplicate functions are my fault -- kja)
 *
 *  linux/arch/m68k/kernel/entry.S
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file README.legal in the main directory of this archive
 * for more details.
 *
 * Linux/m68k support by Hamish Macdonald
 *
 * 68060 fixes by Jesper Skov
 *
 */

/*
 * entry.S  contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call.
 *
 */

/*
 * 12/03/96 Jes: Currently we only support m68k single-cpu systems, so
 *               all pointers that used to be 'current' are now entry
 *               number 0 in the 'current_set' list.
 */

#include <linux/sys.h>
#include <linux/config.h>
#include <linux/linkage.h>
#include <asm/setup.h>
#include <asm/segment.h>

LENOSYS = 38

/*
 * these are offsets into the task-struct
 */
LTASK_STATE	=  0
LTASK_COUNTER	=  4
LTASK_PRIORITY	=  8
LTASK_SIGNAL	= 12
LTASK_BLOCKED	= 16
LTASK_FLAGS	= 20

/* the following macro is used when enabling interrupts */

LER0		= 0x14
LORIG_ER0	= 0x18
LCCR		= 0x21

	.h8300h

/*
 * This defines the normal kernel pt-regs layout.
 *
 * regs are a2-a6 and d6-d7 preserved by C code
 * the kernel doesn't mess with usp unless it needs to
 */
	.macro	SAVE_ALL
	mov.l	er0,@int_er0
	stc	ccr,r0l
	btst	#4,r0l
	bne	1f	
	mov.l	sp,@enter_sp
	mov.l	@sp,er0
	mov.l	@kernel_sp,sp
	stc	ccr,@-sp
	mov.l	er0,@-sp
	orc	#0x10,ccr
	bra	2f
1:
	stc	ccr,@-sp
	mov.l	er0,@-sp
2:	
	mov.l	@int_er0,er0
	mov.l	er0,@-sp
	mov.l	er0,@-sp
	mov.l	er5,@-sp
	mov.l	er4,@-sp
	mov.l	er3,@-sp
	mov.l	er2,@-sp
	mov.l	er1,@-sp
	.endm

	.macro	RESTORE_ALL
	mov.l	@sp+,er1
	mov.l	@sp+,er2
	mov.l	@sp+,er3
	mov.l	@sp+,er4
	mov.l	@sp+,er5
	mov.l	@sp+,er0
	adds	#4,sp
	adds	#4,sp
	orc	#0x80,ccr
	mov.l	er0,@int_er0
	mov.w	@sp+,r0
	btst	#4,r0l
	bne	1f
	mov.l	sp,@kernel_sp
	mov.l	@enter_sp,sp
1:
	mov.l	@int_er0,er0
	rte
	.endm

#define SWITCH_STACK_SIZE (1*4+4)	/* includes return address */

#define SAVE_SWITCH_STACK \
	mov.l	er6,@-sp     

#define RESTORE_SWITCH_STACK \
	mov.l	@sp+,er6

.globl SYMBOL_NAME(system_call)
.globl SYMBOL_NAME(resume), SYMBOL_NAME(ret_from_exception)
.globl SYMBOL_NAME(ret_from_signal)
.globl SYMBOL_NAME(sys_call_table)
.globl SYMBOL_NAME(sys_fork), SYMBOL_NAME(sys_clone)
.globl SYMBOL_NAME(ret_from_interrupt), SYMBOL_NAME(bad_interrupt)
.globl SYMBOL_NAME(rdusp),SYMBOL_NAME(wrusp)
	
	.section .text
	.align	1
interrupt_entry:
	sub.l	er1,er1
	dec.l	#1,er1
	mov.l	er1,@(LORIG_ER0)	
	mov.l	@SYMBOL_NAME(intr_count),er1
	inc.l	#1,er1
	mov.l	er1,@SYMBOL_NAME(intr_count)
	mov.l	sp,er1
	sub.w	e0,e0
	jsr	@SYMBOL_NAME(process_int)
	jmp	@SYMBOL_NAME(ret_from_interrupt)
	
SYMBOL_NAME_LABEL(interrupt7)
	SAVE_ALL
	mov.w	#7,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt12)
	SAVE_ALL
	mov.w	#12,r0
	bra.w	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt13)
	SAVE_ALL
	mov.w	#13,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt14)
	SAVE_ALL
	mov.w	#14,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt15)
	SAVE_ALL
	mov.w	#15,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt16)
	SAVE_ALL
	mov.w	#16,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt17)
	SAVE_ALL
	mov.w	#17,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt20)
	SAVE_ALL
	mov.w	#20,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt21)
	SAVE_ALL
	mov.w	#21,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt22)
	SAVE_ALL
	mov.w	#22,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt23)
	SAVE_ALL
	mov.w	#23,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt24)
	SAVE_ALL
	mov.w	#24,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt25)
	SAVE_ALL
	mov.w	#25,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt26)
	SAVE_ALL
	mov.w	#26,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt27)
	SAVE_ALL
	mov.w	#27,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt28)
	SAVE_ALL
	mov.w	#28,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt29)
	SAVE_ALL
	mov.w	#29,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt30)
	SAVE_ALL
	mov.w	#30,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt31)
	SAVE_ALL
	mov.w	#31,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt32)
	SAVE_ALL
	mov.w	#32,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt33)
	SAVE_ALL
	mov.w	#33,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt34)
	SAVE_ALL
	mov.w	#34,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt35)
	SAVE_ALL
	mov.w	#35,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt36)
	SAVE_ALL
	mov.w	#36,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt37)
	SAVE_ALL
	mov.w	#37,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt38)
	SAVE_ALL
	mov.w	#38,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt39)
	SAVE_ALL
	mov.w	#39,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt40)
	SAVE_ALL
	mov.w	#40,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt41)
	SAVE_ALL
	mov.w	#41,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt42)
	SAVE_ALL
	mov.w	#42,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt43)
	SAVE_ALL
	mov.w	#43,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt44)
	SAVE_ALL
	mov.w	#44,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt45)
	SAVE_ALL
	mov.w	#45,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt46)
	SAVE_ALL
	mov.w	#46,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt47)
	SAVE_ALL
	mov.w	#47,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt48)
	SAVE_ALL
	mov.w	#48,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt49)
	SAVE_ALL
	mov.w	#49,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt50)
	SAVE_ALL
	mov.w	#50,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt51)
	SAVE_ALL
	mov.w	#51,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt52)
	SAVE_ALL
	mov.w	#52,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt53)
	SAVE_ALL
	mov.w	#53,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt54)
	SAVE_ALL
	mov.w	#54,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt55)
	SAVE_ALL
	mov.w	#55,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt56)
	SAVE_ALL
	mov.w	#56,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt57)
	SAVE_ALL
	mov.w	#57,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt58)
	SAVE_ALL
	mov.w	#58,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt59)
	SAVE_ALL
	mov.w	#59,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt60)
	SAVE_ALL
	mov.w	#60,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt61)
	SAVE_ALL
	mov.w	#61,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(interrupt62)
	SAVE_ALL
	mov.w	#62,r0
	bra	interrupt_entry:16

SYMBOL_NAME_LABEL(interrupt63)
	SAVE_ALL
	mov.w	#63,r0
	bra	interrupt_entry:16
	
SYMBOL_NAME_LABEL(reschedule)
	/* save top of frame */
	mov.l	sp,er0
	mov.l	er0,@-sp
	jsr	@SYMBOL_NAME(set_esp0)
	adds	#4,sp

	mov.l	#SYMBOL_NAME(ret_from_exception),er0
	mov.l	er0,@-sp
	jmp	@SYMBOL_NAME(schedule)

SYMBOL_NAME_LABEL(system_call)
	SAVE_ALL
	mov.l	er0,er3
	mov.l	#-LENOSYS,er0
	mov.l	er0,@(LER0,sp)

	/* save top of frame */
	mov.l	sp,er0
	jsr	@SYMBOL_NAME(set_esp0)

	cmp.l	#NR_syscalls,er3
	bcc	SYMBOL_NAME(ret_from_exception):16
	shll.l	er3
	shll.l	er3
	mov.l	#SYMBOL_NAME(sys_call_table),er0
	add.l	er3,er0
	mov.l	@er0,er0
	beq	SYMBOL_NAME(ret_from_exception):16	
	mov.l	@SYMBOL_NAME(current_set),er2
	mov.b	@(LTASK_FLAGS+3,er2),r2l
	btst	#5,r2l			/* PF_TRACESYS */
	bne	1f
	mov.l	er0,er5
	mov.l	@(0,sp),er0
	mov.l	@(4,sp),er1
	mov.l	@(8,sp),er2
	mov.l	@(12,sp),er3
	mov.l	@(16,sp),er4
	mov.l	er4,@-sp
	mov.l	er3,@-sp
	jsr	@er5
	adds	#4,sp
	adds	#4,sp
	mov.l	er0,@(LER0,sp)		/* save the return value */
	jmp	@SYMBOL_NAME(ret_from_exception)
1:
	subs	#4,sp
	SAVE_SWITCH_STACK
	mov.l	er0,er5
	jsr	SYMBOL_NAME(syscall_trace)
	RESTORE_SWITCH_STACK
	adds	#4,sp
	mov.l	@(0,sp),er0
	mov.l	@(4,sp),er1
	mov.l	@(8,sp),er2
	mov.l	@(12,sp),er3
	mov.l	@(16,sp),er4
	mov.l	er4,@-sp
	mov.l	er3,@-sp
	jsr	@er5
	adds	#4,sp
	adds	#4,sp
	mov.l	er0,@(LER0,sp)		/* save the return value */
	subs	#4,sp
	SAVE_SWITCH_STACK
	jsr	SYMBOL_NAME(syscall_trace)
SYMBOL_NAME_LABEL(ret_from_signal)
	RESTORE_SWITCH_STACK
	adds	#4,sp
SYMBOL_NAME_LABEL(ret_from_exception)
	mov.b	@(LCCR,sp),r0l
	btst	#4,r0l			/* check if returning to kernel */
	bne	2f:16			/* if so, skip resched, signals */
	mov.l	@SYMBOL_NAME(need_resched),er0
	beq	1f
	jmp	@SYMBOL_NAME(reschedule)
1:	mov.l	@SYMBOL_NAME(current_set),er0
	cmp.l	#SYMBOL_NAME(task),er0	/* task[0] cannot have signals */
	beq	2f:16
	mov.b	@(LTASK_FLAGS+1,er0),r1l
	mov.b	r1l,r1h
	bclr	#5,r1h
	mov.b	r1h,@(LTASK_FLAGS+1,er0)
	bclr	#5,r1l			/* check for delayed trace */
	beq	1f
	sub.l	er1,er1
	inc.l	#1,er1
	mov.l	er1,@-sp		/* send SIGTRAP */
	mov.l	er0,@-sp
	adds	#4,er1
	mov.l	er1,@-sp
	jsr	@SYMBOL_NAME(send_sig)
	add.l	#12,sp
	mov.l	@SYMBOL_NAME(current_set),er0

1:
	mov.l	@(LTASK_STATE,er0),er1	/* state */
	beq	1f
	jmp	@SYMBOL_NAME(reschedule)
1:	
	mov.l	@(LTASK_COUNTER,er0),er1	/* counter */
	bne	1f
	jmp	@SYMBOL_NAME(reschedule)
1:	
	mov.l	@(LTASK_BLOCKED,er0),er1
	mov.l	er1,er2			/* save blocked in d1 for sig handling */
	not.l	er1
	mov.b	@(LTASK_FLAGS+3,er0),r3l
	btst	#4,r3l			/* PF_PTRACED */
	beq	1f
	sub.l	er1,er1
	dec.l	#1,er1			/* let the debugger see all signals */
1:	mov.l	@(LTASK_SIGNAL,er0),er0
	and.l	er0,er1
	bne	Lsignal_return:16

2:
	mov.l	@fork_ret_vec,er0
	sub.l	er1,er1
	mov.l	er1,@fork_ret_vec
	mov.l	er0,er0
	beq.w	3f			/* Normal Return */
	/* vfork return */
	mov.w	e0,r1
	mov.b	r1h,r1l
	mov.b	#0x07,r1h
	mov.w	r1,@fork_ret		/* ldc #??,ccr */
	mov.w	e0,r1
	mov.b	#0x5a,r1h
	mov.w	r1,e0
	mov.l	er0,@fork_ret+2		/* jmp @?? */
	mov.l	@sp+,er1
	mov.l	@sp+,er2
	mov.l	@sp+,er3
	mov.l	@sp+,er4
	mov.l	@sp+,er5
	mov.l	@sp+,er0
	adds	#4,sp
	add	#4,sp
	orc	#0x80,ccr
	mov.l	er0,@int_er0
	mov.w	@sp+,r0
	btst	#4,r0l
	bne	1f
	mov.l	sp,@kernel_sp
	mov.l	@enter_sp,sp
1:
	mov.l	@int_er0,er0
	beq	2f
	adds	#4,sp
2:	
	jmp	@fork_ret
3:
	RESTORE_ALL			/* Does RTE */

Lsignal_return:
	subs	#4,sp
	SAVE_SWITCH_STACK
	mov.l	sp,er1
	adds	#4,er1
	adds	#4,er1
	mov.l	er2,er0
	jsr	@SYMBOL_NAME(do_signal)
	RESTORE_SWITCH_STACK
	adds	#4,sp
	RESTORE_ALL

SYMBOL_NAME_LABEL(ret_from_interrupt)
	/* check if we need to do software interrupts */
1:
	mov.l	@SYMBOL_NAME(intr_count),er1
	dec.l	#1,er1
	bne	4f
2:
	mov.l	@SYMBOL_NAME(bh_active),er0
	mov.l	@SYMBOL_NAME(bh_mask),er2
	and.l	er2,er0
	beq	3f
	jsr	@SYMBOL_NAME(do_bottom_half)
	bra	2b
3:	
	sub.l	er0,er0
	mov.l	er0,@SYMBOL_NAME(intr_count)	/* deliver signals, reschedule etc.. */
	jmp	@SYMBOL_NAME(ret_from_exception)
4:
	mov.l	er1,@SYMBOL_NAME(intr_count)
	RESTORE_ALL


/* Handler for uninitialized and spurious interrupts */

SYMBOL_NAME_LABEL(bad_interrupt)
	mov.l	@SYMBOL_NAME(num_spurious),er0
	inc.l	#1,er0
	mov.l	er0,@SYMBOL_NAME(num_spurious)
	rts

SYMBOL_NAME_LABEL(trace_break)
	SAVE_ALL
	sub.l	er1,er1
	dec.l	#1,er1
	mov.l	er1,@(LORIG_ER0)	
	mov.l	sp,er0
	jsr	@SYMBOL_NAME(set_esp0)
/*
	subs	#4,sp
	SAVE_SWITCH_STACK
*/
	mov.l	@enter_sp,er0
	mov.l	@er0,er1
	subs	#2,er1
	mov.l	er1,@er0	
	and.w	#0xff,e1
	mov.l	er1,er0
	jsr	@SYMBOL_NAME(trace_trap)
/*
	RESTORE_SWITCH_STACK
	adds	#4,sp
*/
	jmp	@SYMBOL_NAME(ret_from_exception)	

SYMBOL_NAME_LABEL(sys_fork)
	SAVE_SWITCH_STACK
	mov.l	@enter_sp,er6
	mov.l	@er6,er6
	mov.l	sp,er0
	add.l	#SWITCH_STACK_SIZE+8,er0
	jsr	@SYMBOL_NAME(h8300_fork)
	mov.l	er6,@fork_ret_vec
	RESTORE_SWITCH_STACK
	rts

SYMBOL_NAME_LABEL(sys_clone)
	SAVE_SWITCH_STACK
	mov.l	sp,er0
	add.l	#SWITCH_STACK_SIZE+8,er0
	jsr	@SYMBOL_NAME(h8300_clone)
	RESTORE_SWITCH_STACK
	rts

SYMBOL_NAME_LABEL(sys_sigsuspend)
	SAVE_SWITCH_STACK
	mov.l	sp,er0
	add.l	#SWITCH_STACK_SIZE+8,er0
	jsr	@SYMBOL_NAME(do_sigsuspend)
	RESTORE_SWITCH_STACK
	rts

SYMBOL_NAME_LABEL(sys_sigreturn)
	SAVE_SWITCH_STACK
	mov.l	sp,er0
	add.l	#SWITCH_STACK_SIZE+8,er0
	jsr	@SYMBOL_NAME(do_sigreturn)
	RESTORE_SWITCH_STACK
	rts

LFLUSH_I_AND_D = 0x00000808
LTSS_KSP	= 0
LTSS_USP	= 4
LTSS_SR		= 8
LTSS_FS		= 10
LTSS_CRP	= 12
LTSS_FPCTXT	= 24

SYMBOL_NAME_LABEL(resume)
	/*
	 * Beware - when entering resume, offset of tss is in d1,
	 * prev (the current task) is in a0, next (the new task)
	 * is in a1 and d2.b is non-zero if the mm structure is
	 * shared between the tasks, so don't change these
	 * registers until their contents are no longer needed.
	 */
	/* offset of tss struct (processor state) from beginning
	   of task struct */
	add.l	er1,er0

	/* save sr */
	sub.w	r3,r3
	stc	ccr,r3l
	mov.w	r3,@(LTSS_SR,er0)
	mov.l	er6,@-sp

	/* disable interrupts */
	orc	#0x80,ccr
	mov.l	@enter_sp,er3
	mov.l	er3,@(LTSS_USP,er0)
	mov.l	sp,@(LTSS_KSP,er0)
	
	/* get pointer to tss struct (a1 contains new task) */
	mov.l	er2,@SYMBOL_NAME(current_set)
	add.l	er1,er2

	/* Skip address space switching if they are the same. */
	/* FIXME: what did we hack out of here, this does nothing! */

	mov.l	@(LTSS_USP,er2),er0
	mov.l	er0,@enter_sp
	mov.l	@(LTSS_KSP,er2),sp
	mov.l	@sp+,er6
		
	/* restore status register */
	mov.w	@(LTSS_SR,er2),r3

	ldc	r3l,ccr

	rts

SYMBOL_NAME_LABEL(rdusp)
	mov.l	@enter_sp,er0
	rts

SYMBOL_NAME_LABEL(wrusp)
	mov.l	er0,@enter_sp
	rts

.text
SYMBOL_NAME_LABEL(sys_call_table)
	.long SYMBOL_NAME(sys_setup)		/* 0 */
	.long SYMBOL_NAME(sys_exit)
	.long SYMBOL_NAME(sys_fork)
	.long SYMBOL_NAME(sys_read)
	.long SYMBOL_NAME(sys_write)
	.long SYMBOL_NAME(sys_open)		/* 5 */
	.long SYMBOL_NAME(sys_close)
	.long SYMBOL_NAME(sys_waitpid)
	.long SYMBOL_NAME(sys_creat)
	.long SYMBOL_NAME(sys_link)
	.long SYMBOL_NAME(sys_unlink)		/* 10 */
	.long SYMBOL_NAME(sys_execve)
	.long SYMBOL_NAME(sys_chdir)
	.long SYMBOL_NAME(sys_time)
	.long SYMBOL_NAME(sys_mknod)
	.long SYMBOL_NAME(sys_chmod)		/* 15 */
	.long SYMBOL_NAME(sys_chown)
	.long SYMBOL_NAME(sys_break)
	.long SYMBOL_NAME(sys_stat)
	.long SYMBOL_NAME(sys_lseek)
	.long SYMBOL_NAME(sys_getpid)		/* 20 */
	.long SYMBOL_NAME(sys_mount)
	.long SYMBOL_NAME(sys_umount)
	.long SYMBOL_NAME(sys_setuid)
	.long SYMBOL_NAME(sys_getuid)
	.long SYMBOL_NAME(sys_stime)		/* 25 */
	.long SYMBOL_NAME(sys_ptrace)
	.long SYMBOL_NAME(sys_alarm)
	.long SYMBOL_NAME(sys_fstat)
	.long SYMBOL_NAME(sys_pause)
	.long SYMBOL_NAME(sys_utime)		/* 30 */
	.long SYMBOL_NAME(sys_stty)
	.long SYMBOL_NAME(sys_gtty)
	.long SYMBOL_NAME(sys_access)
	.long SYMBOL_NAME(sys_nice)
	.long SYMBOL_NAME(sys_ftime)		/* 35 */
	.long SYMBOL_NAME(sys_sync)
	.long SYMBOL_NAME(sys_kill)
	.long SYMBOL_NAME(sys_rename)
	.long SYMBOL_NAME(sys_mkdir)
	.long SYMBOL_NAME(sys_rmdir)		/* 40 */
	.long SYMBOL_NAME(sys_dup)
	.long SYMBOL_NAME(sys_pipe)
	.long SYMBOL_NAME(sys_times)
	.long SYMBOL_NAME(sys_prof)
	.long SYMBOL_NAME(sys_brk)		/* 45 */
	.long SYMBOL_NAME(sys_setgid)
	.long SYMBOL_NAME(sys_getgid)
	.long SYMBOL_NAME(sys_signal)
	.long SYMBOL_NAME(sys_geteuid)
	.long SYMBOL_NAME(sys_getegid)		/* 50 */
	.long SYMBOL_NAME(sys_acct)
	.long SYMBOL_NAME(sys_phys)
	.long SYMBOL_NAME(sys_lock)
	.long SYMBOL_NAME(sys_ioctl)
	.long SYMBOL_NAME(sys_fcntl)		/* 55 */
	.long SYMBOL_NAME(sys_mpx)
	.long SYMBOL_NAME(sys_setpgid)
	.long SYMBOL_NAME(sys_ulimit)
	.long SYMBOL_NAME(sys_olduname)
	.long SYMBOL_NAME(sys_umask)		/* 60 */
	.long SYMBOL_NAME(sys_chroot)
	.long SYMBOL_NAME(sys_ustat)
	.long SYMBOL_NAME(sys_dup2)
	.long SYMBOL_NAME(sys_getppid)
	.long SYMBOL_NAME(sys_getpgrp)		/* 65 */
	.long SYMBOL_NAME(sys_setsid)
	.long SYMBOL_NAME(sys_sigaction)
	.long SYMBOL_NAME(sys_sgetmask)
	.long SYMBOL_NAME(sys_ssetmask)
	.long SYMBOL_NAME(sys_setreuid)		/* 70 */
	.long SYMBOL_NAME(sys_setregid)
	.long SYMBOL_NAME(sys_sigsuspend)
	.long SYMBOL_NAME(sys_sigpending)
	.long SYMBOL_NAME(sys_sethostname)
	.long SYMBOL_NAME(sys_setrlimit)	/* 75 */
	.long SYMBOL_NAME(sys_getrlimit)
	.long SYMBOL_NAME(sys_getrusage)
	.long SYMBOL_NAME(sys_gettimeofday)
	.long SYMBOL_NAME(sys_settimeofday)
	.long SYMBOL_NAME(sys_getgroups)	/* 80 */
	.long SYMBOL_NAME(sys_setgroups)
	.long SYMBOL_NAME(old_select)
	.long SYMBOL_NAME(sys_symlink)
	.long SYMBOL_NAME(sys_lstat)
	.long SYMBOL_NAME(sys_readlink)		/* 85 */
	.long SYMBOL_NAME(sys_uselib)
	.long SYMBOL_NAME(sys_swapon)
	.long SYMBOL_NAME(sys_reboot)
	.long SYMBOL_NAME(old_readdir)
	.long SYMBOL_NAME(old_mmap)		/* 90 */
	.long SYMBOL_NAME(sys_munmap)
	.long SYMBOL_NAME(sys_truncate)
	.long SYMBOL_NAME(sys_ftruncate)
	.long SYMBOL_NAME(sys_fchmod)
	.long SYMBOL_NAME(sys_fchown)		/* 95 */
	.long SYMBOL_NAME(sys_getpriority)
	.long SYMBOL_NAME(sys_setpriority)
	.long SYMBOL_NAME(sys_profil)
	.long SYMBOL_NAME(sys_statfs)
	.long SYMBOL_NAME(sys_fstatfs)		/* 100 */
	.long SYMBOL_NAME(sys_ioperm)
	.long SYMBOL_NAME(sys_socketcall)
	.long SYMBOL_NAME(sys_syslog)
	.long SYMBOL_NAME(sys_setitimer)
	.long SYMBOL_NAME(sys_getitimer)	/* 105 */
	.long SYMBOL_NAME(sys_newstat)
	.long SYMBOL_NAME(sys_newlstat)
	.long SYMBOL_NAME(sys_newfstat)
	.long SYMBOL_NAME(sys_uname)
	.long SYMBOL_NAME(sys_ni_syscall)	/* iopl for i386 */ /* 110 */
	.long SYMBOL_NAME(sys_vhangup)
	.long SYMBOL_NAME(sys_idle)
	.long SYMBOL_NAME(sys_ni_syscall)	/* vm86 for i386 */
	.long SYMBOL_NAME(sys_wait4)
	.long SYMBOL_NAME(sys_swapoff)		/* 115 */
	.long SYMBOL_NAME(sys_sysinfo)
	.long SYMBOL_NAME(sys_ipc)
	.long SYMBOL_NAME(sys_fsync)
	.long SYMBOL_NAME(sys_sigreturn)
	.long SYMBOL_NAME(sys_clone)		/* 120 */
	.long SYMBOL_NAME(sys_setdomainname)
	.long SYMBOL_NAME(sys_newuname)
	.long SYMBOL_NAME(sys_cacheflush)	/* modify_ldt for i386 */
	.long SYMBOL_NAME(sys_adjtimex)
	.long SYMBOL_NAME(sys_mprotect)		/* 125 */
	.long SYMBOL_NAME(sys_sigprocmask)
	.long SYMBOL_NAME(sys_create_module)
	.long SYMBOL_NAME(sys_init_module)
	.long SYMBOL_NAME(sys_delete_module)
	.long SYMBOL_NAME(sys_get_kernel_syms)	/* 130 */
	.long SYMBOL_NAME(sys_quotactl)
	.long SYMBOL_NAME(sys_getpgid)
	.long SYMBOL_NAME(sys_fchdir)
	.long SYMBOL_NAME(sys_bdflush)
	.long SYMBOL_NAME(sys_sysfs)		/* 135 */
	.long SYMBOL_NAME(sys_personality)
	.long SYMBOL_NAME(sys_ni_syscall)	/* for afs_syscall */
	.long SYMBOL_NAME(sys_setfsuid)
	.long SYMBOL_NAME(sys_setfsgid)
	.long SYMBOL_NAME(sys_llseek)		/* 140 */
	.long SYMBOL_NAME(sys_getdents)
	.long SYMBOL_NAME(sys_select)
	.long SYMBOL_NAME(sys_flock)
	.long SYMBOL_NAME(sys_msync)
	.long SYMBOL_NAME(sys_readv)		/* 145 */
	.long SYMBOL_NAME(sys_writev)
	.long SYMBOL_NAME(sys_getsid)
	.long SYMBOL_NAME(sys_fdatasync)
	.long SYMBOL_NAME(sys_sysctl)
	.long SYMBOL_NAME(sys_mlock)		/* 150 */
	.long SYMBOL_NAME(sys_munlock)
	.long SYMBOL_NAME(sys_mlockall)
	.long SYMBOL_NAME(sys_munlockall)
	.long SYMBOL_NAME(sys_sched_setparam)
	.long SYMBOL_NAME(sys_sched_getparam)   /* 155 */
	.long SYMBOL_NAME(sys_sched_setscheduler)
	.long SYMBOL_NAME(sys_sched_getscheduler)
	.long SYMBOL_NAME(sys_sched_yield)
	.long SYMBOL_NAME(sys_sched_get_priority_max)
	.long SYMBOL_NAME(sys_sched_get_priority_min)  /* 160 */
	.long SYMBOL_NAME(sys_sched_rr_get_interval)
	.long SYMBOL_NAME(sys_nanosleep)
	.long SYMBOL_NAME(sys_mremap)
	.space (NR_syscalls-163)*4


	.section	.bss
enter_sp:	.space	4	
kernel_sp:	.space	4
int_er0:	.space	4
fork_ret_vec:	.space	4
fork_ret:	.space	6

#ifdef CONFIG_ROMKERNEL
#define IVT(adr) .long adr
	;;.macro	IVT,adr
	;;.long	adr
	;; .endm
#define VECTOR_SECTION .vectors
#endif

#ifdef CONFIG_RAMKERNEL
#define IVT(adr) jmp @adr
	;;.macro	IVT,adr
	;;jmp	@ adr
	;; .endm
#define VECTOR_SECTION .ram_vec
#endif

#ifndef VECTOR_SECTION
#error illigal configration
#endif
	
/* Interrupt Vector Table */
	.section	VECTOR_SECTION
	IVT(SYMBOL_NAME(_start))
	.space		6*4
	IVT(SYMBOL_NAME(interrupt7))
	IVT(SYMBOL_NAME(system_call))
	.space		2*4
	IVT(SYMBOL_NAME(trace_break))
	IVT(SYMBOL_NAME(interrupt12))
	IVT(SYMBOL_NAME(interrupt13))
	IVT(SYMBOL_NAME(interrupt14))
	IVT(SYMBOL_NAME(interrupt15))
	IVT(SYMBOL_NAME(interrupt16))
	IVT(SYMBOL_NAME(interrupt17))
	.space		2*4
	IVT(SYMBOL_NAME(interrupt20))
	IVT(SYMBOL_NAME(interrupt21))
	IVT(SYMBOL_NAME(interrupt22))
	IVT(SYMBOL_NAME(interrupt23))
	IVT(SYMBOL_NAME(interrupt24))
	IVT(SYMBOL_NAME(interrupt25))
	IVT(SYMBOL_NAME(interrupt26))
	IVT(SYMBOL_NAME(interrupt27))
	IVT(SYMBOL_NAME(interrupt28))
	IVT(SYMBOL_NAME(interrupt29))
	IVT(SYMBOL_NAME(interrupt30))
	IVT(SYMBOL_NAME(interrupt31))
	IVT(SYMBOL_NAME(interrupt32))
	IVT(SYMBOL_NAME(interrupt33))
	IVT(SYMBOL_NAME(interrupt34))
	IVT(SYMBOL_NAME(interrupt35))
	IVT(SYMBOL_NAME(interrupt36))
	IVT(SYMBOL_NAME(interrupt37))
	IVT(SYMBOL_NAME(interrupt38))
	IVT(SYMBOL_NAME(interrupt39))
	IVT(SYMBOL_NAME(interrupt40))
	IVT(SYMBOL_NAME(interrupt41))
	IVT(SYMBOL_NAME(interrupt42))
	IVT(SYMBOL_NAME(interrupt43))
	IVT(SYMBOL_NAME(interrupt44))
	IVT(SYMBOL_NAME(interrupt45))
	IVT(SYMBOL_NAME(interrupt46))
	IVT(SYMBOL_NAME(interrupt47))
	IVT(SYMBOL_NAME(interrupt48))
	IVT(SYMBOL_NAME(interrupt49))
	IVT(SYMBOL_NAME(interrupt50))
	IVT(SYMBOL_NAME(interrupt51))
	IVT(SYMBOL_NAME(interrupt52))
	IVT(SYMBOL_NAME(interrupt53))
	IVT(SYMBOL_NAME(interrupt54))
	IVT(SYMBOL_NAME(interrupt55))
	IVT(SYMBOL_NAME(interrupt56))
	IVT(SYMBOL_NAME(interrupt57))
	IVT(SYMBOL_NAME(interrupt58))
	IVT(SYMBOL_NAME(interrupt59))
	IVT(SYMBOL_NAME(interrupt60))
	IVT(SYMBOL_NAME(interrupt61))
	IVT(SYMBOL_NAME(interrupt62))
	IVT(SYMBOL_NAME(interrupt63))
