/*
  21Mar2001    1.1    dgt/microtronix: Altera Excalibur/Nios32 port
*/

/*
 *  linux/arch/niosnommu/kernel/setup.c
 *
 *  Copyright (C) 2001       Vic Phillips {vic@microtronix.com}
 *  Copyleft  (C) 2000       James D. Schettine {james@telos-systems.com}
 *  Copyright (C) 1999       Greg Ungerer <gerg@uClinux.org>
 *  Copyright (C) 1998-2000  D. Jeff Dionne <jeff@uClinux.org>
 *  Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
 *  Copyright (C) 1995       Hamish Macdonald
 */

/*
 * This file handles the architecture-dependent parts of system setup
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/major.h>

#include <asm/irq.h>
#include <asm/byteorder.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#include <asm/pgtable.h>
#endif

#ifdef CONFIG_NIOS_SPI
#include <linux/spi.h>
#endif

extern void register_console(void (*proc)(const char *));
extern void console_print_NIOS(const char * b);

#ifdef CONFIG_CONSOLE
extern struct consw *conswitchp;
#ifdef CONFIG_FRAMEBUFFER
extern struct consw fb_con;
#endif
#endif

unsigned long rom_length;
unsigned long memory_start;
unsigned long memory_end;

#define COMMAND_LINE_SIZE 256

#ifndef CONFIG_CMDLINE
#define CONFIG_CMDLINE	"CONSOLE=/dev/ttyS0 root=/dev/rom0 ro"
#endif

static char default_command_line[] = CONFIG_CMDLINE;
static char command_line[COMMAND_LINE_SIZE] = { 0, };
       char saved_command_line[COMMAND_LINE_SIZE];

char console_device[16];
char console_default[] = "/dev/ttyS0";

extern int cpu_idle(void);
static struct pt_regs fake_regs[] = {{ 0, cpu_idle, 0, 0, {0,}, 0}};

#ifdef CONFIG_UCCS8900
unsigned char *cs8900a_hwaddr;
unsigned char cs8900a_hwaddr_array[6];
#endif

#define CPU "NIOS"

/* setup some dummy routines */
static void dummy_waitbut(void)
{
}

/*
 *	Setup the console. There may be a console defined in
 *	the command line arguments, so we look there first. If the
 *	command line arguments don't exist then we default to the
 *	first serial port. If we are going to use a console then
 *	setup its device name and the UART for it.
 */
void setup_console(void)
{
	char *sp, *cp;
	int i;
	extern void rs_console_print(const char *b);
	extern int rs_console_setup(char *arg);

	/* Quickly check if any command line arguments for CONSOLE. */
	for (sp = NULL, i = 0; (i < sizeof(command_line)-8); i++) {
		if (command_line[i] == 0)
			break;
		if (command_line[i] == 'C') {
			if (!strncmp(&command_line[i], "CONSOLE=", 8)) {
				sp = &command_line[i + 8];
				break;
			}
		}
	}

	/* If no CONSOLE defined then default one */
	if (sp == NULL) {
		strcpy(console_device, console_default);
	}
	else {
		/* If console specified then copy it into name buffer */
		for (cp = sp, i = 0; (i < (sizeof(console_device) - 1)); i++) {
			if ((*sp == 0) || (*sp == ' ') || (*sp == ','))
				break;
			console_device[i] = *sp++;
		}
		console_device[i] = 0;
	}

#ifdef CONFIG_NIOS_SERIAL
	/* If a serial console then init it */
	if (!strncmp(console_device, "/dev/ttyS", 9) 
	 || !strncmp(console_device, "/dev/cua", 8)) {
		register_console(console_print_NIOS);
	}
#endif
}


#ifdef CONFIG_EXCALIBUR
inline void flash_command(int base, int offset, short data)
{
	volatile unsigned short * ptr=(unsigned short*) (base);

	ptr[0x555]=0xaa;
	ptr[0x2aa]=0x55;
	ptr[offset]=data;
}

