
#include <asm/leon.h>
#include "hex.h"

static void puts(unsigned char *s)
{
	struct lregs *regs = (struct lregs *) 0x80000000;

	while(*s) {
		while (!(regs->uartstatus1 & 0x4));
		regs->uartdata1 = *s++;
	}
	while (!(regs->uartstatus1 & 0x4));
	regs->uartdata1 = '\r';
	while (!(regs->uartstatus1 & 0x4));
	regs->uartdata1 = '\n';

}

static unsigned long testvar = 0xdeadbeef;

int main()
{
        extern unsigned long __data_rom_start;
        extern unsigned long __data_start;
        extern unsigned long __data_end;
        extern unsigned long __bss_start;
        extern unsigned long __bss_end;
 
        unsigned long *src;
        unsigned long *dest;
	unsigned long tmp;
 
	struct lregs *regs = (struct lregs *) 0x80000000;
#if 0     
        puts("Testing RAM\n");

        puts("Write...\n");
        for (dest = (unsigned long *)0x40000000; dest < (unsigned long *)0x40080000; dest++) {
                *dest = (unsigned long)0x5a5a5a5a ^ (unsigned long)dest;
        }
 
        puts("Read...\n");
        for (dest = (unsigned long *)0x40000000; dest < (unsigned long *)0x40080000; dest++) {
                tmp = (unsigned long)0x5a5a5a5a ^ (unsigned long)dest;
                if (*dest != tmp) {
                        puts("Failed.");
			outhex32((unsigned long)dest);
			puts("is");
			outhex32(*dest);
			puts("wrote");
			outhex32(tmp);
                        while(1);
                }
        }
#endif 
        puts("512k RAM\n");
        if (testvar == 0xdeadbeef) puts("Found my key\n");
        else puts("My keys are missing!\n");
#if 0 
        src = &__data_rom_start;
        dest = &__data_start;
        while (dest < &__data_end) *(dest++) = *(src++);
#endif
 
        dest = &__bss_start;
        while (dest < &__bss_end) *(dest++) = 0;
 
        puts("Moved .data\n");

        if (testvar == 0xdeadbeef) puts("Found my key\n");
        else puts("My keys are missing!\n");
 
        testvar = 0;

	start_kernel();
}

