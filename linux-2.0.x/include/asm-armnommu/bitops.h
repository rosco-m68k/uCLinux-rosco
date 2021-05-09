#ifndef __ASM_ARM_BITOPS_H
#define __ASM_ARM_BITOPS_H

/*
 * Copyright 1995, Russell King.
 * Various bits and pieces copyrights include:
 *  Linus Torvalds (test_bit).
 */

/*
 * These should be done with inline assembly.
 * All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

extern int set_bit(int nr, void * addr);
extern int clear_bit(int nr, void * addr);
extern int change_bit(int nr, void * addr);

/*
 * This routine doesn't need to be atomic.
 */
extern __inline__ int test_bit(int nr, const void * addr)
{
    return ((unsigned char *) addr)[nr >> 3] & (1U << (nr & 7));
}	

/*
 * Find-bit routines..
 */
extern int find_first_zero_bit(void * addr, unsigned size);
extern int find_next_zero_bit (void * addr, int size, int offset);

/*
 * ffz = Find First Zero in word. Undefined if no zero exists.
 */
extern __inline__ unsigned long ffz(unsigned long word)
{
	int k;

	word = ~word;
	k = 31;
	if (word & 0x0000ffff) { k -= 16; word <<= 16; }
	if (word & 0x00ff0000) { k -= 8;  word <<= 8;  }
	if (word & 0x0f000000) { k -= 4;  word <<= 4;  }
	if (word & 0x30000000) { k -= 2;  word <<= 2;  }
	if (word & 0x40000000) { k -= 1; }
        return k;
}

extern __inline__ int __ext2_set_bit(int nr,void * addr) { return set_bit(nr, addr); }
extern __inline__ int __ext2_clear_bit(int nr, void * addr) { return clear_bit(nr, addr); }
extern __inline__ int __ext2_test_bit(int nr, const void * addr) { return test_bit(nr, addr); }
extern __inline__ unsigned long __ext2_find_next_zero_bit(void *addr, unsigned long size, unsigned long offset)
{
	return find_next_zero_bit(addr, size, offset);
}

extern __inline__ unsigned long __ext2_find_first_zero_bit(void *addr, unsigned long size)
{
	return find_first_zero_bit(addr, size);
}


#endif /* _ARM_BITOPS_H */
