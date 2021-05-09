/*     include/asm- sh/crt0.s
*
*
* Copyright (C) 2002  Ushustech pvt ltd www.ushustech.com
*
*
* 22-08-2002 written by BINOJ.G.S
*
*
*
;********************************************************************************************************/

.file "shcrt0.s"


.equ SYSTEM_STACK_SIZE, 2048            /* Number of bytes in system stack*/

.equ INITIAL_SR,  0xF0                  /* Initial SR interrupts masked*/

.equ INITIAL_GBR, 0x0

.equ INITIAL_VBR,  0x06000000

.equ VEC_TABLE_LENGTH, 0x03fc           /* Length of vector table*/

;/************************ FMR **************************************************************************/
.equ FMR,    0xFFFFFE90
.equ INIT_FMR, 0xAA
;/************************ INTC *************************************************************************/
.equ  IPRA,  0xFFFFFEE2       /* Interrupt priority register where*/
.equ  IPRB,  0xFFFFFE60       /* Interrupt priority register where*/
.equ  IPRD,  0xFFFFFE40       /* Interrupt priority register where*/
.equ  IPRE,  0xFFFFFEC0       /* Interrupt priority register where*/
.equ  VCRA,  0xFFFFFE62       /* Address of VCRA*/
.equ  VCRC,  0xFFFFFE66       /* Address of VCRC*/
.equ  VCRD,  0xFFFFFE68       /* Address of VCRD*/
.equ  VCRL,  0xFFFFFE50       /* Address of VCRL*/
.equ  VCRM,  0xFFFFFE52       /* Address of VCRM*/
.equ  VCRN,  0xFFFFFE54       /* Address of VCRN*/
.equ  VCRO,  0xFFFFFE56       /* Address of VCRO*/
.equ  VCRWDT,0xFFFFFEE4       /* Address of VCRWDT*/

.equ INIT_IPRA,  0x00c0
.equ INIT_IPRB,  0x6D00     /* FRT priority D*/
.equ INIT_IPRD,  0x000A
.equ INIT_IPRE,  0xA000
.equ INIT_VCRA,  0x4f00
.equ INIT_VCRC,  0x504D
.equ INIT_VCRD,  0x0000
.equ INIT_VCRL,  0x4C4D
.equ INIT_VCRM , 0x4E4E
.equ INIT_VCRN,  0x5455
.equ INIT_VCRO,  0x5656
.equ INIT_VCRWDT,0x4600/**Posh2 vector no of wdt 70 for time being*/

;/************************ FRT ***************************************************************************/

.equ TIER,          0xFFFFFE10      /*Timer interrupt enable reg*/
.equ FTCSR,         0xFFFFFE11      /*Timer control / status reg*/
.equ FRC_H,         0xFFFFFE12      /*Free running counter High*/
.equ FRC_L,         0xFFFFFE13      /* Free running counter Low*/
.equ OCRA_H,        0xFFFFFE14      /*Output compare reg A High*/
.equ OCRA_L,        0xFFFFFE15      /* Output compare reg A Low*/
.equ TCR,           0xFFFFFE16      /*Timer Control Register*/
.equ TOCR,          0xFFFFFE17      /*Timer output compare control reg*/


.equ INIT_TIER,      0x09            /*Enable timer interrupt A*/
.equ INIT_FTCSR,     0x01            /*Clear counter and int flag*/
.equ INIT_FRC_H,     0x00            /* Counter values*/
.equ INIT_FRC_L,     0x00
.equ INIT_OCRA_H,    0x07            /* Compare values*/
.equ INIT_OCRA_L,    0xA1
.equ INIT_TCR,       0x02            /* Count at clock/128*/
.equ INIT_TOCR,      0xE0

.equ CLR_FTCSR,      0x01            /*Clear counter and int flag*/

;/********************************************************************* WDT *******************************/
.equ WTCSR_CNT,         0xFFFFFE80
.equ RSTCSR,            0xFFFFFE82

