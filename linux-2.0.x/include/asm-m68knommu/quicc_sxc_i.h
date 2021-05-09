/*
 *  Internal constants used for both SMCs and SCCs on the 68360's CPM.
 *
 *  Copyright (c) 2002 SED Systems, a Division of Calian Ltd.
 *      <hamilton@sedsystems.ca>
 */

#ifndef __QUICC_SXC_I_H__
#define __QUICC_SXC_I_H__

#include <linux/config.h>
#ifdef __KERNEL__
#if defined(CONFIG_M68360)

/* Constants used for UART buffers and buffer descriptors. */
enum { NUMBER_TX_BUFFERS = 4 };
enum { NUMBER_RX_BUFFERS = 4 };
enum { BUFFER_SIZE = 512 };
/* Constants used for Ethernet buffer and buffer descriptors. */
enum { NUMBER_ENET_TX_BUFFERS = 64 };
enum { NUMBER_ENET_RX_BUFFERS = 64 };
enum { TX_RING_MOD_MASK       = 63 };

struct serial_buffer_t
{
    unsigned int        current_character;
    char                buffer[BUFFER_SIZE] __attribute__ ((aligned(4)));
};

enum { DEFAULT_CONSOLE_DATARATE = 19200 };
enum { DEFAULT_SERIAL_CONSOLE   = 0 };
/*
 * Constants used for SxC receive buffer descriptor control fields
 * The SCC has a couple more but they are used in multidrop uarts and not
 * needed
 */
#define RX_BD_EMPTY             ((unsigned short)(0x0001 << 15))
#define RX_BD_WRAP              ((unsigned short)(0x0001 << 13))
#define RX_BD_INTERRUPT         ((unsigned short)(0x0001 << 12))
#define RX_BD_CONTINUOUS_MODE   ((unsigned short)(0x0001 <<  9))
#define RX_BD_CONTAINS_IDLES    ((unsigned short)(0x0001 <<  8))
#define RX_BD_RXED_BREAKS       ((unsigned short)(0x0001 <<  5))
#define RX_BD_FRAMING_ERROR     ((unsigned short)(0x0001 <<  4))
#define RX_BD_PARITY_ERROR      ((unsigned short)(0x0001 <<  3))
#define RX_BD_OVERRUN           ((unsigned short)(0x0001 <<  1))
#define RX_BD_CARRIER_LOST      ((unsigned short)(0x0001 <<  0))

/*
 *  Constants used for SxC transmit buffer descriptor control fields
 *  The SCC has a couple more but they are used in multidrop uarts and CTS/RTS
 *  flow control and not needed
 */
#define TX_BD_FULL              ((unsigned short)(0x0001 << 15))
#define TX_BD_WRAP              ((unsigned short)(0x0001 << 13))
#define TX_BD_INTERRUPT         ((unsigned short)(0x0001 << 12))
#define TX_BD_CONTINUOUS_MODE   ((unsigned short)(0x0001 <<  9))
#define TX_BD_PREAMBLE          ((unsigned short)(0x0001 <<  8))

/*
 *      Define a local serial stats structure
 */
struct quicc_serial_stats
{
        unsigned int rx;
        unsigned int tx;
        unsigned int rxbreak;
        unsigned int rxframing;
        unsigned int rxparity;
        unsigned int rxoverrun;
};

enum { SXC_MAX_IDL = 10 };
/*
 *  I picked this value. It tells the serial port how long to wait receiving
 *   nothing after receiving a character before closing the receive buffer.
 *   This is the number of character lengths of idle line. The amount of time
 *   this corresponds to varies with the baudrate. See 7-278 and 7-156 of the
 *    manual.
 */

#endif
#endif
#endif
