#ifndef _ASM_NIOS_BITOPS_H_
#define _ASM_NIOS_BITOPS_H_

#ifdef __KERNEL__
#include <asm/system.h>
#endif

/*
 * Adapted to NIOS from generic bitops.h:
 *
 * For the benefit of those who are trying to port Linux to another
 * architecture, here are some C-language equivalents.  You should
 * recode these in the native assembly language, if at all possible.
 * To guarantee atomicity, these routines call cli() and sti() to
 * disable interrupts while they operate.  (You have to provide inline
 * routines to cli() and sti().)
 *
 * Also note, these routines assume that you have 32 bit integers.
 * You will have to change this if you are trying to port Linux to the
 * Alpha architecture or to a Cray.  :-)
 * 
 * C language equivalents written by Theodore Ts'o, 9/26/92
 */

extern __inline__ int set_bit(int nr, void * a)
{
	int 	* addr = a;
	int	mask, retval;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	_disable_interrupts();
	retval = (mask & *addr) != 0;
	*addr |= mask;
	_enable_interrupts();
	return retval;
}

extern __inline__ int clear_bit(int nr, void * a)
{
	int 	* addr = a;
	int	mask, retval;

	addr += nr >> 5;
	mask = 1 << (nr & 0x1f);
	_disable_interrupts();
	retval = (mask & *addr) != 0;
	*addr &= ~mask;
	_enable_interrupts();
	return retval;
}

extern __inline__ unsigned long change_bit(unsigned long nr,  void *addr)
{
	int mask;
	unsigned long *ADDR = (unsigned long *) addr;
	unsigned long oldbit;

	ADDR += nr >> 5;
	mask = 1 << (nr & 31);
	_disable_interrupts();
	oldbit = (mask & *ADDR);
	*ADDR ^= mask;
	_enable_interrupts();
	return oldbit != 0;
}

extern __inline__ int test_bit(int nr, void * a)
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

/* Now for the ext2 filesystem bit operations and helper routines.
 *
 * Both NIOS and ext2 are little endian, so these are the same as above.
 */

#define __ext2_set_bit   set_bit
#define __ext2_clear_bit clear_bit
#define __ext2_test_bit  test_bit

#define __ext2_find_first_zero_bit find_first_zero_bit
#define __ext2_find_next_zero_bit  find_next_zero_bit

#endif /* _ASM_NIOS_BITOPS_H */
