/************************************************************************/
/*                                                                      */
/*  mbus.c - functions for the mbus driver                              */
/*                                                                      */
/*  (C) Copyright 1999, Martin Floeer (mfloeer@axcent.de)               */
/*  (C) Copyright 2000, Tom Dolsky (tomtek@tomtek.com)                  */
/*                                                                      */
/*                                                                      */
/*TODO:no slave interupt support                                        */
/*TODO:clear all flags on init-Done????                                 */
/*TODO:timeout for busy wait-Done???                                    */
/*TODO:ioctl for remaining char count                                   */
/*TODO:Double buffer on open                                            */
/*                                                                      */
/************************************************************************/



#ifndef __KERNEL__
#define __KERNEL__
#endif
#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <asm/coldfire.h>
#include <asm/types.h>
#include <asm/mcfsim.h>
#include <asm/mcfmbus.h>

#include <asm/system.h>
#include <asm/segment.h>

#include "mbus.h"

#define BUFFER_SIZE 128

//#define debug_flow 1 		/*enabling debugging puts a long delay between bytes transferred*/
//#define debug 1 		/*enabling debugging puts a long delay between bytes transferred*/
//#define debug_int 1 		/*enabling debugging puts a long delay between bytes transferred*/
//#define debug_int_rd 1 		/*enabling debugging puts a long delay between bytes transferred*/
//#define debug_ioctl 1 		/*enabling debugging puts a long delay between bytes transferred*/

int mbus_major = MBUS_MAJOR;
int mbus_nr_devs = MBUS_NR_DEVS;

Mbus_Dev *mbus_devices;

unsigned char doublebuf[BUFFER_SIZE];

volatile char *mbus_base=(char*) (MCF_MBAR + MCFMBUS_BASE); 

static unsigned char mbus_idle(void)
{
	int n = 1000000;	
while(n-- && (*(mbus_base + MCFMBUS_MBSR) & MCFMBUS_MBSR_MBB)); 


	if(n == 0x00)				/*if n == 0 then a timeout occured*/
		return 0;
	else
		return 1;
}

static void mbus_clear_interrupt(void)
{
	*(mbus_base + MCFMBUS_MBSR) = *(mbus_base + MCFMBUS_MBSR) & ~MCFMBUS_MBSR_MIF;
}	


static unsigned char mbus_arbitration_lost(void)
{
	unsigned char temp;
temp = *(mbus_base + MCFMBUS_MBSR);		/*multiple reads on MBSR was causing unexpected results*/
	if(temp & MCFMBUS_MBSR_MAL) 
		{
		*(mbus_base + MCFMBUS_MBSR) = *(mbus_base + MCFMBUS_MBSR) & ~MCFMBUS_MBSR_MAL;	/*this should clear the flag*/
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_arbitration_lost() MBSR = 0x%02x\n",__FILE__, __LINE__,temp);
#endif
		return 1;
		}
	return 0;
}

static unsigned char mbus_received_acknowledge(void)
{
	unsigned char temp;
	temp = *(mbus_base + MCFMBUS_MBSR);		/*multiple reads on MBSR was causing unexpected results*/
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_received_acknowledge() MBSR = 0x%02x\n",__FILE__, __LINE__,temp);
#endif
	if(temp & MCFMBUS_MBSR_RXAK) 
		{
		return 0x00;			/*this means no ack*/
		}
	return 0x01;				/*this means I got an ack*/
}


static unsigned int mbus_set_address(unsigned char address)
{
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_set_address(0x%02x)\n",__FILE__, __LINE__,address);
#endif
	if(address)
 		{
		*(mbus_base + MCFMBUS_MADR) = MCFMBUS_MADR_ADDR(address);
		return 0;
		}
	else 
		{
		return *(mbus_base + MCFMBUS_MADR);
		}
}


static unsigned int mbus_set_clock(unsigned char clock)
{
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_set_clock(0x%02x)\n",__FILE__, __LINE__,clock);
#endif
	if(clock) 
		{
		*(mbus_base + MCFMBUS_MFDR) = clock;
		return 0;
		}
	else
 		{
		return  *(mbus_base + MCFMBUS_MFDR);
		}
}

