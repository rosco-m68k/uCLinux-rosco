/*****************************************************************************/

/*
 *	bios32.c -- PCI access code for embedded CO-MEM Lite PCI controller.
 *
 *	(C) Copyright 1999-2000, Greg Ungerer (gerg@moreton.com.au).
 */

/*****************************************************************************/

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/bios32.h>
#include <linux/pci.h>
#include <asm/anchor.h>

/*****************************************************************************/
#ifdef CONFIG_PCI
/*****************************************************************************/

unsigned long pcibios_init(unsigned long mem_start, unsigned long mem_end)
{
	volatile unsigned long	*rp;
	int			slot;

#if 0
	printk("%s(%d): pcibios_init()\n", __FILE__, __LINE__);
#endif

	/*
	 *	Do some sort of basic check to see if the CO-MEM part
	 *	is present...
	 */
	rp = (volatile unsigned long *) COMEM_BASE;
	if ((rp[COMEM_LBUSCFG] & 0xffff) != 0x0b50) {
		printk("PCI: no PCI bus present\n");
		return(mem_start);
	}

	/*
	 *	Do a quick scan of the PCI bus and see what is here.
	 */
	for (slot = COMEM_MINIDSEL; (slot <= COMEM_MAXIDSEL); slot++) {
		rp[COMEM_DAHBASE] = COMEM_DA_CFGRD | COMEM_DA_ADDR(0x1 << slot);
		rp[COMEM_PCIBUS] = 0; /* Clear bus */
		printk("PCI: slot %d  -->  %08x\n", slot, rp[COMEM_PCIBUS]);
	}

	return(mem_start);
}

/*****************************************************************************/

unsigned long pcibios_fixup(unsigned long mem_start, unsigned long mem_end)
{
	return(mem_start);
}

/*****************************************************************************/

int pcibios_present(void)
{
	return(1);
}

/*****************************************************************************/

int pcibios_read_config_dword(unsigned char bus, unsigned char dev,
	unsigned char offset, unsigned int *val)
{
#if 0
	printk("%s(%d): pcibios_read_config_dword(bus=%x,dev=%x,offset=%x,"
		"val=%x)\n", __FILE__, __LINE__, bus, dev, offset, val);
#endif
	*val = 0xffffffff;
	return(PCIBIOS_SUCCESSFUL);
}

/*****************************************************************************/

int pcibios_read_config_word(unsigned char bus, unsigned char dev,
	unsigned char offset, unsigned short *val)
{
#if 0
	printk("%s(%d): pcibios_read_config_word(bus=%x,dev=%x,offset=%x)\n",
		__FILE__, __LINE__, bus, dev, offset);
#endif
	*val = 0xffff;
	return(PCIBIOS_SUCCESSFUL);
}

/*****************************************************************************/

int pcibios_read_config_byte(unsigned char bus, unsigned char dev,
	unsigned char offset, unsigned char *val)
{
#if 0
	printk("%s(%d): pcibios_read_config_byte(bus=%x,dev=%x,offset=%x)\n",
		__FILE__, __LINE__, bus, dev, offset);
#endif
	*val = 0xff;
	return(PCIBIOS_SUCCESSFUL);
}

/*****************************************************************************/

int pcibios_write_config_dword(unsigned char bus, unsigned char dev,
	unsigned char offset, unsigned int val)
{
#if 0
	printk("%s(%d): pcibios_write_config_dword(bus=%x,dev=%x,offset=%x)\n",
		__FILE__, __LINE__, bus, dev, offset);
#endif
	return(PCIBIOS_SUCCESSFUL);
}

/*****************************************************************************/

int pcibios_write_config_word(unsigned char bus, unsigned char dev,
	unsigned char offset, unsigned short val)
{
#if 0
	printk("%s(%d): pcibios_write_config_word(bus=%x,dev=%x,offset=%x)\n",
		__FILE__, __LINE__, bus, dev, offset);
#endif
	return(PCIBIOS_SUCCESSFUL);
}

/*****************************************************************************/

int pcibios_write_config_byte(unsigned char bus, unsigned char dev,
	unsigned char offset, unsigned char val)
{
#if 0
	printk("%s(%d): pcibios_write_config_byte(bus=%x,dev=%x,offset=%x)\n",
		__FILE__, __LINE__, bus, dev, offset);
#endif
	return(PCIBIOS_SUCCESSFUL);
}

/*****************************************************************************/

int pcibios_find_device(unsigned short vendor, unsigned short devid,
	unsigned short index, unsigned char *bus, unsigned char *dev)
{
	unsigned int	vendev, val;
	unsigned char	devnr;

#if 0
	printk("%s(%d): pcibios_find_device(vendor=%04x,devid=%04x,"
		"index=%d)\n", __FILE__, __LINE__, vendor, devid, index);
#endif

	if (vendor == 0xffff)
		return(PCIBIOS_BAD_VENDOR_ID);

	vendev = (devid << 16) | vendor;
	for (devnr = 0; (devnr < 32); devnr++) {
		pcibios_read_config_dword(0, devnr, PCI_VENDOR_ID, &val);
		if (vendev == val) {
			if (index-- == 0) {
				*bus = 0;
				*dev = devnr;
				return(PCIBIOS_SUCCESSFUL);
			}
		}
	}

	return(PCIBIOS_DEVICE_NOT_FOUND);
}

/*****************************************************************************/

int pcibios_find_class(unsigned int class, unsigned short index,
	unsigned char *bus, unsigned char *dev)
{
	unsigned int	val;
	unsigned char	devnr;

#if 0
	printk("%s(%d): pcibios_find_class(class=%04x,index=%d)\n",
		__FILE__, __LINE__, class, index);
#endif

	for (devnr = 0; (devnr < 32); devnr++) {
		pcibios_read_config_dword(0, devnr, PCI_CLASS_REVISION, &val);
		if ((val >> 8) == class) {
			if (index-- == 0) {
				*bus = 0;
				*dev = devnr;
				return(PCIBIOS_SUCCESSFUL);
			}
		}
	}

	return(PCIBIOS_DEVICE_NOT_FOUND);
}

/*****************************************************************************/
#endif	/* CONFIG_PCI */
