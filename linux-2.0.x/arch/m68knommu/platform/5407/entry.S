/* -*- mode: asm -*-
 *
 *  linux/arch/m68knommu/platform/5407/entry.S
 *
 *  Copyright (C) 2000  Lineo (www.lineo.com)
 *  Copyright (C) 1999  Greg Ungerer (gerg@snapgear.com)
 *  Copyright (C) 1998  D. Jeff Dionne <jeff@lineo.ca>,
 *                      Kenneth Albanowski <kjahds@kjahds.com>,
 *
 * Based on:
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
 * ColdFire support by Greg Ungerer (gerg@snapgear.com)
 * 5307 fixes by David W. Miller
 */

/*
 * entry.S  contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call.
 *
 * Stack layout in 'ret_from_exception':
 *
 *	This allows access to the syscall arguments in registers d1-d5
 *
 *	 0(sp) - d1
 *	 4(sp) - d2
 *	 8(sp) - d3
 *	 C(sp) - d4
 *	10(sp) - d5
 *	14(sp) - a0
 *	18(sp) - a1
 *	1C(sp) - d0
 *	20(sp) - orig_d0
 *	24(sp) - stack adjustment
 *	28(sp) - format & vector      }
 *	2A(sp) - sr                   } different to m68k
 *	2C(sp) - pc                   }
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
#define ALLOWINT 0xf8ff
#define	MAX_NOINT_IPL	0


LD0		= 0x1C
LORIG_D0	= 0x20
LSR		= 0x2a
LFORMATVEC	= 0x28
LPC		= 0x2c

/*
 * This defines the normal kernel pt-regs layout.
 *
 * regs are a2-a6 and d6-d7 preserved by C code
 * the kernel doesn't mess with usp unless it needs to
 *
 *	This is made a little more tricky on the ColdFire. There is no
 *	separate kernel and user stack pointers. Need to artificially
 *	construct a usp in software... When doing this we need to disable
 *	interrupts, otherwise bad things could happen.
 */
#define SAVE_ALL						\
	move	#0x2700,%sr;		/* disable intrs */	\
	btst	#5,%sp@(2);		/* from user? */	\
	bnes	1f;			/* no, skip */		\
	movel	%sp,sw_usp;		/* save user sp */	\
	addql	#8,sw_usp;		/* remove exception */	\
	movel	sw_ksp,%sp;		/* kernel sp */		\
	subql	#8,%sp;			/* room for exception */\
	clrl	%sp@-;			/* stk_adj */		\
	movel	%d0,%sp@-;		/* orig d0 */		\
	movel	%d0,%sp@-;		/* d0 */		\
	subl	#28,%sp;		/* space for 7 regs */	\
	moveml	%d1-%d5/%a0-%a1,%sp@;				\
	movel	sw_usp,%a0;		/* get usp */		\
	moveml	%a0@(-8),%d1-%d2;	/* get exception */	\
	moveml	%d1-%d2,%sp@(LFORMATVEC); /* copy exception */	\
	bra	2f;						\
	1:							\
	clrl	%sp@-;			/* stk_adj */		\
	movel	%d0,%sp@-;		/* orig d0 */		\
	movel	%d0,%sp@-;		/* d0 */		\
	subl	#28,%sp;		/* space for 7 regs */	\
	moveml	%d1-%d5/%a0-%a1,%sp@;				\
	2:

#define RESTORE_ALL						\
	btst	#5,%sp@(LSR);		/* going user? */	\
	bnes	1f;			/* no, skip */		\
	move	#0x2700,%sr;		/* disable intrs */	\
	movel	sw_usp,%a0;		/* get usp */		\
	moveml	%sp@(LFORMATVEC),%d1-%d2; /* copy exception */	\
	moveml	%d1-%d2,%a0@(-8);				\
	moveml	%sp@,%d1-%d5/%a0-%a1;				\
	addl	#28,%sp;		/* space for 7 regs */	\
	movel	%sp@+,%d0;					\
	addql	#4,%sp;			/* orig d0 */		\
	addl	%sp@+,%sp;		/* stk adj */		\
	addql	#8,%sp;			/* remove exception */	\
	movel	%sp,sw_ksp;		/* save ksp */		\
	subql	#8,sw_usp;		/* set exception */	\
	movel	sw_usp,%sp;		/* restore usp */	\
	rte;							\
	1:							\
	moveml	%sp@,%d1-%d5/%a0-%a1;				\
	addl	#28,%sp;		/* space for 7 regs */	\
	movel	%sp@+,%d0;					\
	addql	#4,%sp;			/* orig d0 */		\
	addl	%sp@+,%sp;		/* stk adj */		\
	rte

