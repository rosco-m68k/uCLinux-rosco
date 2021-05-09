
MEMORY {
	/* make room for VECBASE(0x400) and STACK(0x1000) */
	ram    : ORIGIN = 0x00100400, LENGTH = 0x0fec00
	/* rom (flash) actually starts at 0xffe00000, but linux can co-exist
	   with the shipping product as an alternative boot mode. */
	rom    : ORIGIN = 0xfff00000, LENGTH = 0x100000
}

SECTIONS {

        .text : {
		_stext = . ;
        	*(.text)
		_etext = ALIGN(0x4) ;
		__data_rom_start = ALIGN(0x4) ;
        } > rom

	.data : AT(_etext) {
		_sdata = . ;
		__data_start = . ;
		*(.rodata)
        	*(.data)
		_edata = ALIGN(0x4) ;
        } > ram

        .bss BLOCK(0x4) : {
		_sbss = . ;
		*(.bss)
		*(COMMON)
		_ebss = ALIGN(0x4) ;
		_end = ALIGN(0x4) ;
	} > ram
}

