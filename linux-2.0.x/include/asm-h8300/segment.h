#ifndef _H8300_SEGMENT_H
#define _H8300_SEGMENT_H

/* define constants */
/* Address spaces (FC0-FC2) */
#define USER_DATA     (1)
#ifndef USER_DS
#define USER_DS       (USER_DATA)
#endif
#define USER_PROGRAM  (2)
#define SUPER_DATA    (5)
#ifndef KERNEL_DS
#define KERNEL_DS     (SUPER_DATA)
#endif
#define SUPER_PROGRAM (6)
#define CPU_SPACE     (7)

#ifndef __ASSEMBLY__

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

#define __ptr(x) ((unsigned long *)(x))

static inline void __put_user(unsigned long x, void * y, int size)
{
	switch (size) {
		case 1:
			*(unsigned char *)y = x;
			break;
		case 2:
			*(unsigned short *)y = x;
			break;
		case 4:
			*(unsigned long *)y = x;
			break;
		default:
			bad_user_access_length();
	}
}

static inline unsigned long __get_user(const void * y, int size)
{
	switch (size) {
		case 1:
		  return *(unsigned char *)y;
		case 2:
		  return *(unsigned short *)y;
		case 4:
		  return *(unsigned long *)y;
		default:
			return bad_user_access_length();
	}
}
#undef __ptr

/*
 * These are deprecated..
 *
 * Use "put_user()" and "get_user()" with the proper pointer types instead.
 */

#define get_fs_byte(addr) __get_user((const unsigned char *)(addr),1)
#define get_fs_word(addr) __get_user((const unsigned short *)(addr),2)
#define get_fs_long(addr) __get_user((const unsigned int *)(addr),4)

#define put_fs_byte(x,addr) __put_user((x),(unsigned char *)(addr),1)
#define put_fs_word(x,addr) __put_user((x),(unsigned short *)(addr),2)
#define put_fs_long(x,addr) __put_user((x),(unsigned int *)(addr),4)

#ifdef WE_REALLY_WANT_TO_USE_A_BROKEN_INTERFACE

static inline unsigned char get_user_byte(const char * addr)
{
	return __get_user(addr,1);
}

static inline unsigned short get_user_word(const short *addr)
{
	return __get_user(addr,2);
}

static inline unsigned long get_user_long(const int *addr)
{
	return __get_user(addr,4);
}

static inline void put_user_byte(char val,char *addr)
{
	__put_user(val, addr, 1);
}

static inline void put_user_word(short val,short * addr)
{
	__put_user(val, addr, 2);
}

static inline void put_user_long(unsigned long val,int * addr)
{
	__put_user(val, addr, 4);
}

#endif

static inline void memcpy_fromfs(void *to,void *from,int n)
{
         if (from < to) {
	         (char *)to=(char *)to+n-1;
	         (char *)from=(char *)from+n-1;
                 for (;n>0;n--)
                         *((char *)to)--=*((char *)from)--;
         } else {
                 for (;n>0;n--)
		         *((char *)to)++=*((char *)from)++;
	 }
}
                       
static inline void memcpy_tofs(void *to,void *from,int n)
{
         if (from < to) {
	         (char *)to=(char *)to+n-1;
	         (char *)from=(char *)from+n-1;
                 for (;n>0;n--)
                         *((char *)to)--=*((char *)from)--;
         } else {
                 for (;n>0;n--)
		         *((char *)to)++=*((char *)from)++;
         }
}

/*
 * Get/set the SFC/DFC registers for MOVES instructions
 */

static inline unsigned long get_fs(void)
{
#ifdef NO_MM
	return USER_DS;
#else
	unsigned long _v;
	__asm__ ("movec %/dfc,%0":"=r" (_v):);

	return _v;
#endif
}

static inline unsigned long get_ds(void)
{
    /* return the supervisor data space code */
    return KERNEL_DS;
}

static inline void set_fs(unsigned long val)
{
#ifndef NO_MM
	__asm__ __volatile__ ("movec %0,%/sfc\n\t"
			      "movec %0,%/dfc\n\t"
			      : /* no outputs */ : "r" (val) : "memory");
#endif
}

#endif /* __ASSEMBLY__ */

#endif /* _H8300H_SEGMENT_H */
