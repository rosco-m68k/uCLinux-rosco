
#ifndef mcfqspi_h
#define mcfqspi_h
#include <linux/config.h>

#define QSPIIOCS_DOUT_HIZ	1	/* QMR[DOHIE] set hi-z dout between transfers */
#define QSPIIOCS_BITS		2	/* QMR[BITS] set transfer size */
#define QSPIIOCG_BITS		3	/* QMR[BITS] get transfer size */
#define QSPIIOCS_CPOL		4	/* QMR[CPOL] set SCK inactive state */
#define QSPIIOCS_CPHA		5	/* QMR[CPHA] set SCK phase, 1=rising edge */
#define QSPIIOCS_BAUD		6	/* QMR[BAUD] set SCK baud rate divider */
#define QSPIIOCS_QCD		7	/* QDLYR[QCD] set start delay */
#define QSPIIOCS_DTL		8	/* QDLYR[DTL] set after delay */
#define QSPIIOCS_CONT		9	/* continuous CS asserted during transfer */
#define QSPIIOCS_READDATA	10	/* set data send during read */

typedef struct qspi_read_data
{
	unsigned int length;
	unsigned char buf[32];	/* data to send during read */
	int loop : 1;
} qspi_read_data;

#endif