/*
 *	Quick exception save, use current stack only.
 */
#define SAVE_LOCAL						\
	move	#0x2700,%sr;		/* disable intrs */	\
	clrl	%sp@-;			/* stk_adj */		\
	movel	%d0,%sp@-;		/* orig d0 */		\
	movel	%d0,%sp@-;		/* d0 */		\
	subl	#28,%sp;		/* space for 7 regs */	\
	moveml	%d1-%d5/%a0-%a1,%sp@;

#define RESTORE_LOCAL						\
	moveml	%sp@,%d1-%d5/%a0-%a1;				\
	addl	#28,%sp;		/* space for 7 regs */	\
	movel	%sp@+,%d0;					\
	addql	#4,%sp;			/* orig d0 */		\
	addl	%sp@+,%sp;		/* stk adj */		\
	rte


#define SWITCH_STACK_SIZE (7*4+4)	/* includes return address */

#define SAVE_SWITCH_STACK		\
	subl    #28,%sp; /* 7 regs */	\
	moveml	%a2-%a6/%d6-%d7,%sp@

#define RESTORE_SWITCH_STACK		\
	moveml	%sp@,%a2-%a6/%d6-%d7;	\
	addl	#28,%sp /* 7 regs */

/*
 *	Software copy of the user and kernel stack pointers... Ugh...
 *	Need these to get around ColdFire not having separate kernel
 *	and user stack pointers.
 */
.globl SYMBOL_NAME(sw_usp)
.globl SYMBOL_NAME(sw_ksp)

.data

sw_ksp:
.long	0

sw_usp:
.long	0

.text


.globl SYMBOL_NAME(buserr)
.globl SYMBOL_NAME(trap)
.globl SYMBOL_NAME(system_call)
.globl SYMBOL_NAME(resume), SYMBOL_NAME(ret_from_exception)
.globl SYMBOL_NAME(ret_from_signal)
.globl SYMBOL_NAME(sys_call_table)
.globl SYMBOL_NAME(sys_fork), SYMBOL_NAME(sys_clone)
.globl SYMBOL_NAME(ret_from_interrupt)
.globl SYMBOL_NAME(intrhandler)
.globl SYMBOL_NAME(fasthandler)

.text

ENTRY(buserr)
	SAVE_ALL
	moveq	#-1,%d0
	movel	%d0,%sp@(LORIG_D0)	| a -1 in the ORIG_D0 field
					| signifies that the stack frame
					| is NOT for syscall

	movel	%sp,%sp@- 		| stack frame pointer argument
	jsr	SYMBOL_NAME(buserr_c)
	addql	#4,%sp
	jra	SYMBOL_NAME(ret_from_exception)


ENTRY(reschedule)
	| save top of frame
	pea	%sp@
	jbsr	SYMBOL_NAME(set_esp0)
	addql	#4,%sp

	pea	SYMBOL_NAME(ret_from_exception)
	jmp	SYMBOL_NAME(schedule)

ENTRY(system_call)
	SAVE_ALL
	move	#0x2000,%sr;		| enable intrs again

	movel	#-LENOSYS,%d2
	movel	%d2,LD0(%sp)		| default return value in d0
					| original D0 is in orig_d0
	movel	%d0,%d2

	| save top of frame
	pea	%sp@
	jbsr	SYMBOL_NAME(set_esp0)
	addql	#4,%sp

	cmpl	#NR_syscalls,%d2
	jcc	SYMBOL_NAME(ret_from_exception)
	lea	SYMBOL_NAME(sys_call_table),%a0
	lsll	#2,%d2		| movel	%a0@(%d2:l:4),%d3
	movel	%a0@(%d2),%d3
	jeq	SYMBOL_NAME(ret_from_exception)
	lsrl	#2,%d2
	movel	SYMBOL_NAME(current_set),%a0
	btst	#5,%a0@(LTASK_FLAGS+3)	| PF_TRACESYS
	bnes	1f
	movel	%d3,%a0
	jbsr	%a0@
	movel	%d0,%sp@(LD0)		| save the return value
	jra	SYMBOL_NAME(ret_from_exception)
