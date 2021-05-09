/************************************************************************/
/*                                                                      */
/*  mcf_qspi.c - QSPI driver for MCF5272                                */
/*                                                                      */
/*  (C) Copyright 2001, Wayne Roberts (wroberts1@home.com)              */
/*                                                                      */
/*  Driver has an 8bit mode, and a 16bit mode.                          */
/*  Transfer size QMR[BITS] is set thru QSPIIOCS_BITS.                  */
/*  When size is 8, driver works normally:                              */
/*        a char is sent for every transfer                             */
/*  When size is 9 to 16bits, driver reads & writes the QDRs with       */
/*  the buffer cast to unsigned shorts.  The QTR & QRR registers can    */
/*  be filled with up to 16bits.  The length passed to read/write must  */
/*  be of the number of chars (2x number of shorts). This has been      */
/*  tested with 10bit a/d and d/a converters.                           */
/*                                                                      */
/*  * QSPIIOCS_READDATA:                                                */
/*    data to send out during read                                      */
/*  * all other ioctls are global                                       */
/*                                                                      */
/*                                                                      */
/************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>

#if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif        

#include <linux/fs.h>
#include <linux/wrapper.h>

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,2,0)
#include <asm/uaccess.h> /* for put_user */
#endif

#include <linux/signal.h>
#include <linux/sched.h>
#include <asm/ptrace.h>
#include <asm/coldfire.h>
#include <asm/m5272sim.h>
#include <linux/malloc.h>
#include "mcf_qspi.h"

#define DEVICE_NAME "qspi"

int qspi_init(void);
int init(void);
int init_module(void);
void cleanup_module(void);
struct wait_queue *wqueue;
static unsigned char dbuf[1024];
static struct semaphore sem = MUTEX;

void qspi_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned short qir = QIR;
	if (qir & QIR_WCEF)
		printk("WCEF ");	/* write collison */
	if (qir & QIR_ABRT)
		printk("ABRT ");	/* aborted by clearing of QDLYR[SPE] */
	if (qir & QIR_SPIF) {
		wake_up(&wqueue);	/* transfer finished */
	}

	QIR |= QIR_WCEF | QIR_ABRT | QIR_SPIF;	/* clear all bits */
}

static int qspi_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int error, n, rc = 0;
	qspi_dev *dev = file->private_data;
	qspi_read_data *user_read_data;

	switch (cmd) {
		case QSPIIOCS_DOUT_HIZ:
			n = QMR & ~QMR_DOHIE;
			QMR = n | ((arg << 14) & QMR_DOHIE);
			break;
		case QSPIIOCS_BITS:	/* set QMR[BITS] */
			n = QMR & ~QMR_BITS;
			QMR = n | ((arg << 10) & QMR_BITS);
			break;
		case QSPIIOCG_BITS:	/* get QMR[BITS] */
			*((int *)arg) = (QMR >> 10) & 0xf;
			break;
		case QSPIIOCS_CPOL:	/* set SCK inactive state */
			n = QMR & ~QMR_CPOL;
			QMR = n | ((arg << 9) & QMR_CPOL);
			break;
		case QSPIIOCS_CPHA:	/* set SCK phase, 1=rising edge */
			n = QMR & ~QMR_CPHA;
			QMR = n | ((arg << 8) & QMR_CPHA);
			break;
		case QSPIIOCS_BAUD:	/* set SCK baud rate divider */
			n = QMR & ~QMR_BAUD;
			QMR = n | (arg & QMR_BAUD);
			break;
		case QSPIIOCS_QCD:	/* set start delay */
			n = QDLYR & ~QDLYR_QCD;
			QDLYR = n | ((arg << 8) & QDLYR_QCD);
			break;
		case QSPIIOCS_DTL:	/* set after delay */
			n = QDLYR & ~QDLYR_DTL;
			QDLYR = n | (arg & QDLYR_DTL);
			break;
		case QSPIIOCS_CONT:	/* continuous CS asserted during transfer */
			dev->qcr_cont = arg ? 1 : 0;
			break;
		case QSPIIOCS_READDATA:	/* set data to be sent during reads */
			user_read_data = (qspi_read_data *)arg;
			error = verify_area(VERIFY_READ, user_read_data,
							sizeof(qspi_read_data));
			if (error)
				return error;

			if (user_read_data->length > sizeof(user_read_data->buf))
				return -EINVAL;

			dev = file->private_data;
			memcpy_fromfs(&dev->read_data, user_read_data,
							sizeof(qspi_read_data));
			break;
		default:
			rc = -EINVAL;
			break;
			
	}

	return rc;
}

