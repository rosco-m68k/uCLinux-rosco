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
      dram  :  org = 0xA3C08000, len = 0x1ff8000   /* 4M less monitor */
      isram :  org = 0x00000100, len = 0x300
   }


SECTIONS
   {
      .text :
         {
         } >dram

      .data :
         {
         } >dram

      .bss : 
         {
         } >dram


/* For R4.0 of the compiler & libraries, the following lines */
/* should not be enclosed in comments. */
/*
        SFP_AC :
        {
           fpem_CA_AC = .;
        } >isram
*/
   }

/* Bounds of heap: */
/* The heap may be placed in a separate memory region, if desired. */

/* _stackbase marks the base of the kernel's initial stack. We put it way at
 * the top of memory. */
__stext = __Btext;
__etext = __Etext;
__sdata = __Bdata;
__edata = __Edata;
__end = _end;
_stackbase = 0xa3ff0000;

ENTRY(_main)
/* Don't use low-level libs anymore */
/*
STARTUP ("crt960*")
HLL ()
SYSLIB ("libmn*")
SYSLIB ("libll*")
*/
FLOAT
