MEMORY 
	{
	sram   : ORIGIN = 0x00000000, LENGTH = 0x40000
	flash  : ORIGIN = 0x00200000, LENGTH = 0x200000
	dram   : ORIGIN = 0x00400000, LENGTH = 0x200000
	eram   : ORIGIN = 0x00600000, LENGTH = 1
	}

SECTIONS
{
	/*
	 * Put exception vector table in the beginning of SRAM
	 * (it must start at 0x00000000 anyway
	 */
	.ramvec :
	{
		_ramvec = . ;
		*(.ramvec)
	} > sram

	/*
	 * Let's just define symbols for the flash
	 */
	_flash_start = 0x00200000;
	_flash_size  = 0x00200000;

	/*
	 * Now let's load the whole image into DRAM. We'll put it into the
	 * end of it to make the life easier
	 */
	.romvec :
	{
		_romvec = . ;
		*(.romvec)
	} > flash

        .text :
	{
		text_start = . ;
       		*(.text)
		_etext = . ;
		__data_rom_start = ALIGN ( 4 ) ;
        } > flash
	
        .data :
	{
	__data_start = . ;
        *(.data)
	_edata = . ;
	edata = ALIGN( 0x10 ) ;
        } > dram

        .bss :
        {
	__bss_start = ALIGN( 0x10 ) ;
	__data_end = ALIGN( 0x10 ) ;
	*(.bss)
	*(COMMON)
	end = ALIGN( 0x10 ) ;
	_end = ALIGN( 0x10 ) ;
	} > dram

	.eram :
	{
	_boot_stack = . - 4;
	_ramend = . ;
	} > eram
}