.equ INIT_WTCSR,       0xA51F   /*WDT Disable*/
.equ INIT_WTCNT,       0x5A00
.equ INIT_RSTCSR,      0x5ADF 

;/************************ BSC ***************************************************************************/
.equ BCR1,                     0xFFFFFFE0
.equ BCR2,                     0xFFFFFFE4
.equ BCR3,                     0xFFFFFFFC
.equ WCR1,                     0xFFFFFFE8
.equ WCR2,                     0xFFFFFFC0
.equ WCR3,                     0xFFFFFFC4
.equ MCR,                      0xFFFFFFEC
.equ RTCSR,                    0xFFFFFFF0
.equ RTCNT,                    0xFFFFFFF4
.equ RTCOR,                    0xFFFFFFF8

.equ  INIT_BCR1,               0xA55A4031
.equ  INIT_BCR2,               0xA55A00F8
.equ  INIT_BCR3,               0xA55A0100

.equ  INIT_WCR1,               0xA55A55DF

.equ  INIT_WCR2,               0xA55A0B06
.equ  INIT_WCR3,               0xA55A0000

.equ  INIT_MCR,                0xA55AC1C8

.equ  INIT_RTCSR,              0xA55A0090

.equ  INIT_RTCOR,              0xA55A0019

;/******************************************************************* SDRAM ******************************/


 .equ  MRS,                 0xFFFF8880

 .equ   INIT_MRS,           0xFFFF0880


;/*********************** CCR ***************************************************************************/

.equ CCR,                     0xFFFFFE92      /* Cash Control reg*/
.equ INIT_CCR,                0x0000

;/*********************** PFC ***************************************************************************/
.equ PBCR,                    0xFFFFFC88
.equ PBCR2,                   0xFFFFFC8E
.equ PBIOR,                   0xFFFFFC8A



.equ  INIT_PBCR,              0x2A80

.equ  INIT_PBCR2,             0x0A00
.equ  INIT_PBIOR,             0x0000

.equ  PBDR,                    0xFFFFFC8C

.equ  INIT_PBDR,               0x0000

;/********************************************************************************************************/


;/********************************************************************************************************/

/*bss area*/


       .section .bss

       .align 4


/* Define the system stack  and first available free
   memory.  */

          .SECTION   .stack

          .align    4

/* Define the system stack area.  */

          .global     _System_SP_Area
_System_SP_Area:
          .space     SYSTEM_STACK_SIZE
          .align    4
          .global     _System_SP_Start
/*_System_SP_Start:
          .space    4*/



;/**********************************************************************************************************/
          .text

          .global     _start
 _start:

    /* Setup system registers.  */

          MOV.L       INT1_SR,r2            /* Pickup initial SR*/
          LDC         r2,sr                 /* Lockout interrupts */

          MOV.L       INT1_GBR,r2           /* Pickup initial GBR */
          LDC         r2,gbr                /* Setup GBR register*/
	
	 MOV.L	      INT1_VBR, r2
	 LDC	      r2,vbr
;


/*BSC*/


          MOV.L   INT1_BCR1,r0
          MOV.L   INT1_BCR1_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_BCR2,r0
          MOV.L   INT1_BCR2_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_BCR3,r0
          MOV.L   INT1_BCR3_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_WCR1,r0
          MOV.L   INT1_WCR1_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_WCR2,r0
          MOV.L   INT1_WCR2_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_WCR3,r0
          MOV.L   INT1_WCR3_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_MCR,r0
          MOV.L   INT1_MCR_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_RTCSR,r0
          MOV.L   INT1_RTCSR_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_RTCOR,r0
          MOV.L   INT1_RTCOR_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_MRS,r0
          MOV.L   INT1_MRS_INIT,r1
          MOV.W   r1,@r0

          MOV.L   INT1_RTCSR,r0
          MOV.L   INT1_RTCSR_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_RTCOR,r0
          MOV.L   INT1_RTCOR_INIT,r1
          MOV.L   r1,@r0

          MOV.L   INT1_MRS,r0
          MOV.L   INT1_MRS_INIT,r1
          MOV.W   r1,@r0


    /* Setup system stack pointer.  */

          MOV.L       INT1_System_SP_Start,r15



