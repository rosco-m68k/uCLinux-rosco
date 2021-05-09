#ifndef __ASM_ARM_SEGMENT_H
#define __ASM_ARM_SEGMENT_H

#define KERNEL_CS   0x0
#define KERNEL_DS   0x0

#define USER_CS     0x1
#define USER_DS     0x1

#ifndef __ASSEMBLY__

#ifdef NO_INLINE
#define __INLINE__
#else
#define __INLINE__ inline
#endif

#include <linux/sched.h>

/*
 * A better way to do this might be to pass to put/get user a flag
 * indicating the possibility of kernel memory accesses...  That
 * would allow gcc to optimize the following routines a bit better
 * than that which is possible at the moment.
 */
#define IS_USER_SEG (current->tss.fs != KERNEL_DS)

/*
 * Uh, these should become the main single-value transfer routines..
 * They automatically use the right size if we just have the right
 * pointer type..
 */
#define put_user(x,ptr) __put_user((unsigned long)(x),(ptr),sizeof(*(ptr)))
#define get_user(ptr) ((__typeof__(*(ptr)))__get_user((ptr),sizeof(*(ptr))))

/*
 * This is a silly but good way to make sure that
 * the __put_user function is indeed always optimized,
 * and that we use the correct sizes..
 */
extern int bad_user_access_length(void);

#ifdef __KERNEL__

#include <asm/proc/segment.h>

/*
 * these are depreciated..
 */
static __INLINE__ unsigned char get_user_byte(const char *addr)
{
	return __get_user(addr, 1);
}

#define get_fs_byte(addr) get_user_byte((char *)(addr))

static __INLINE__ unsigned short get_user_word(const short *addr)
{
	return __get_user(addr, 2);
}

#define get_fs_word(addr) get_user_word((short *)(addr))

static __INLINE__ unsigned long get_user_long(const long *addr)
{
	return __get_user(addr, 4);
}

#define get_fs_long(addr) get_user_long((long *)(addr))

static __INLINE__ void put_user_byte(char val,char *addr)
{
	__put_user(val, addr, 1);
}

#define put_fs_byte(x,addr) put_user_byte((x),(char *)(addr))

static __INLINE__ void put_user_word(short val,short *addr)
{
	__put_user(val, addr, 2);
}

#define put_fs_word(x,addr) put_user_word((x),(short *)(addr))

static __INLINE__ void put_user_long(long val,long *addr)
{
	__put_user(val, addr, 4);
}

#define put_fs_long(x,addr) put_user_long((x),(long *)(addr))


extern void memcpy_fromfs(void * to, const void * from, unsigned long n);
extern void __memcpy_tofs(void * to, const void * from, unsigned long n);
static __INLINE__ void memcpy_tofs(void *to, const void *from, unsigned long n)
{
#ifndef NO_MM
	if(IS_USER_SEG)
		__memcpy_tofs(to, from, n);
	else
#endif
		memcpy(to, from, n);
}

static inline unsigned long get_fs(void)
{
	return current->tss.fs;
}

static inline unsigned long get_ds(void)
{
	return KERNEL_DS;
}

static inline void set_fs(unsigned long val)
{
	current->tss.fs = val;
}

#endif /* __KERNEL__ */

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARM_SEGMENT_H */

