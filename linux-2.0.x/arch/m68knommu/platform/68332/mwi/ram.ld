MEMORY 
	{
		/* 
		 * The interrupt vector resides at the begin of ram 
		 * so the interruptroutines can be switched!
		 */

		ramvec : ORIGIN = 0x100000, LENGTH = 0x000400

		/*
		 * Put all the stuff in ram.
		 * There is no need to copy data to ram, it's allready there
		 * Subtract space for stack and interrupt vector
		 */

		ram    : ORIGIN = 0x100400, LENGTH = 0x080000 - 0x1000 - 0x400
	}

SECTIONS
{	
	.ramvec : {
		_ramvec = . ;
		*(.ramvec)
	} > ramvec

        .text : {
		_stext = . ;
	        *(.text)
		_etext = ALIGN ( 0x8 ) ;
        } > ram

        .data BLOCK(0x08) : {
		_sdata = . ;
		*(.rodata)
		*(.data)
		_edata = ALIGN ( 0x8 ) ;
        } > ram

        .bss BLOCK(0x08) : {
		_sbss = . ;
		*(.bss)
		*(COMMON)
		_ebss = ALIGN ( 0x8 ) ;
		_end = ALIGN ( 0x8 ) ;
	} > ram
}
