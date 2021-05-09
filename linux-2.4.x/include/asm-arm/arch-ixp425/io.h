/*
 * linux/include/asm-arm/arch-ixp425/io.h
 *
 * Author: Deepak Saxena <dsaxena@mvista.com>
 *
 * Copyright (C) 2001  MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

/*
 * Needed for VMALLOC_STARt
 */
#include <linux/mm.h>
#include <asm/arch/vmalloc.h>

#define IO_SPACE_LIMIT 0xffffffff

#define __mem_isa(a)		((unsigned long)(a))

/*
 * IXP425 does not have a transparent cpu -> PCI I/O translation
 * window.  Instead, it has a set of registers that must be tweaked
 * with the proper byte lanes, command types, and address for the
 * transaction.  This means that we need to override the default
 * I/O functions.
 */

extern void ixp425_outb(u8 v, u32 p);
extern void ixp425_outw(u16 v, u32 p);
extern void ixp425_outl(u32 v, u32 p);

extern u8 ixp425_inb(u32 p);
extern u16 ixp425_inw(u32 p);
extern u32 ixp425_inl(u32 p);

extern void ixp425_outsb(u32 p, u8 *addr, u32 count);
extern void ixp425_outsw(u32 p, u16 *addr, u32 count);
extern void ixp425_outsl(u32 p, u32 *addr, u32 count);

extern void ixp425_insb(u32 p, u8 *addr, u32 count);
extern void ixp425_insw(u32 p, u16 *addr, u32 count);
extern void ixp425_insl(u32 p, u32 *addr, u32 count);

#define	outb(v, p)	ixp425_outb(v, p)
#define	outw(v, p)	ixp425_outw(v, p)
#define	outl(v, p)	ixp425_outl(v, p)

#define	inb(p)		ixp425_inb(p)
#define	inw(p)		ixp425_inw(p)
#define	inl(p)		ixp425_inl(p)

#define	outsb(p, a, c)	ixp425_outsb(p, a, c)
#define	outsw(p, a, c)	ixp425_outsw(p, a, c)
#define	outsl(p, a, c)	ixp425_outsl(p, a, c)

#define	insb(p, a, c)	ixp425_insb(p, a, c)
#define	insw(p, a, c)	ixp425_insw(p, a, c)
#define	insl(p, a, c)	ixp425_insl(p, a, c)



/*
 * Generic virtual read/write
 */
#define __arch_getb(a)		(*(volatile unsigned char *)(a))
#define __arch_getl(a)		(*(volatile unsigned int  *)(a))

extern __inline__ unsigned int __arch_getw(unsigned long a)
{
	unsigned int value;
	__asm__ __volatile__("ldr%?h	%0, [%1, #0]	@ getw"
		: "=&r" (value)
		: "r" (a));
	return value;
}


#define __arch_putb(v,a)	(*(volatile unsigned char *)(a) = (v))
#define __arch_putl(v,a)	(*(volatile unsigned int  *)(a) = (v))

extern __inline__ void __arch_putw(unsigned int value, unsigned long a)
{
	__asm__ __volatile__("str%?h	%0, [%1, #0]	@ putw"
		: : "r" (value), "r" (a));
}


/*
 * IXP425 also does not have a transparent PCI MEM translation
 * window.  For this reason, we have to use the NP registers for
 * all PCI mem acceses and have to implement custom 
 * ioremap/unmap/etc functions. :( We need to check and make
 * sure that the address being accessed is a PCI address and
 * if not, fall back to the normal code.
 */

static inline void 
__writeb(u8 v, u32 p)
{
	u32 n, byte_enables, data;

	if(p > VMALLOC_START) {
		__raw_writeb(v, p);
		return;
	}

	n = p % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP425_PCI_NP_CBE_BESL;
	data = v << (8*n);
	ixp425_pci_write(p, byte_enables | NP_CMD_MEMWRITE, data);
}

static inline void 
__writew(u16 v, u32 p)
{
	u32 n, byte_enables, data;

	if(p > VMALLOC_START) {
		__raw_writew(v, p);
		return;
	}

	n = p % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP425_PCI_NP_CBE_BESL;
	data = v << (8*n);
	ixp425_pci_write(p, byte_enables | NP_CMD_MEMWRITE, data);
}

static inline void 
__writel(u32 v, u32 p)
{
	if(p > VMALLOC_START) {
		__raw_writel(v, p);
		return;
	}

	ixp425_pci_write(p, NP_CMD_MEMWRITE, v);
}

static inline unsigned char 
__readb(u32 p)
{
	u32 n, byte_enables, data;

	if(p > VMALLOC_START)
		return __raw_readb(p);

	n = p % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP425_PCI_NP_CBE_BESL;
	if (ixp425_pci_read(p, byte_enables | NP_CMD_MEMREAD, &data))
		return 0xff;

	return data >> (8*n);
}

static inline unsigned short 
__readw(u32 p)
{
	u32 n, byte_enables, data;

	if(p > VMALLOC_START)
		return __raw_readw(p);

	n = p % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP425_PCI_NP_CBE_BESL;
	if (ixp425_pci_read(p, byte_enables | NP_CMD_MEMREAD, &data))
		return 0xffff;

	return data>>(8*n);
}

static inline unsigned long 
__readl(u32 p)
{
	u32 data;

	if(p > VMALLOC_START)
		return __raw_readl(p);

	if (ixp425_pci_read(p, NP_CMD_MEMREAD, &data))
		return 0xffffffff;

	return data;
}

#define	readb(addr)	__readb(addr)
#define	readw(addr)	__readw(addr)
#define	readl(addr)	__readl(addr)
#define	writeb(v, addr)	__writeb(v, addr)
#define	writew(v, addr)	__writew(v, addr)
#define	writel(v, addr)	__writel(v, addr)

/*
 * We can use the built-in functions b/c they end up calling
 * our versions of writeb/readb
 */
#define memset_io(c,v,l)		_memset_io((c),(v),(l))
#define memcpy_fromio(a,c,l)		_memcpy_fromio((a),(c),(l))
#define memcpy_toio(c,a,l)		_memcpy_toio((c),(a),(l))

#define eth_io_copy_and_sum(s,c,l,b) \
				eth_copy_and_sum((s),(c),(l),(b))

static inline void *
__arch_ioremap(unsigned long phys_addr, size_t size, unsigned long flags)
{
	extern void * __ioremap(unsigned long, size_t, unsigned long);

	if((phys_addr < 0x48000000) || (phys_addr > 0x4bffffff))
		return __ioremap(phys_addr, size, flags);

	return (void *)phys_addr;
}

static inline void
__arch_iounmap(void *addr)
{
	extern void __iounmap(void *);

	if(((u32)addr < 0x48000000) || ((u32)addr > 0x4bffffff))
		__iounmap(addr);
}



#endif	//  __ASM_ARM_ARCH_IO_H

