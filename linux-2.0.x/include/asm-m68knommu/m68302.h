#ifndef _m68302_h_
#define _m68302_h_


#ifndef REG8 /* 8-bit reg */
#define REG8(addr)  *((volatile unsigned char * ) (addr))
#endif
#ifndef REG16 /* 16-bit reg */
#define REG16(addr) *((volatile unsigned short * ) (addr))
#endif
#ifndef REG32 /* 32-bit reg */
#define REG32(addr) *((volatile unsigned long * ) (addr))
#endif

#ifndef AREG8 /* 8-bit reg */
#define AREG8(addr)  ((volatile unsigned char * ) (addr))
#endif
#ifndef AREG16 /* 16-bit reg */
#define AREG16(addr) ((volatile unsigned short * ) (addr))
#endif
#ifndef AREG32 /* 32-bit reg */
#define AREG32(addr) ((volatile unsigned short * ) (addr))
#endif

/* These values must be the same as in begin.s */
#define BAR		0xF00000
//#define BAR		0xFFF000

#define GIMR    BAR+0x812 /* Global Interrupt Mode Reg's Memory Address */
#define IMR		BAR+0x816 /* Interrupt Mask Reg'S Memory Address */
#define ISR		BAR+0x818
#define TMR1    BAR+0x840  /* Timer Mode Register 1 */
#define TRR1    BAR+0x842  /* Timer Reference Register 1 */
#define TCR1    BAR+0x844  /* Timer Capture Register 1 */
#define TCN1    BAR+0x846  /* Timer Counter 1 */
#define TER1    BAR+0x849  /* Timer Event Register 1 */
#define TMR2    BAR+0x850  /* Timer Mode Register 2 */
#define TRR2    BAR+0x852  /* Timer Reference Register 2 */
#define TCR2    BAR+0x854  /* Timer Capture Register 2 */
#define TCN2    BAR+0x856  /* Timer Counter 2 */
#define TER2    BAR+0x859  /* Timer Event Register 2 */
#define WRR     BAR+0x84A  /* Watchdog Reference Register */
#define WCN     BAR+0x84C  /* Watchdog Counter */

#define ISR_TMR1 (1 << 9)  /* TIMER1 bit in the ISR and IMR */
#define ISR_TMR2 (1 << 6)  /* TIMER3 bit in the ISR and IMR */
#define ISR_PB10 (1 << 14)  /* PB10 bit in the ISR and IMR */
#define ISR_PB11 (1 << 15)  /* PB11 bit in the ISR and IMR */
#define ISR_SCC1 (1 << 13)  /* SCC1 bit in the ISR and IMR */
#define ISR_SCC2 (1 << 10)  /* SCC2 bit in the ISR and IMR */
#define ISR_SCC3 (1 << 8)  /* SCC3 bit in the ISR and IMR */

struct _68302_TMR
{
	unsigned short  PS:8;
	unsigned short  CE:2;
	unsigned short  OM:1;
	unsigned short  ORI:1;
	unsigned short  FRR:1;
	unsigned short  ICLK:2;
	unsigned short  RST:1;
};

struct _68302_TIMER
{
	struct _68302_TMR TMR;
	unsigned short    TRR;
	unsigned short    TCR;
	unsigned short    TCN;
	unsigned char	  RES;
	unsigned char     TER;
};	


// port A 
#define PACNT    REG16(BAR+0x81E)
#define PADDR    REG16(BAR+0x820)
#define PADAT    REG16(BAR+0x822)
#define A_PADAT  AREG16(BAR+0x822)

/* port B */
#define PBCNT   	 REG16(BAR+0x824)
#define PBDDR   	 REG16(BAR+0x826) 
#define PBDAT   	 REG16(BAR+0x828)
#define A_PBDAT   	 AREG16(BAR+0x828)


// Common SCC
#define SIMODE     REG16(BAR+0x8B4)
#define SIMASK     REG16(BAR+0x8B2)
#define SPMODE     REG16(BAR+0x8B0)
#define SMC2Rx     REG16(BAR+0x66A)
#define SMC1Rx     REG16(BAR+0x666)
#define SMC2Tx     REG16(BAR+0x66C)
#define SCC_BD_SIZE	    4						// Each BD has 4 ushorts

struct _68302_scc{
	unsigned short res1;
	unsigned short scon;			// configuration
	unsigned short scm;				// mode
	unsigned short dsr;				// data sync 
	unsigned char scce;		// event
	unsigned char res2;		
	unsigned char sccm;		// mask
	unsigned char res3;
	unsigned char sccs;		// status
	unsigned char res4;
	unsigned short res5;
};

#define SCC1_BASE    REG16(BAR+0x880)
#define SCC2_BASE    REG16(BAR+0x890)
#define SCC3_BASE    REG16(BAR+0x8A0)


#define SCM_MODE0	(1 << 0)
#define SCM_MODE1	(1 << 1)
#define SCM_ENT		(1 << 2)
#define SCM_ENR		(1 << 3)
#define SCM_DIAG0	(1 << 4)
#define SCM_DIAG1	(1 << 5)