inline void exit_se_flash(int base)  
{ 
	flash_command(base, 0x555, 0x90);
	*(unsigned short*)base=0;
}
#endif /* CONFIG_EXCALIBUR */

void setup_arch(char **cmdline_p,
		unsigned long * memory_start_p, unsigned long * memory_end_p)
{
	extern int _stext, _etext;
	extern int _edata, _end;
#ifdef DEBUG
	extern int _sdata, _sbss, _ebss;
#ifdef CONFIG_BLK_DEV_BLKMEM
	extern int *romarray;
#endif
#endif
	unsigned char *psrc=(unsigned char *)((((int)na_flash_kernel) + ((int)na_flash_kernel_end))/2);
	int i=0;

	memory_start = &_end;
	memory_end = (int) nasys_program_mem_end - 4096; /* <- stack area */

	/* copy the command line from booting paramter region */
	flash_command((int)psrc, 0x555, 0x88);
	while ((*psrc!=0xFF) && (i<sizeof(command_line))) {
		command_line[i++]=*psrc++;
	}
	command_line[i]=0;
	exit_se_flash( ((((int)na_flash_kernel) + ((int)na_flash_kernel_end))/2) );
	if (command_line[0]==0)
		memcpy(command_line, default_command_line, sizeof(default_command_line));

	setup_console();
	printk("\x0F\r\n\nuClinux/NIOS\n");
	printk("Altera NIOS Excalibur support (C) 2001 Microtronix Datacom Ltd.\n");
#ifdef DEBUG
	printk("KERNEL -> TEXT=0x%08x-0x%08x DATA=0x%08x-0x%08x "
		"BSS=0x%08x-0x%08x\n", (int) &_stext, (int) &_etext,
		(int) &_sdata, (int) &_edata,
		(int) &_sbss, (int) &_ebss);
	printk("KERNEL -> ROMFS=0x%06x-0x%06x MEM=0x%06x-0x%06x "
		"STACK=0x%06x-0x%06x\n",
#ifdef CONFIG_BLK_DEV_BLKMEM
	       (int) romarray, ((int) romarray) + ntohl(romarray[2]),
#else
	       (int) &_ebss, (int) memory_start,
#endif
		(int) memory_start, (int) memory_end,
		(int) memory_end, (int) nasys_program_mem_end);
#endif
	init_task.mm->start_code = (unsigned long) &_stext;
	init_task.mm->end_code = (unsigned long) &_etext;
	init_task.mm->end_data = (unsigned long) &_edata;
	init_task.mm->brk = (unsigned long) &_end;
	init_task.tss.kregs = &fake_regs;

	ROOT_DEV = MKDEV(BLKMEM_MAJOR,0);

	/* Keep a copy of command line */
	*cmdline_p = &command_line[0];

	memcpy(saved_command_line, command_line, sizeof(saved_command_line));
	saved_command_line[sizeof(saved_command_line)-1] = 0;

#ifdef DEBUG
	if (strlen(*cmdline_p)) 
		printk("Command line: '%s'\n", *cmdline_p);
	else
		printk("No Command line passed\n");
#endif
	*memory_start_p = memory_start;
	*memory_end_p = memory_end;


#ifdef CONFIG_UCCS8900
	/* now read the hwaddr of the ethernet --wentao*/
	flash_command(((int)na_flash_kernel), 0x555, 0x88);
	memcpy(cs8900a_hwaddr_array,na_flash_kernel,6);
	exit_se_flash((int)na_flash_kernel);
	/* now do the checking, make sure we got a valid addr */
	if (cs8900a_hwaddr_array[0] & (unsigned char)1)
	{
		printk("Invalid ethernet hardware addr, fixed\n");
		cs8900a_hwaddr_array[0] ^= (unsigned char)1;
	}
	cs8900a_hwaddr=cs8900a_hwaddr_array;
#ifdef DEBUG
	printk("Setup the hardware addr for ethernet\n\t %02x %02x %02x %02x %02x %02x\n",
		cs8900a_hwaddr[0],cs8900a_hwaddr[1],
		cs8900a_hwaddr[2],cs8900a_hwaddr[3],
		cs8900a_hwaddr[4],cs8900a_hwaddr[5]);
#endif
#endif /* CONFIG_UCCS8900 */

#ifdef CONFIG_NIOS_SPI
	if ( register_NIOS_SPI() )
		printk( "*** Cannot initialize SPI device.\n" );
#endif

/* console */
#ifdef CONFIG_CONSOLE
#ifdef CONFIG_FRAMEBUFFER
	conswitchp = &fb_con;
#else
	conswitchp = 0;
#endif
#endif

#ifdef DEBUG
	printk("Done setup_arch\n");
#endif

}
 
