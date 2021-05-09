/*
 *
 *  linux/arch/m68knommu/kernel/entry.S
 *
 *  Copyright (C) 1999  Keith Adams <kma@cse.ogi.edu>,
 *			Oregon Graduate Institute
 *
 */

/*
 * syscall, interrupt, and fault entry points
 */

#include <asm/i960.h>
#include <asm/unistd.h>
#define ALL_ONES 0xffffffff

#include <linux/sys.h>
#include <linux/config.h>
#include <linux/linkage.h>
#include <asm/setup.h>
#include <asm/segment.h>
#ifdef CONFIG_I960VH
#include <asm/i960jx.h>
#endif


	.globl	SYMBOL_NAME(atmod)
SYMBOL_NAME_LABEL(atmod)
	atmod	g0, g1, g2
	mov	g2, g0
	ret

/*
 * The birthplace of kernel threads. Once here, we find the function to call
 * in register r3, and its argument in r4. See kernel_thread in asm/unistd.h
 * for why this exists.
 */
	.globl	SYMBOL_NAME(kthread_trampoline)
SYMBOL_NAME_LABEL(kthread_trampoline)
	mov	r3, g0
	mov	r4, g1
	call	SYMBOL_NAME(kthread_start)
	ldconst	0, g0
	b	SYMBOL_NAME(do_exit)
/* no ret */

/*
 * signal handlers start here in signal_head.
 */
	.globl	SYMBOL_NAME(signal_head)
SYMBOL_NAME_LABEL(signal_head)
	mov	g0, r5
	mov	r4, g0
	callx	(r3)
	mov	r5, g0
	ret

#define	USER_AC	0x3b001000
#define USER_PC	0x00002000	/* bit 13 set: interrupted mode */
#define KSP_OFFSET	0x58

#if 0
#define CKPT(x,y)	\
	ldconst	x, g0;	\
	mov	y, g1;	\
	call	SYMBOL_NAME(ckpt);
#endif

	/* XXX: todo: emulate an interrupt: put PC and AC on stack */
/*
 * This is branched to by intr and syscall; it switches to the kernel stack,
 * and returns to the ip in r6. The choice of registers lets us economize
 * on store operations.
 *
 * Be sure to branch here, rather than call; we can't use a single frame of
 * the shared intr/syscall stacks.
 *
 * Observe weird register conventions (because we can't alter g* regs):
 *	ip after switch:		r6
 * 	r3 in new stack:		r7 (for intr/fault)
 * 	PC to be seen in new stack:	r8
 * 	AC to be seen in new stack:	r9
 */
	.align	4
switch_to_kstack:
	ld	SYMBOL_NAME(current_set), r3
	ld	KSP_OFFSET(r3), r3
	ldconst	64, r5
	addo	r3, r5, r3		# get some space on stack (for PC/AC)
/*
 * r3 now points to the address of the new stack frame. We build an image of
 * the new stack frame in regs r4-r7. The intr needs to
 * remember its intr vector, so we also put r7 on the stack, in the r3
 * position.
 */

	mov	pfp, r4			# newframe->pfp = current pfp
	addo	r5, r3, r5		# newframe->sp = newframe + 64
					# newframe->rip = rip (passed in r6)
					# newframe->r3 = r7
	stq	r4, (r3)

/* Store AC, PC as if in interrupt context */
	stl	r8, -16(r3)		# pc passed in r8, ac passed in r9.
	flushreg
	mov	r3, pfp
	flushreg
	ret				# returns to ip from r6

/*
 * System calls all end up here.
 */
	.globl	SYMBOL_NAME(syscall)
	.align	4
SYMBOL_NAME_LABEL(syscall)
	/*
	 * first, examine pfp; if its 1st and 2nd bits are 1 and 0 respectively,
	 * we're coming from user mode, so we need to switch stacks.
	 */

    modpc 0, 0, r9
	bbs	2, pfp, 1f	# if bit 2 is set, we're cool
	bbc	1, pfp, 1f	# same if bit 1 is clear
	/* uh-oh, we're coming from user mode: switch stacks */
	lda	1f, r6
	ldconst	USER_PC, r8
	b	switch_to_kstack

1:
	SAVE_ALL(r3)
	subo	16, fp, g0
	call	SYMBOL_NAME(csyscall)
	RESTORE_ALL(r3)
	ret

/*
 * This is the main interrupt handler; it is also the entry point for 
 * system calls.
 */
	.globl	SYMBOL_NAME(intr)
	.align	4
SYMBOL_NAME_LABEL(intr)
	ld	-16(fp), r3
   	ld  -12(fp), r9     # Couldn't these two directly be loaded together in r8 and r9?
	ldob	-8(fp), r7
	bbs	1, r3, 1f		# if we're supervisor, forget it
	/* uh-oh, switch stacks */
	lda	2f, r6
	mov	r3, r8
	b	switch_to_kstack

1:
	mov	r7, r3
2:
/*
 * by now we're on a valid stack, and the interrupt vector number is in
 * r3.
 */
	SAVE_ALL(r4)
	mov	r3, g0		# 1st arg is intr vector
	flushreg		# get regs on stack
	subo	16, fp, g1	# 2nd arg is pointer to pt_regs
	call	SYMBOL_NAME(cintr)
	RESTORE_ALL(r3)
	ret			# back to user-level

/*
 * Again, if we're coming from user mode, switch to kstack. We get this info
 * from the fault record, 16 bytes below the current fp. We play games similar
 * to intr to hold onto the fault record...
 */
	.globl	SYMBOL_NAME(fault)
SYMBOL_NAME_LABEL(fault)
    modac 0, 0, r9        
   	subo	16, fp, r7
	ld	(r7), r3	# r3 gets pc of faulting instr
	bbs	1, r3, 1f	# if it was a supervisor fault, don't switch

	lda	2f, r6
	ldconst	USER_PC, r8
	b	switch_to_kstack
1:
	mov	r7, r3
2:
	SAVE_ALL(r4)
	mov	r3, g0
	subo	16, fp, g1
	call	SYMBOL_NAME(cfault)
	RESTORE_ALL(r4)
	ret

/*
 * The table of system calls.
 */
	.globl	SYMBOL_NAME(syscall_tab)
SYMBOL_NAME_LABEL(syscall_tab)
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
	.long SYMBOL_NAME(do_pipe)
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
	.long SYMBOL_NAME(do_sigsuspend)
	.long SYMBOL_NAME(sys_sigpending)
	.long SYMBOL_NAME(sys_sethostname)
	.long SYMBOL_NAME(sys_setrlimit)	/* 75 */
	.long SYMBOL_NAME(sys_getrlimit)
	.long SYMBOL_NAME(sys_getrusage)
	.long SYMBOL_NAME(sys_gettimeofday)
	.long SYMBOL_NAME(sys_settimeofday)
	.long SYMBOL_NAME(sys_getgroups)	/* 80 */
	.long SYMBOL_NAME(sys_setgroups)
	.long SYMBOL_NAME(sys_select)
	.long SYMBOL_NAME(sys_symlink)
	.long SYMBOL_NAME(sys_lstat)
	.long SYMBOL_NAME(sys_readlink)		/* 85 */
	.long SYMBOL_NAME(sys_uselib)
	.long SYMBOL_NAME(sys_swapon)
	.long SYMBOL_NAME(sys_reboot)
	.long SYMBOL_NAME(sys_ni_syscall)
	.long SYMBOL_NAME(sys_ni_syscall)		/* 90 */
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
	.long SYMBOL_NAME(do_sigreturn)
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