static int qspi_open(struct inode *inode, struct file *file)
{
	qspi_dev *dev;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	MOD_INC_USE_COUNT;
#endif

	if ((dev = kmalloc(sizeof(qspi_dev), GFP_KERNEL)) == NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
		MOD_DEC_USE_COUNT;
#endif
		return -ENOMEM;
	}

	dev->read_data.length = 0;
	dev->read_data.buf[0] = 0;
	dev->read_data.buf[1] = 0;
	dev->read_data.loop = 0;
	file->private_data = dev;


	return 0;
}

static void qspi_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	MOD_DEC_USE_COUNT;
#endif
}

static int qspi_read(struct inode *inode,
	struct file *filep,
	char *buffer,
	int length) 
{
	int qcr_cs, total = 0, i = 0;
	unsigned char bits, word = 0;
	qspi_dev *dev;
	int rdi = 0;

	down(&sem);

	qcr_cs = (~MINOR(inode->i_rdev) << 8) & 0xf00;	/* CS for QCR */
	bits = (QMR >> 10) % 0x10;
	if (bits == 0 || bits > 0x08)
		word = 1;	/* 9 to 16bit transfers */

	dev = filep->private_data;
	while (i < length) {
		unsigned short *sp = (unsigned short *)&buffer[i];
		unsigned char *cp = &buffer[i];
		unsigned short *rd_sp = (unsigned short *)dev->read_data.buf;
		int x, n;

		QAR = TX_RAM_START;		/* address first QTR */
		for (n=0; n<16; n++) {
			if (rdi != -1) {
				if (word) {
					QDR = rd_sp[rdi++];
					if (rdi == dev->read_data.length>>1)
						rdi = dev->read_data.loop ? 0 : -1;
				} else {
					QDR = dev->read_data.buf[rdi++];
					if (rdi == dev->read_data.length)
						rdi = dev->read_data.loop ? 0 : -1;
				}
			} else
				QDR = 0;

			i++;
			if (word)
				i++;
			if (i > length) break;
		}
		QAR = COMMAND_RAM_START;	/* address first QCR */
		for (x=0; x<n; x++) {
			/* QCR write */
			if (dev->qcr_cont) {
				if (x == n-1 && i == length)
					QDR = QCR_SETUP | qcr_cs;	/* last transfer */
				else
					QDR = QCR_CONT | QCR_SETUP | qcr_cs;
			} else
				QDR = QCR_SETUP | qcr_cs;
		}

		QWR = QWR_CSIV | ((n-1) << 8);
		QIR = QIR_SETUP;
		QDLYR |= QDLYR_SPE;

		interruptible_sleep_on(&wqueue);

		QAR = RX_RAM_START;	/* address: first QRR */
		if (word) {
			/* 9 to 16bit transfers */
			for (x=0; x<n; x++) {
				put_user(*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QDR),
							sp++);
			}
		} else {
			/* 8bit transfers */
			for (x=0; x<n; x++)
				put_user(*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QDR),
								cp++);
		}

		if (word)
			n <<= 1;

		total += n;
	}

	up(&sem);

	return total;
}

