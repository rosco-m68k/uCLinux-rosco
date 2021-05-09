/* #include <linux/config.h> */

/*
 *   This is the linker directive file to link the kernel to run
 *   on the IQ80303 development board under Mon960.
 *
 *   (C) 2002 Okiok Data.
 *
 *   malloc() uses the symbols _heap_base and _heap_end to manage the heap.
 *   _heap_base is located at the end of program memory (.text + .data +.bss).
 *   _heap_size is defined as 0x20000.
 *   The C runtime stack starts at _heap_end and grows up.
 *   There must be enough room after the program memory in the dram for
 *   the program's heap and stack.
 */


MEMORY
{

	/* 
	 * leave 128K for initialization code + stack - kernel starts at 128K
	 * eram should be end of ram? leave 4096 bytes at end for endofram
	 */
    dram : ORIGIN = 0xA0018500, LENGTH = 0x1fe6b00 
    eram : ORIGIN = 0xA1FFF000, LENGTH = 0x01000
}

ENTRY(_main)

SECTIONS
{
  .text :
	{
	text_start = . ;
/*
	__data_rom_start = ALIGN ( 4 ) ;
	data_rom_start = . ;
	_data_rom_start = . ;
	___data_rom_start = . ;
*/
	stext = . ;
	_stext = . ;
	__stext = . ;
	  *(.text)
  	etext = . ;
  	_etext = . ;
  	__etext = . ;
	. = ALIGN(10);

	rootfs_data = .;
/*  	rootfs.o	*/
	rootfs_data_end = .;
	. = ALIGN(4);


	} > dram
	
.data :
	{
	__data_start = . ;
	___data_start = . ;
	sdata = . ;
	_sdata = . ;
	__sdata = . ;
        *(.data)

	edata = ALIGN( 0x10 ) ;
	_edata = . ;
	__edata = . ;
	data_rom_end = . ;
	_data_rom_end = . ;
	__data_rom_end = . ;
	___data_rom_end = . ;
	} > dram
	
	
.bss :
	{
	__bss_start = ALIGN( 0x10 ) ;
	__data_end = ALIGN( 0x10 ) ;
	*(.bss)
	*(COMMON)
	end = ALIGN( 0x100 ) ;
	_end = . ;
	__end = . ;
	} > dram
	
.eram :
	{
	_ramend = ALIGN( 0x100 ) ;
	stackbase = ALIGN( 0x100 )  ;
	_stackbase = . ;
	} > eram

	
	
/* Bounds of heap: */
/* The heap may be placed in a separate memory region, if desired. */


}
