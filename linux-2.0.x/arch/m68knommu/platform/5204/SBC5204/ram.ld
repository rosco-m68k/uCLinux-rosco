/* arch/m68knommu/platform/5204/SBC5204/crt0_ram.ld
 * 
 * Copyright (C) 1999  Vladimir Gurevich <vgurevic@cisco.com>
 *                     Bear & Hare Software, Inc.
 *
 * Based on: arch/m68knommu/platform/68EZ328/ucsimm.ld
 */

MEMORY 
	{
        ramvec : ORIGIN = 0x00010000, LENGTH = 0x400
	romvec : ORIGIN = 0x00010400, LENGTH = 0x400
	ram    : ORIGIN = 0x00010800, LENGTH = 0x100000 - 0x10800
	eram   : ORIGIN = 0x00400000, LENGTH = 1
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
		_etext = . ;
		__data_rom_start = ALIGN ( 4 ) ;
        } > ram
	
        .data :
	{
		__data_start = . ;
        	*(.data)
		_edata = . ;
		edata = ALIGN( 0x10 ) ;
        } > ram

        .bss :
        {
		__bss_start = ALIGN( 0x10 ) ;
		__data_end  = ALIGN( 0x10 ) ;
		*(.bss)
		*(COMMON)
		end  = ALIGN( 0x10 ) ;
		_end = ALIGN( 0x10 ) ;
	} > ram

	.eram :
	{
		_boot_stack = . - 4;
		_ramend = . ;
	} > ram

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