1:
	subql	#4,%sp
	SAVE_SWITCH_STACK
	jbsr	SYMBOL_NAME(syscall_trace)
	RESTORE_SWITCH_STACK
	addql	#4,%sp
	movel	%d3,%a0
	jbsr	%a0@
	movel	%d0,%sp@(LD0)		| save the return value
	subql	#4,%sp			| dummy return address
	SAVE_SWITCH_STACK
	jbsr	SYMBOL_NAME(syscall_trace)

SYMBOL_NAME_LABEL(ret_from_signal)
	RESTORE_SWITCH_STACK
	addql	#4,%sp

SYMBOL_NAME_LABEL(ret_from_exception)
	btst	#5,%sp@(LSR)		| check if returning to kernel
	bnes	2f			| if so, skip resched, signals
	tstl	SYMBOL_NAME(need_resched)
	jne	SYMBOL_NAME(reschedule)
	movel	SYMBOL_NAME(current_set),%a0
	cmpl	#SYMBOL_NAME(task),%a0	| task[0] cannot have signals
	jeq	2f
	bclr	#5,%a0@(LTASK_FLAGS+1)	| check for delayed trace
	jeq	1f
	bclr	#7,%sp@(LSR)		| clear trace bit in SR
	pea	1			| send SIGTRAP
	movel	%a0,%sp@-
	pea	5
	jbsr	SYMBOL_NAME(send_sig)
	addql	#8,%sp
	addql	#4,%sp
	movel	SYMBOL_NAME(current_set),%a0

1:
	tstl	%a0@(LTASK_STATE)	| state
	jne	SYMBOL_NAME(reschedule)
	tstl	%a0@(LTASK_COUNTER)	| counter
	jeq	SYMBOL_NAME(reschedule)

	movel	%a0@(LTASK_BLOCKED),%d0
	movel	%d0,%d1			| save blocked in d1 for sig handling
	notl	%d0
	btst	#4,%a0@(LTASK_FLAGS+3)	| PF_PTRACED
	jeq	1f
	moveq	#-1,%d0			| let the debugger see all signals
1:	andl	%a0@(LTASK_SIGNAL),%d0
	jne	Lsignal_return
2:	RESTORE_ALL			| Does RTE

Lsignal_return:
	subql	#4,%sp			| dummy return address
	SAVE_SWITCH_STACK
	pea	%sp@(SWITCH_STACK_SIZE)
	movel	%d1,%sp@-
	jsr	SYMBOL_NAME(do_signal)
	addql	#8,%sp
	RESTORE_SWITCH_STACK
	addql	#4,%sp

	btst    #5,%sp@(LSR);	/* going user? */   
	bnes    not_user;	/* no, skip */      
	move    #0x2700,%sr;	/* disable intrs */ 
	movel   sw_usp,%a0;	/* get usp */       
	moveml  %sp@(LFORMATVEC),%d1-%d2; /* copy exception */  
	moveml  %d1-%d2,%a0@(-8);               
	bclr    #5,%a0@(-8);  /* clear format byte, bit 5 to make stack appear modulo 4 which it WILL be when we do the rte because it was generated in setup_frame */
	bclr    #4,%a0@(-8);  /* clear format byte, bit 5 to make stack appear modulo 4 which it WILL be when we do the rte because it was generated in setup_frame */
	moveml  %sp@,%d1-%d5/%a0-%a1;               
	addl    #28,%sp;        /* space for 7 regs */  
	movel   %sp@+,%d0;                  
	addql   #4,%sp;         /* orig d0 */       
	addl    %sp@+,%sp;      /* stk adj */       
	addql   #8,%sp;         /* remove exception */  
	movel   %sp,sw_ksp;     /* save ksp */      
	movel   sw_usp,%sp;     /* restore usp */   
	subql   #8,%sp;         /* set exception */ 
	rte;                            
