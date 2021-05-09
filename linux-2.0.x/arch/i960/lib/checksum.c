/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.
 *
 *		IP/TCP/UDP checksumming routines
 *
 * Authors:	Keith Adams,	<kma@cse.ogi.edu>
 * 		Jorge Cwik, <jorge@laser.satlink.net>
 *		Arnt Gulbrandsen, <agulbra@nvg.unit.no>
 *		Tom May, <ftom@netcom.com>
 *		Andreas Schwab, <schwab@issan.informatik.uni-dortmund.de>
 *		Lots of code moved from tcp.c and ip.c; see those files
 *		for more names.
 *
 */
 
#include <net/checksum.h>

#if 1
static unsigned long do_csum(const unsigned char * buff, int len)
{
	int odd, count;
	unsigned long result = 0;

	if (len <= 0)
		goto out;
	odd = 1 & (unsigned long) buff;
	if (odd) {
		result = *buff;
		len--;
		buff++;
	}
	count = len >> 1;		/* nr of 16-bit words.. */
	if (count) {
		if (2 & (unsigned long) buff) {
			result += *(unsigned short *) buff;
			count--;
			len -= 2;
			buff += 2;
		}
		count >>= 1;		/* nr of 32-bit words.. */
		if (count) {
#if 1
		        unsigned long carry = 0;
			do {
				unsigned long w = *(unsigned long *) buff;
				count--;
				buff += 4;
				result += carry;
				result += w;
				carry = (w > result);
			} while (count);
			result += carry;
#else
			asm("cmpo	0, 1");		/* clear carry */
			do {
				unsigned long w = *(unsigned long*) buff;
				count--;
				buff += 4;
				asm("addc	0, %0, %0\n\t"	/* r += carry */
				    "cmpo	0, 1\n\t"	/* reset c */
				    "addc	%1, %0, %0"	/* r += w */
				    : "=&r"(result)
				    : "r"(w), "0"(result));
			} while (count);
			asm("addc	0, %0, %0" 	/* result += carry */
			    : "=r"(result) : "0"(result));
#endif
			result = (result & 0xffff) + (result >> 16);
		}
		if (len & 2) {
			result += *(unsigned short *) buff;
			buff += 2;
		}
	}
	if (len & 1)
		result += *buff;

	result = from32to16(result);
	if (odd)
		return ntohs(result);
out:
	return result;
}
#else

/*
 * Naive implementation stolen from Stevens
 */
static unsigned long do_csum(const unsigned char * buff, int len)
{
	long sum=0;
	
	while (len > 1) {
		sum += *((unsigned short*)buff)++;
		if (sum & (1<<31))
			sum = (sum & 0xffff);
		len -= 2;
	}
	
	if (len)
		sum += (unsigned short) *buff;
	
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return sum;
}
#endif

/*
 *	This is a version of ip_compute_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 */
unsigned short ip_fast_csum(unsigned char * iph, unsigned int ihl)
{
	return ~do_csum(iph,ihl*4);
}

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
unsigned int csum_partial(const unsigned char * buff, int len, unsigned int sum)
{
	unsigned int result = do_csum(buff, len);

	/* add in old sum, and carry.. */
	result += sum;
	/* 16+c bits -> 16 bits */
	result = (result & 0xffff) + (result >> 16);
	return result;
}

/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */
unsigned short ip_compute_csum(const unsigned char * buff, int len)
{
	return ~do_csum(buff,len);
}


/*
 * copy from fs while checksumming, otherwise like csum_partial
 */

unsigned int
csum_partial_copy_fromuser(const char *src, char *dst, int len, int sum)
{
	memcpy(dst, src, len);
	return csum_partial(dst, len, sum);
}
/*
 * copy from ds while checksumming, otherwise like csum_partial
 */

unsigned int
csum_partial_copy(const char *src, char *dst, int len, int sum)
{
	memcpy(dst, src, len);
	return csum_partial(dst, len, sum);
}
