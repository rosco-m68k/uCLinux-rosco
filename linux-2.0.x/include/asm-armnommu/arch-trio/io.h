/*
 * linux/include/asm-arm/arch-a5k/io.h
 *
 * Copyright (C) 1997 Russell King
 * Copyright (C) 1999 Aplio SA
 *
 * Modifications:
 *  06-Dec-1997	RMK	Created.
 */
#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

/*
 * This architecture does not require any delayed IO, and
 * has the constant-optimised IO
 */
#undef	ARCH_IO_DELAY


#if 0
/*
 * We use two different types of addressing - PC style addresses, and ARM
 * addresses.  PC style accesses the PC hardware with the normal PC IO
 * addresses, eg 0x3f8 for serial#1.  ARM addresses are 0x80000000+
 * and are translated to the start of IO.  Note that all addresses are
 * shifted left!
 */
#define __PORT_PCIO(x)	(!((x) & 0x80000000))

/*
 * Dynamic IO functions - let the compiler
 * optimize the expressions
 */
extern __inline__ void __outb (unsigned int value, unsigned int port)
{
	unsigned long temp;
	__asm__ __volatile__(
	"tst	%2, #0x80000000\n\t"
	"mov	%0, %4\n\t"
	"addeq	%0, %0, %3\n\t"
	"strb	%1, [%0, %2, lsl #2]"
	: "=&r" (temp)
	: "r" (value), "r" (port), "Ir" (PCIO_BASE - IO_BASE), "Ir" (IO_BASE)
	: "cc");
}

extern __inline__ void __outw (unsigned int value, unsigned int port)
{
	unsigned long temp;
	__asm__ __volatile__(
	"tst	%2, #0x80000000\n\t"
	"mov	%0, %4\n\t"
	"addeq	%0, %0, %3\n\t"
	"str	%1, [%0, %2, lsl #2]"
	: "=&r" (temp)
	: "r" (value|value<<16), "r" (port), "Ir" (PCIO_BASE - IO_BASE), "Ir" (IO_BASE)
	: "cc");
}

extern __inline__ void __outl (unsigned int value, unsigned int port)
{
	unsigned long temp;
	__asm__ __volatile__(
	"tst	%2, #0x80000000\n\t"
	"mov	%0, %4\n\t"
	"addeq	%0, %0, %3\n\t"
	"str	%1, [%0, %2, lsl #2]"
	: "=&r" (temp)
	: "r" (value), "r" (port), "Ir" (PCIO_BASE - IO_BASE), "Ir" (IO_BASE)
	: "cc");
}

#define DECLARE_DYN_IN(sz,fnsuffix,instr)					\
extern __inline__ unsigned sz __in##fnsuffix (unsigned int port)		\
{										\
	unsigned long temp, value;						\
	__asm__ __volatile__(							\
	"tst	%2, #0x80000000\n\t"						\
	"mov	%0, %4\n\t"							\
	"addeq	%0, %0, %3\n\t"							\
	"ldr" ##instr## "	%1, [%0, %2, lsl #2]"				\
	: "=&r" (temp), "=r" (value)						\
	: "r" (port), "Ir" (PCIO_BASE - IO_BASE), "Ir" (IO_BASE)		\
	: "cc");								\
	return (unsigned sz)value;						\
}

extern __inline__ unsigned int __ioaddr (unsigned int port)			\
{										\
	if (__PORT_PCIO(port))							\
		return (unsigned int)(PCIO_BASE + (port << 2));			\
	else									\
		return (unsigned int)(IO_BASE + (port << 2));			\
}



#undef DECLARE_DYN_IN

/*
 * Constant address IO functions
 *
 * These have to be macros for the 'J' constraint to work -
 * +/-4096 immediate operand.
 */
#define __outbc(value,port)							\
({										\
	if (__PORT_PCIO((port)))						\
		__asm__ __volatile__(						\
		"strb	%0, [%1, %2]"						\
		: : "r" (value), "r" (PCIO_BASE), "Jr" ((port) << 2));		\
	else									\
		__asm__ __volatile__(						\
		"strb	%0, [%1, %2]"						\
		: : "r" (value), "r" (IO_BASE), "r" ((port) << 2));		\
})

