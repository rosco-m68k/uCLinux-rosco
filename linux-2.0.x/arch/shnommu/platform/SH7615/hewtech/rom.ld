
MEMORY
    {
         rom    : ORIGIN = 0x00000000 , LENGTH = 0x0007ffff
         flash2  : ORIGIN = 0x02200000 , LENGTH = 0x022fffff - 0x02200000
	 flash1 : ORIGIN = 0x02000000, LENGTH = 0x020fffff - 0x02000000
         SRAM   : ORIGIN = 0x04000000 , LENGTH = 0x040FFFFF - 0x04000000
         SDRAM  : ORIGIN = 0x06000000 , LENGTH = 0x06FFFFFF - 0x06000000
	 eram : ORIGIN = 0x040fffff , LENGTH = 1
         esram : ORIGIN = 0x06efffff , LENGTH = 1
        netsec  :ORIGIN = 0x04000000  , LENGTH =0x040FFFFF - 0x04000000
      }



SECTIONS

{
	.romvec :
	{
	romvect_begin = . ;
        __romvec = . ;
	__ramvec = . ;
	*(.vectors)
	romvect_end = . ;
	romdata_begin = . ;
        romdata_end = (. + 1000) ;
	} > SDRAM


	.text :
	{

	_start = . ;
	__stext = . ;
        *(.text)
	_data_rom_start = . ;
	__etext = . ;
	*(.rodata)
        } > SDRAM

	.ramvec :
	{
         begin_vectors = . ;
        
	} > SDRAM
 
       .data :
	{
        ___data_start = . ;	
       
	data_begin = . ;
        *(.data)
	__edata = . ;
	edata = ALIGN( 0x10 ) ;
        } > SDRAM
       
        .bss :
        {
 	bss_begin = ALIGN( 0x10 ) ;
       
	*(.bss)
	*(COMMON)
	bss_end = . ;
	__end = ALIGN( 0x10 ) ;
	} > SDRAM

	
	.eram :
	{
	  _System_SP_Start =( . - 0x03) ;
	 __ramend = . ;
	} > esram
	
	 .romfs :
    {
      __flashstart = . ;
    } > flash2

     .net :
  {
       _txPkt = ALIGN(0x10) ;
           . = . + ( 16 * 128 );
       _rxPkt = ALIGN(0x10) ;
           . = . + ( 16 * 128 );
        _txbuf = .;
        . = . + ( 2048 * 128 );
        _rxbuf = . ;
        . = . + ( 2048 * 128 );


  } >netsec

   	
	}


