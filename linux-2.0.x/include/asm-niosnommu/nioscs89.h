/* $Id: nioscs89.h,v 1.1 2003/02/26 04:13:07 gerg Exp $ */

#ifndef __NIOS_nioscs89_H__
  #define __NIOS_nioscs89_H__


    #include <asm/nios.h>

    #define NET_BASE             ((int) (na_enet))       // eg: 0x00004000

    #define nioscs89_pin_addr    ((int) (na_enet_reset)) // eg: 0x00004020
        #define nioscs89_reset      0x00000001
        #define nioscs89_activate   0x00000000
//    #define nioscs89_oset_pin_addr          0x1800
//        #define nioscs89_pins_off       0x001c
//        #define nioscs89_pins_bhe       0x001d
//        #define nioscs89_pins_reset     0x011c
//      // ?other examples use 0x18F0
//      //   (and "clearsbhe" value   0x0000
//      //        "setsbhe"   value   0x0001
//      //        "reset"     value   0x0002)

    // mem space goes from + 0x0000 to + 0x0FFF

    #define nioscs89_oset_io_base   0
/*  #define nioscs89_oset_io_base   (0x1000  +              \           */
/*                                   0x0300)             //     0x1300  */
      // io space is only 8x4=32 (0x20) bytes deep
/*    // io space is only 8x2=16 (0x10) bytes deep                      */
      //   NET_BASE + nioscs89_oset_io_base +
      //     I/O Port #00 Rx/Tx Data Port 0      // eg: 0x00004000
/*    //     I/O Port #00 Rx/Tx Data Port 0      // eg: 0x00005300      */
/*    //            (also available in mem mode                         */
/*    //              at na_enet +                                      */
/*    //                0x0400:    Rx frame sts  // eg: 0x00004400      */
/*    //                0x0402:    Rx frame len  // eg: 0x00004402      */
/*    //                0x0404...: Rx frame dat  // eg: 0x00004404...   */
/*    //                0x0A00...: Tx frame dat) // eg: 0x00004A00...   */
      //       04 Rx/Tx Data Port 1              //     0x00004004
/*    //       02 Rx/Tx Data Port 1              //     0x00005302      */
      //       08 TxCmd Port                     //     0x00004008
/*    //       04 TxCmd Port                     //     0x00005304      */
/*    //            (also available in mem mode                         */
/*    //              at na_enet + 0x0144)                              */
      //       0C TxLength Port                  //     0x0000400C
/*    //       06 TxLength Port                  //     0x00005306      */
/*    //            (also available in mem mode                         */
/*    //              at na_enet + 0x0146)                              */
      //       10 ISQ (Register R0)              //     0x00004010
/*    //       08 ISQ (Register R0)              //     0x00005308      */
/*    //            (also available in mem mode                         */
/*    //              at na_enet + 0x0120)                              */
      //       14 PP Ptr  Port                   //     0x00004014
/*    //       0A PP Ptr  Port                   //     0x0000530A      */
      //       18 PP Data Port 0                 //     0x00004018
/*    //       0C PP Data Port 0                 //     0x0000530C      */
      //       1C PP Data Port 1                 //     0x0000401C
/*    //       0E PP Data Port 1                 //     0x0000530E      */


    #define IRQ5_IRQ_NUM            na_enet_irq


    /* ...fixme...use "jiffies" countdown?...                           */
    /*                                                                  */
    #define nioscs89_sleep(n)                               \
      {                                                     \
        volatile unsigned int v;                            \
        for(v=0;v<(n); v++) v=v;                            \
      }
      /*                                                                */
      /* Approx 10*n gcc -O2 instructions                               */
      /*                    (excluding "setup/teardown")    ???         */
      /*    300*n  nanoseconds @ 33 mhz                                 */
      /*   n:      100    30 microseconds                               */
      /*         3,333     1 millisecond                                */
      /*        26,666     8 milliseconds                               */
      /*       100,000    30 milliseconds                               */
      /*       333,333   100 milliseconds                               */
      /*  ...31Jul2001...dgt...datascope possibly indicated we're       */
      /*  ... sleeping about 6 times too long...?...                    */
      /*                                                                */
        #define nioscs89_sleep_1_milsec                     3333
        /*                                                              */
        #define nioscs89_sleep_8_milsecs                    \
                    ((nioscs89_sleep_1_milsec) * 8)
        /*                                                              */
        #define nioscs89_sleep_100_milsecs                  \
                    ((nioscs89_sleep_1_milsec) * 100)

/*    #define nioscs89_disconnect_irq(dev)                    \         */
/*                                                            \         */
/*                *((volatile unsigned int *)                 \         */
/*                     (((unsigned char *)                    \         */
/*                          (dev->base_addr)) +               \         */
/*                          nioscs89_oset_pin_addr)) = 0x0000           */

/*    #define nioscs89_connect_irq(dev)                       \         */
/*                                                            \         */
/*                *((volatile unsigned int *)                 \         */
/*                     (((unsigned char *)                    \         */
/*                          (dev->base_addr)) +               \         */
/*                          nioscs89_oset_pin_addr)) = 0x0004           */
//    /* Note 0x0004 included in nioscs89_pins_off   (0x001c)           */
//    /*                         nioscs89_pins_bhe   (0x001d)           */
//    /*                     and nioscs89_pins_reset (0x011c)           */

    /* Set up access to a (16 bit) register for                         */
    /*  subequent I/O operation.                                        */
    /*                                                                  */
    #define nioscs89_set_ioptr(mem_base_addr, regoset)      \
                                                            \
                *((volatile unsigned short *)               \
                     (((unsigned char *)                    \
                          mem_base_addr)        +           \
                      nioscs89_oset_io_base     +           \
                      ADD_PORT)) =                          \
                         (volatile unsigned short)          \
                            (regoset)

    /* Write a (16 bit) register using I/O operations                   */
    /* Access thereto must have already been set up                     */
    /*  (nioscs89_set_ioptr).                                           */
    /*                                                                  */
    #define nioscs89_set_iod16(mem_base_addr, value)        \
                                                            \
                *((volatile unsigned short *)               \
                     (((unsigned char *)                    \
                          mem_base_addr)        +           \
                      nioscs89_oset_io_base     +           \
                      DATA_PORT)) =                         \
                         (volatile unsigned short)          \
                            (value)

    /* Write a (16 bit) register using I/O operations                   */
    /*                                                                  */
    #define nioscs89_iow16(mem_base_addr, regoset, value)   \
                                                            \
                nioscs89_set_ioptr(mem_base_addr,           \
                                   regoset);                \
                nioscs89_set_iod16(mem_base_addr,           \
                                   value)


#endif /* __NIOS_nioscs89_H__ */
