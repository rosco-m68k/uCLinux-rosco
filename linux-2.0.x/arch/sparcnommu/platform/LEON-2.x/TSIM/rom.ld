/* 
 * arch/sparcnommu/rom.ld
 * 
 * Copyright (C) 2000, 2001
 * The LEOX Team <team@leox.org>, http://www.leox.org
 *
 * LEOX.org is about the development of free hardware and software resources
 *   for system on chip.
 *
 * Description: Linker script used when uClinux kernel is run from ROM area.
 * ~~~~~~~~~~~    (for more information: see 'ld.info' file)
 *
 */

/*
 * Mapping of the LEON core under TSIM-x.x.x (evaluation version):
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 0x0000 0000 - 0x001F FFFF  ROM               (2 MBytes) 32 bits
 * 0x2000 0000 - 0x3FFF FFFF  Memory bus I/O
 * 0x4000 0000 - 0x007F FFFF  RAM               (4 MBytes) 1 bank - 32 bits
 *
 * 0x8000 0000 - 0x9FFF FFFF  On-Chip registers
 */

/*
 * The MEMORY command describes the available memory areas of the LEON
 *   core implemented under TSIM-x.x.x (evaluation version).
 * It complements the SECTIONS command by describing the location and size 
 *   of blocks of memory in the target.
 */
MEMORY 
	{
	/*
         * Here we describe which memory regions in RAM and ROM areas may 
         *   be used by the linker, and which memory regions it must avoid.
         *
         * The available memory regions into ROM area are:
         *     o flash       : first byte of the PROM area.
         *     o eflash      : last byte  of the PROM area.
         */
	flash  : ORIGIN = 0x00000000,      LENGTH = 2M
	eflash : ORIGIN = 0x00000000+2M,   LENGTH = 1
	/*
	 * The available memory regions into RAM area are:
         *     o ram        : first byte of the RAM area.
         *     o eram       : last byte  of the RAM area.
         */
	ram    : ORIGIN = 0x40000000,      LENGTH = 4M
	eram   : ORIGIN = 0x40000000+4M,   LENGTH = 1
	}

/*
 * The SECTIONS command specifies a "picture" of the output file's layout,
 *   in varying degrees of detail. It controls exactly where input sections
 *   are placed into output sections, their order in the output file, and
 *   which output sections they are allocated.
 */
SECTIONS
{
        /*****************************************
	**                                      **
	**          ROM memory layout           **
	**                                      **
        *****************************************/

        /*
	 * .text section contains the exception vector table and 
	 *    the processor code instructions.
         */
        .text :
	{
		/*
		 * Put exception vector table in the beginning of ROM
		 * (it must start at 0x00000000 anyway)
		 */
		_romvec = . ;
		__rom_start = . ;
		*(.romvec)
		/*
		 * the processor code instructions set hereafter
		 */
		_stext = . ;
		text_start = . ;
        	*(.text)
		_etext = . ;
		__data_rom_start = ALIGN ( 4 ) ;
        } > flash

	.eflash :
	{
		/*
		 * Define a tag at end of ROM area.
		 */
		_flashend = . ;
	} > eflash

	/*****************************************
	**                                      **
	**          RAM memory layout           **
	**                                      **
        *****************************************/

	/*
	 * .data section contains the initialized global variables
         */
        .data :
	{
		_sdata = . ;
		__data_start = . ;
		*(.rodata)
        	*(.data)
		_edata = . ;
		edata = ALIGN( 0x10 ) ;
        } > ram

        /*
	 * .bss section contains the uninitialized global variables
         */
        .bss :
        {
		_sbss = ALIGN( 0x10 ) ;
		__bss_start = ALIGN( 0x10 ) ;
		__data_end = ALIGN( 0x10 ) ;
		*(.bss)
		*(COMMON)
		_ebss = . ;
		__bss_end = . ;
		end = ALIGN( 0x10 ) ;
		_end = ALIGN( 0x10 ) ;
	} > ram

	.eram :
	{
		/*
		 * Define a tag at end of RAM area.
		 */
		_ramend = . ;
	} > eram
}
