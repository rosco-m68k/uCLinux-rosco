#ifndef _H8300_IO_H
#define _H8300_IO_H

/*
 * readX/writeX() are used to access memory mapped devices. On some
 * architectures the memory mapped IO stuff needs to be accessed
 * differently. On the m68k architecture, we just read/write the
 * memory location directly.
 */
/* ++roman: The assignments to temp. vars avoid that gcc sometimes generates
 * two accesses to memory, which may be undesireable for some devices.
 */
#define readb(addr) \
    ({ unsigned char __v = (*(volatile unsigned char *) (addr)); __v; })
#define readw(addr) \
    ({ unsigned short __v = (*(volatile unsigned short *) (addr)); __v; })
#define readl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#define writeb(b,addr) ((*(volatile unsigned char *) (addr)) = (b))
#define writew(b,addr) ((*(volatile unsigned short *) (addr)) = (b))
#define writel(b,addr) ((*(volatile unsigned int *) (addr)) = (b))

#if 0

/* There is no difference between I/O and memory on 68k, these are the same */
#define inb(addr) \
    ({ unsigned char __v = (*(volatile unsigned char *) (addr)); printk("inb(%x)=%02x\n", (addr), __v); __v; })
#define inw(addr) \
    ({ unsigned short __v = (*(volatile unsigned short *) (addr)); printk("inw(%x)=%04x\n", (addr), __v); __v; })
#define inl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); printk("inl(%x)=%08x\n", (addr), __v); __v; })

#define outb(b,addr) { ((*(volatile unsigned char *) (addr)) = (b)) ; printk("outb(%x)=%02x\n", (addr), (b)); }
#define outw(b,addr) { ((*(volatile unsigned short *) (addr)) = (b)) ; printk("outw(%x)=%04x\n", (addr), (b)); }
#define outl(b,addr) { ((*(volatile unsigned int *) (addr)) = (b)) ; printk("outl(%x)=%08x\n", (addr), (b)); }

#else

/* There is no difference between I/O and memory on 68k, these are the same */
#define inb(addr) \
    ({ unsigned char __v = (*(volatile unsigned char *) (addr)); __v; })
#define inw(addr) \
    ({ unsigned short __v = (*(volatile unsigned short *) (addr)); __v; })
#define inl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#define outb(b,addr) ((*(volatile unsigned char *) (addr)) = (b))
#define outw(b,addr) ((*(volatile unsigned short *) (addr)) = (b))
#define outl(b,addr) ((*(volatile unsigned int *) (addr)) = (b))

#endif

#define	inb_p	inb
#define	inw_p	inw
#define	outb_p	outb
#define	outw_p	outw


static inline void outsb(void *addr, void *buf, int len)
{
	volatile unsigned char *ap = (volatile unsigned char *) addr;
	unsigned char *bp = (unsigned char *) buf;
	while (len--)
		*ap = *bp++;
}

static inline void outsw(void *addr, void *buf, int len)
{
	volatile unsigned short *ap = (volatile unsigned short *) addr;
	unsigned short *bp = (unsigned short *) buf;
	while (len--)
		*ap = *bp++;
}

static inline void outsl(void *addr, void *buf, int len)
{
	volatile unsigned int *ap = (volatile unsigned int *) addr;
	unsigned int *bp = (unsigned int *) buf;
	while (len--)
		*ap = *bp++;
}

static inline void insb(void *addr, void *buf, int len)
{
	volatile unsigned char *ap = (volatile unsigned char *) addr;
	unsigned char *bp = (unsigned char *) buf;
	while (len--)
		*bp++ = *ap;
}

static inline void insw(void *addr, void *buf, int len)
{
	volatile unsigned short *ap = (volatile unsigned short *) addr;
	unsigned short *bp = (unsigned short *) buf;
	while (len--)
		*bp++ = *ap;
}

static inline void insl(void *addr, void *buf, int len)
{
	volatile unsigned int *ap = (volatile unsigned int *) addr;
	unsigned int *bp = (unsigned int *) buf;
	while (len--)
		*bp++ = *ap;
}


static inline unsigned char get_user_byte_io(const char * addr)
{
	register unsigned char _v;

	__asm__ __volatile__ ("moveb %1,%0":"=dm" (_v):"m" (*addr));
	return _v;
}

static inline void put_user_byte_io(char val,char *addr)
{
	__asm__ __volatile__ ("moveb %0,%1"
			      : /* no outputs */
			      :"idm" (val),"m" (*addr)
			      : "memory");
}

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
 */
#define virt_to_bus virt_to_phys
#define bus_to_virt phys_to_virt


#endif /* _H8300_IO_H */
