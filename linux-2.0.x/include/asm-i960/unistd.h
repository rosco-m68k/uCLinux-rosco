#ifndef _ASM_I960_UNISTD_H_
#define _ASM_I960_UNISTD_H_

#ifndef __ASSEMBLY__ 
#ifdef CONFIG_I960VH
#include <asm/i960jx.h>
#endif	/* CONFIG_I960VH */
#endif	/* __ASSEMBLY__ */


/*
 * This file contains the system call numbers.
 */

#define __NR_setup		  0	/* used only by init, to get system going */
#define __NR_exit		  1
#define __NR_fork		  2
#define __NR_read		  3
#define __NR_write		  4
#define __NR_open		  5
#define __NR_close		  6
#define __NR_waitpid		  7
#define __NR_creat		  8
#define __NR_link		  9
#define __NR_unlink		 10
#define __NR_execve		 11
#define __NR_chdir		 12
#define __NR_time		 13
#define __NR_mknod		 14
#define __NR_chmod		 15
#define __NR_chown		 16
#define __NR_break		 17
#define __NR_oldstat		 18
#define __NR_lseek		 19
#define __NR_getpid		 20
#define __NR_mount		 21
#define __NR_umount		 22
#define __NR_setuid		 23
#define __NR_getuid		 24
#define __NR_stime		 25
#define __NR_ptrace		 26
#define __NR_alarm		 27
#define __NR_oldfstat		 28
#define __NR_pause		 29
#define __NR_utime		 30
#define __NR_stty		 31
#define __NR_gtty		 32
#define __NR_access		 33
#define __NR_nice		 34
#define __NR_ftime		 35
#define __NR_sync		 36
#define __NR_kill		 37
#define __NR_rename		 38
#define __NR_mkdir		 39
#define __NR_rmdir		 40
#define __NR_dup		 41
#define __NR_pipe		 42
#define __NR_times		 43
#define __NR_prof		 44
#define __NR_brk		 45
#define __NR_setgid		 46
#define __NR_getgid		 47
#define __NR_signal		 48
#define __NR_geteuid		 49
#define __NR_getegid		 50
#define __NR_acct		 51
#define __NR_phys		 52
#define __NR_lock		 53
#define __NR_ioctl		 54
#define __NR_fcntl		 55
#define __NR_mpx		 56
#define __NR_setpgid		 57
#define __NR_ulimit		 58
#define __NR_oldolduname	 59
#define __NR_umask		 60
#define __NR_chroot		 61
#define __NR_ustat		 62
#define __NR_dup2		 63
#define __NR_getppid		 64
#define __NR_getpgrp		 65
#define __NR_setsid		 66
#define __NR_sigaction		 67
#define __NR_sgetmask		 68
#define __NR_ssetmask		 69
#define __NR_setreuid		 70
#define __NR_setregid		 71
#define __NR_sigsuspend		 72
#define __NR_sigpending		 73
#define __NR_sethostname	 74
#define __NR_setrlimit		 75
#define __NR_getrlimit		 76
#define __NR_getrusage		 77
#define __NR_gettimeofday	 78
#define __NR_settimeofday	 79
#define __NR_getgroups		 80
#define __NR_setgroups		 81
#define __NR_select		 82
#define __NR_symlink		 83
#define __NR_oldlstat		 84
#define __NR_readlink		 85
#define __NR_uselib		 86
#define __NR_swapon		 87
#define __NR_reboot		 88
#define __NR_readdir		 89
#define __NR_mmap		 90
#define __NR_munmap		 91
#define __NR_truncate		 92
#define __NR_ftruncate		 93
#define __NR_fchmod		 94
#define __NR_fchown		 95
#define __NR_getpriority	 96
#define __NR_setpriority	 97
#define __NR_profil		 98
#define __NR_statfs		 99
#define __NR_fstatfs		100
#define __NR_ioperm		101
#define __NR_socketcall		102
#define __NR_syslog		103
#define __NR_setitimer		104
#define __NR_getitimer		105
#define __NR_stat		106
#define __NR_lstat		107
#define __NR_fstat		108
#define __NR_olduname		109
#define __NR_iopl		/* 110 */ not supported
#define __NR_vhangup		111
#define __NR_idle		112
#define __NR_vm86		/* 113 */ not supported
#define __NR_wait4		114
#define __NR_swapoff		115
#define __NR_sysinfo		116
#define __NR_ipc		117
#define __NR_fsync		118
#define __NR_sigreturn		119
#define __NR_clone		120
#define __NR_setdomainname	121
#define __NR_uname		122
#define __NR_cacheflush		123
#define __NR_adjtimex		124
#define __NR_mprotect		125
#define __NR_sigprocmask	126
#define __NR_create_module	127
#define __NR_init_module	128
#define __NR_delete_module	129
#define __NR_get_kernel_syms	130
#define __NR_quotactl		131
#define __NR_getpgid		132
#define __NR_fchdir		133
#define __NR_bdflush		134
#define __NR_sysfs		135
#define __NR_personality	136
#define __NR_afs_syscall	137 /* Syscall for Andrew File System */
#define __NR_setfsuid		138
#define __NR_setfsgid		139
#define __NR__llseek		140
#define __NR_getdents		141
#define __NR__newselect		142
#define __NR_flock		143
#define __NR_msync		144
#define __NR_readv		145
#define __NR_writev		146
#define __NR_getsid		147
#define __NR_fdatasync		148
#define __NR__sysctl		149
#define __NR_mlock		150
#define __NR_munlock		151
#define __NR_mlockall		152
#define __NR_munlockall		153
#define __NR_sched_setparam		154
#define __NR_sched_getparam		155
#define __NR_sched_setscheduler		156
#define __NR_sched_getscheduler		157
#define __NR_sched_yield		158
#define __NR_sched_get_priority_max	159
#define __NR_sched_get_priority_min	160
#define __NR_sched_rr_get_interval	161
#define __NR_nanosleep		162
#define __NR_mremap		163
#ifdef DEBUG
#define __NR_dbg_break		164
#define	__NR_dbg_hexprint	165
#define __NR_nocall		166
#else
#define __NR_nocall		164
#endif

