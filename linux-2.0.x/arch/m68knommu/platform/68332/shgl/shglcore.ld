MEMORY 
	{
	romvec : ORIGIN = 0x00020000, LENGTH = 0x00400
	flash  : ORIGIN = 0x00020400, LENGTH = 0xffc00 - 0x80000
	ramvec : ORIGIN = 0x00100000, LENGTH = 1024
	ram    : ORIGIN = 0x00100400, LENGTH = 0xffc00
	eram   : ORIGIN = 0x00200000, LENGTH = 1
	}

SECTIONS
{
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
        } > flash
	.ramvec :
	{
	_ramvec = . ;
	} > ramvec
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
	__data_end = ALIGN( 0x10 ) ;
	*(.bss)
	*(COMMON)
	end = ALIGN( 0x10 ) ;
	_end = ALIGN( 0x10 ) ;
	} > ram
	.eram :
	{
	_ramend = . ;
	} > eram
}
