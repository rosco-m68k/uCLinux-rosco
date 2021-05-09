MEMORY 
        {
        romvec : ORIGIN = 0x04000000, LENGTH = 0x2000
        flash  : ORIGIN = 0x04002000, LENGTH = 0x00800000 - 0x2000
        eflash : ORIGIN = 0x04800000, LENGTH = 0
        ramvec : ORIGIN = 0x00000000, LENGTH = 0x00002000
        ram    : ORIGIN = 0x00002000, LENGTH = 0x00800000 - 0x2000
        eram   : ORIGIN = 0x00800000, LENGTH = 0
        }

SECTIONS
{
        .romvec :
        {
        __romvec = . ;
        *(.romvec)
        } > romvec

        .text :
        {
        ___rom_start = . ;
        text_start = . ;
        *(.text)
        } > flash

        .rodata :
        {
        *(.rodata)
        __etext = . ;
        } > flash

        .initrd :
        {
        __initrd_start = . ;
        *(.initrd)
        __initrd_end = . ;
        ___data_rom_start = . ;
        } > flash

        .eflash :
        {
        __flashend = . ;
        } > eflash

        .data :
        AT ( ADDR (.initrd) + SIZEOF (.initrd))
        {
	__ramstart = . ;
        __sdata = . ;
        ___data_start = . ;
        *(.data)
        __edata = . ;
        __end_data = . ;
        __data_end = . ;
        edata = ALIGN( 0x10 ) ;
        } > ram

        .bss :
        {
        __sbss = ALIGN( 0x10 ) ;
        ___bss_start = ALIGN( 0x10 ) ;
        *(.bss)
        *(COMMON)
        __ebss = . ;
        ___bss_end = . ;
        end = ALIGN( 0x10 ) ;
        __end = ALIGN( 0x10 ) ;
        } > ram

        .ramvec :
        AT ( ADDR (.initrd) + SIZEOF (.initrd) + SIZEOF (.data))
        {
        __ramvec_start = . ;
        *(.ramvec)
        __ramvec_end = . ;
        } > ramvec

        .eram :
        {
        __ramend = . ;
        } > eram
}
