/*
 * linux/include/asm-arm/proc-armv/segment.h
 *
 * Copyright (C) 1996 Russell King
 */

#ifndef __ASM_PROC_SEGMENT_H
#define __ASM_PROC_SEGMENT_H

#ifdef NO_MM
static __inline__ void __put_user(unsigned long x, void * y, int size)
{
	switch (size) {
	case 1:
	*(unsigned char*)y = x;
	break;
	case 2:
	*(unsigned short*)y = x;
	break;
	case 4:
	*(unsigned long*)y = x;
	break;
	default:
	bad_user_access_length ();
	}
}

static __inline__ unsigned long __get_user(const void *y, int size)
{
	switch (size) {
	case 1:
	return *(unsigned char*)y;
	case 2:
	return *(unsigned short*)y;
	case 4:
	return *(unsigned long*)y;
	default:
	return bad_user_access_length ();
	}
}		

#else
static __inline__ void __put_user(unsigned long x, void * y, int size)
{
	switch (size) {
	case 1:
	__asm__(
       "teq	%2, #0\n"
       "strnebt	%0, [%1]\n"
       "streqb	%0, [%1]\n"
	: : "r" (x), "r" (y), "r" (current->tss.fs)
	: "lr", "cc");
	break;
	case 2:
	{ register unsigned long tmp1 = x; void *tmp2 = y;
	__asm__ __volatile__(
       "teq	%4, #0\n"
       "strnebt	%0, [%1], #1\n"
       "streqb	%0, [%1], #1\n"
	   "mov	%0, %0, lsr #8\n"
       "strnebt	%0, [%1]\n"
       "streqb	%0, [%1]\n"
	: "=&r" (tmp1), "=&r" (tmp2)
	: "0" (tmp1), "1" (tmp2), "r" (current->tss.fs)
	: "lr", "cc");
	}
	break;
	case 4:
	__asm__(
       "teq	%2, #0\n"
       "strnet	%0, [%1]\n"
       "streq	%0, [%1]\n"
	: : "r" (x), "r" (y), "r" (current->tss.fs)
	: "lr", "cc");
	break;
	default:
	bad_user_access_length ();
	}
}

static __inline__ unsigned long __get_user(const void *y, int size)
{
	unsigned long result;
	unsigned long tmp;

	switch (size) {
	case 1:
	__asm__(
       "teq	%2, #0\n"
		"ldrnebt	%0, [%1]\n"
       "ldreqb	%0, [%1]\n"
	: "=r" (result)
	: "r" (y), "r" (current->tss.fs)
	: "lr", "cc");
	return result;
	case 2:
	__asm__(
       "teq	%2, #0\n"
		"ldreqh	%0, [%1]\n"
		"ldrnebt	%0, [%1],#1\n"
		"ldrnebt	%3, [%1]\n"
		"movne	%0, %0, lsl #8\n"
		"orrne	%0, %0, %3\n"
		"mov	%0, %0, lsl #16\n"
		"mov	%0, %0, lsr #16\n"
	: "=&r" (result)
	: "r" (y), "r" (current->tss.fs), "r" (tmp)
	: "lr", "cc");
	return result;
	case 4:
	__asm__(
       "teq	%2, #0\n"
       "ldrnet	%0, [%1]\n"
       "ldreq	%0, [%1]\n"
	: "=r" (result)
	: "r" (y), "r" (current->tss.fs)
	: "lr", "cc");
	return result;
	default:
	return bad_user_access_length ();
	}
}		
#endif

#endif /* __ASM_PROC_SEGMENT_H */

