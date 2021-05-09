/*
 *   FILE: syscall.c
 * AUTHOR: kma
 *  DESCR: High-level system call path.
 */

#ident "$Id: syscall.c,v 1.1 1999/12/03 06:02:34 gerg Exp $"
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>

#include <asm/unistd.h>
#include <asm/cachectl.h>
#include <asm/dprintk.h>
#include <asm/mman.h>

#ifdef DEBUG
#define CHECK_STACK() \
do { \
	unsigned long sp;	\
	\
	__asm__ __volatile__("mov sp, %0" : "=r"(sp));	\
	if (current->pid	\
	    && (!(sp > current->kernel_stack_page	\
	     && sp < (current->kernel_stack_page + PAGE_SIZE))))	\
		panic("bad ksp: 0x%8x\ncurrent ksp: 0x%8x", sp,	\
		      current->kernel_stack_page);	\
} while(0)
#else
#define CHECK_STACK() \
do { } while (0)
#endif

extern void system_break(void);
extern int (*syscall_tab[])(int, int, int, int, int);
static int sys_mmap(struct pt_regs* regs);

asmlinkage void
csyscall(struct pt_regs* regs)
{
	unsigned long	num = regs->gregs[13];
	extern void stack_trace(void);
	extern void leave_kernel(struct pt_regs* regs);
	
	CHECK_STACK();
#if 0
	if (user_mode(regs)) {
		printk("syscall %d; pc == 0x%8x\n", num, get_pc());
		stack_trace();
	}
#endif
	if (num >= 0 && num < __NR_nocall) {
		switch(num) {
			/*
			 * system calls that need the regs
			 */
			case __NR_fork:
			case __NR_clone:
			case __NR_execve:
			case __NR_sigsuspend:
				regs->gregs[0] = ((int (*)(int))(syscall_tab[num]))((int)regs);
				break;

#ifdef DEBUG	/* help debug user applications */
			case __NR_dbg_break:
				printk("break: %s\n", regs->gregs[0]);
				system_break();
				break;

			case __NR_dbg_hexprint:
				printk("value: %x\n", regs->gregs[0]);
				break;
#endif
			case __NR_mmap:
				regs->gregs[0] = sys_mmap(regs);
#if 0
				dprintk("mmap: returning 0x%8x\n",
					regs->gregs[0]);
#endif
				break;

			default:
				regs->gregs[0] = 
					syscall_tab[num](regs->gregs[0],
							 regs->gregs[1],
							 regs->gregs[2],
							 regs->gregs[3],
							 regs->gregs[4]);
				break;
		}
	} else {
		regs->gregs[0] = -ENOSYS;
	}
#if 0	
	printk("csyscall: returning %p\n", regs->gregs[0]);
	stack_trace();
#endif
	
	leave_kernel(regs);
}

static int
sys_mmap(struct pt_regs* regs)
{
	struct file* file = 0;
	unsigned long flags = regs->gregs[3];

	if (!(flags & MAP_ANON)) {
		unsigned long fd = regs->gregs[4];
		if (fd >= NR_OPEN || !(file = current->files->fd[fd]))
			return -EBADF;
	}
	
	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
	return do_mmap(file, regs->gregs[0], regs->gregs[1], regs->gregs[2], 
		       flags, regs->gregs[5]);
}

asmlinkage int
sys_fork(struct pt_regs* regs)
{
	return do_fork(SIGCHLD|CLONE_WAIT, regs->lregs[PT_SP], regs);
}

asmlinkage int
sys_clone(struct pt_regs* regs)
{
	dprintk("in sys_clone\n");
	return do_fork(regs->gregs[0], regs->gregs[1], regs);
}

asmlinkage int
sys_execve(struct pt_regs* regs)
{
	dprintk("sys_execve: %s, %p, %p\n",
		regs->gregs[0], regs->gregs[1], regs->gregs[2]);
	return do_execve((void*)regs->gregs[0], (void*)regs->gregs[1], 
			 (void*)regs->gregs[2], regs);
}

asmlinkage int
sys_ioperm(unsigned long from, unsigned long num, int turn_on)
{
	return -EINVAL;
}

asmlinkage int
sys_ipc (uint call, int first, int second, int third, void* ptr, long fifth)
{
	int version;
	
	version = call >> 16;
	call &= 0xffff;

	if (call <= SEMCTL)
		switch (call) {
		case SEMOP:
			return sys_semop (first, (struct sembuf *)ptr, second);
		case SEMGET:
			return sys_semget (first, second, third);
		case SEMCTL: {
			union semun fourth;
			int err;
			if (!ptr)
				return -EINVAL;
			if ((err = verify_area (VERIFY_READ, ptr, sizeof(long))))
				return err;
			fourth.__pad = (void *) get_fs_long(ptr);
			return sys_semctl (first, second, third, fourth);
			}
		default:
			return -EINVAL;
		}
	if (call <= MSGCTL) 
		switch (call) {
		case MSGSND:
			return sys_msgsnd (first, (struct msgbuf *) ptr, 
					   second, third);
		case MSGRCV:
			switch (version) {
			case 0: {
				struct ipc_kludge tmp;
				int err;
				if (!ptr)
					return -EINVAL;
				if ((err = verify_area (VERIFY_READ, ptr, sizeof(tmp))))
					return err;
				memcpy_fromfs (&tmp,(struct ipc_kludge *) ptr,
					       sizeof (tmp));
				return sys_msgrcv (first, tmp.msgp, second, tmp.msgtyp, third);
				}
			case 1: default:
				return sys_msgrcv (first, (struct msgbuf *) ptr, second, fifth, third);
			}
		case MSGGET:
			return sys_msgget ((key_t) first, second);
		case MSGCTL:
			return sys_msgctl (first, second, (struct msqid_ds *) ptr);
		default:
			return -EINVAL;
		}
	if (call <= SHMCTL) 
		switch (call) {
		case SHMAT:
			switch (version) {
			case 0: default: {
				ulong raddr;
				int err;
				if ((err = verify_area(VERIFY_WRITE, (ulong*) third, sizeof(ulong))))
					return err;
				err = sys_shmat (first, (char *) ptr, second, &raddr);
				if (err)
					return err;
				put_fs_long (raddr, (ulong *) third);
				return 0;
				}
			case 1:	/* iBCS2 emulator entry point */
				if (get_fs() != get_ds())
					return -EINVAL;
				return sys_shmat (first, (char *) ptr, second, (ulong *) third);
			}
		case SHMDT: 
			return sys_shmdt ((char *)ptr);
		case SHMGET:
			return sys_shmget (first, second, third);
		case SHMCTL:
			return sys_shmctl (first, second, (struct shmid_ds *) ptr);
		default:
			return -EINVAL;
		}
	return -EINVAL;
	
}

asmlinkage int
sys_cacheflush(char* addr, int nbytes, int cache)
{
	if (cache & ICACHE)
		__asm__ __volatile__ ("icctl	2, g0, g0");
			
	if (cache & DCACHE)
		__asm__ __volatile__ ("dcctl	2, g0, g0"); 

	return 0;
}

void
kthread_start(void (*func)(void*), void* arg)
{
	extern long _etext, _stext;
	
	CHECK_STACK();
	if ( ((unsigned long)func > (unsigned long)&_etext)
	     || ((unsigned long)func < (unsigned long)&_stext) ) {
		panic("XXX: bad kthread addr: %p\n", func);
	}
	func(arg);
}