#define SCCME_CTS	(1 << 7)
#define SCCME_CD	(1 << 6)
#define SCCME_IDL	(1 << 5)
#define SCCME_BRK	(1 << 4)		// receive break
#define SCCME_CCR	(1 << 3)		// control char received
#define SCCME_BSY	(1 << 2)		// receive overrun
#define SCCME_TX	(1 << 1)		// buffer transmitted
#define SCCME_RX	(1 << 0)		// buffer received

#define SCCS_CTS	(1 << 0)
#define SCCS_CD	(1 << 1)

/*  SCC1 */

#define SCON1   	 REG16(BAR+0x882)  
#define SCM1    	 REG16(BAR+0x884)
#define SCCE1		REG8(BAR+0x888)
#define SCCM1		REG8(BAR+0x88A)
#define SCCS1		REG8(BAR+0x88C)

#define SCC1_BD_BASE    REG16(BAR+0x400)

#define SCC1_TXBD0_1W  	 REG16(BAR+0x440)  
#define SCC1_TXBD0_2W  	 REG16(BAR+0x442)
#define SCC1_TXBD0_1L  	 REG32(BAR+0x444)
#define SCC1_RXBD0_1W  	 REG16(BAR+0x400) 
#define SCC1_RXBD0_2W  	 REG16(BAR+0x402)
#define SCC1_RXBD0_1L  	 REG32(BAR+0x404)
#define SCC1_MAX_IDL  	 REG16(BAR+0x49C)
#define SCC1_BRKCR  	 REG16(BAR+0x4A0)
#define SCC1_PAREC  	 REG16(BAR+0x4A2)
#define SCC1_FRMEC  	 REG16(BAR+0x4A4)
#define SCC1_NOSEC  	 REG16(BAR+0x4A6)
#define SCC1_BRKEC  	 REG16(BAR+0x4A8) 
#define SCC1_MRBLR  	 REG16(BAR+0x482)
#define SCC1_RFCR   	 REG16(BAR+0x480)
#define SCC1_CARACT		 REG16(BAR+0x4B0)
#define SCC1_UADDR1 	 REG16(BAR+0x4aa)
#define SCC1_UADDR2 	 REG16(BAR+0x4ac)

/*  SCC2 */

#define SCON2			 REG16(BAR+0x892) 
#define SCM2			 REG16(BAR+0x894)
#define SCCE2			REG8(BAR+0x898)
#define SCCM2			REG8(BAR+0x89A)
#define SCCS2			REG8(BAR+0x89C)

#define SCC2_BD_BASE    REG16(BAR+0x500)

#define SCC2_TXBD0_1W  	 REG16(BAR+0x540)  
#define SCC2_TXBD0_2W  	 REG16(BAR+0x542)
#define SCC2_TXBD0_1L	 REG32(BAR+0x544)
#define SCC2_RXBD0_1W  	 REG16(BAR+0x500) 
#define SCC2_RXBD0_2W  	 REG16(BAR+0x502)
#define SCC2_RXBD0_1L	 REG32(BAR+0x504)
#define SCC2_MAX_IDL  	 REG16(BAR+0x59C)
#define SCC2_BRKCR  	 REG16(BAR+0x5A0)
#define SCC2_PAREC  	 REG16(BAR+0x5A2)
#define SCC2_FRMEC  	 REG16(BAR+0x5A4)
#define SCC2_NOSEC  	 REG16(BAR+0x5A6)
#define SCC2_BRKEC  	 REG16(BAR+0x5A8) 
#define SCC2_MRBLR  	 REG16(BAR+0x582)
#define SCC2_RFCR   	 REG16(BAR+0x580)
#define SCC2_CARACT		 REG16(BAR+0x5B0)
#define SCC2_UADDR1 	 REG16(BAR+0x5aa)
#define SCC2_UADDR2 	 REG16(BAR+0x5ac)

/*  SCC3 */

#define SCON3			 REG16(BAR+0x8A2) 
#define SCM3			 REG16(BAR+0x8A4)
#define SCCE3			REG8(BAR+0x8A8)
#define SCCM3			REG8(BAR+0x8AA)
#define SCCS3			REG8(BAR+0x8AC)

#define SCC3_BD_BASE    REG16(BAR+0x600)

#define SCC3_TXBD0_1W  	 REG16(BAR+0x640)  
#define SCC3_TXBD0_2W  	 REG16(BAR+0x642)
#define SCC3_TXBD0_1L	 REG32(BAR+0x644)
#define SCC3_RXBD0_1W  	 REG16(BAR+0x600) 
#define SCC3_RXBD0_2W  	 REG16(BAR+0x602)
#define SCC3_RXBD0_1L	 REG32(BAR+0x604)
#define SCC3_MAX_IDL  	 REG16(BAR+0x69C)
#define SCC3_BRKCR  	 REG16(BAR+0x6A0)
#define SCC3_PAREC  	 REG16(BAR+0x6A2)
#define SCC3_FRMEC  	 REG16(BAR+0x6A4)
#define SCC3_NOSEC  	 REG16(BAR+0x6A6)
#define SCC3_BRKEC  	 REG16(BAR+0x6A8) 
#define SCC3_MRBLR  	 REG16(BAR+0x682)
#define SCC3_RFCR   	 REG16(BAR+0x680)
#define SCC3_CARACT		 REG16(BAR+0x6B0)
#define SCC3_UADDR1 	 REG16(BAR+0x6aa)
#define SCC3_UADDR2 	 REG16(BAR+0x6ac)

#endif
