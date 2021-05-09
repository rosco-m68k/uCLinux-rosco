
MEMORY {
	ram    : ORIGIN = 0x20000, LENGTH = 0x3e0000
}

SECTIONS {

        .text 0x20000 : {
		_stext = . ;
        	*(.text)
		_etext = ALIGN(0x4) ;
        } > ram

        .data BLOCK(0x4) : {
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

