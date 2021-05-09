/*
 * linux/arch/arm/kernel/sys-arm.c
 *
 * Copyright (C) People who wrote linux/arch/i386/kernel/sys_i386.c
 * Copyright (C) 1995, 1996 Russell King.
 * 
 * This file contains various random system calls that
 * have a non-standard calling sequence on the Linux/arm
 * platform.
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/mman.h>

#include <asm/segment.h>

/*
 * Constant strings used in inlined functions in header files
 */
/* proc/system.h */
const char xchg_str[] = "xchg";
/* arch/dma.h */
const char dma_str[] = "%s: dma %d not supported\n";

/*
 * sys_pipe() is the normal C calling standard for creating
 * a pipe. It's not the way unix tranditionally does this, though.
 */
asmlinkage int sys_pipe(unsigned long * fildes)
{
	int fd[2];
	int error;

	error = verify_area(VERIFY_WRITE,fildes,8);
	if (error)
		return error;
	error = do_pipe(fd);
	if (error)
		return error;
	put_fs_long(fd[0],0+fildes);
	put_fs_long(fd[1],1+fildes);
	return 0;
}

/*
 * Perform the select(nd, in, out, ex, tv) and mmap() system
 * calls. Linux/i386 didn't use to be able to handle more than
 * 4 system call parameters, so these system calls used a memory
 * block for parameter passing..
 */
asmlinkage int old_mmap(unsigned long *buffer)
{
	int error;
	unsigned long flags;
	struct file * file = NULL;

	error = verify_area(VERIFY_READ, buffer, 6*sizeof(long));
	if (error)
		return error;
	flags = get_user(buffer+3);
	if (!(flags & MAP_ANONYMOUS)) {
		unsigned long fd = get_user(buffer+4);
		if (fd >= NR_OPEN || !(file = current->files->fd[fd]))
			return -EBADF;
	}
	return do_mmap(file, get_user(buffer), get_user(buffer+1),
		       get_user(buffer+2), flags, get_user(buffer+5));
}


extern asmlinkage int sys_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

asmlinkage int old_select(unsigned long *buffer)
{
	int n;
	fd_set *inp;
	fd_set *outp;
	fd_set *exp;
	struct timeval *tvp;

	n = verify_area(VERIFY_READ, buffer, 5*sizeof(unsigned long));
	if (n)
		return n;
	n = get_user(buffer);
	inp = (fd_set *) get_user(buffer+1);
	outp = (fd_set *) get_user(buffer+2);
	exp = (fd_set *) get_user(buffer+3);
	tvp = (struct timeval *) get_user(buffer+4);
	return sys_select(n, inp, outp, exp, tvp);
}

/*
 * sys_ipc() is the de-multiplexer for the SysV IPC calls..
 *
 * This is really horribly ugly.
 */
asmlinkage int sys_ipc (uint call, int first, int second, int third, void *ptr, long fifth)
{
	int version;

	version = call >> 16; /* hack for backward compatibility */
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

static inline unsigned long get_instr(struct pt_regs *regs)
{
    unsigned long *pcv;

    pcv = (unsigned long *)instruction_pointer(regs);

    return get_user (pcv);
}

/*
 * Old functions - we used to pass 5 parameters as r0, r1, r2, *r3, *(r3+4)
 * Now use r0 - r4.  Note that these will stay for a while until people have upgraded...
 * (so that we can still use old binaries).
 *
 * Eventually these functions will disappear.
 */
asmlinkage int
sys_old_arm_init_module (char *module_name, char *code, unsigned codesize,
	struct mod_routines *modr, struct symbol_table *syms)
{
    extern int sys_init_module (char *, char *, unsigned, struct mod_routines *,
    		struct symbol_table *);
    struct pt_regs *regs;

    __asm__ ("mov %0, r9": "=r" (regs));

    if (get_instr(regs) == 0xe1a0300d) {
	unsigned long *__p;
    	__p = (unsigned long *)modr;
    	if (!(current->flags & 0x40000000)) {
		printk (KERN_NOTICE "%s (%d): Old init_module call\n", current->comm, current->pid);
		current->flags |= 0x40000000;
	}
        return sys_init_module (module_name, code, codesize, (struct mod_routines *)__p[0],
    		(struct symbol_table *)__p[1]);
    }
    return sys_init_module (module_name, code, codesize, modr, syms);
}

asmlinkage int
sys_old_arm_llseek (unsigned int fd, unsigned long offset_high, unsigned long offset_low,
		loff_t *result, unsigned int origin)
{
    extern int sys_llseek (unsigned int, unsigned long, unsigned long, loff_t *, unsigned int);
    struct pt_regs *regs;

    __asm__ ("mov %0, r9": "=r" (regs));

    if (get_instr(regs) == 0xe1a0300d) {
	unsigned long *__p;
	__p = (unsigned long *)result;
	if (!(current->flags & 0x20000000)) {
		printk (KERN_NOTICE "%s (%d): Old llseek call\n", current->comm, current->pid);
		current->flags |= 0x20000000;
	}
	return sys_llseek (fd, offset_high, offset_low, (loff_t *)__p[0], (unsigned int)__p[1]);
    }
    return sys_llseek (fd, offset_high, offset_low, result, origin);
}

asmlinkage int
sys_old_arm_mount (char *devname, char *dirname, char *type, unsigned long flags, void *data)
{
    extern int sys_mount (char *, char *, char *, unsigned long, void *);
    struct pt_regs *regs;

    __asm__ ("mov %0, r9": "=r" (regs));

    if (get_instr(regs) == 0xe1a0300d) {
	unsigned long *__p;
	__p = (unsigned long *)flags;
	if (!(current->flags & 0x10000000)) {
		printk (KERN_NOTICE "%s (%d): Old mount call\n", current->comm, current->pid);
		current->flags |= 0x10000000;
	}
	return sys_mount (devname, dirname, type, __p[0], (void *)__p[1]);
    }
    return sys_mount (devname, dirname, type, flags, data);
}
