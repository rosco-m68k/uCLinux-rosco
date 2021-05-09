#ifndef _I960_BYTEORDER_H
#define _I960_BYTEORDER_H

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif

#ifndef __LITTLE_ENDIAN_BITFIELD
#define __LITTLE_ENDIAN_BITFIELD
#endif

#undef ntohl
#undef ntohs
#undef htonl
#undef htons

extern unsigned long int	ntohl(unsigned long int);
extern unsigned short int	ntohs(unsigned short int);
extern unsigned long int	htonl(unsigned long int);
extern unsigned short int	htons(unsigned short int);

extern __inline__ unsigned long int	__ntohl(unsigned long int);
extern __inline__ unsigned short int	__ntohs(unsigned short int);

extern __inline__ unsigned long int __ntohl(unsigned long int x)
{
	__asm__("bswap	%1, %0"
		:"=r"(x)
		:"0"(x));
	return x;
}

extern __inline__ unsigned short int __ntohs(unsigned short int x)
{
	register unsigned long lx = x << 16;
	__asm__("bswap %0, %0"
		:"=r"(lx)
		: "0"(lx));
	return (short) lx&0xffff;
}

#define __htonl(x) __ntohl(x)
#define __htons(x) __ntohs(x)

#define ntohl(x) __ntohl(x)
#define ntohs(x) __ntohs(x)
#define htonl(x) __htonl(x)
#define htons(x) __htons(x)

#endif /* _I960_BYTEORDER_H */