static void mbus_set_direction(unsigned char state)
{
	if(state == MBUS_TRANSMIT)
		{
#ifdef debug_flow
		 printk(KERN_ERR "%s(%d): mbus_set_direction(MBUS_TRANSMIT)\n",__FILE__, __LINE__);
#endif
		*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) | MCFMBUS_MBCR_MTX;	/* transmit mode */
		*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) & ~MCFMBUS_MBCR_TXAK;	/* clear TXAK always start out this way*/
		}
	else 
		{
#ifdef debug_flow
		printk(KERN_ERR "%s(%d): mbus_set_direction(MBUS_RECEIVE)\n",__FILE__, __LINE__);
#endif
		*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) & ~MCFMBUS_MBCR_MTX;	/* receive mode */
		*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) & ~MCFMBUS_MBCR_TXAK;	/* clear TXAK always start out this way*/
		}
}


static void mbus_set_mode(unsigned char state)
{
	if(state == MBUS_MASTER)
		{			
#ifdef debug_flow
		printk(KERN_ERR "%s(%d): mbus_set_mode(MBUS_MASTER)\n",__FILE__, __LINE__);
#endif
		*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) | MCFMBUS_MBCR_MSTA;	/* master mode */
		}
	else 
		{
#ifdef debug_flow
		printk(KERN_ERR "%s(%d): mbus_set_mode(MBUS_SLAVE)\n",__FILE__, __LINE__);
#endif
		*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) & ~MCFMBUS_MBCR_MSTA;	/* slave mode */
		MBUS_STATE = MBUS_STATE_IDLE;																		/*must be idle after stop*/		
		}
}

static int mbus_get_mode(void)
{

	if((*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) & MCFMBUS_MBCR_MSTA))
		return MBUS_MASTER;
	else
		return MBUS_SLAVE;
}

static int mbus_error(void)
{
	int error = 0;
	unsigned char temp;
temp = *(mbus_base + MCFMBUS_MBSR);		/*multiple reads on MBSR was causing unexpected results*/
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_error() MBSR = 0x%02x\n",__FILE__, __LINE__,temp);
#endif
	if(temp & MCFMBUS_MBSR_MAL) 
		{
		*(mbus_base + MCFMBUS_MBSR) = *(mbus_base + MCFMBUS_MBSR) & ~MCFMBUS_MBSR_MAL;	/*this should clear the flag*/
		error = error + -1;
		}
	if(temp & MCFMBUS_MBSR_RXAK) 
		{
		error = error + -2;
		}
	return error;
}

void mbus_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	mbus_clear_interrupt();
#ifdef debug_int
	printk(KERN_ERR "int C=%d\n",MBUS_DATA_COUNT);
#endif	
	switch(mbus_error())
		{
		case -3:
			mbus_set_mode(MBUS_SLAVE);							/* generate stop signal */
			printk(KERN_ERR "%s(%d): mbus_interrupt() Bus Arbitration Lost and No ack from slave 0x%02x\n",__FILE__, __LINE__,MBUS_SLAVE_ADDRESS_BUFFER);
			return;
			break;
		case -2:
			if(!((MBUS_MODE == 0x0) && (MBUS_DATA_COUNT == MBUS_DATA_NUMBER)))		/*this is a hack but I don't know how to fix*/
				{
				printk(KERN_ERR "%s(%d): mbus_interrupt() No ack from slave 0x%02x\n",__FILE__, __LINE__,MBUS_SLAVE_ADDRESS_BUFFER);
				}
			else
				{
				mbus_set_mode(MBUS_SLAVE);						/* generate stop signal */	
				return;
				}
			break;
		case -1:
			mbus_set_mode(MBUS_SLAVE);							/* generate stop signal */
			printk(KERN_ERR "%s(%d): mbus_interrupt() Arbitration Lost\n",__FILE__, __LINE__);
			return;
			break;
		default:
			break;
		}

	if(MBUS_MODE == 0x1)									/* which mbus mode*/
		{
												/*** transmit ***/
		if(MBUS_DATA_COUNT == -1)
			*(mbus_base + MCFMBUS_MBDR) = MBUS_SUB_ADDRESS_BUFFER;			/* put destination address to mbus */
		else if(MBUS_DATA_COUNT != MBUS_DATA_NUMBER)					/* last byte transmit ? */
			*(mbus_base + MCFMBUS_MBDR) = *MBUS_DATA++;				/* put data to mbus */
		else if(MBUS_DATA_COUNT == MBUS_DATA_NUMBER)					/* last byte transmit ? */
			mbus_set_mode(MBUS_SLAVE);						/* generate stop signal */
	} 
	else
	{											/*** receive ***/

		if(MBUS_DATA_COUNT == -3)
			*(mbus_base + MCFMBUS_MBDR) = MBUS_SUB_ADDRESS_BUFFER;			/* put destination address to mbus */
		else if(MBUS_DATA_COUNT == -2)
			{
			*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) | MCFMBUS_MBCR_RSTA;	/* generate repeat start */
			*(mbus_base + MCFMBUS_MBDR) = ((MBUS_SLAVE_ADDRESS_BUFFER<<1) | MCFMBUS_MBDR_READ);			/* put destination address to mbus */
			}
		else if(MBUS_DATA_COUNT == -1)
			{
			mbus_set_direction(MBUS_RECEIVE);					/* receive mbus */
			*MBUS_DATA = *(mbus_base + MCFMBUS_MBDR);				/* dummy read from mbus */
			}		
		else if(MBUS_DATA_COUNT == MBUS_DATA_NUMBER)
			{
			mbus_set_mode(MBUS_SLAVE);						/* generate stop signal */
#ifdef debug_int_rd
			printk(KERN_ERR "int gs %x\n",*(mbus_base + MCFMBUS_MBDR));
#endif	
			*MBUS_DATA++ = *(mbus_base + MCFMBUS_MBDR);
			}
		else if(MBUS_DATA_COUNT == (MBUS_DATA_NUMBER-1))
			{
			*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) | MCFMBUS_MBCR_TXAK;	/* set TXAK*/