/* The following  assignments are for the serial port.*/

         MOV.L   INT1_VCRA,r3                /*Address of VCRA*/
         MOV.W   INT1_VCRA_INIT,r4                /*Vector #'s for VCRA*/
         MOV.W   r4,@r3

         MOV.L    INT1_VCRC,r3               /* Address of VCRC */
         MOV.W    INT1_VCRC_INIT,r4               /* Vector #'s for VCRC */
         MOV.W    r4,@r3

         MOV.L    INT1_VCRD,r3               /* Address of VCRD*/
         MOV.W    INT1_VCRD_INIT,r4               /* Vector #'s for VCRD*/
         MOV.W    r4,@r3                     /* this is the FR timer*/

         MOV.L    INT1_VCRL,r3               /* Address of VCRL*/
         MOV.W    INT1_VCRL_INIT,r4               /*Vector # for VCRL*/
         MOV.W    r4,@r3

         MOV.L    INT1_VCRM,r3               /*Address of VCRM*/
         MOV.W    INT1_VCRM_INIT,r4               /* Vector # for VCRM*/
         MOV.W    r4,@r3

         MOV.L    INT1_VCRN,r3               /*Address of VCRN*/
         MOV.W    INT1_VCRN_INIT,r4          /*   Vector # for VCRN */
         MOV.W    r4,@r3

         MOV.L    INT1_VCRO,r3              /* Address of VCRO */
         MOV.W    INT1_VCRO_INIT,r4              /* Vector # for VCRO */
         MOV.W    r4,@r3

         MOV.L    INT1_VCRWDT,r3            /*Address of VCRWDT */
         MOV.W    INT1_VCRWDT_INIT,r4       /*Vector #'s for VCRWDT*/
         MOV.W    r4,@r3

	 MOV.L    INT1_VCRL,r3                                        /* this is the WDT*/
         MOV.W    INT1_VCRL_INIT,r4               /*Vector # for VCRL*/
         MOV.W    r4,@r3

         MOV.L    INT1_VCRM,r3               /*Address of VCRM*/
         MOV.W    INT1_VCRM_INIT,r4               /* Vector # for VCRM*/
         MOV.W    r4,@r3

         MOV.L    INT1_VCRN,r3               /*Address of VCRN*/
         MOV.W    INT1_VCRN_INIT,r4          /*   Vector # for VCRN */
         MOV.W    r4,@r3

         MOV.L    INT1_VCRO,r3              /* Address of VCRO */
         MOV.W    INT1_VCRO_INIT,r4              /* Vector # for VCRO */
         MOV.W    r4,@r3

         MOV.L    INT1_VCRWDT,r3            /*Address of VCRWDT */
         MOV.W    INT1_VCRWDT_INIT,r4       /*Vector #'s for VCRWDT*/
         MOV.W    r4,@r3                    /* this is the WDT*/


    /* This is normally where the timer interrupt source is initialized.
        Here we use the FRT, but the Watchdog Timer in "interval mode" could
        be used instead.  */

        MOV.L   INT1_IPRA,r0                   /*Set priority of FRT to 12
        MOV.W   INT1_IPRA_INIT,r1               /* and the serial priority to 15 */
        MOV.W   r1,@r0                         /*they share the IPRA */

        MOV.L   INT1_IPRB,r0                   /*Set priority of FRT to D */
        MOV.W   INT1_IPRB_INIT,r1                   /* and the serial priority to 15 */
        MOV.W   r1,@r0                         /* they share the IPRB */

        MOV.L   INT1_IPRD,r0                   /*Set priority of FRT to 12 */
        MOV.W   INT1_IPRD_INIT,r1                   /* and the serial priority to 15 */
        MOV.W   r1,@r0                         /*they share the IPRD */

        MOV.L   INT1_IPRE,r0                   /* Set priority of FRT to 12*/
        MOV.W   INT1_IPRE_INIT,r1                   /*and the serial priority to 15 */
        MOV.W   r1,@r0                         /*they share the IPRE */

        MOV.L   INT1_TOCR,r0                   /* set to output 0 on match A */
        MOV.W   INT1_TOCR_INIT,r1
        MOV.B   r1,@r0


        MOV.L   INT1_PBCR,r0                   /* PFC*/
        MOV.W   INT1_PBCR_INIT,r1
        MOV.W   r1,@r0

        MOV.L   INT1_PBCR2,r0                  /*PFC*/
        MOV.W   INT1_PBCR2_INIT,r1
        MOV.W   r1,@r0

	 MOV.L   INT1_PBIOR,r0                  /*PFC*/
        MOV.W   INT1_PBIOR_INIT,r1
        MOV.W   r1,@r0

        MOV.L   INT1_PBDR,r0                    /*PFC*/
        MOV.W   INT1_PBDR_INIT,r1
        MOV.W   r1,@r0


       /* Watch Dog timer Init */

      /*  MOV.L   INT1_WTCSR_CNT,r0
        MOV.W   INT1_WTCSR_INIT,r1
        MOV.W   r1,@r0

        MOV.L   INT1_RSTCSR,r0
        MOV.W   INT1_RSTCSR_INIT,r1
        MOV.W   r1,@r0 */


    /* Cache control  */


         MOV.L   INT1_CCR,r1
         MOV.B   @r1,r0

         AND     #0xFE,r0
         MOV.B   r0,@r1

         OR      #0x06,r0
         MOV.B   r0,@r1

         OR      #0x10,r0
         MOV.B   r0,@r1

         OR      #0x01,r0
         MOV.B   r0,@r1

