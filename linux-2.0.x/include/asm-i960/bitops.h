#ifndef _ASM_GENERIC_BITOPS_H_
#define _ASM_GENERIC_BITOPS_H_

#include <asm/system.h>

/*
 * N.B.: we shouldn't assume addresses are long*'s; nr can be > 31.
 */
static __inline__ long set_bit(long nr, void *a)
{
	long*	addr = a;
	int	mask, oldval;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	__asm__ __volatile__
		("atmod	%3, %2, %2"
		 : "=m" (*addr), "=r" (oldval)
		 : "1" (mask), "r" (addr), "m"(*addr));
	return (oldval & mask) != 0;
}

static __inline__ long clear_bit(long nr, void *a)
{
	long*	addr = a;
	int	mask, oldval;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	__asm__ __volatile__
		("atmod	%4, %3, %2"
		 : "=m" (*addr), "=r"(oldval)
		 : "1" (0), "r" (mask), "r"(addr), "m"(*addr));
	return (oldval & mask) != 0;
}


static __inline__ long change_bit(long nr, void *a)
{
	long*	addr = a;
	long	flags;
	int	bitpos, retval;

	addr += nr >> 5;
	bitpos = (nr & 0x1f);
	save_flags(flags);
	cli();

#if 1
	retval = (*addr & (1 << bitpos));
	*addr ^= (1 << bitpos);
#else
	__asm__
		("ld		(%2), r3\n\t"
		 "notbit	%1, r3, r4\n\t"
		 "st		r4, (%2)\n\t"
		 "ldconst	1, r4\n\t"
		 "shlo		%1, r4, r4\n\t"
		 "and		r4, r3, %0\n\t"
		 : "=&r" (retval)
		 : "r" (bitpos), "r" (addr)
		 : "r3", "r4", "memory");
#endif
	restore_flags(flags);
	return retval != 0;
}

static __inline__ long test_bit(long nr, void *a)
{
	int 	* addr = a;
	int	mask;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	return ((mask & *addr) != 0);
}

/* The easy/cheese version for now. */
extern __inline__ unsigned long ffz(unsigned long word)
{
	unsigned long result = 0;

	while(word & 1) {
		result++;
		word >>= 1;
	}
	return result;
}

/* find_next_zero_bit() finds the first zero bit in a bit string of length
 * 'size' bits, starting the search at bit 'offset'. This is largely based
 * on Linus's ALPHA routines, which are pretty portable BTW.
 */

extern __inline__ unsigned long find_next_zero_bit(void *addr, unsigned long size, unsigned long offset)
{
	unsigned long *p = ((unsigned long *) addr) + (offset >> 5);
	unsigned long result = offset & ~31UL;
	unsigned long tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if (offset) {
		tmp = *(p++);
		tmp |= ~0UL >> (32-offset);
		if (size < 32)
			goto found_first;
		if (~tmp)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while (size & ~31UL) {
		if (~(tmp = *(p++)))
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	tmp |= ~0UL >> size;
found_middle:
	return result + ffz(tmp);
}

/* Linus sez that gcc can optimize the following correctly, we'll see if this
 * holds on the Sparc as it does for the ALPHA.
 */

#define find_first_zero_bit(addr, size) \
        find_next_zero_bit((addr), (size), 0)

/* Jan 14, 2002 Fred Finster fredf@aol.com included code  from asm-m68knommu/bitops.h  file  below */
/* Now for the ext2 filesystem bit operations and helper routines. */

extern __inline__ int ext2_set_bit(int nr,void * addr)
{
	int		mask, retval, flags;
	unsigned char	*ADDR = (unsigned char *) addr;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	save_flags(flags); cli();
	retval = (mask & *ADDR) != 0;
	*ADDR |= mask;
	restore_flags(flags);
	return retval;
}

extern __inline__ int ext2_clear_bit(int nr, void * addr)
{
	int		mask, retval, flags;
	unsigned char	*ADDR = (unsigned char *) addr;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	save_flags(flags); cli();
	retval = (mask & *ADDR) != 0;
	*ADDR &= ~mask;
	restore_flags(flags);
	return retval;
}

extern __inline__ int ext2_test_bit(int nr, const void * addr)
{
	int			mask;
	const unsigned char	*ADDR = (const unsigned char *) addr;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	return ((mask & *ADDR) != 0);
}

#define ext2_find_first_zero_bit(addr, size) \
        ext2_find_next_zero_bit((addr), (size), 0)

extern __inline__ unsigned long ext2_find_next_zero_bit(void *addr, unsigned long size, unsigned long offset)
{
	unsigned long *p = ((unsigned long *) addr) + (offset >> 5);
	unsigned long result = offset & ~31UL;
	unsigned long tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if(offset) {
		tmp = *(p++);
		tmp |= ~0UL << (32-offset);
		if(size < 32)
			goto found_first;
		if(~tmp)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while(size & ~31UL) {
		if(~(tmp = *(p++)))
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if(!size)
		return result;
	tmp = *p;

found_first:
	tmp |= ~0UL << size;
found_middle:
	tmp = ((tmp>>24) | ((tmp>>8)&0xff00) | ((tmp<<8)&0xff0000) | (tmp<<24));
	return result + ffz(tmp);
}

#define __ext2_set_bit ext2_set_bit
#define __ext2_clear_bit ext2_clear_bit

extern __inline__ int __ext2_test_bit(int nr, __const__ void * addr)
{
	int			mask;
	__const__ unsigned char	*ADDR = (__const__ unsigned char *) addr;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	return ((mask & *ADDR) != 0);
}

extern __inline__ unsigned short __swab16(unsigned short value)
{
        return ((value >> 8) | (value << 8));
}

extern __inline__ unsigned long __swab32(unsigned long value)
{
        return ((value >> 24) | ((value >> 8) & 0xff00) |
               ((value << 8) & 0xff0000) | (value << 24));
}

#define __ext2_find_first_zero_bit(addr, size) \
        __ext2_find_next_zero_bit((addr), (size), 0)

extern __inline__ unsigned long __ext2_find_next_zero_bit(void *addr, unsigned long size, unsigned long offset)
{
	unsigned long *p = ((unsigned long *) addr) + (offset >> 5);
	unsigned long result = offset & ~31UL;
	unsigned long tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if(offset) {
		tmp = *(p++);
		tmp |= __swab32(~0UL >> (32-offset));
		if(size < 32)
			goto found_first;
		if(~tmp)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while(size & ~31UL) {
		if(~(tmp = *(p++)))
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if(!size)
		return result;
	tmp = *p;

found_first:
	return result + ffz(__swab32(tmp) | (~0UL << size));
found_middle:
	return result + ffz(__swab32(tmp));
}



#endif /* _ASM_I960_BITOPS_H */
