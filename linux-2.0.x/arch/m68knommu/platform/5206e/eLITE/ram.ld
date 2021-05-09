/* default for Motorola eLITE board, with 4 MB DRAM SIMM installed */
MEMORY {
	ram    : ORIGIN = 0x00000400, LENGTH = 0x000e0000 
}

SECTIONS {

        .text 0x00000400 : { 
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

