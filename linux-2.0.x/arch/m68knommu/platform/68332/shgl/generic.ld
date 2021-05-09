MEMORY 
	{
	ramvec : ORIGIN = 0x00000, LENGTH = 0x3FC
	prgram : ORIGIN = 0x00400, LENGTH = 0xffffc - 0x400
	endram : ORIGIN = 0xffffc, LENGTH = 1
	}

SECTIONS
{
	.ramvec :
	{
	_ramvec = . ;
	*(.ramvec)
	} > ramvec
        .text :
	{
	_stext = . ;
	text_start = . ;
	_prgram = . ;
        *(.text)
	_etext = . ;
	__data_rom_start = ALIGN ( 4 ) ;
        } > prgram
        .data :
	{
	__data_start = . ;
        *(.data)
	_edata = . ;
	edata = ALIGN( 0x10 ) ;
        } > prgram
        .bss :
        {
	__bss_start = ALIGN( 0x10 ) ;
	__data_end = ALIGN( 0x10 ) ;
	*(.bss)
	*(COMMON)
	end = ALIGN( 0x10 ) ;
	_end = ALIGN( 0x10 ) ;
	} > prgram
	.eram :
	{
	_ramend = . ;
	_endram = . ;
	} > endram
}
