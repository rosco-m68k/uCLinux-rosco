/*
 *   FILE: mon960_calls.c
 * AUTHOR: kma
 *  DESCR: system calls to mon960
 */

#ident "$Id: mon960_calls.c,v 1.1 1999/12/03 06:02:33 gerg Exp $"

#include <linux/config.h>
#include <asm/mon960.h>

#define SYSCALL(number)	\
asm("calls	%0; ret"	\
	: : "lI"(number): "g0");

unsigned long mon_entry(void)
{
	SYSCALL(254);
}

unsigned long get_prcbptr(void)
{
	SYSCALL(245);
}

void mon960_exit(int val)
{
	SYSCALL(257);
}

#ifdef CONFIG_PCI

int mon960_pcibios_present(void* info)
{ SYSCALL(100); }
int mon960_pcibios_find_device(int vnd, int dev, int idx, void* loc)
{ SYSCALL(101); }
int mon960_pcibios_find_class(int class, int idx, void* dev)
{ SYSCALL(102); }

#define BIOS_OP(op,sz,type,nr)	\
int	\
mon960_pcibios_ ## op ## _config_ ##sz(unsigned short vec,	\
					 unsigned short dev,	\
					 unsigned short func,	\
					 unsigned char off,	\
					 type val)	\
{ SYSCALL(nr); }

BIOS_OP(read,byte,unsigned char*,104)
BIOS_OP(read,word,unsigned short*,105)
BIOS_OP(read,dword,unsigned int*,106)
BIOS_OP(write,byte,unsigned char,107)
BIOS_OP(write,word,unsigned short,108)
BIOS_OP(write,dword,unsigned int,109)

#endif
