MEMORY 
	{
	romvec : ORIGIN = 0x00108000, LENGTH = 0x400
	ramvec : ORIGIN = 0x00108400, LENGTH = 0x400
	ram    : ORIGIN = 0x00108800, LENGTH = 0x100000 - 0x8800
	eram   : ORIGIN = 0x00200000, LENGTH = 1
	}

SECTIONS
{
	.romvec :
	{
	_romvec = . ;
	*(.romvec)
	} > romvec
	.ramvec :
	{
	_ramvec = . ;
	*(.ramvec)
	} > ramvec
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