/*
 * The interrupt vector used for system calls. We will map XINT1 to this vector
 * via the IMAP registers.
 */
#define SYSCALL_VEC	2
#ifndef __ASSEMBLY__
/*
 * This is always the same; the arguments are in global regs, so we don't need
 * to do anything special to marshall them. Notice that linux doesn't use the
 * i960's standard convention for syscalls; instead, we use interrupt XINT1 
 * and place the real syscall vector number in register g13.
 */

#ifdef __KERNEL__
#define __SYSCALL_RETURN(res,type)	do { return (type)res; } while(0)
#else
#define __SYSCALL_RETURN(res,type) do {	\
	if (res >= 0) \
		return (type) res; \
	errno = -res; \
	return (type) -1; \
} while(0)
#endif

#define __SYSCALL_BODY(type,name) \
{ \
	long res; \
	asm volatile("ldconst	%1, g13\n\t"	\
		     "calls	0\n\t"	\
		     "mov	g0, %0\n\t" \
		     : "=r"(res) : "n"(__NR_##name)	\
		     : "g0", "g13", "memory");	\
	__SYSCALL_RETURN(res,type);	\
}

/* since these are all inline, the only code we explicitly need to write
 * puts them in the g-regs; if these were normal functions, we wouldn't even
 * need that */
#define _syscall0(type,name) \
type name(void)	\
{	\
	__SYSCALL_BODY(type,name)	\
}

#define _syscall1(type,name,atype,a) \
type name(atype a)	\
{	\
	asm("mov	%0, g0" : :"l"(a):"g0");	\
	__SYSCALL_BODY(type,name)	\
}

#define _syscall2(type,name,atype,a,btype,b) \
type name(atype a, btype b)	\
{	\
	asm("mov	%0, g0\n\t"	\
	    "mov	%1, g1\n\t"	\
	    : :"l"(a), "l"(b) : "g0", "g1");	\
	__SYSCALL_BODY(type,name)	\
}

#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
type name(atype a, btype b, ctype c)	\
{	\
	asm("mov	%0, g0\n\t"	\
	    "mov	%1, g1\n\t"	\
	    "mov	%2, g2\n\t"	\
	    : :"l"(a), "l"(b), "l"(c) : "g0", "g1", "g2");	\
	__SYSCALL_BODY(type,name)	\
}

#define _syscall4(type,name,atype,a,btype,b,ctype,c,dtype,d) \
type name(atype a, btype b, ctype c, dtype d)	\
{	\
	asm("mov	%0, g0\n\t"	\
	    "mov	%1, g1\n\t"	\
	    "mov	%2, g2\n\t"	\
	    "mov	%3, g3\n\t"	\
	    : :"l"(a), "l"(b), "l"(c), "l"(d) : "g0", "g1", "g2", "g3");\
	__SYSCALL_BODY(type,name)	\
}

#define _syscall5(type,name,atype,a,btype,b,ctype,c,dtype,d,etype,e) \
type name(atype a, btype b, ctype c, dtype d, etype e)	\
{	\
	asm("mov	%0, g0\n\t"	\
	    "mov	%1, g1\n\t"	\
	    "mov	%2, g2\n\t"	\
	    "mov	%3, g3\n\t"	\
	    "mov	%4, g4\n\t"	\
	    : :"l"(a), "l"(b), "l"(c), "l"(d), "l"(e)\
	    : "g0", "g1", "g2", "g3", "g4");\
	__SYSCALL_BODY(type,name)	\
}


#define __NR__exit __NR_exit
#ifdef __KERNEL_SYSCALLS__
static inline _syscall0(int,idle)
static inline _syscall0(int,fork)
static inline _syscall2(int,clone,unsigned long,flags,char *,usp)
static inline _syscall0(int,pause)
static inline _syscall0(int,setup)
static inline _syscall0(int,sync)
static inline _syscall0(pid_t,setsid)
static inline _syscall3(int,write,int,fd,const char *,buf,off_t,count)
static inline _syscall3(int,read,int,fd,char *,buf,off_t,count)
static inline _syscall1(int,dup,int,fd)
static inline _syscall3(int,execve,const char *,file,char **,argv,char **,envp)
static inline _syscall3(int,open,const char *,file,int,flag,int,mode)
static inline _syscall1(int,close,int,fd)
static inline _syscall1(int,_exit,int,exitcode)
static inline _syscall3(pid_t,waitpid,pid_t,pid,int *,wait_stat,int,options)

static inline pid_t wait(int * wait_stat)
{
	return waitpid(-1,wait_stat,0);
}

static inline struct task_struct* pid_to_task(pid_t pid)
{
	int ii;
	struct task_struct *p = 0;
	
        for (ii=1; ii < NR_TASKS; ii++) {
		p = task[ii];
                if (p && p->pid == pid)
			break;
        }

	return p;
}
/*
 * This is the mechanism for creating a new kernel thread.
 *
 * NOTE! Only a kernel-only process(ie the swapper or direct descendants
 * who haven't done an "execve()") should use this: it will work within
 * a system call from a "real" process, but the process memory space will
 * not be free'd until both the parent and the child have exited.
 */
static inline pid_t kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
	pid_t childpid = 0;
	struct task_struct* child;
	struct frametop {
		unsigned long pfp;
		void*	sp;
		void*	rip;
		void*	r[13];	/* r3-15 */
	} *childframe;

	extern void kthread_trampoline(void);
	extern void stack_trace_from(unsigned long pfp);
	
#if 0
	printk("kernel_thread: ksp: 0x%8x\n", current->kernel_stack_page);
	printk("ipl: %d\n", get_ipl());
#endif
	childpid = do_fork(flags | CLONE_VM, 0, 0);
#if 0
	printk("after do_fork: ipl: %d\n", get_ipl());
#endif

	child = pid_to_task(childpid);
#if 0
	printk("kernel_thread: child stack: 0x%8x\n", child->kernel_stack_page);
#endif
	childframe = (struct frametop*) child->kernel_stack_page;
	childframe->pfp = 0;
	childframe->sp = childframe + 1;
#if 0
	printk("setting return to %p\n", fn);
#endif
	childframe->rip = kthread_trampoline;
	childframe->r[0] = fn;
	childframe->r[1] = arg;
	child->tss.pfp = (unsigned long) childframe;
	
#if 0
	printk("child stack trace:\n");
	stack_trace_from(child->tss.pfp);

	printk("returning %d\n", childpid);
#endif
	return childpid;
}

#endif	/* __KERNEL_SYSCALLS__ */
#endif	/* __ASSEMBLY__ */

#endif /* _ASM_I960_UNISTD_H_ */
