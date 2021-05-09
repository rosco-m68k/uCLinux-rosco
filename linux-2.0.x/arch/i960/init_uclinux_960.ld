

/* #include <linux/config.h> */

/*
 *   This is the linker directive file to link an application to run
 *   under mon960 on the CY board with a Cx, Jx, or Hx processor.
 *   Programs linked with this file are downloaded to DRAM after the
 *   data area used by mon960.
 *   malloc() uses the symbols _heap_base and _heap_end to manage the heap.
 *   _heap_base is located at the end of program memory (.text + .data +.bss).
 *   _heap_size is defined as 0x20000.
 *   The C runtime stack starts at _heap_end and grows up.
 *   There must be enough room after the program memory in the dram for
 *   the program's heap and stack.
 */


MEMORY
{

/* #ifdef CONFIG_CYVH_32MB  */  /* for 32 MB dram on eval board */
    dram : ORIGIN = 0xA0C08000, LENGTH = 0x1ff8000
    eram : ORIGIN = 0xA1FFF000, LENGTH = 0x01000
/*
#endif
#	romvec : ORIGIN = 0x10c00000, LENGTH = 0x10400
#	flash  : ORIGIN = 0x10c10400, LENGTH = 0xfec00
#       eflash : ORIGIN = 0x10d00000, LENGTH = 1
#	ramvec : ORIGIN = 0x00000000, LENGTH = 1024
#	ram    : ORIGIN = 0x10000400, LENGTH = 0x100000-0x400
#	eram   : ORIGIN = 0x10100000, LENGTH = 1
*/

}

SECTIONS
{
  .text :
	{
	text_start = . ;
	__data_rom_start = ALIGN ( 4 ) ;
	_data_rom_start = . ;
	data_rom_start = . ;
	___data_rom_start = . ;
	stext = . ;
	_stext = . ;
	__stext = . ;
	  *(.text)
  	etext = . ;
  	_etext = . ;
  	__etext = . ;
	
	} > dram
	.data :
	{
	__data_start = . ;
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
	end = ALIGN( 0x10 ) ;
	_end = . ;
	__end = . ;
	} > dram
	.eram :
	{
	_ramend = . ;
	} > eram

}

