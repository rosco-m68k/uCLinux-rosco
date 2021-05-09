MEMORY 
	{
	romvec : ORIGIN = 0x00002000, LENGTH = 0x000400
	flash  : ORIGIN = 0x00002400, LENGTH = 0x080000 - 0x02400
	eflash : ORIGIN = 0x00080000, LENGTH = 0
	ramvec : ORIGIN = 0x00800000, LENGTH = 0x000400
	ram    : ORIGIN = 0x00800400, LENGTH = 0x180000 - 0x00400
	eram   : ORIGIN = 0x00980000, LENGTH = 0
	}

SECTIONS
{
	.romvec :
		{
			_romvec = . ;
			*(.romvec)
		} > romvec
	.flash :
		{
			_romstart = .;
		}
	.eflash :
		{
			_romend = . ;
		} > eflash
	.ramvec :
		{
			_ramvec = . ;
		} > ramvec
	.text :
		{
			_ramstart = . ;
			text_start = . ;
      		*(.text)
			_etext = . ;
			__data_rom_start = ALIGN ( 4 ) ;
        } > ram
	.data :
		{
			__data_start = . ;
			*(.rodata)
      		*(.data)
			_edata = . ;
			edata = ALIGN( 0x10 ) ;
			__data_end = ALIGN( 0x10 ) ;
       	} > ram
	.bss :
       	{
			__bss_start = ALIGN( 0x10 ) ;
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
