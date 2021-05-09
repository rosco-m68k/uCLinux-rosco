#include <linux/config.h>
/*
 *   This is the linker directive file to link an application to run
 *   under mon960 on the CY board with a Cx, Jx, or Hx processor.
 *   Programs linked with this file are downloaded to DRAM after the
 *   data area used by mon960.
 *   malloc() uses the symbols _heap_base and _heap_end to manage the heap.
 *   _heap_base is located at the end of program memory (.text + .data +.bss).
 *   _heap_size is defined as 0x20000.
 *   The C runtime stack starts at _heap_end and grows up.
 *   There must be enough room after the program memory in the dram for
 *   the program's heap and stack.
 */


MEMORY
{
#ifdef CONFIG_CYVH_4MB
    data:       o=0xA3C08000,l=0x7f8000  /* for 4 MB dram on eval board */
#endif
#ifdef CONFIG_CYVH_8MB
    data:       o=0xA3808000,l=0xff8000  /* for 8 MB dram on eval board */
#endif
#ifdef CONFIG_CYVH_16MB
    data:       o=0xA3008000,l=0x1ff8000 /* for 16 MB dram on eval board */
#endif
#ifdef CONFIG_CYVH_32MB
    data:       o=0xA2008000,l=0x3ff8000 /* for 32 MB dram on eval board */
#endif
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

