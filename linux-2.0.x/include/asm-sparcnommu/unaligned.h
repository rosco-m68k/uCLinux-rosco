#ifndef __M68K_UNALIGNED_H
#define __M68K_UNALIGNED_H

/*
 * The m68k can do unaligned accesses itself. 
 *
 * The strange macros are there to make sure these can't
 * be misused in a way that makes them not work on other
 * architectures where unaligned accesses aren't as simple.
 */ 

#ifdef BLASTED_MOTOROLA

#define get_unaligned(ptr) (*(ptr))

#define put_unaligned(val, ptr) ((void)( *(ptr) = (val) ))

#else

/* However, the simpler architecture m68k devices, including '328 and '332,
   cannot perform unaligned access. - kja */

#define get_unaligned(ptr) ({			\
	typeof((*(ptr))) x;			\
	memcpy(&x, (void*)ptr, sizeof(*(ptr)));	\
	x;					\
})

#define put_unaligned(val, ptr) ({		\
	typeof((*(ptr))) x = val;		\
	memcpy((void*)ptr, &x, sizeof(*(ptr)));	\
})

#endif

#endif
