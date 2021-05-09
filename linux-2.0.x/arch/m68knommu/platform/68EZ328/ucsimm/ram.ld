MEMORY 
	{
	romvec : ORIGIN = 0x10c10000, LENGTH = 0x00000400
	flash  : ORIGIN = 0x10c10400, LENGTH = 0x00200000 - 0x00010400
	eflash : ORIGIN = 0x10e00000, LENGTH = 0
	ramvec : ORIGIN = 0x00000000, LENGTH = 0x00000400
	bvec   : ORIGIN = 0x00020000, LENGTH = 0x00000400
	ram    : ORIGIN = 0x00020400, LENGTH = 0x00800000 - 0x00020400
	eram   : ORIGIN = 0x00800000, LENGTH = 0
	}

SECTIONS
{
	.fakevec :
	{
	} > romvec
	.rom :
	{
	__rom_start = . ;
	} > flash
	.eflash :
	{
	_flashend = . ;
	} > eflash
	.realvec :
	{
	_ramvec = . ;
	} > ramvec
	.romvec :
	{
	_romvec = . ;
	} > bvec
        .text :
	{
	__ram_start = . ;
	text_start = . ;
        *(.text)
        *(.rodata)
	_etext = . ;
	__data_rom_start = ALIGN ( 4 ) ;
        } > ram
        .data :
	{
	_sdata = . ;
	__data_start = . ;
        *(.data)
	_edata = . ;
	edata = ALIGN( 0x10 ) ;
        } > ram
        .bss :
        {
	_sbss = ALIGN( 0x10 ) ;
	__bss_start = ALIGN( 0x10 ) ;
	__data_end = ALIGN( 0x10 ) ;
	*(.bss)
	*(COMMON)
	_ebss = . ;
	__bss_end = . ;
	end = ALIGN( 0x10 ) ;
	_end = ALIGN( 0x10 ) ;
	} > ram
	.eram :
	{
	_ramend = . ;
	} > eram
}