not_user:                          
	moveml  %sp@,%d1-%d5/%a0-%a1;               
	addl    #28,%sp;        /* space for 7 regs */  
	movel   %sp@+,%d0;                  
	addql   #4,%sp;         /* orig d0 */       
	addl    %sp@+,%sp;      /* stk adj */       
	rte

/*--------------------------------------------------------------------------*/

/*
 *	Common ColdFire trap handler. Most traps come through here first.
 */
ENTRY(trap)
	SAVE_ALL
	moveq	#-1,%d0
	movel	%d0,%sp@(LORIG_D0)	| a -1 in the ORIG_D0 field
					| signifies that the stack frame
					| is NOT for syscall
	movew   %sp@(LFORMATVEC),%d0	| get exception vector
	andl	#0x03fc,%d0		| mask out vector only
	lsrl	#2,%d0			| calculate real vector #
	movel	%sp,%sp@- 		| stack frame pointer argument
	movel	%d0,%sp@- 		| push vector # on stack

	jsr	SYMBOL_NAME(trap_c)
	addql	#8,%sp
	jra	SYMBOL_NAME(ret_from_exception)


/*
 *	This is the generic interrupt handler (for all hardware interrupt
 *	sources). It figures out the vector number and calls the appropriate
 *	interrupt service routine directly.
 */
SYMBOL_NAME_LABEL(intrhandler)
	SAVE_ALL

	moveq	#-1,%d0
	movel	%d0,%sp@(LORIG_D0)	| a -1 in the ORIG_D0 field
					| signifies that the stack frame
					| is NOT for syscall
	addql	#1,SYMBOL_NAME(intr_count)
					| put exception # in d0
	movew   %sp@(LFORMATVEC),%d0
	andl	#0x03fc,%d0		| mask out vector only

	movel	SYMBOL_NAME(irq_kstat_interrupt),%a0
					| get addr of kstat struct
	addql	#1,%a0@(%d0)		| incr irq intr count

	lsrl	#2,%d0			| calculate real vector #
	movel	%d0,%d1			| calculate array offset
	lsll	#4,%d1
	lea	SYMBOL_NAME(vec_list),%a0
	addl	%d1,%a0			| pointer to array struct

	movel	%sp,%sp@-		| push regs arg onto stack
	movel	%a0@(8),%sp@-		| push devid arg
	movel	%d0,%sp@- 		| push vector # on stack

	movel	%a0@,%a0		| get function to call
	jbsr	%a0@			| call vector handler
	addl	#12,%sp			| pop parameters off stack

	bra	ret_from_interrupt	| this was fallthrough


/*
 *	This is the fast interrupt handler (for certain hardware interrupt
 *	sources). Unlike the normal interrupt handler it just uses the
 *	current stack (doesn't care if it is user or kernel). It also
 *	doesn't bother doing the bottom half handlers.
 */
SYMBOL_NAME_LABEL(fasthandler)
	SAVE_LOCAL

	movew   %sp@(LFORMATVEC),%d0
	andl	#0x03fc,%d0		| mask out vector only

	movel	SYMBOL_NAME(irq_kstat_interrupt),%a0
					| get addr of kstat struct
	addql	#1,%a0@(%d0)		| incr irq intr count

	movel	%sp,%sp@-		| push regs arg onto stack
	clrl	%d1
	movel	%d1,%sp@- 		| push devid arg
	lsrl	#2,%d0			| calculate real vector #
	movel	%d0,%sp@- 		| push vector # on stack

	lsll	#4,%d0			| adjust for array offset
	lea	SYMBOL_NAME(vec_list),%a0
	movel	%a0@(%d0),%a0		| get function to call
	jbsr	%a0@			| call vector handler
	addl	#12,%sp			| pop parameters off stack

	RESTORE_LOCAL

/*--------------------------------------------------------------------------*/

SYMBOL_NAME_LABEL(ret_from_interrupt)
	/* check if we need to do software interrupts */