#define __inbc(port)								\
({										\
	unsigned char result;							\
	if (__PORT_PCIO((port)))						\
		__asm__ __volatile__(						\
		"ldrb	%0, [%1, %2]"						\
		: "=r" (result) : "r" (PCIO_BASE), "Jr" ((port) << 2));		\
	else									\
		__asm__ __volatile__(						\
		"ldrb	%0, [%1, %2]"						\
		: "=r" (result) : "r" (IO_BASE), "r" ((port) << 2));		\
	result;									\
})

#define __outwc(value,port)							\
({										\
	unsigned long v = value;						\
	if (__PORT_PCIO((port)))						\
		__asm__ __volatile__(						\
		"str	%0, [%1, %2]"						\
		: : "r" (v|v<<16), "r" (PCIO_BASE), "Jr" ((port) << 2));	\
	else									\
		__asm__ __volatile__(						\
		"str	%0, [%1, %2]"						\
		: : "r" (v|v<<16), "r" (IO_BASE), "r" ((port) << 2));		\
})

#define __inwc(port)								\
({										\
	unsigned short result;							\
	if (__PORT_PCIO((port)))						\
		__asm__ __volatile__(						\
		"ldr	%0, [%1, %2]"						\
		: "=r" (result) : "r" (PCIO_BASE), "Jr" ((port) << 2));		\
	else									\
		__asm__ __volatile__(						\
		"ldr	%0, [%1, %2]"						\
		: "=r" (result) : "r" (IO_BASE), "r" ((port) << 2));		\
	result & 0xffff;							\
})

#define __outlc(v,p) __outwc((v),(p))

#define __inlc(port)								\
({										\
	unsigned long result;							\
	if (__PORT_PCIO((port)))						\
		__asm__ __volatile__(						\
		"ldr	%0, [%1, %2]"						\
		: "=r" (result) : "r" (PCIO_BASE), "Jr" ((port) << 2));		\
	else									\
		__asm__ __volatile__(						\
		"ldr	%0, [%1, %2]"						\
		: "=r" (result) : "r" (IO_BASE), "r" ((port) << 2));		\
	result;									\
})

#define __ioaddrc(port)								\
({										\
	unsigned long addr;							\
	if (__PORT_PCIO((port)))						\
		addr = PCIO_BASE + ((port) << 2);				\
	else									\
		addr = IO_BASE + ((port) << 2);					\
	addr;									\
})


#else

/*
 * Translated address IO functions
 *
 * IO address has already been translated to a virtual address
 */
#define outb_t(v,p)	(*(volatile unsigned char *)(p) = (v))
#define outl_t(v,p)	(*(volatile unsigned long *)(p) = (v))
#define inb_t(p) 	 (*(volatile unsigned char *)(p))
#define inl_t(p)	(*(volatile unsigned long *)(p))

#if 1
#define outw_t(v,p)	(*(volatile unsigned short *)(p) = (v))
#define inw_t(p) 	(*(volatile unsigned short *)(p))
#else

#define outw_t(value,port)			\
			({										\
			__asm__ __volatile__(						\
			"strh	%0, [%1]"						\
			: : "r" (value), "r" (port));		\
			})

#define inw_t(port) \
			({										\
			unsigned short result;							\
			__asm__ __volatile__(						\
			"ldrh	%0, [%1]"						\
			: "=r" (result) : "r" (port));		\
			result & 0xffff;							\
			})

#endif

extern __inline__ void __outb (unsigned int value, unsigned int port) { outb_t(value,port); }
extern __inline__ void __outw (unsigned int value, unsigned int port) { outw_t(value,port); }
extern __inline__ void __outl (unsigned int value, unsigned int port) { outl_t(value,port); }


#define DECLARE_DYN_IN(sz,fnsuffix,instr)					\
extern __inline__ unsigned sz __in##fnsuffix (unsigned int port) { return in##fnsuffix##_t(port); }


DECLARE_DYN_IN(char,b,"b")
DECLARE_DYN_IN(short,w,"")
DECLARE_DYN_IN(long,l,"")

#undef DECLARE_DYN_IN

#define __outbc(value,port)	outb_t(value,port)
#define __outwc(value,port)	outw_t(value,port)
#define __outlc(value,port)	outl_t(value,port)

#endif


#endif
