MEMORY 
	{
		/* 
		 * The interrupt vector resides at the begin of ram 
		 * so the interruptroutines can be switched!
		 */

		ramvec : ORIGIN = 0x100000, LENGTH = 0x000400

		rom    : ORIGIN = 0x000000, LENGTH = 0x100000

		ram    : ORIGIN = 0x100400, LENGTH = 0x080000 - 0x400
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
        } > rom

        .data : AT( _etext ) {
		_sdata = . ;
		*(.rodata)
		*(.data)
		_edata = ALIGN ( 0x8 ) ;
        } > ram


	.bss : AT ( _etext + ( _edata - _sdata ) ) {
		__romfs = _etext + _edata - _sdata ;
		_sbss = . ;
		*(.bss)
		*(COMMON)
		_ebss = ALIGN ( 0x8 ) ;
		_end = ALIGN ( 0x8 ) ;
	} > ram

}
