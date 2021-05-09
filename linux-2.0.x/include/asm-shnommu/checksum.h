#ifndef _SH7615_CHECKSUM_H
#define _SH7615_CHECKSUM_H

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

 unsigned int ip_fast_csum(unsigned char *iph, unsigned int ihl);
static  unsigned int csum_fold(unsigned int sum) ;


/*
 *	This is a version of ip_compute_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 *
 */

/* Posh2 didn't understood copied from sh porting on linux.So check*/
 static __inline__  unsigned int
ip_fast_csum(unsigned char *iph, unsigned int ihl)
{
	unsigned int sum, __dummy0, __dummy1;

         __asm__ __volatile__(
                 "mov.l  @%1+, %0\n\t"
                 "mov.l  @%1+, %3\n\t"
                 "add    #-2, %2\n\t"
                 "clrt\n\t"
                 "1:\t"
                 "addc   %3, %0\n\t"
                 "movt   %4\n\t"
                 "mov.l  @%1+, %3\n\t"
                 "dt     %2\n\t"
                 "bf/s   1b\n\t"
                 " cmp/eq #1, %4\n\t"
                 "addc   %3, %0\n\t"
                 "addc   %2, %0"     /* Here %2 is 0, add carry-bit */
         /* Since the input registers which are loaded with iph and ihl
            are modified, we must also specify them as outputs, or gcc
            will assume they contain their original values. */
         : "=r" (sum), "=r" (iph), "=r" (ihl), "=&r" (__dummy0), "=&z" (__dummy1)
         : "1" (iph), "2" (ihl)
         : "t");
 
		return csum_fold(sum);

}


/*
 *	Fold a partial checksum
 */

static inline unsigned int csum_fold(unsigned int sum)
{

	unsigned int __dummy;
     __asm__("swap.w %0, %1\n\t"
                 "extu.w %0, %0\n\t"
                 "extu.w %1, %1\n\t"
                 "add    %1, %0\n\t"
                 "swap.w %0, %1\n\t"
                 "add    %1, %0\n\t"
                 "not    %0, %0\n\t"
                 : "=r" (sum), "=&r" (__dummy)
                 : "0" (sum)
                 : "t");
         return sum;

}


/* Posh2 didn't understood copied from sh porting on linux.So check*/
static __inline__ unsigned long csum_tcpudp_nofold(unsigned long saddr,
                                                    unsigned long daddr,
                                                    unsigned short len,
                                                    unsigned short proto,
                                                    unsigned int sum) 
{
#ifdef __LITTLE_ENDIAN__
        unsigned long len_proto = (ntohs(len)<<16)+proto*256;
#else
        unsigned long len_proto = (proto<<16)+len;
#endif
         __asm__("clrt\n\t"
                 "addc   %0, %1\n\t"

                 "addc   %2, %1\n\t"
                 "addc   %3, %1\n\t"
                 "movt   %0\n\t"
                 "add    %1, %0"
                 : "=r" (sum), "=r" (len_proto)
                 : "r" (daddr), "r" (saddr), "1" (len_proto), "0" (sum)
                 : "t","memory");
         return sum;
}
 
/* Posh2 didn't understood copied from sh porting on linux.So check*/
 /*
  * computes the checksum of the TCP/UDP pseudo-header
  * returns a 16-bit checksum, already complemented
  */
static __inline__ unsigned short int csum_tcpudp_magic(unsigned long saddr,
                                                        unsigned long daddr,
                                                        unsigned short len,
                                                        unsigned short proto,
                                                        unsigned int sum) 
{
        return csum_fold(csum_tcpudp_nofold(saddr,daddr,len,proto,sum));
 }


/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */


extern unsigned short ip_compute_csum(const unsigned char * buff, int len);



#endif /* _SH7615_CHECKSUM_H */