#ifdef debug_int_rd
			printk(KERN_ERR "int txak %x\n",*(mbus_base + MCFMBUS_MBDR));
#endif	
			*MBUS_DATA++ = *(mbus_base + MCFMBUS_MBDR);
			}
		else
			{
#ifdef debug_int_rd
			printk(KERN_ERR "int %x\n",*(mbus_base + MCFMBUS_MBDR));
#endif	
			*MBUS_DATA++ = *(mbus_base + MCFMBUS_MBDR);
			}
	}
MBUS_DATA_COUNT++;
}

/* 
 * This function starts the receiving of one char  
 * 
 * Paramters: 
 *  buf					destination data buffer
 * Returns: 
 *							None 
 */
void mbus_getchars(unsigned char *buf,int count)
{
	if(mbus_arbitration_lost() == 0x1) 
		*(mbus_base + MCFMBUS_MBSR) = *(mbus_base + MCFMBUS_MBSR) & ~MCFMBUS_MBSR_MAL;	/*this should clear the flag*/
	if(mbus_idle() == 0)								/* wait for bus idle */
		{
		printk(KERN_ERR "%s(%d): mbus_getchar() timeout mbus busy\n",__FILE__, __LINE__);
		return;
		}

	MBUS_STATE = MBUS_STATE_BUSY;

#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_getchar() count = %d\n",__FILE__, __LINE__,count);
#endif

	mbus_set_direction(MBUS_TRANSMIT);				/* transmit mbus */
	mbus_set_mode(MBUS_MASTER);

	MBUS_MODE = MBUS_RECEIVE;
	MBUS_DATA_COUNT = -3;						/* INIT data counter */
	MBUS_DATA_NUMBER = count;					/* INIT data number */
	MBUS_DATA = buf;

	*(mbus_base + MCFMBUS_MBDR) = (MBUS_SLAVE_ADDRESS_BUFFER<<1);	/* send mbus_address*/
}

/* 
 * This function starts the transmission of n char  
 * 
 * Paramters: 
 *  buf					source data buffer
 * Returns: 
 *							None 
 */
void mbus_putchars(unsigned char *doublebuf, int number)
{
	if(mbus_arbitration_lost() == 0x1) 
		*(mbus_base + MCFMBUS_MBSR) = *(mbus_base + MCFMBUS_MBSR) & ~MCFMBUS_MBSR_MAL;	/*this should clear the flag*/
	if(mbus_idle() == 0)								/* wait for bus idle */
		{
		printk(KERN_ERR "%s(%d): mbus_getchar() timeout mbus busy\n",__FILE__, __LINE__);
		return;
		}

	MBUS_STATE = MBUS_STATE_BUSY;					/*Why??*/

#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_putchars()\n",__FILE__, __LINE__);
#endif

	mbus_set_direction(MBUS_TRANSMIT);				/* transmit mbus */
	mbus_set_mode(MBUS_MASTER);
	MBUS_MODE = MBUS_TRANSMIT;
	MBUS_DATA_COUNT = -1;						/* INIT data counter */
	MBUS_DATA_NUMBER = number;					/* INIT data number */
	MBUS_DATA =  doublebuf;
	*(mbus_base + MCFMBUS_MBDR) = (MBUS_SLAVE_ADDRESS_BUFFER<<1) & ~MCFMBUS_MBDR_READ;	/* send mbus_address and write access*/
	
}
 

