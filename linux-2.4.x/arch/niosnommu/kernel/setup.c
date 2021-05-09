/*
  21Mar2001    1.1    dgt/microtronix: Altera Excalibur/Nios32 port
*/

/*
 *  linux/arch/niosnommu/kernel/setup.c
 *
 *  Copyright (C) 2001       Vic Phillips {vic@microtronix.com}
 *  Copyleft  (C) 2000       James D. Schettine {james@telos-systems.com}
 *  Copyright (C) 1999       Greg Ungerer (gerg@moreton.com.au)
 *  Copyright (C) 1998,2000  D. Jeff Dionne <jeff@uClinux.org>
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
#include <linux/bootmem.h>
#include <linux/seq_file.h>

#include <asm/irq.h>
#include <asm/byteorder.h>
#include <asm/niosconf.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <linux/blk.h>
#include <asm/pgtable.h>
#endif

#ifdef CONFIG_NIOS_SPI
#include <asm/spi.h>
extern ssize_t spi_write(struct file *filp, const char *buf, size_t count, loff_t *ppos);
extern ssize_t spi_read (struct file *filp, char *buf, size_t count, loff_t *ppos);
extern loff_t spi_lseek (struct file *filp, loff_t offset, int origin);
extern int spi_open     (struct inode *inode, struct file *filp);
extern int spi_release  (struct inode *inode, struct file *filp);
#endif

#ifdef CONFIG_CONSOLE
extern struct consw *conswitchp;
#endif

unsigned long rom_length;
unsigned long memory_start;
unsigned long memory_end;

struct task_struct *_current_task;

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
static struct pt_regs fake_regs = { 0, (unsigned long)cpu_idle, 0, 0, {0,}};

#define CPU "NIOS"

#if defined (CONFIG_CS89x0) || (CONFIG_SMC91111)
unsigned char *cs8900a_hwaddr;
unsigned char cs8900a_hwaddr_array[6];
#endif

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

void setup_arch(char **cmdline_p)
{
	int bootmap_size;
	extern int _stext, _etext;
	extern int _edata, _end;
#ifdef DEBUG
	extern int _sdata, _sbss, _ebss;
#ifdef CONFIG_BLK_DEV_BLKMEM
	extern int *romarray;
#endif
#endif
	unsigned char *psrc=(unsigned char *)((NIOS_FLASH_START + NIOS_FLASH_END)>>1);
	int i=0;

	memory_start = (unsigned long)&_end;
	memory_end = (int) nasys_program_mem_end;

	/* copy the command line from booting paramter region */
	flash_command((int)psrc, 0x555, 0x88);
	while ((*psrc!=0xFF) && (i<sizeof(command_line))) {
		command_line[i++]=*psrc++;
	}
	command_line[i]=0;
	exit_se_flash(((NIOS_FLASH_START + NIOS_FLASH_END)>>1) );
	if (command_line[0]==0)
		memcpy(command_line, default_command_line, sizeof(default_command_line));

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

	init_mm.start_code = (unsigned long) &_stext;
	init_mm.end_code = (unsigned long) &_etext;
	init_mm.end_data = (unsigned long) &_edata;
	init_mm.brk = (unsigned long) 0; 
//vic FIXME - this necessary ?
	init_task.thread.kregs = &fake_regs;

#if 0
	ROOT_DEV = MKDEV(BLKMEM_MAJOR,0);
#endif

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


#if defined (CONFIG_CS89x0) || (CONFIG_SMC91111)
	/* now read the hwaddr of the ethernet --wentao*/
	flash_command(NIOS_FLASH_START, 0x555, 0x88);
	memcpy(cs8900a_hwaddr_array,(void*)NIOS_FLASH_START,6);
	exit_se_flash(NIOS_FLASH_START);
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
#endif


	/*
	 * give all the memory to the bootmap allocator,  tell it to put the
	 * boot mem_map at the start of memory
	 */
	bootmap_size = init_bootmem_node(
			NODE_DATA(0),
			memory_start >> PAGE_SHIFT, /* map goes here */
			PAGE_OFFSET >> PAGE_SHIFT,	/* 0 on coldfire */
			memory_end >> PAGE_SHIFT);
	/*
	 * free the usable memory,  we have to make sure we do not free
	 * the bootmem bitmap so we then reserve it after freeing it :-)
	 */
	free_bootmem(memory_start, memory_end - memory_start);
	reserve_bootmem(memory_start, bootmap_size);
	/*
	 * get kmalloc into gear
	 */
	paging_init();
