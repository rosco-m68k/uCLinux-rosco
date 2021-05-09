#ifndef _OR32_BYTEORDER_H
#define _OR32_BYTEORDER_H

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 4321
#endif

#ifndef __BIG_ENDIAN_BITFIELD
#define __BIG_ENDIAN_BITFIELD
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

extern __inline__ unsigned long int
__ntohl(unsigned long int x)
{
	return x;
}

extern __inline__ unsigned short int
__ntohs(unsigned short int x)
{
	return x;
}

#define __htonl(x) __ntohl(x)
#define __htons(x) __ntohs(x)

#define ntohl(x) __ntohl(x)
#define ntohs(x) __ntohs(x)
#define htonl(x) __htonl(x)
#define htons(x) __htons(x)

#define le16_to_cpu(x) \
        ((__u16)((((__u16)(x) & 0x00FFU) << 8) | \
                 (((__u16)(x) & 0xFF00U) >> 8)))
 
#define le32_to_cpu(x) \
        ((__u32)((((__u32)(x) & 0x000000FFU) << 24) | \
                 (((__u32)(x) & 0x0000FF00U) <<  8) | \
                 (((__u32)(x) & 0x00FF0000U) >>  8) | \
                 (((__u32)(x) & 0xFF000000U) >> 24)))
 
#define le64_to_cpu(x) \
        ((__u64)((((__u64)(x) & 0x00000000000000FFULL) << 56) | \
                 (((__u64)(x) & 0x000000000000FF00ULL) << 40) | \
                 (((__u64)(x) & 0x0000000000FF0000ULL) << 24) | \
                 (((__u64)(x) & 0x00000000FF000000ULL) <<  8) | \
                 (((__u64)(x) & 0x000000FF00000000ULL) >>  8) | \
                 (((__u64)(x) & 0x0000FF0000000000ULL) >> 24) | \
                 (((__u64)(x) & 0x00FF000000000000ULL) >> 40) | \
                 (((__u64)(x) & 0xFF00000000000000ULL) >> 56)))
 
#define cpu_to_le16(x) (le16_to_cpu(x))
#define cpu_to_le32(x) (le32_to_cpu(x))
#define cpu_to_le64(x) (le64_to_cpu(x))

#endif
