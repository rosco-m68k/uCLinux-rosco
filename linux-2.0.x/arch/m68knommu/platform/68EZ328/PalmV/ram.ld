MEMORY 
	{
	romvec : ORIGIN = 0x10c10000, LENGTH = 0x00000400
	flash  : ORIGIN = 0x10c10400, LENGTH = 0x00200000 - 0x00010400
	eflash : ORIGIN = 0x10e00000, LENGTH = 0
	ramvec : ORIGIN = 0x00000000, LENGTH = 0x00000400
	bvec   : ORIGIN = 0x00000400, LENGTH = 0x00000400
	ram    : ORIGIN = 0x00000800, LENGTH = 0x00200000 - 0x00000800
	eram   : ORIGIN = 0x00200000, LENGTH = 0
	}

SECTIONS
{
	.fakevec : {
	} > romvec

	.rom : {
		__rom_start = . ;
	} > flash

	.eflash : {
		_flashend = . ;
	} > eflash

	.realvec : {
		_ramvec = . ;
	} > ramvec

	.romvec : {
		_romvec = . ;
	} > bvec

    .text : {
		. = ALIGN(4);
		__ram_start = .;
		text_start = .;
        *(.text)
		*(.rodata)
		. = ALIGN(4);
		_etext = .;
		__data_rom_start = .;
	} > ram

	.data : {
		. = ALIGN(4);
		_sdata = .;
		__data_start = .;
        *(.data)
		. = ALIGN(0x10);
		_edata = .;
		edata = .;
	} > ram

	.bss : {
		. = ALIGN(0x10);
		_sbss = .;
		__bss_start = .;
		__data_end = .;
		*(.bss)
		*(COMMON)
		. = ALIGN(16);
		_ebss = .;
		__bss_end = .;
		end = .;
		_end = .;
	} > ram

	.eram : {
		_ramend = . ;
	} > eram
}
