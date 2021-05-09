#ifndef __H8300H_UNALIGNED_H
#define __H8300H_UNALIGNED_H

/*
 * The H8/300H can do unaligned accesses itself. 
 *
 * The strange macros are there to make sure these can't
 * be misused in a way that makes them not work on other
 * architectures where unaligned accesses aren't as simple.
 */ 

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
