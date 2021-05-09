OUTPUT_ARCH(h8300h)
ENTRY("__start")

INPUT("rootimage.o")

MEMORY 
	{
	vector : ORIGIN = 0x000000, LENGTH = 0x000100
	rom    : ORIGIN = 0x000100, LENGTH = 0x100000-0x000100
	erom   : ORIGIN = 0x100000, LENGTH = 1
	ram    : ORIGIN = 0x200000, LENGTH = 0x100000
	eram   : ORIGIN = 0x300000, LENGTH = 1
	}

SECTIONS
{
	.vectors :
	{
	__vector = . ;
	} > vector
        .text :
	{
	__stext = . ;
        *(.text)
	__etext = . ;
        } > rom
	.rodata :
	{
	___data_rom_start = ALIGN ( 4 ) ;
	} > rom
	.data : AT( ADDR(.rodata)+SIZEOF(.rodata))
	{
	__ramstart = . ;
	__sdata = . ;
	___data_start = . ;
        *(.data)
	__edata = . ;
	edata = ALIGN( 0x10 ) ;
	___data_end = ALIGN( 0x10 ) ;
	} > ram
	__begin_data = LOADADDR(.data) ;
	.rootimg : AT( LOADADDR(.data) + SIZEOF(.data))
	{
	__rootimage = . ;
	} > rom
	.erom :
	{
	__erom = . ;
	} > erom
        .bss :
        {
	_sbss = ALIGN( 0x10 ) ;
	___bss_start = ALIGN( 0x10 ) ;
	*(.bss)
	*(COMMON)
	___bss_end = . ;
	end = ALIGN( 0x10 ) ;
	__end = ALIGN( 0x10 ) ;
	} > ram
        .dummy :
        {
        _COMMAND_START = . - 0x200 ;
	__ramend = . ;
	} > eram
}
