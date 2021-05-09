/*
 *   FILE: mon960.h
 * AUTHOR: kma
 *  DESCR: mon960 calls
 */

#ifndef MON960_H
#define MON960_H

#ident "$Id: mon960.h,v 1.1 1999/12/03 06:02:35 gerg Exp $"

unsigned long mon_entry(void);
unsigned long get_prcbptr(void);
void mon960_exit(int val);

#ifdef CONFIG_PCI
typedef struct {
	int	bus;
	int	dev;
	int	fn;
} pci_dev_info;

/*
 * mon960 system calls for pci management
 */
extern int mon960_pcibios_present(void* info);
extern int mon960_pcibios_find_device(int vendor, int dev, int idx, void* loc);
extern int mon960_pcibios_find_class(int class, int idx, void* dev);

#define MON960_BIOS_DECL(op,sz,type)	\
extern int	\
mon960_pcibios_ ## op ## _config_ ##sz(unsigned short vec, 	\
					 unsigned short dev,	\
					 unsigned short func,	\
					 unsigned char off,	\
					 type val);

MON960_BIOS_DECL(read,byte,unsigned char*);
MON960_BIOS_DECL(read,word,unsigned short*);
MON960_BIOS_DECL(read,dword,unsigned int*);
MON960_BIOS_DECL(write,byte,unsigned char);
MON960_BIOS_DECL(write,word,unsigned short);
MON960_BIOS_DECL(write,dword,unsigned int);
#undef MON960_BIOS_DECL
#endif	/* CONFIG_PCI */
#endif
