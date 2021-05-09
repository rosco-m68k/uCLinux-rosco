MEMORY
{
    romvec   : ORIGIN = 0x00000000,                 LENGTH = 1024
    bootrom  : ORIGIN = 1024,                       LENGTH = 0x00040000 - 1024
    flash    : ORIGIN = 0x01400000,                 LENGTH = 0x00200000
    eflash   : ORIGIN = 0x01400000 + 0x00200000,    LENGTH = 1
    ramvec   : ORIGIN = 0x00F80000,                 LENGTH = 1024
    ram      : ORIGIN = 0x00F80000 + 1024,          LENGTH = 0x00480000 - 1024
    eram     : ORIGIN = 0x00F80000 + 0x00480000,    LENGTH = 1
    nvram    : ORIGIN = 0xffffc000,                 LENGTH = 0x00002000
    envram   : ORIGIN = 0xffffc000 + 0x00002000,    LENGTH = 1
    dpram    : ORIGIN = 0xffffe000,                 LENGTH = 0x00002000
}

SECTIONS
{
    .dpram :
    {
        _dprbase = . ;
    } > dpram

    .romvec :
    {
        _romvec = . ;
        __rom_start = . ;
        *(.data.initvect)
        . = ALIGN(4);
    } > romvec

    .bootrom :
    {
        boot_rom_start = .;
        *(.text.bootrom)
    } > bootrom

    .text :
    {
        text_start = . ;
        *(.text)
        . = ALIGN(4) ;
        _etext = . ;
    } > ram

    .rom :
    {
        __kernel_image_start = ALIGN(4);
        . = . + SIZEOF(.text);
        . = ALIGN(4);
        __kernel_image_end = .;
        __data_rom_start = ALIGN(4);
    } > flash
   
    .eflash :
    {
        _flashend = . ;
    } > eflash


    .ramvec :
    {
        __ram_start = . ;
        _ramvec = . ;
    } > ramvec
   
    .data :
    {
        _sdata = . ;
        __data_start = . ;
        *(.rodata)
        . = ALIGN(4) ;
        *(.data)
        . = ALIGN(16) ;
        _edata = . ;
        edata = ALIGN( 0x10 ) ;
        __data_end = ALIGN(0x10) ;
    } > ram

    .bss :
    {
        _sbss = ALIGN( 0x10 ) ;
        __bss_start = ALIGN( 0x10 ) ;
        *(.bss)
        . = ALIGN(4);
        *(COMMON)
        . = ALIGN(16);
        _ebss = . ;
        __bss_end = . ;
        end = ALIGN( 0x10 ) ;
        _end = ALIGN( 0x10 ) ;
    } > ram

    .eram :
    {
        _boot_stack = . - 4;
        _ramend = . ;
    } > eram

    .nvram :
    {
	    _start_nvram = .;
    } > nvram

    .envram :
    {
	    _end_nvram = . ;
    } > envram
}