#ifdef CONFIG_VT
#if defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
#endif

#ifdef DEBUG
	printk("Done setup_arch\n");
#endif

}
 
int get_cpuinfo(char * buffer)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq;

    cpu = CPU;
    mmu = "none";
    fpu = "none";
 
    clockfreq = nasys_clock_freq;

    return(sprintf(buffer, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "Clocking:\t%lu.%1luMHz\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   clockfreq/1000000,(clockfreq/100000)%10,
		   (loops_per_jiffy*HZ)/500000,((loops_per_jiffy*HZ)/5000)%100,
		   (loops_per_jiffy*HZ)));

}

/*
 *	Get CPU information for use by the procfs.
 */

static int show_cpuinfo(struct seq_file *m, void *v)
{
    char *cpu, *mmu, *fpu;
    u_long clockfreq, clockfactor=1;

    cpu = CPU;
    mmu = "none";
    fpu = "none";

    clockfreq = nasys_clock_freq;

    seq_printf(m, "CPU:\t\t%s\n"
		   "MMU:\t\t%s\n"
		   "FPU:\t\t%s\n"
		   "Clocking:\t%lu.%1luMHz\n"
		   "BogoMips:\t%lu.%02lu\n"
		   "Calibration:\t%lu loops\n",
		   cpu, mmu, fpu,
		   clockfreq/1000000,(clockfreq/100000)%10,
		   (loops_per_jiffy*HZ)/500000,((loops_per_jiffy*HZ)/5000)%100,
		   (loops_per_jiffy*HZ));

	return 0;
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

	spi_lseek( NULL, clockCS, 0 /* == SEEK_SET */ );

	spi_data.register_addr = clock_write_control;
	spi_data.value         = 0x40; // Write protect
	spi_write( NULL, (const char *)&spi_data, 3, NULL  );

	spi_data.register_addr = clock_read_sec;
	spi_data.value         = 0;
	spi_read( NULL, (char *)&spi_data, 3, NULL );
	*sec = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_min;
	spi_data.value         = 0;
	spi_read( NULL, (char *)&spi_data, 3, NULL  );
	*min = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_hour;
	spi_data.value         = 0;
	spi_read( NULL, (char *)&spi_data, 3, NULL  );
	hr = (int)bcd2char( spi_data.value );
	if ( hr & 0x40 )  // Check 24-hr bit
 	    hr = (hr & 0x3F) + 12;     // Convert to 24-hr

	*hour = hr;



	spi_data.register_addr = clock_read_date;
	spi_data.value         = 0;
	spi_read( NULL, (char *)&spi_data, 3, NULL  );
	*date = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_month;
	spi_data.value         = 0;
	spi_read( NULL, (char *)&spi_data, 3, NULL  );
	*month = (int)bcd2char( spi_data.value );

	spi_data.register_addr = clock_read_year;
	spi_data.value         = 0;
	spi_read( NULL, (char *)&spi_data, 3, NULL  );
	*year = (int)bcd2char( spi_data.value );


	spi_release( NULL, NULL );
#else
	*year = *month = *date = *hour = *min = *sec = 0;

#endif
}

static void *cpuinfo_start (struct seq_file *m, loff_t *pos)
{
	return *pos < NR_CPUS ? ((void *) 0x12345678) : NULL;
}

static void *cpuinfo_next (struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return cpuinfo_start (m, pos);
}

static void cpuinfo_stop (struct seq_file *m, void *v)
{
}

struct seq_operations cpuinfo_op = {
	start:	cpuinfo_start,
	next:	cpuinfo_next,
	stop:	cpuinfo_stop,
	show:	show_cpuinfo
};