unsigned char mbus_busy_event(void)
{
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_busy_event()\n",__FILE__, __LINE__);
#endif
	if(MBUS_STATE == MBUS_STATE_BUSY) 
		return 1;

	return 0;
}



int mbus_open (struct inode * inode, struct file *filp)
{
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_open() MBSR = 0x%02x\n",__FILE__, __LINE__,*(mbus_base + MCFMBUS_MBSR));
	printk(KERN_ERR "%s(%d): mbus_open() MBCR = 0x%02x\n",__FILE__, __LINE__,*(mbus_base + MCFMBUS_MBCR));
#endif	
	return 0;
	*(mbus_base + MCFMBUS_MBCR) = 0x00 ;	/* first disnable M-BUS	*/
	*(mbus_base + MCFMBUS_MBSR) = 0x00;	/* clear status reg	*/


	*(mbus_base + MCFMBUS_MBCR) = MCFMBUS_MBCR_MEN ;	/* first enable M-BUS	*/
							/* Interrupt disable	*/
							/* set to Slave mode	*/
							/* set to receive mode	*/
							/* with acknowledge	*/
  
	mbus_clear_interrupt();

	*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) | MCFMBUS_MBCR_MIEN ;		/* Interrupt enable*/
}

void mbus_release(struct inode * inode, struct file *filp)
	{
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_release()\n",__FILE__, __LINE__);
#endif
	}

/*
* Using the read function:	first Byte of buf slave address
*				second Byte of buf dest address
*				following Bytes read buffer
*/
			
read_write_t mbus_read (struct inode *inode, struct file *filp, unsigned char *buf,count_t count)
{
	int n = 1000000;	
	
	Mbus_Dev *dev= filp->private_data;

#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_read() slave_address=0x%02x sub_address=0x%02x count=%d\n",__FILE__, __LINE__,MBUS_SLAVE_ADDRESS_BUFFER,MBUS_SUB_ADDRESS_BUFFER, count);
#endif
	dev->usage++;
 
	mbus_getchars(buf,count);

while(n-- && (MBUS_STATE != MBUS_STATE_IDLE));																		/*must be idle after stop*/								/* wait for bus idle */
if(n == 0x00)
	{
	printk(KERN_ERR "%s(%d): mbus_read() timeout mbus busy\n",__FILE__, __LINE__);
	return -1;
	}
	
	dev->usage--;
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_read() buf=0x%02x count=%d\n",__FILE__, __LINE__,buf[0], count);
#endif	

	return count;
}

/*
* Using the write function:      	first Byte of buf slave address
*                               	second Byte of buf dest address
*                               	following Bytes  write buffer
*/ 
                                                                    	
read_write_t mbus_write (struct inode *inode, struct file *filp,unsigned char *buf, count_t count)
{
	int n = 1000000;	
	Mbus_Dev *dev= filp->private_data;
#ifdef debug_flow
	printk(KERN_ERR "%s(%d): mbus_write() slave_address=0x%02x sub_address=0x%02x *buf=%x count=%d\n",__FILE__, __LINE__,MBUS_SLAVE_ADDRESS_BUFFER,MBUS_SUB_ADDRESS_BUFFER, *buf, count);
#endif	
	dev->usage++;
	
	memcpy_fromfs(doublebuf, buf, count); 	
	if(count) mbus_putchars(doublebuf, count);	/*make sure we have data to send then send*/
while(n-- && (MBUS_STATE != MBUS_STATE_IDLE));																		/*must be idle after stop*/								/* wait for bus idle */
if(n == 0x00)
	{
	printk(KERN_ERR "%s(%d): mbus_read() timeout mbus busy\n",__FILE__, __LINE__);
	return -1;
	}
	
	dev->usage--;
	return count;
}
	
