/* arch/m68knommu/platform/68EZ328/alma_ans_ram.ld
 * 
 * Copyright (C) 1999  Vladimir Gurevich <vgurevic@cisco.com>
 *                     Bear & Hare Software, Inc.
 *
 * Based on: arch/m68knommu/platform/68EZ328/ucsimm.ld
 * Copyright (C) 1998  D. Jeff Dionne <jeff@lineo.ca>,
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 */

MEMORY 
	{
        ramvec : ORIGIN = 0x00000000, LENGTH = 0x400
	romvec : ORIGIN = 0x00000400, LENGTH = 0x400
	dram   : ORIGIN = 0x00000800, LENGTH = 0x200000 - 0x800
	eram   : ORIGIN = 0x00200000, LENGTH = 1
	flash  : ORIGIN = 0x10000000, LENGTH = 0x200000
	eflash : ORIGIN = 0x10200000, LENGTH = 1
	}

SECTIONS
{
	/*
	 * Put exception vector table in the beginning of DRAM
	 * (it must start at 0x00000000 anyway
	 */
	.ramvec :
	{
		_ramvec = . ;
		*(.ramvec)
	} > ramvec

	/*
	 * Now let's load the whole image into DRAM. 
	 */
	.romvec :
	{
		_romvec = . ;
		*(.romvec)
	} > romvec

        .text :
	{
		text_start = . ;
       		*(.text)
       		*(.rodata)
		_etext = . ;
		__data_rom_start = ALIGN ( 4 ) ;
        } > dram
	
        .data :
	{
		__data_start = . ;
        	*(.data)
		_edata = . ;
		edata = ALIGN( 0x10 ) ;
        } > dram

        .bss :
        {
		__bss_start = ALIGN( 0x10 ) ;
		__data_end  = ALIGN( 0x10 ) ;
		*(.bss)
		*(COMMON)
		end  = ALIGN( 0x10 ) ;
		_end = ALIGN( 0x10 ) ;
	} > dram

	.eram :
	{
		_boot_stack = . - 4;
		_ramend = . ;
	} > eram

	/*
	 * These sections will just help us to define the FLASH size
	 */
	.flash : 
	{
		_flashstart = . ;
	} > flash

	.eflash :
	{
		_flashend = . ;
	} > eflash
}