/*********************************************************************************************************/

mov.l _bss_start,r7
mov.l _bss_end,r8
1:
mov #0,r0
mov.l r0,@r7
mov #4,r0
add r0,r7
cmp/hi r8,r7
bf 1b
nop

/*Posh2 mov.l  isrno,r0
shll2	r0
mov.l	vecno,r3
stc	vbr,r4
mov.l	r3,@(r0,r4)
*/
 MOV.L label_start,r1
 JMP @r1
 NOP




/*********************************************************************************************************/
.ALIGN 4

label_start: .long _start_kernel 
vecno:       .long     _intwdt	
isrno:	     .long	68
_bss_start:   .long bss_begin
_bss_end:     .long bss_end

INT1_System_SP_Start:

          .long     _System_SP_Start
.align 4
INT1_SR:
          .long     INITIAL_SR
INT1_GBR:
          .long     INITIAL_GBR
INT1_VBR:
	  .long	    INITIAL_VBR

;    /* Define the data section used by the INT_Initialize function.  This
;       section is necessary because the fixed-size, 16-bit instructions
;       have to build 32/16 bit values with PC-relative references.  */
;
          .ALIGN      4

INT1_VCRA_INIT:
          .word     INIT_VCRA
INT1_VCRC_INIT:
          .word     INIT_VCRC
INT1_VCRD_INIT:
          .word     INIT_VCRD
INT1_VCRL_INIT:
          .word     INIT_VCRL
INT1_VCRM_INIT:
          .word     INIT_VCRM
INT1_VCRN_INIT:
          .word     INIT_VCRN
INT1_VCRO_INIT:
          .word     INIT_VCRO
INT1_VCRWDT_INIT:
          .word     INIT_VCRWDT
INT1_IPRA_INIT:
          .word     INIT_IPRA
INT1_IPRB_INIT:
          .word     INIT_IPRB
INT1_IPRD_INIT:
          .word     INIT_IPRD
INT1_IPRE_INIT:
          .word     INIT_IPRE
INT1_TCR_INIT:
          .word     INIT_TCR
INT1_TOCR_INIT:
          .word     INIT_TOCR
INT1_OCRA_H_INIT:
          .word     INIT_OCRA_H
