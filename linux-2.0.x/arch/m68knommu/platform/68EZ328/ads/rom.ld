/* arch/m68knommu/platform/68EZ328/ads_flash.ld
 * 
 * Copyright (C) 1999  Vladimir Gurevich <vgurevic@cisco.com>
 *                     Bear & Hare Software, Inc.
 *
 * Based on: arch/m68knommu/platform/68EZ328/ucsimm.ld
 */

MEMORY 
	{
	ramvec : ORIGIN = 0x00000000, LENGTH = 0x400
	dram   : ORIGIN = 0x00000400, LENGTH = 0x400000 - 0x400
	eram   : ORIGIN = 0x00400000, LENGTH = 1
	romvec : ORIGIN = 0x10000000, LENGTH = 0x400
	flash  : ORIGIN = 0x10000400, LENGTH = 0x200000 - 0x400
	eflash : ORIGIN = 0x10200000, LENGTH = 1
	}

SECTIONS
{
	/*
	 * Put exception vector table in the beginning of SRAM
	 * (it must start at 0x00000000 anyway
	 */
	.ramvec :
	{
		_ramvec = . ;
		*(.ramvec)
	} > ramvec
	
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
		__data_end = ALIGN( 0x10 ) ;
		*(.bss)
		*(COMMON)
		end = ALIGN( 0x10 ) ;
		_end = ALIGN( 0x10 ) ;
	} > dram

	.eram :
	{
		_boot_stack = . - 4;
		_ramend = . ;
	} > eram

	/*
	 * FLASH portion
	 */
	.romvec :
	{
		_romvec = . ;
		_flashstart = . ;
		*(.romvec)
	} > romvec

        .text :
	{
		text_start = . ;
       		*(.text)
       		*(.rodata)
		_etext = . ;
		__data_rom_start = ALIGN ( 4 ) ;
        } > flash

	.eflash :
	{
		_flashend = . ;
	} > eflash
}
