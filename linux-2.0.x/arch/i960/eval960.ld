/*----------------------------------------------------------*/
/*              Linker Directive File (init_jx.ld)             */
/*----------------------------------------------------------*/
MEMORY
{
    data:       o=0xA3C00000,l=0x40000   /* for 4 MB dram on eval board */
/*    data:       o=0xA3800000,l=0x80000    for 8 MB dram on eval board */
/*    data:       o=0xA3000000,l=0x100000    for 16 MB dram on eval board */
/*    data:       o=0xA2C00000,l=0x200000    for 32 MB dram on eval board */
}

SECTIONS
{
  .text :
	{
	  *(.text)
	  .=ALIGN(0x10);
	} > data
	.data :
	{
	} > data
	.bss :
	{
	} > data
}

