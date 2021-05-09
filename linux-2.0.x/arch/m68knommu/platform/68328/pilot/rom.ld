MEMORY 
	{
	romvec : ORIGIN = 0x10c00000, LENGTH = 0x10400
	flash  : ORIGIN = 0x10c10400, LENGTH = 0xfec00
        eflash : ORIGIN = 0x10d00000, LENGTH = 1
	ramvec : ORIGIN = 0x00000000, LENGTH = 1024
	ram    : ORIGIN = 0x10000400, LENGTH = 0x100000-0x400
	eram   : ORIGIN = 0x10100000, LENGTH = 1
	}

SECTIONS
{
	.romvec :
	{
		_flashstart = . ;
		_romvec = . ;
	} > romvec
	.text :
	{
		text_start = . ;
        *(.text)
		. = ALIGN(4) ; 
		_etext = . ;
		__data_rom_start = . ;
	} > flash
	.eflash :
	{
        _flashend = . ;
	} > eflash
	.ramvec :
	{
		_ramvec = . ;
	} > ramvec
	.data :
	{
		__data_start = . ;
		*(.rodata)
		. = ALIGN(4) ;
        *(.data)
		. = ALIGN(16) ;
		_edata = . ;
		edata = . ;
		__data_end = . ;
	} > ram
	.bss :
	{
		__bss_start = ALIGN( 0x10 ) ;
		*(.bss)
		. = ALIGN(4) ;
		*(COMMON)
		. = ALIGN(16) ;
		end = . ;
		_end = . ;
	} > ram
	.eram :
	{
		_ramend = . ;
	} > eram
}
