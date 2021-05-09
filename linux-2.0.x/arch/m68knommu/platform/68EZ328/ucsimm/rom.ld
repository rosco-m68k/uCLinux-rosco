MEMORY {
	romvec : ORIGIN = 0x10c10000, LENGTH = 0x00400
	flash  : ORIGIN = 0x10c10400, LENGTH = 0xfec00
	eflash : ORIGIN = 0x10d00000, LENGTH = 1
	ramvec : ORIGIN = 0x00000000, LENGTH = 1024
	ram	   : ORIGIN = 0x00020000, LENGTH = 0x00800000 - 0x20000
	eram   : ORIGIN = 0x00800000, LENGTH = 1
}

SECTIONS {

	.romvec : {
		. = ALIGN(4) ;
		_romvec = . ;
		__rom_start = . ;
	} > romvec

	.text : {
		. = ALIGN(4) ;
		text_start = . ;
		*(.text)
		*(.rodata)
		. = ALIGN(4) ;
		_etext = . ;
		__data_rom_start = . ;
	} > flash

	.eflash : {
		. = ALIGN(4) ;
		_flashend = . ;
	} > eflash

	.ramvec : {
		. = ALIGN(4) ;
		__ram_start = . ;
		_ramvec = . ;
	} > ramvec

	.data : {
		. = ALIGN(4) ;
		_sdata = . ;
		__data_start = . ;
		*(.data)
		. = ALIGN( 0x10 ) ;
		_edata = . ;
		edata = . ;
	} > ram

	.bss : {
		. = ALIGN( 0x10 ) ;
		_sbss = . ;
		__bss_start = . ;
		__data_end = . ;
		*(.bss)
		*(COMMON)
		. = ALIGN(0x10) ;
		_ebss = . ;
		__bss_end = . ;
		end = . ;
		_end = . ;
	} > ram

	.eram : {
		. = ALIGN(4) ;
		_ramend = . ;
	} > eram
}
