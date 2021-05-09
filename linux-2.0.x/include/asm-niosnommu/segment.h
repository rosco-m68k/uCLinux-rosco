#ifndef _NIOS_SEGMENT_H
#define _NIOS_SEGMENT_H

#ifndef __ASSEMBLY__

/*
 * This is a gcc optimization barrier, which essentially
 * inserts a sequence point in the gcc RTL tree that gcc
 * can't move code around. This is needed when we enter
 * or exit a critical region (in this case around user-level
 * accesses that may sleep, and we can't let gcc optimize
 * global state around them).
 */
#define __gcc_barrier() __asm__ __volatile__("": : :"memory")

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

/* I should make this use unaligned transfers etc.. */
static inline void __put_user(unsigned long x, void * y, int size)
{
	__gcc_barrier();
	switch (size) {
		case 1:
			*(char *) y = x;
			break;
		case 2:
			*(short *) y = x;
			break;
		case 4:
			*(int *) y = x;
			break;
		default:
			bad_user_access_length();
	}
	__gcc_barrier();
}

/* I should make this use unaligned transfers etc.. */
static inline unsigned long __get_user(const void * y, int size)
{
	unsigned long result;

	__gcc_barrier();
	switch (size) {
		case 1:
			result = *(unsigned char *) y;
			break;
		case 2:
			result = *(unsigned short *) y;
			break;
		case 4:
			result = *(unsigned int *) y;
			break;
		default:
			result = bad_user_access_length();
	}
	__gcc_barrier();
	return result;
}


/*
 * Deprecated routines
 */

static inline unsigned char get_user_byte(const char * addr)
{
	return *addr;
}

#define get_fs_byte(addr) get_user_byte((char *)(addr))

static inline unsigned short get_user_word(const short *addr)
{
	return *addr;
}

#define get_fs_word(addr) get_user_word((short *)(addr))

static inline unsigned long get_user_long(const int *addr)
{
	return *addr;
}

#define get_fs_long(addr) get_user_long((int *)(addr))

static inline void put_user_byte(char val,char *addr)
{
	*addr = val;
}

#define put_fs_byte(x,addr) put_user_byte((x),(char *)(addr))

static inline void put_user_word(short val,short * addr)
{
	*addr = val;
}

#define put_fs_word(x,addr) put_user_word((x),(short *)(addr))

static inline void put_user_long(unsigned long val,int * addr)
{
	*addr = val;
}

#define put_fs_long(x,addr) put_user_long((x),(int *)(addr))


static inline void memcpy_fromfs(void * to, const void * from, unsigned long n)
{
	__gcc_barrier();
	memcpy(to, from, n);
	__gcc_barrier();
}

static inline void memcpy_tofs(void * to, const void * from, unsigned long n)
{
	__gcc_barrier();
	memcpy(to, from, n);
	__gcc_barrier();
}

/* Nios is not segmented, however we need to be able to fool verify_area()
 * when doing system calls from kernel mode legitimately.
 */
#define KERNEL_DS   0
#define USER_DS     1

extern int active_ds;

static inline int get_fs(void)
{
	return active_ds;
}

static inline int get_ds(void)
{
	return KERNEL_DS;
}

static inline void set_fs(int val)
{
	active_ds = val;
}

#endif  /* __ASSEMBLY__ */

#endif /* _NIOS_SEGMENT_H */
