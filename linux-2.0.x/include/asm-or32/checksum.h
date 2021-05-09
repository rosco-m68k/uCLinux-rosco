#ifndef _OR32_CHECKSUM_H
#define _OR32_CHECKSUM_H

#include <linux/config.h>

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

/*
 *	Fold a partial checksum without adding pseudo headers
 */

static inline unsigned short 
csum_fold(unsigned int sum)
{
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return ~sum;
}

/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */

static inline unsigned short int
csum_tcpudp_magic(unsigned long saddr, unsigned long daddr, unsigned short len,
		  unsigned short proto, unsigned int sum)
{
	unsigned long result = saddr;
	
	result += daddr;
	result += (daddr > result);

	result += len;
	result += (len > result);

	result += proto;
	result += (proto > result);

	result += sum;
        result += (sum > result);

	result = (result >> 16) + (result & 0xffff);
	result = (result >> 16) + (result & 0xffff);

	return ~result;
}

extern unsigned short
ip_compute_csum(const unsigned char * buff, int len);

#endif /* _OR32_CHECKSUM_H */
