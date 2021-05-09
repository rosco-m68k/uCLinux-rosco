/*
 * Copyright (C) 1999	Keith Adams	<kma@cse.ogi.edu>
 * 			Oregon Graduate Institute
 * 
 * Bios and PCI stuff.
 */

#include <linux/pci.h>
#include <linux/bios32.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <asm/system.h>

unsigned long bios32_init(unsigned long memory_start, unsigned long memory_end)
{
	return memory_start;
}

unsigned long pcibios_init(unsigned long memory_start, unsigned long memory_end)
{
	return memory_start;
}

unsigned long pcibios_fixup(unsigned long memory_start, unsigned long memory_end)
{
	return memory_start;
}

#ifdef CONFIG_MON960

#include <asm/mon960.h>

/*
 * mon960 system calls for pci management
 */
extern int mon960_pcibios_present(void* info);
extern int mon960_pcibios_find_device(int vendor, int dev, int idx, void* loc);
extern int mon960_pcibios_find_class(int class, int idx, void* dev); 

int pcibios_present(void)
{
	return 1;
}

int pcibios_find_device(unsigned short vendor, unsigned short dev, 
			unsigned short index, unsigned char* bus, 
			unsigned char* dev_fn)
{
	pci_dev_info	loc;
	int		status;

	if ((status=mon960_pcibios_find_device(vendor, dev, index, &loc)))
		return -1;

	printk("PCI device found: %x, %x, %x\n", loc.bus, loc.dev, loc.fn);
	*bus = loc.bus;
	*dev_fn = loc.dev;
	return 0;
}

int pcibios_find_class(unsigned int class, unsigned short idx,
		       unsigned char* bus, unsigned char* dev_fn)
{
	pci_dev_info	loc;
	int		status;
	
	if ((status=mon960_pcibios_find_class(class, idx, &loc)))
		return -1;

	*bus = loc.bus;
	*dev_fn = loc.dev;
	return 0;
}

#define CONFIG_RDWR(op,sz,type) \
int pcibios_ ## op ## _config_ ## sz(unsigned char bus, unsigned char fn, \
				unsigned char offset, type val)	\
{	\
	return mon960_pcibios_ ## op ## _config_ ## sz(bus, fn, 0, offset, val); \
}

CONFIG_RDWR(read,byte,unsigned char*);
CONFIG_RDWR(read,word,unsigned short*);
CONFIG_RDWR(read,dword,unsigned int*);
CONFIG_RDWR(write,byte,unsigned char);
CONFIG_RDWR(write,word,unsigned short);
CONFIG_RDWR(write,dword,unsigned int);

#endif	/* CONFIG_MON960 */

/*
 * Dealing with the ATU. We just do all accesses translated; checking 
 * whether we can directly use the address might not be any faster.
 */

/* external address = (local address & 0x03ffffff) | ATU_OMWVR */
#define OIOW_BASE	0x90000000UL	/* outbound I/O space window */
#define ATU_OIOWVR	((unsigned long*) 0x125c)	/* I/O window val */

/* Set up ATU, return local address to use */
static unsigned long program_atu(unsigned long pciaddr)
{
	unsigned long retval;
	unsigned long pci_hibits = pciaddr & 0xfc000000;
	*ATU_OIOWVR = pci_hibits;
	retval = (pciaddr & 0x03ffffff) | OIOW_BASE;
	return retval;
}

/* Note that we synchronize ATU access with the ipl.
 * 
 * XXX: diddling the ipl every time seems to be a performance problem. Is
 * there any way to use the i960 hardware in just one memory op, instead of
 * two?
 */

#define READ_OP(type,name)	\
type name(char* addr)	\
{	\
	volatile type *local_addr;	\
	type retval;	\
	unsigned long oldatu = *ATU_OIOWVR;	\
	local_addr = (volatile type*)	\
		program_atu((unsigned long)addr);	\
	retval = *local_addr;	\
	*ATU_OIOWVR = oldatu;	\
	return retval;	\
}
READ_OP(unsigned char,readb)
READ_OP(unsigned short,readw)
READ_OP(unsigned int,readl)
#undef READ_OP

#define WRITE_OP(type,name)	\
type name(type val, char* addr)	\
{	\
	volatile type *local_addr;	\
	type retval;	\
	unsigned long oldatu = *ATU_OIOWVR;	\
	local_addr = (volatile type*)	\
		program_atu((unsigned long)addr);	\
	retval = *local_addr = val;	\
	*ATU_OIOWVR = oldatu;	\
	return retval;	\
}
WRITE_OP(unsigned char,writeb)
WRITE_OP(unsigned short,writew)
WRITE_OP(unsigned int,writel)
#undef WRITE_OP
