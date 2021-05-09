/* Serial configuration as needed by mon960_console.h */

#define SER16552_LSR	((volatile unsigned char*)0xe0000014)	/* line status */
#define SER16552_THR	((volatile unsigned char*)0xe0000000)	/* send fifo */
#define SER16552_RBR	((volatile unsigned char*)0xe0000000)	/* rcv fifo */
#define SER16552_IER	((volatile unsigned char*)0xe0000004)	/* int enable */

#define LSR_READY		1
#define LSR_SEND_READY		0x20
#define	LSR_RECEIVE_READY	0x1
#define LSR_BREAK		(1 << 4)

#define IER_RCV_ENABLE		1
#define IER_SEND_ENABLE		2
