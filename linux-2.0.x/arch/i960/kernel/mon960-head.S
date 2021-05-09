/* mon960 head.S code: should be extremely simple, since mon960 takes care
   of almost all our requirements for us.  We're at ipl 31 (highest),
   interrupts are off, we have a C stack frame set up for us, etc.
   This is modelled after the alpha head.S  (excerpt below), which
   actually does some other cruft before calling start_kernel, but without
   knowning the alpha instruction set I can't tell for sure what's
   happening.

   For now I'll stick with just a call, as it seems to work in other
   places.

alpha head.S:
__start:
        br      $27,1f
1:      ldgp    $29,0($27)
        lda     $27,start_kernel
        jsr     $26,($27),start_kernel
        halt
        .end __start
*/


#define __ASSEMBLY__
#include <asm/system.h>

	.align 4
	.globl _stext
	.globl __start
	.ent __start

_stext:
__start:
	call start_kernel
	halt
	.end __start
