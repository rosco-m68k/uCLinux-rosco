#ifndef __NIOS_CHECKSUM_H
#define __NIOS_CHECKSUM_H

/*  checksum.h:  IP/UDP/TCP checksum routines on the NIOS.
 *
 *  Copyright(C) 1995 Linus Torvalds
 *  Copyright(C) 1995 Miguel de Icaza
 *  Copyright(C) 1996 David S. Miller
 *  Copyright(C) 2001 Ken Hill
 *
 * derived from:
 *	Alpha checksum c-code
 *      ix86 inline assembly
 *      Spar nommu
 */


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */

extern inline unsigned short csum_tcpudp_magic(unsigned long saddr,
					       unsigned long daddr,
					       unsigned short len,
					       unsigned short proto,
					       unsigned int sum)
{
	__asm__ __volatile__("
		add    	%0, %3
		skps	cc_nc
		addi	%0, 1
		add	%0, %4
		skps	cc_nc
		addi	%0, 1
		add	%0, %5
		skps	cc_nc
		addi	%0, 1

		; We need the carry from the addition of 16-bit
		; significant addition, so we zap out the low bits
		; in one half, zap out the high bits in another,
		; shift them both up to the top 16-bits of a word
		; and do the carry producing addition, finally
		; shift the result back down to the low 16-bits.

		; Actually, we can further optimize away two shifts
		; because we know the low bits of the original
		; value will be added to zero-only bits so cannot
		; affect the addition result nor the final carry
		; bit.

		mov	%1,%0			; Need a copy to fold with
		lsli	%1, 16			; Bring the LOW 16 bits up
		add	%0, %1			; add and set carry, neat eh?
		lsri	%0, 16			; shift back down the result
		skps	cc_nc			; get remaining carry bit
		addi	%0, 1
		not	%0			; negate
		"
	        : "=&r" (sum), "=&r" (saddr)
		: "0" (sum), "1" (saddr), "r" (ntohl(len+proto)), "r" (daddr));
		return ((unsigned short) sum); 
}


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


  extern inline unsigned short from32to16(unsigned long x)
  {
	__asm__ __volatile__("
		add	%0, %1
		lsri	%0, 16
		skps	cc_nc
		addi	%0, 1
		"
		: "=r" (x)
		: "r" (x << 16), "0" (x));
	return x;
  }


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


extern inline unsigned long do_csum(unsigned char * buff, int len)
{
 int odd, count;
 unsigned long result = 0;

 if (len <= 0)
 	goto out;
 odd = 1 & (unsigned long) buff;
 if (odd) {
////result = *buff;                     // dgt: Big    endian
 	result = *buff << 8;                // dgt: Little endian

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
 		result = (result & 0xffff) + (result >> 16);
 	}
 	if (len & 2) {
 		result += *(unsigned short *) buff;
 		buff += 2;
 	}
 }
 if (len & 1)
 	result += *buff;  /* This is little machine, byte is right */
 result = from32to16(result);
 if (odd)
 	result = ((result >> 8) & 0xff) | ((result & 0xff) << 8);
