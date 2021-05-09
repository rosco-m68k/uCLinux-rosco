#ifndef _I960_IO_H
#define _I960_IO_H

/*
 * readX/writeX() are used to access I/O space-- the address space the PCI
 * speaks in. On the i960 architecture, some PCI addresses can be translated
 * by the atu directly, and some cannot. These functions are non-trivial.
 */
extern unsigned char readb(char* addr);
extern unsigned short readw(char* addr);
extern unsigned int readl(char* addr);

extern unsigned char writeb(unsigned char val, char* addr);
extern unsigned short writew(unsigned short val, char* addr);
extern unsigned int writel(unsigned int val, char* addr);

#define outb_p	writeb
#define outb	writeb
#define outw	writew
#define outl	writel
#define inb_p	readb
#define inb	readb
#define inw	readw
#define inl	readl


/*
 * Change virtual addresses to physical addresses and vv.
 * These are trivial on the 1:1 Linux/i386 mapping (but if we ever
 * make the kernel segment mapped at 0, we need to do translation
 * on the i386 as well)
 */
extern unsigned long mm_vtop(unsigned long addr);
extern unsigned long mm_ptov(unsigned long addr);

extern inline unsigned long virt_to_phys(volatile void * address)
{
	return (unsigned long) mm_vtop((unsigned long)address);
}

extern inline void * phys_to_virt(unsigned long address)
{
	return (void *) mm_ptov(address);
}

/*
 * IO bus memory addresses are also 1:1 with the physical address
 * XXX: umm, not really
 */
#define virt_to_bus virt_to_phys
#define bus_to_virt phys_to_virt


#endif /* _I960_IO_H */
