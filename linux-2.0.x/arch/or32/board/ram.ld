MEMORY 
        {
        romvec : ORIGIN = 0x04010000, LENGTH = 0x00002000
        flash  : ORIGIN = 0x04012000, LENGTH = 0x00200000 - 0x00012000
        eflash : ORIGIN = 0x04200000, LENGTH = 0
        ramvec : ORIGIN = 0x00000000, LENGTH = 0x00002000
        ram    : ORIGIN = 0x00020400, LENGTH = 0x00400000 - 0x00020400
        eram   : ORIGIN = 0x00400000, LENGTH = 0
        initrd : ORIGIN = 0x00700000, LENGTH = 0x00100000
        }

SECTIONS
{
        .ramvec :
        {
        __ramvec_start = . ;
        *(.ramvec)
        __ramvec_end = . ;
        } > ramvec
        .romvec :
        {
        __romvec = . ;
        } > romvec
        .text :
        {
        __ramstart = . ;
        text_start = . ;
        *(.text)
        __etext = . ;
        ___data_rom_start = ALIGN ( 4 ) ;
        } > ram
        .data :
        {
        __sdata = . ;
        ___data_start = . ;
        *(.data)
        __edata = . ;
        __end_data = . ;
        edata = ALIGN( 0x10 ) ;
        } > ram
        .bss :
        {
        __sbss = ALIGN( 0x10 ) ;
        ___bss_start = ALIGN( 0x10 ) ;
        __data_end = ALIGN( 0x10 ) ;
        *(.bss)
        *(COMMON)
        __ebss = . ;
        ___bss_end = . ;
        end = ALIGN( 0x10 ) ;
        __end = ALIGN( 0x10 ) ;
        } > ram
        .eram :
        {
        __ramend = . ;
        } > eram
        .initrd :
        {
        __initrd_start = . ;
        *(.initrd)
        __initrd_end = . ;
        } > initrd
}
