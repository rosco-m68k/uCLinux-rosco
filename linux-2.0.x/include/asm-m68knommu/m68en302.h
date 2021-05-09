#ifndef _m68en302_h_
#define _m68en302_h_



#ifndef _m68302_h_
#include <asm/m68302.h>
#endif



//#define MOBARV	0xFFE000
#define MOBARV	0xF80000
#define MOBARREG 0xEE

#define IER     MOBARV+2		/* INTERRUPT EXTENSION REGISTER */



#define ECNTRL0		MOBARV+0x800
#define EDMA0		MOBARV+0x802
#define EMRBRL0		MOBARV+0x804
#define INTR_VECT0  MOBARV+0x806
#define INTR_EVENT0 MOBARV+0x808
#define INTR_MASK0	MOBARV+0x80A
#define ECNFIG0		MOBARV+0x80C
#define ETHER_TEST0	MOBARV+0x80E
#define AR_CNTRL0	MOBARV+0x810
#define CET0		MOBARV+0xA00
#define EBD0		MOBARV+0xC00


#define ECNTRL_GTS 0x04
#define ECNTRL_ETHER_EN 0x02
#define ECNTRL_RESET 0x01

struct _68EN302_ECNTRL
{
	unsigned short MBZ:13;		/* must be zero */
	unsigned short GTS:1;		/* graceful transmit stop */
	unsigned short ETHER_EN:1;	/* ethernet enable */
	unsigned short RESET:1;		/* ethernet controller reset */
	
};


struct _68EN302_EDMA
{
	unsigned short BDERR:7;		/* Buffer descriptor error number */
	unsigned short MBZ:1;		/* must be zero */
	unsigned short BDSIZE:2;	/* buffer descritor size */
	unsigned short TSRLY:1;		/* transmit start early */
	unsigned short WMRK:2;		/* FIFO water mark */
	unsigned short BLIM:3;		/* burst limit     */
};






/* ethernet test regsiter bits */
#define ETHR_TWS	0x0001
#define	ETHR_RWS	0x0002
#define	ETHR_DRTY	0x0004
#define ETHR_COLL	0x0008
#define ETHR_SLOT	0x0010
#define ETHR_TRND	0x0020
#define ETHR_TBO	0x0040
#define ETHR_BGT	0x0080



/* address control register bits */

#define ETHR_HASH_EN	0x8000
#define ETHR_INDEX_EN	0x4000
#define	ETHR_MULT		0x3000
#define	ETHR_PA_REJ		0x0800
#define ETHR_PROM		0x0400


struct _68EN302_AR_CNTRL
{
	unsigned short HASH_EN:1;
	unsigned short INDEX_EN:1;
	unsigned short MULT:2;
	unsigned short PA_REJ:1;
	unsigned short PROM:1;
	unsigned short MBZ:10;
};


struct	_68EN302_ETHR_IVEC
{
	unsigned short MBZ:7;
	unsigned short VG:1;
	unsigned short INV:8;
};

union _68EN302_ETHR_RXBD_FLAGS
{
	unsigned short w;
	struct
	{
		unsigned short E:1,RO:1,W:1,I:1,L:1,F:1, :1, M:1, :2, LG:1,NO:1,SH:1,CR:1,OV:1,CL:1;
	} s;
};

struct _68EN302_ETHR_RXBD
{
	union _68EN302_ETHR_RXBD_FLAGS flags;
	unsigned short length;
	union
	{
		unsigned long   l;
		struct
		{
			unsigned  long Reason:2;
			unsigned  long ARIndex:6;
			unsigned  long Pointer:24;
		} s;
	} address;
};

		
union _68EN302_ETHR_TXBD_FLAGS
{
	unsigned short w;
	struct
	{
		unsigned short R:1,TO:1,W:1,I:1,L:1,TC:1,
					   DEF:1, HB:1, LC:1;
		unsigned short RL:1,
					   xRC:4,
						UN:1, CSL:1;
	} s;
};



struct _68EN302_ETHR_TXBD
{
	union _68EN302_ETHR_TXBD_FLAGS flags;
	unsigned short length;
	unsigned long  address;
};


union  _68EN302_ETHR_BD
{
	struct	_68EN302_ETHR_TXBD tx;
	struct	_68EN302_ETHR_RXBD rx;
};


struct _68EN302_ETHR
{
	struct _68EN302_ECNTRL		ECNTRL;
	struct _68EN302_EDMA		EDMA;
	unsigned short				EMRBRL;
	struct	_68EN302_ETHR_IVEC	INTR_VEC;

/* interrupt masks */	
	unsigned short				INTR_EVENT;
	unsigned short				INTR_MASK;
#define ETHR_RXB	0x0001
#define ETHR_TXB	0x0002 
#define ETHR_BSY	0x0004
#define ETHR_RFINT	0x0008
#define ETHR_TFINT	0x0010
#define ETHR_EBERR	0x0020
#define ETHR_BOD	0x0040
#define ETHR_GRA	0x0080
#define ETHR_BABT	0x0100
#define ETHR_BABR	0x0200
#define ETHR_HBERR	0x0400

	
/* ethernet configuration refitser bits */
	unsigned short				ECNFIG;
#define ETHR_LOOP 0x0001
#define ETHR_FDEN 0x0002
#define ETHR_HDC  0x0004
#define ETHR_RDT  0x0008

	unsigned short				ETHER_TEST;
	struct _68EN302_AR_CNTRL	AR_CNTRL;
};

#define _68EN302_MAX_CET 64
struct _68EN302_ETHR_BLOCK
{
	struct	_68EN302_ETHR		info;
	unsigned char				dummy[0x9ff+1-0x812];
	unsigned char				CET[_68EN302_MAX_CET][8];
	union  _68EN302_ETHR_BD		BD[128];
};

	


#endif
