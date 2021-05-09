/************************************************************************/
/*                                                                      */
/*  mbus.h -   definitions for the mbus driver		                */
/*                                                                      */
/*                                                                      */
/*  (C) Copyright 1999, Martin Floeer (mfloeer@axcent.de)               */      
/*  (C) Copyright 2000, Tom Dolsky (tomtek@tomtek.com)                  */
/*                                                                      */
/************************************************************************/
                                                                             


#ifndef	__MBUS_H__
#define	__MBUS_H__

#include <linux/ioctl.h>



#ifndef MBUS_MAJOR
#define MBUS_MAJOR 127 		/*static definition of major nr.*/
#endif

#ifndef MBUS_NR_DEVS
#define MBUS_NR_DEVS 1
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define count_t int
#define read_write_t int

#define	MBUS_TRANSMIT 0x1
#define	MBUS_RECEIVE 0x0

#define	MBUS_MASTER 0x1
#define	MBUS_SLAVE 0x0



volatile int		MBUS_STATE;		/*  1 = mbus busy */
						/*  0 = mbus idle no errror */
						/* -1 = mbus idle at last transfer arbitration lost */
						/* -2 = mbus idle at last transfer no acknowledge receive */
#define	MBUS_STATE_BUSY				1
#define	MBUS_STATE_IDLE				0
#define	MBUS_STATE_ARBITRATION_LOST		-1
#define	MBUS_STATE_NO_ACKNOWLEDGE		-2

volatile unsigned char			MBUS_MODE;			/* true = tranceive : false = receive */
volatile unsigned char			MBUS_SUB_ADDRESS_BUFFER;	/* Buffer for the sub_address */
volatile unsigned char			MBUS_SLAVE_ADDRESS_BUFFER;
volatile unsigned char			*MBUS_DATA;			/* Pointer to data */
volatile int				MBUS_DATA_COUNT;		/* counter for data bytes */
volatile int				MBUS_DATA_NUMBER;	

void mbus_start(unsigned char address,unsigned char clock,char IRQ_LEVEL);

extern void mbus_interrupt(int irq, void *dev_id, struct pt_regs *regs);

extern void mbus_getchars(unsigned char *data, int count);

extern void mbus_putchars(unsigned char *data, int number);                                                 


typedef struct Mbus_Dev 
{
	struct Mbus_Dev *next;
	char *name;
	unsigned int usage;
} Mbus_Dev;

extern int mbus_major;
extern int mbus_nr_devs;	

extern struct file_operations mbus_fops;

read_write_t mbus_read(struct inode *inode, struct file *filp,unsigned char *buf,count_t count);
read_write_t mbus_write(struct inode *inode, struct file *filp,unsigned char *buf,count_t count);

#define MBUS_BUSY mbus_busy_event()
extern unsigned char mbus_busy_event(void);


#endif /* __MBUS_H__ */

