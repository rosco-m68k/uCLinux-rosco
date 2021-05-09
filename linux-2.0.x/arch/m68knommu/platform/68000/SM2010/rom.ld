/*****************************************************************************/
/* 
 *  crt0_rom.S v1.0 <2003-07-28 17:43:43 gc>
 * 
 *  linux/arch/m68knommu/platform/68000/SM2010/rom.ld
 *
*   SM2010 GNU ld command file for running kernel directly from ROM
 *  you probably need an own version of this file for your board
 *
 *  Author:     Guido Classen (classeng@clagi.de)
 *              Weiss-Electronic GmbH
 *
 *  This program is free software;  you can redistribute it and/or modify it
 *  under the  terms of the GNU  General Public License as  published by the
 *  Free Software Foundation.  See the file COPYING in the main directory of
 *  this archive for more details.
 *
 *  This program  is distributed  in the  hope that it  will be  useful, but
 *  WITHOUT   ANY   WARRANTY;  without   even   the   implied  warranty   of
 *  MERCHANTABILITY  or  FITNESS FOR  A  PARTICULAR  PURPOSE.   See the  GNU
 *  General Public License for more details.
 * *
 *  Change history:
 *       2001-07-13 G. Classen: initial version
 *
 */
 /****************************************************************************/

MEMORY 
	{
          /* romvec only used to determinate size of ROM */
          romvec   : ORIGIN = 0x00E00000, LENGTH = 0x00000000 
            
          /* SM2010 ID String (never overwritten bei downloading program) */
          prog_id  : ORIGIN = 0x00E00000, LENGTH = 0x00000040
            
          /* SM2010 flash memory (512k) */
          flash    : ORIGIN = 0x00E00040, LENGTH = 0x00080000 - 0x40
        
	  /* SM2010 flash memory end */
          eflash   : ORIGIN = 0x00E80000, LENGTH = 1

          /* interrupt vectors in RAM */
	  ramvec   : ORIGIN = 0x00000000, LENGTH = 1024

          /* user RAM memory */
	  ram      : ORIGIN = 0x00000400, LENGTH = 0x00100000 - 0x400

          /* end of RAM (used to set %sp) */
	  eram     : ORIGIN = 0x00100000, LENGTH = 1
	}

SECTIONS
{
 	.romvec :
	{
	_romvec = . ;
	__rom_start = . ;
	} > romvec

        .text :
	{
	text_start = . ;
        *(.text)
        . = ALIGN(4) ;
	_etext = . ;
        *(.rodata)
        . = ALIGN(4) ;
	__data_rom_start = ALIGN ( 4 ) ;
        } > flash

	.eflash :
	{
	_flashend = . ;
	} > eflash

	.ramvec :
	{
	__ram_start = . ;
	_ramvec = . ;
	} > ramvec

        .data :
	{
	_sdata = . ;
	__data_start = . ;
        /* testweise .rodata ins RAM kopieren */
        /* 
         * *(.rodata)
         * . = ALIGN(4) ;
         */
        *(.data)
        . = ALIGN(16) ;
	_edata = . ;
	edata = ALIGN( 0x10 ) ;
        } > ram

        .bss :
        {
	_sbss = ALIGN( 0x10 ) ;
	__bss_start = ALIGN( 0x10 ) ;
	__data_end = ALIGN( 0x10 ) ;
	*(.bss)
        . = ALIGN(4);
	*(COMMON)
	_ebss = . ;
	__bss_end = . ;
        . = ALIGN(16) ;
	end = ALIGN( 0x10 ) ;
	_end = ALIGN( 0x10 ) ;
	} > ram

	.eram :
	{
	_boot_stack = . - 4;
	_ramend = . ;
	} > eram
}

/*
 *Local Variables:
 * mode: c
 * c-indent-level: 8
 * c-brace-imaginary-offset: 0
 * c-brace-offset: -8
 * c-argdecl-indent: 8
 * c-label-offset: -8
 * c-continued-statement-offset: 8
 * c-continued-brace-offset: 0
 * indent-tabs-mode: nil
 * c-file-style: "Linux"
 * fill-column: 76
 * tab-width: 8
 * time-stamp-pattern: "4/<%%>"
 * End:
 */
