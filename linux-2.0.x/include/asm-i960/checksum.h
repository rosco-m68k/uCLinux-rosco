#ifndef _I960_CHECKSUM_H
#define _I960_CHECKSUM_H

/*
 * computes the checksum of a memory block at buff, length len,
 * and adds in "sum" (32-bit)
 *
 * returns a 32-bit number suitable for feeding into itself
 * or csum_tcpudp_magic
 *
 * this function must be called with even lengths, except
 * for the last fragment, which may be odd
 *
 * it's best to have buff aligned on a 32-bit boundary
 */
unsigned int csum_partial(const unsigned char * buff, int len, unsigned int sum);

/*
 * the same as csum_partial_copy, but copies from src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */

unsigned int csum_partial_copy(const char *src, char *dst, int len, int sum);


/*
 * the same as csum_partial_copy, but copies from user space.
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */

unsigned int csum_partial_copy_fromuser(const char *src, char *dst, int len, int sum);

unsigned short ip_fast_csum(unsigned char *iph, unsigned int ihl);

static inline unsigned short from32to16(unsigned long sum)
{
	unsigned long tmp;
	asm("addo	%2, %3, %0
	    shro	16, %0, %1
	    clrbit	16, %0, %0
	    addo	%0, %1, %0"
	    : "=&r" (sum), "=r"(tmp)
	    : "r"(sum & 0xffff), "r"(sum >> 16), "0"(sum));
	return sum;
}
/*
 *	Fold a partial checksum
 */

static inline unsigned int csum_fold(unsigned int sum)
{
#if 1
	return ~from32to16(sum);
#else
	sum = (sum & 0xffff) + (sum >> 16);	/* add top and bottom */
	sum = (sum & 0xffff) + (sum >> 16);	/* add carries from previous */
	return ~sum;
#endif
}


/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */

static inline unsigned short 
csum_tcpudp_magic(unsigned long saddr, unsigned long daddr, unsigned short len,
		  unsigned short proto, unsigned int sum)
{
#if 0
	register unsigned long lenproto = ((ntohs(len)<<16) + proto * 256);
	sum += saddr;
	sum += daddr + (saddr > sum);
	sum += lenproto + (daddr > sum);
	sum += (lenproto > sum);
#else
	asm("cmpo	1, 0
	    addc	%1, %2, %0
	    addc	%1, %3, %0
	    addc	%1, %4, %0
	    addc	0, %1, %0"
	    : "=&r"(sum)
	    : "0"(sum), "r"(saddr), "r"(daddr), 
	    "r"((ntohs(len)<<16) + proto * 256));
#endif

	return csum_fold(sum);
}


/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */

extern unsigned short
ip_compute_csum(const unsigned char * buff, int len);

#endif /* _I960_CHECKSUM_H */
