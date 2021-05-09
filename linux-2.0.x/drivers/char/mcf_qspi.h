#include <asm/mcfqspi.h>

#define QSPI_MAJOR	126
#define MCFQSPI_IRQ_VECTOR	89

/* QSPI registers */
#define	MCFSIM_QMR		0xa0	/* mode */
#define	MCFSIM_QDLYR	0xa4	/* delay */
#define	MCFSIM_QWR		0xa8	/* wrap */
#define	MCFSIM_QIR		0xac	/* interrupt */
#define	MCFSIM_QAR		0xb0	/* address */
#define	MCFSIM_QDR		0xb4	/* address */

#define TX_RAM_START	0x00
#define RX_RAM_START	0x10
#define COMMAND_RAM_START	0x20

#define	QMR		*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QMR)
#define QAR		*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QAR)
#define QDR		*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QDR)
#define QWR		*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QWR)
#define QDLYR	*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QDLYR)
#define QIR		*(volatile unsigned short *)(MCF_MBAR + MCFSIM_QIR)

/* bits */
#define QMR_MSTR	0x8000	/* master mode enable: must always be set */
#define QMR_DOHIE	0x4000	/* shut off (hi-z) Dout between transfers */
#define QMR_BITS	0x3c00	/* bits per transfer (size) */
#define QMR_CPOL	0x0200	/* clock state when inactive */
#define QMR_CPHA	0x0100	/* clock phase: 1 = data taken at rising edge */
#define QMR_BAUD	0x00ff	/* clock rate divider */

#define QIR_WCEF	0x0008	/* write collison */
#define QIR_ABRT	0x0004	/* abort */
#define QIR_SPIF	0x0001	/* finished */
#define QIR_SETUP	0xdd0f	/* setup QIR for tranfer start */

#define QWR_CSIV	0x1000	/* 1 = active low chip selects */

#define QDLYR_SPE	0x8000	/* initiates transfer when set */
#define QDLYR_QCD	0x7f00	/* start delay between CS and first clock */
#define QDLYR_DTL	0x00ff	/* delay after CS release */

/* QCR: chip selects return to inactive, bits set in QMR[BITS],
 * after delay set in QDLYR[DTL], pre-delay set in QDLYR[QCD] */
#define QCR_SETUP	0x7000
#define QCR_CONT	0x8000	/* 1=continuous CS after transfer */

typedef struct qspi_dev
{
	qspi_read_data read_data;
	unsigned int qcr_cont : 1;	/* keep CS active throughout transfer */
} qspi_dev;