INT1_OCRA_L_INIT:
          .word     INIT_OCRA_L
INT1_FTCSR_INIT:
          .word     INIT_FTCSR
INT1_FRC_H_INIT:
          .word     INIT_FRC_H
INT1_FRC_L_INIT:
          .word     INIT_FRC_L
INT1_TIER_INIT:
          .word     INIT_TIER
INT1_WTCSR_INIT:
          .word     INIT_WTCSR
INT1_RSTCSR_INIT:
          .word     INIT_RSTCSR
INT1_PBCR_INIT:
          .word     INIT_PBCR
INT1_PBCR2_INIT:
          .word     INIT_PBCR2
INT1_PBIOR_INIT:
          .word     INIT_PBIOR
INT1_PBDR_INIT:
          .word     INIT_PBDR
.align 4

INT1_BCR1:       .long    BCR1
INT1_BCR2:       .long    BCR2
INT1_BCR3:       .long    BCR3
INT1_WCR1:       .long    WCR1
INT1_WCR2:       .long    WCR2
INT1_WCR3:       .long    WCR3
INT1_MCR:        .long    MCR
INT1_RTCSR:      .long    RTCSR
INT1_RTCNT:      .long    RTCNT
INT1_RTCOR:      .long    RTCOR
INT1_CCR:        .long    CCR
INT1_MRS:        .long    MRS

INT1_BCR1_INIT:  .long    INIT_BCR1
INT1_BCR2_INIT:  .long    INIT_BCR2
INT1_BCR3_INIT:  .long    INIT_BCR3
INT1_WCR1_INIT:  .long    INIT_WCR1
INT1_WCR2_INIT:  .long    INIT_WCR2
INT1_WCR3_INIT:  .long    INIT_WCR3
INT1_MCR_INIT:   .long    INIT_MCR
INT1_RTCSR_INIT: .long    INIT_RTCSR
INT1_RTCOR_INIT: .long    INIT_RTCOR
INT1_MRS_INIT:   .long    INIT_MRS


.align 4



INT1_IPRA:
          .long     IPRA
INT1_IPRB:
          .long     IPRB
INT1_IPRD:
          .long     IPRD
INT1_IPRE:
          .long     IPRE
INT1_TCR:
          .long     TCR
INT1_TOCR:
          .long     TOCR
INT1_OCRA_H:
          .long     OCRA_H
INT1_OCRA_L:
          .long     OCRA_L
INT1_FTCSR:
          .long     FTCSR
INT1_FRC_H:
          .long     FRC_H
INT1_FRC_L:
          .long     FRC_L
INT1_TIER:
          .long     TIER
INT1_WTCSR_CNT:
          .long     WTCSR_CNT
INT1_RSTCSR:
          .long     RSTCSR

INT1_VCRA:
          .long     VCRA

INT1_VCRC:
          .long     VCRC
INT1_VCRD:
          .long     VCRD
INT1_VCRL:
          .long     VCRL
INT1_VCRM:
          .long     VCRM
INT1_VCRN:
          .long     VCRN
INT1_VCRO:
          .long     VCRO
INT1_VCRWDT:
          .long     VCRWDT
INT1_PBCR:
          .long     PBCR
INT1_PBCR2:
          .long     PBCR2
INT1_PBIOR:
          .long     PBIOR
INT1_PBDR:
          .long     PBDR

.global BEGIN_BSS

   BEGIN_BSS :  .long bss_begin

  .global END_BSS

   END_BSS   :  .long bss_end

  .global BEGIN_ROMDATA

   BEGIN_ROMDATA : .long romdata_begin

  .global  END_ROMDATA

   END_ROMDATA   : .long romdata_end

  .global BEGIN_DATA

   BEGIN_DATA     : .long data_begin

  .global BEGIN_ROMVECT

   BEGIN_ROMVECT : .long romvect_begin

  .global END_ROMVECT

   END_ROMVECT    : .long romvect_end

  .global BEGIN_VECTORS

   BEGIN_VECTORS : .long begin_vectors







	
