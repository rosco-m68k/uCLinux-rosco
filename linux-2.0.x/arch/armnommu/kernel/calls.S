/*
 * linux/arch/arm/lib/calls.h
 *
 * Copyright (C) 1995, 1996 Russell King
 */
#ifndef NR_SYSCALLS
#define NR_syscalls 256
#define NR_SYSCALLS 163
#else

/* 0 */		.long	sys_setup
		.long	sys_exit
		.long	sys_fork
		.long	sys_read
		.long	sys_write
/* 5 */		.long	sys_open
		.long	sys_close
		.long	sys_waitpid
		.long	sys_creat
		.long	sys_link
/* 10 */	.long	sys_unlink
		.long	sys_execve
		.long	sys_chdir
		.long	sys_time
		.long	sys_mknod
/* 15 */	.long	sys_chmod
		.long	sys_chown
		.long	sys_break
		.long	sys_stat
		.long	sys_lseek
/* 20 */	.long	sys_getpid
		.long	sys_old_arm_mount			/* *** 5 *** */
		.long	sys_umount
		.long	sys_setuid
		.long	sys_getuid
/* 25 */	.long	sys_stime
		.long	sys_ptrace
		.long	sys_alarm
		.long	sys_fstat
		.long	sys_pause
/* 30 */	.long	sys_utime
		.long	sys_stty
		.long	sys_gtty
		.long	sys_access
		.long	sys_nice
/* 35 */	.long	sys_ftime
		.long	sys_sync
		.long	sys_kill
		.long	sys_rename
		.long	sys_mkdir
/* 40 */	.long	sys_rmdir
		.long	sys_dup
		.long	sys_pipe
		.long	sys_times
		.long	sys_prof
/* 45 */	.long	sys_brk
		.long	sys_setgid
		.long	sys_getgid
		.long	sys_signal
		.long	sys_geteuid
/* 50 */	.long	sys_getegid
		.long	sys_acct
		.long	sys_phys
		.long	sys_lock
		.long	sys_ioctl
/* 55 */	.long	sys_fcntl
		.long	sys_mpx
		.long	sys_setpgid
		.long	sys_ulimit
		.long	sys_olduname
/* 60 */	.long	sys_umask
		.long	sys_chroot
		.long	sys_ustat
		.long	sys_dup2
		.long	sys_getppid
/* 65 */	.long	sys_getpgrp
		.long	sys_setsid
		.long	sys_sigaction
		.long	sys_sgetmask
		.long	sys_ssetmask
/* 70 */	.long	sys_setreuid
		.long	sys_setregid
		.long	sys_sigsuspend
		.long	sys_sigpending
		.long	sys_sethostname
/* 75 */	.long	sys_setrlimit
		.long	sys_getrlimit
		.long	sys_getrusage
		.long	sys_gettimeofday
		.long	sys_settimeofday
/* 80 */	.long	sys_getgroups
		.long	sys_setgroups
		.long	sys_select
		.long	sys_symlink
		.long	sys_lstat
/* 85 */	.long	sys_readlink
		.long	sys_uselib
		.long	sys_swapon
		.long	sys_reboot
		.long	old_readdir
/* 90 */	.long	old_mmap
		.long	sys_munmap
		.long	sys_truncate
		.long	sys_ftruncate
		.long	sys_fchmod
/* 95 */	.long	sys_fchown
		.long	sys_getpriority
		.long	sys_setpriority
		.long	sys_profil
		.long	sys_statfs
/* 100 */	.long	sys_fstatfs
		.long	sys_ni_syscall			/* .long	sys_ioperm */
		.long	sys_socketcall
		.long	sys_syslog
		.long	sys_setitimer
/* 105 */	.long	sys_getitimer
		.long	sys_newstat
		.long	sys_newlstat
		.long	sys_newfstat
		.long	sys_uname
/* 110 */	.long	sys_iopl
		.long	sys_vhangup
		.long	sys_idle
		.long	sys_syscall			/* call a syscall */
		.long	sys_wait4
/* 115 */	.long	sys_swapoff
		.long	sys_sysinfo
		.long	sys_ipc
		.long	sys_fsync
		.long	sys_sigreturn
		.long	sys_clone
		.long	sys_setdomainname
		.long	sys_newuname
		.long	sys_ni_syscall			/* .long	sys_modify_ldt */
		.long	sys_adjtimex
/* 125 */	.long	sys_mprotect
		.long	sys_sigprocmask
		.long	sys_create_module
		.long	sys_old_arm_init_module	/* *** 5 *** */
		.long	sys_delete_module
/* 130 */	.long	sys_get_kernel_syms
		.long	sys_quotactl
		.long	sys_getpgid
		.long	sys_fchdir
		.long	sys_bdflush
/* 135 */	.long	sys_sysfs
		.long	sys_personality
		.long	sys_ni_syscall			/* .long	sys_afs_syscall */
		.long	sys_setfsuid
		.long	sys_setfsgid
/* 140 */	.long	sys_old_arm_llseek		/* *** 5 *** */
		.long	sys_getdents
		.long	sys_select
		.long	sys_flock
		.long	sys_msync
/* 145 */	.long	sys_readv
		.long	sys_writev
		.long	sys_getsid
		.long	sys_fdatasync
		.long	sys_sysctl
/* 150 */	.long	sys_mlock
		.long	sys_munlock
		.long	sys_mlockall
		.long	sys_munlockall
		.long	sys_sched_setparam
/* 155 */	.long	sys_sched_getparam
		.long	sys_sched_setscheduler
		.long	sys_sched_getscheduler
		.long	sys_sched_yield
		.long	sys_sched_get_priority_max
/* 160 */	.long	sys_sched_get_priority_min
		.long	sys_sched_rr_get_interval
		.long	sys_nanosleep
		.long	sys_mremap
		.space	(NR_syscalls - 163) * 4
#endif