1:
	movel	SYMBOL_NAME(intr_count),%d1
	subql	#1,%d1
	jne	4f

	|movel	%sp@(LSR),%d0
	|lsr	#5,%d0
	moveb	%sp@(LSR),%d0
	andl	#0x7,%d0
	|bfextu  %sp@(LSR){#5,#3},%d0    | Check for nested interrupt.
#if MAX_NOINT_IPL > 0
	cmpiw	#MAX_NOINT_IPL,%d0
#endif
	jhi	4f

	move	#0x2000,%sr;		| enable intrs again

2:
	movel	SYMBOL_NAME(bh_active),%d0
	andl	SYMBOL_NAME(bh_mask),%d0
	jne	3f

	clrl	SYMBOL_NAME(intr_count)	| deliver signals, reschedule etc..
	jra	SYMBOL_NAME(ret_from_exception)
3:	
	jbsr	SYMBOL_NAME(do_bottom_half)
	jbra	2b
4:
	movel	%d1,SYMBOL_NAME(intr_count)
	RESTORE_ALL


/* Handler for uninitialized and spurious interrupts */

SYMBOL_NAME_LABEL(bad_interrupt)
	addql	#1,SYMBOL_NAME(num_spurious)
	rte

ENTRY(sys_fork)
	SAVE_SWITCH_STACK
	pea	%sp@(SWITCH_STACK_SIZE)
	jbsr	SYMBOL_NAME(m68k_fork)
	addql	#4,%sp
	RESTORE_SWITCH_STACK
	rts

ENTRY(sys_clone)
	SAVE_SWITCH_STACK
	pea	%sp@(SWITCH_STACK_SIZE)
	jbsr	SYMBOL_NAME(m68k_clone)
	addql	#4,%sp
	RESTORE_SWITCH_STACK
	rts

ENTRY(sys_sigsuspend)
	SAVE_SWITCH_STACK
	pea	%sp@(SWITCH_STACK_SIZE)
	jbsr	SYMBOL_NAME(do_sigsuspend)
	addql	#4,%sp
	RESTORE_SWITCH_STACK
	rts

ENTRY(sys_sigreturn)
	SAVE_SWITCH_STACK
	jbsr	SYMBOL_NAME(do_sigreturn)
	RESTORE_SWITCH_STACK
	rts

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
	addl	%d1,%a0

	/* save sr */
	move	%sr,%d0
	movew	%d0,%a0@(LTSS_SR)

	/* disable interrupts */
	oril	#0x700,%d0
	move	%d0,%sr

#ifndef NO_MM
	/* save fs (sfc,%dfc) (may be pointing to kernel memory) */
	movec	%sfc,%d0
	movew	%d0,%a0@(LTSS_FS)
#endif

	/* save usp */
	/* it is better to use a movel here instead of a movew 8*) */
	

	/* save non-scratch registers on stack */
	SAVE_SWITCH_STACK

	/* Use the sw_usp, no real %usp */
	movel	sw_usp,%a2 /* usp */
	movel	%a2,%a0@(LTSS_USP)
#if 0
	/* busted */
	movel	%usp,%d0 /* usp */
	movel	%d0,%a0@(LTSS_USP)
#endif

	/* save current kernel stack pointer */
	movel	%sp,%a0@(LTSS_KSP)

	/* get pointer to tss struct (a1 contains new task) */
	movel	%a1,SYMBOL_NAME(current_set)
	addl	%d1,%a1

	/* Skip address space switching if they are the same. */
	/* FIXME: what did we hack out of here, this does nothing! */
#if 0
	tstb	%d2
	jne	4f
#endif
2:
4:
	/* restore floating point context */

	/* restore the kernel stack pointer */
	movel	%a1@(LTSS_KSP),%sp

	/* restore non-scratch registers */
	RESTORE_SWITCH_STACK

	/* restore user stack pointer */
	movel	%a1@(LTSS_USP),%a0
	/* No %usp, use a software copy */
	movel	%a0,sw_usp

#ifndef NO_MM
	/* restore fs (sfc,%dfc) */
	movew	%a1@(LTSS_FS),%a0
	movec	%a0,%sfc
	movec	%a0,%dfc
#endif

	/* restore status register */
	movel   %d0,%a0
	move	%a1@(LTSS_SR),%d0
	move	%d0,%sr
	movel   %a0,%d0
	rts

.text
ALIGN
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
