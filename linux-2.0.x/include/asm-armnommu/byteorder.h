#ifndef __ASM_ARM_BYTEORDER_H
#define __ASM_ARM_BYTEORDER_H

#undef ntohl
#undef ntohs
#undef htonl
#undef htons

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif

#ifndef __LITTLE_ENDIAN_BITFIELD
#define __LITTLE_ENDIAN_BITFIELD
#endif

extern unsigned long int        ntohl(unsigned long int);
extern unsigned short int       ntohs(unsigned short int);
extern unsigned long int        htonl(unsigned long int);
extern unsigned short int       htons(unsigned short int);

extern unsigned long int        __ntohl(unsigned long int);
extern unsigned short int       __ntohs(unsigned short int);
extern unsigned long int        __constant_ntohl(unsigned long int);
extern unsigned short int       __constant_ntohs(unsigned short int);


#define __constant_ntohl(x) \
	((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
			     (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
			     (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
			     (((unsigned long int)(x) & 0xff000000U) >> 24)))


extern __inline__ unsigned long int
__ntohl(unsigned long int x)
{
#if 0
	unsigned long xx;
	__asm__("eor\t%1, %0, %0, ror #16\n\t"
		"bic\t%1, %1, #0xff0000\n\t"
		"mov\t%0, %0, ror #8\n\t"
		"eor\t%0, %0, %1, lsr #8\n\t"
		: "=r" (x), "=&r" (xx)
		: "0" (x));
	return x;
#else
	return __constant_ntohl(x);
#endif
}


#define __constant_ntohs(x) \
	((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
			      (((unsigned short int)(x) & 0xff00) >> 8)))


extern __inline__ unsigned short int
__ntohs(unsigned short int x)
{
#if 0
	__asm__("eor\t%0, %0, %0, lsr #8\n\t"
		"eor\t%0, %0, %0, lsl #8\n\t"
		"bic\t%0, %0, #0xff0000\n\t"
		"eor\t%0, %0, %0, lsr #8\n\t"
		: "=r" (x) 
		: "0" (x));
	return x;
#else
	return __constant_ntohs(x);
#endif
}


#define __htonl(x) __ntohl(x)
#define __htons(x) __ntohs(x)
#define __constant_htonl(x) __constant_ntohl(x)
#define __constant_htons(x) __constant_ntohs(x)

#ifdef __OPTIMIZE__
#  define ntohl(x) \
(__builtin_constant_p((long)(x)) ? \
 __constant_ntohl((x)) : \
 __ntohl((x)))
#  define ntohs(x) \
(__builtin_constant_p((short)(x)) ? \
 __constant_ntohs((x)) : \
 __ntohs((x)))
#  define htonl(x) \
(__builtin_constant_p((long)(x)) ? \
 __constant_htonl((x)) : \
 __htonl((x)))
#  define htons(x) \
(__builtin_constant_p((short)(x)) ? \
 __constant_htons((x)) : \
 __htons((x)))
#else
#define ntohl __ntohl
#define ntohs __ntohs
#define htonl __htonl
#define htons __htons
#endif
#endif