int get_cpuinfo(char * buffer)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq, clockfactor;

    cpu = CPU;
    mmu = "none";
    fpu = "none";
    clockfactor = 1;

    clockfreq = loops_per_sec*clockfactor;

    return(sprintf(buffer, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "Clocking:\t%lu.%1luMHz\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   clockfreq/100000,(clockfreq/100000)%10,
		   loops_per_sec/500000,(loops_per_sec/5000)%100,
		   loops_per_sec));

}


static int bcd2char( int x )
{
        if ( (x & 0xF) > 0x90 || (x & 0x0F) > 0x09 )
                return 99;

        return (((x & 0xF0) >> 4) * 10) + (x & 0x0F);
}



void arch_gettod(int *year, int *month, int *date, int *hour, int *min, int *sec)
{
#ifdef CONFIG_NIOS_SPI
        /********************************************************************/
  	/* Read the CMOS clock on the Microtronix Datacom O/S Support card. */
  	/* Use the SPI driver code, but circumvent the file system by using */
        /* its internal functions.                                          */
        /********************************************************************/
        int  hr;

	struct                               /*********************************/
        {                                    /* The SPI payload. Warning: the */
	      unsigned short register_addr;  /* sizeof() operator will return */
	      unsigned char  value;          /* a length of 4 instead of 3!   */
        } spi_data;                          /*********************************/


	if ( spi_open( NULL, NULL ) )
	{
	    printk( "Cannot open SPI driver to read system CMOS clock.\n" );
	    *year = *month = *date = *hour = *min = *sec = 0;
	    return;
	}

	spi_lseek( NULL, NULL, clockCS, 0 /* == SEEK_SET */ );

	spi_data.register_addr = clock_write_control;
	spi_data.value         = 0x40; // Write protect
	spi_write( NULL, NULL, (const char *)&spi_data, 3 );

	spi_data.register_addr = clock_read_sec;
	spi_data.value         = 0;
	spi_read( NULL, NULL, (const char *)&spi_data, 3 );
	*sec = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_min;
	spi_data.value         = 0;
	spi_read( NULL, NULL, (const char *)&spi_data, 3 );
	*min = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_hour;
	spi_data.value         = 0;
	spi_read( NULL, NULL, (const char *)&spi_data, 3 );
	hr = (int)bcd2char( spi_data.value );
	if ( hr & 0x40 )  // Check 24-hr bit
 	    hr = (hr & 0x3F) + 12;     // Convert to 24-hr

	*hour = hr;



	spi_data.register_addr = clock_read_date;
	spi_data.value         = 0;
	spi_read( NULL, NULL, (const char *)&spi_data, 3 );
	*date = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_month;
	spi_data.value         = 0;
	spi_read( NULL, NULL, (const char *)&spi_data, 3 );
	*month = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_year;
	spi_data.value         = 0;
	spi_read( NULL, NULL, (const char *)&spi_data, 3 );
	*year = (int)bcd2char( spi_data.value );


	spi_release( NULL, NULL );
#else
	*year = *month = *date = *hour = *min = *sec = 0;

#endif
}