unsigned int mbus_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned int arg)
{
	unsigned int	rc = 0;

#ifdef debug_ioctl
	printk(KERN_ERR "%s(%d): mbus_ioctl() cmd=%d arg=0x%02x\n",__FILE__, __LINE__,cmd,arg);
#endif	

	switch (cmd) 
	{
	case MBUSIOCSCLOCK:
		mbus_set_clock(arg);
		rc = 0;
		break;
	case MBUSIOCGCLOCK:
		rc = mbus_set_clock(0);
		break;
	case MBUSIOCSADDR:
		mbus_set_address(arg);
		rc = 0;
		break;
	case MBUSIOCGADDR:
		rc = mbus_set_address(0);
	break;
	case MBUSIOCSSLADDR:
		MBUS_SLAVE_ADDRESS_BUFFER = arg;
		rc = 0;
		break;
	case MBUSIOCGSLADDR:
		rc = MBUS_SLAVE_ADDRESS_BUFFER;
	break;
	case MBUSIOCSSUBADDR:
			MBUS_SUB_ADDRESS_BUFFER = arg;
			rc = 0;
		break;
	case MBUSIOCGSUBADDR:
		rc = MBUS_SUB_ADDRESS_BUFFER;
	break;
	default:
		rc = -EINVAL;
		break;
	}
#ifdef debug_ioctl
	printk(KERN_ERR "%s(%d): mbus_ioctl()returning %x\n",__FILE__, __LINE__,rc);
#endif	

	return(rc);
}

struct file_operations mbus_fops =
{
NULL,
mbus_read,
mbus_write,		//mbus.c:417: warning: initialization from incompatible pointer type9
NULL,
NULL,
mbus_ioctl,
NULL,
mbus_open,
mbus_release,

};

int mbus_init(void)
{
	int result;
	int minor = 0;
	Mbus_Dev *dev; 
	result = register_chrdev(mbus_major, "mbus",  &mbus_fops);
	printk("%s(%d):Initializing MBUS for coldfire driver v1.1 Tom Dolsky tomtek@tomtek.com.\n",__FILE__, __LINE__);
	if (result<0) 
		{
		printk("%s(%d): mbus_init() can't get Major %d\n",__FILE__, __LINE__,mbus_major);
		} 

	
	mbus_devices = kmalloc(mbus_nr_devs * sizeof(Mbus_Dev), GFP_KERNEL);
	
	if (!mbus_devices)
		{
		result = -ENOMEM;
		goto fail_malloc;
		}

	if (minor >= mbus_nr_devs) {
			return -ENODEV;
			printk("%s(%d): Error no such device \n",__FILE__, __LINE__);
			}
	
		
	dev = &mbus_devices[minor];
	dev->name="mbus";	
	/*Initializing mbus*/
	*(mbus_base + MCFMBUS_MBCR) = 0x00 ;	/* first disnable M-BUS	*/
	*(mbus_base + MCFMBUS_MBSR) = 0x00;	/* clear status reg	*/
	
	mbus_set_clock(MCFMBUS_CLK);

	//mbus_set_address(MCFMBUS_ADDRESS);
	mbus_set_address(0);

	*(mbus_base + MCFMBUS_MBCR) = MCFMBUS_MBCR_MEN ;	/* first enable M-BUS	*/
							/* Interrupt disable	*/
							/* set to Slave mode	*/
							/* set to receive mode	*/
							/* with acknowledge	*/

	if (request_irq(MCFMBUS_IRQ_VECTOR, mbus_interrupt, SA_INTERRUPT, dev->name, NULL)) 
		printk("%s(%d): mbus_open() Unable to attach %s interrupt vector=%d\n",__FILE__, __LINE__,dev->name, MCFMBUS_IRQ_VECTOR);

	*(volatile unsigned char*)(MCF_MBAR + MCFSIM_ICR3) = MCFMBUS_IRQ_LEVEL | MCFSIM_ICR_AUTOVEC;	/* set MBUS IRQ-Level and autovector */
	
	mcf_setimr(mcf_getimr() & ~MCFSIM_IMR_MBUS); 
  
	mbus_clear_interrupt();

	*(mbus_base + MCFMBUS_MBCR) = *(mbus_base + MCFMBUS_MBCR) | MCFMBUS_MBCR_MIEN ;		/* Interrupt enable*/
	
	return 0;

	fail_malloc: 	unregister_chrdev(mbus_major, "mbus");
			return result;
			
}