out:
	return result;
  }


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/* ihl is always 5 or greater, almost always is 5, iph is always word
 * aligned but can fail to be dword aligned very often.
 */

  extern inline unsigned short ip_fast_csum(const unsigned char *iph, unsigned int ihl)
  {
	unsigned int sum;

	__asm__ __volatile__("
		mov	%%g1, %1	; Remember original alignment
		skp1	%1,1		; Aligned on 16 bit boundary, get first 16 bits
		br	1f		; Aligned on 32 bit boundary, go
		ld	%0, [%1]
		ext16d	%0, %1		; Get correct 16 bits
		subi	%2, 1		; Take off for 4 bytes, pickup last 2 at end
		addi	%1, 2		; Adjust pointer to 32 bit boundary
		br	2f
		 nop
1:
		subi	%2, 1
		addi	%1, 4		; Bump ptr a long word
2:
		ld      %%g2, [%1]
1:
		add     %0, %%g2
		skps	cc_nc
		addi	%0, 1
		addi	%1, 0x4
		subi	%2, 0x1
		ld      %%g2, [%1]
		skps	cc_z
		br      1b
		 nop
		skp1	%%g1,1		; Started on 16 bit boundary, pickup last 2 bytes
		br	1f		; 32 bit boundary read to leave
		 ext16d	%%g2, %1	; Get correct 16 bits
		add     %0, %%g2
		skps	cc_nc
		addi	%0, 1
1:
		mov     %2, %0
		lsli	%2, 16
		add     %0, %2
		lsri	%0, 16
		skps	cc_nc
		addi	%0, 1
		not     %0
		"
		: "=&r" (sum), "=&r" (iph), "=&r" (ihl)
		: "1" (iph), "2" (ihl)
		: "g1", "g2");
	return sum;
  }


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


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

  extern inline unsigned int csum_partial(unsigned char * buff, unsigned int len, unsigned int sum)
  {
	__asm__ __volatile__("
		movi	%%g5, 0		; g5 = result
		cmpi	%1, 0
		skps	cc_hi
		br	9f
		mov	%%g7, %0
		skp1	%%g7, 0		; g7 = odd
		br	1f
		 nop
		subi	%1, 1		; if(odd) { result = *buff;
		ld	%%g5, [%0]	;           len--;
		ext8d	%%g5, %0	;	    postion byte (bits 0..7)
        lsli    %%g5, 8     ; little endian
		addi	%0, 1		;           buff++ }
1:
		mov	%%g6, %1
		lsri	%%g6, 1	        ; g6 = count = (len >> 1)
		cmpi	%%g6, 0		; if (count) {
		skps	cc_nz
		br	8f
		 nop
		skp1	%0, 1		; if (2 & buff) {
		br	1f
		 nop
		subi	%1, 2		;	result += *(unsigned short *) buff;
		ld	%%g1, [%0]	;	count--; 
		ext16d	%%g1, %0	;	postion half word (bits 0..15)
		subi	%%g6, 1		;	len -= 2;
		add	%%g5, %%g1	;	buff += 2; 
		addi	%0, 2		; }
1:
		lsri	%%g6, 1		; Now have number of 32 bit values
		cmpi	%%g6, 0		; if (count) {
		skps	cc_nz
		br	2f
		 nop
1:						; do {
		ld	%%g1, [%0]		; csum aligned 32bit words
		addi	%0, 4
		add	%%g5, %%g1
		skps	cc_nc
		addi	%%g5, 1			; add carry
		subi	%%g6, 1
		skps	cc_z
		br	1b			; } while (count)
		 nop
		mov	%%g2, %%g5
		lsri	%%g2, 16
		ext16s	%%g5, 0		       	; g5 & 0x0000ffff
		add	%%g5, %%g2	; }
2:
		skp1	%1, 1		; if (len & 2) {
		br	8f
		 nop
		ld	%%g1, [%0]	;	result += *(unsigned short *) buff; 
		ext16d	%%g1, %0	;	position half word (bits 0..15)
		add	%%g5, %%g1	;	buff += 2; 
		addi	%0, 2		; }
8:
		skp1	%1, 0		; if (len & 1) {
		br	1f
		 nop
		ld	%%g1, [%0]
		ext8d	%%g1, %0	;	position byte (bits 0..7)
		add	%%g5, %%g1	; }	result += (*buff); 
1:
		mov	%%g1, %%g5
		lsli	%%g1, 16
		add	%%g5, %%g1	; result = from32to16(result);
		lsri	%%g5, 16
		skps	cc_nc
		addi	%%g5, 1		; add carry
		skp1	%%g7, 0		; if(odd) {
		br	9f
		 nop
		mov	%%g2, %%g5	;	result = ((result >> 8) & 0xff) |
                pfx     %%hi(0xff)
		and	%%g2, %%lo(0xff);	((result & 0xff) << 8);
		lsli	%%g2, 8
		lsri	%%g5, 8
		or	%%g5, %%g2	; }
9:
		add	%2, %%g5	; add result and sum with carry
		skps	cc_nc
		addi	%2, 1		; add carry
		mov	%%g5,%2		; result = from32to16(result)
		lsli	%%g5,16
		add	%2,%%g5
		lsri	%2,16
		skps	cc_nc
		addi	%2, 1
	" 
	:"=&r" (buff), "=&r" (len), "=&r" (sum)
	:"0" (buff), "1" (len), "2" (sum)
	:"g1", "g2", "g5", "g6", "g7"); 


	return sum;
  }


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*
 * the same as csum_partial, but copies from fs:src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */

extern inline unsigned int csum_partial_copy(char *src, char *dst, int len, int sum)
{
 /*
  * The whole idea is to do the copy and the checksum at
  * the same time, but we do it the easy way now.
  *
  * At least csum on the source, not destination, for cache
  * reasons..
  */
 sum = csum_partial(src, len, sum);
 memcpy(dst, src, len);
 return sum;
}


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */

extern inline unsigned short ip_compute_csum(unsigned char * buff, int len)
{
 return ~from32to16(do_csum(buff,len));
}


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#define csum_partial_copy_fromuser(s, d, l, w)  \
                     csum_partial_copy((char *) (s), (d), (l), (w))


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/*
 *	Fold a partial checksum without adding pseudo headers
 */
extern inline unsigned int csum_fold(unsigned int sum)
{
	__asm__ __volatile__("
		add	%0, %1
		lsri	%0, 16
		skps	cc_nc
		addi	%0, 1
		not	%0
		"
		: "=r" (sum)
		: "r" (sum << 16), "0" (sum)); 
	return sum;
}


/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#endif /* (__NIOS_CHECKSUM_H) */