static int qspi_write(
	struct inode *inode,
	struct file *filep,
	const char *buffer,
	int length)
{
	int qcr_cs, i = 0, total = 0;
	unsigned char bits, word = 0;
	qspi_dev *dev;

	down(&sem);

	bits = (QMR >> 10) % 0x10;
	if (bits == 0 || bits > 0x08)
		word = 1;	/* 9 to 16bit transfers */

	qcr_cs = (~MINOR(inode->i_rdev) << 8) & 0xf00;	/* CS for QCR */
	memcpy_fromfs(dbuf, buffer, length);

	dev = filep->private_data;
	while (i < length) {
		int x, n;
		QAR = TX_RAM_START;		/* address: first QTR */
		if (word) {
			for (n=0; n<16; ) {
				QDR = (dbuf[i] << 8) + dbuf[i+1];	/* tx data: QTR write */
				n++;
				i += 2;
				if (i >= length) break;
			}
		} else {
			/* 8bit transfers */
			for (n=0; n<16; ) {
				QDR = dbuf[i];	/* tx data: QTR write */
				n++;
				i++;
				if (i == length) break;
			}
		}
		QAR = COMMAND_RAM_START;	/* address: first QCR */
		for (x=0; x<n; x++) {
			/* QCR write */
			if (dev->qcr_cont) {
				if (x == n-1 && i == length)
					QDR = QCR_SETUP | qcr_cs;	/* last transfer */
				else
					QDR = QCR_CONT | QCR_SETUP | qcr_cs;
			} else
				QDR = QCR_SETUP | qcr_cs;
		}

		QWR = QWR_CSIV | ((n-1) << 8);	/* QWR[ENDQP] = n << 8 */
		QIR = QIR_SETUP;
		QDLYR |= QDLYR_SPE;

		interruptible_sleep_on(&wqueue);

		if (word)
			n <<= 1;

		total += n;
	}

	up(&sem);

	return total;
}

struct file_operations Fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	owner:		THIS_MODULE,
#endif
	read:		qspi_read, 
	write:		qspi_write,
	ioctl:		qspi_ioctl,
	open:		qspi_open,
	release:	qspi_release  /* a.k.a. close */
};

int
init()
{
	volatile unsigned long *lp;

	/* common init: driver or module: */

	if (request_irq(MCFQSPI_IRQ_VECTOR, qspi_interrupt, SA_INTERRUPT, "ColdFire QSPI", NULL)) {
		printk("QSPI: Unable to attach ColdFire QSPI interrupt "
			"vector=%d\n", MCFQSPI_IRQ_VECTOR);
		return -EINVAL;
	}

	/* set our IPL */
	*((volatile unsigned long *)(MCF_MBAR + MCFSIM_ICR4)) = 0xd0000000;

	/* 1) CS pin setup 17.2.x
	 *	Dout, clk, cs0 always enabled. Din, cs[3:1] must be enabled.
	 *	CS1: PACNT[23:22] = 10
	 *	CS1: PBCNT[23:22] = 10 ?
	 *	CS2: PDCNT[05:04] = 11
	 *	CS3: PACNT[15:14] = 01
	 */
	lp = (volatile unsigned long *)(MCF_MBAR + MCFSIM_PACNT);
	*lp = (*lp & 0x00c0c000) | 0x00804000;	/* 17.2.1 QSPI CS1 & CS3 */
	lp = (volatile unsigned long *)(MCF_MBAR + MCFSIM_PDCNT);
	*lp = (*lp & 0x00000030) | 0x00000030;	/* QSPI_CS2 */

	QMR = 0x8320;// default mode setup: 16 bits, baud, 1Mhz clk.
	QDLYR = 0x0f02;	// default start & end delays

	init_waitqueue(&wqueue);

	printk("MCF5272 QSPI driver ok\n");

	return 0;
}

/* init for compiled-in driver: call from mem.c */
int
qspi_init()
{
	int ret;

	if ((ret = register_chrdev(QSPI_MAJOR, DEVICE_NAME, &Fops) < 0)) {
		printk ("%s device failed with %d\n",
			"Sorry, registering the character", ret);
		return ret;
	}

	return init();
}

/* init for module driver */
int
init_module()
{
	int ret;

	if ((ret = module_register_chrdev(QSPI_MAJOR, DEVICE_NAME, &Fops) < 0)) {
		printk ("%s device failed with %d\n",
			"Sorry, registering the character", ret);
		return ret;
	}

	return init();
}

/* Cleanup - undid whatever init_module did */
void cleanup_module()
{
	int ret;

	free_irq(MCFQSPI_IRQ_VECTOR, NULL);

	/* zero our IPL */
	*((volatile unsigned long *)(MCF_MBAR + MCFSIM_ICR4)) = 0x80000000;

	/* Unregister the device */
	if ((ret = module_unregister_chrdev(QSPI_MAJOR, DEVICE_NAME)) < 0)
		printk("Error in unregister_chrdev: %d\n", ret);
}


