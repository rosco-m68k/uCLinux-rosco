/*
  Hitachi H8/300H SCI Driver
*/

struct sci_struct {
	short			ch;
	long			port;
	short			irq;
	long			trxd_io;
	long			trxd_ddr;
	short			txd_mask;
	short			rxd_mask;
	struct async_struct   	*info;
};

enum SCI_REGS {SCI_SMR,SCI_BRR,SCI_SCR,SCI_TDR,SCI_SSR,SCI_RDR,SCI_SMCR};
enum SCI_INT {SCI_INT_ERI,SCI_INT_RXI,SCI_INT_TXI,SCI_INT_TEI};

#define SCI0_BASE     0xffffb0
#define SCI0_IRQ      52

#define SCI1_BASE     0xffffb8
#define SCI1_IRQ      56

#define SCI2_BASE     0xffffc0
#define SCI2_IRQ      60

#if defined(CONFIG_H83002) || defined(CONFIG_H83048)
#define SCI_CHANNEL 2
#define SCI0_TRXD_DDR 0xffffd0
#define SCI0_TRXD_IO  0xffffd2
#define SCI0_TXD_MASK 0x01
#define SCI0_RXD_MASK 0x02

#define SCI1_TRXD_DDR 0xffffd0
#define SCI1_TRXD_IO  0xffffd2
#define SCI1_TXD_MASK 0x04
#define SCI1_RXD_MASK 0x08

#define SCI2_TRXD_DDR 0
#define SCI2_TRXD_IO  0
#define SCI2_TXD_MASK 0
#define SCI2_RXD_MASK 0
#endif

#if defined(CONFIG_H83007) || defined(CONFIG_H83068)
#define SCI_CHANNEL 3
#define SCI0_TRXD_DDR 0xfee008
#define SCI0_TRXD_IO  0xffffd8
#define SCI0_TXD_MASK 0x01
#define SCI0_RXD_MASK 0x04

#define SCI1_TRXD_DDR 0xfee008
#define SCI1_TRXD_IO  0xffffd8
#define SCI1_TXD_MASK 0x02
#define SCI1_RXD_MASK 0x08

#define SCI2_TRXD_DDR 0xfee00a
#define SCI2_TRXD_IO  0xffffda
#define SCI2_TXD_MASK 0x40
#define SCI2_RXD_MASK 0x80
#endif

#define SCI_SCR_CKE0 0x01
#define SCI_SCR_CKE1 0x02
#define SCI_SCR_TEIE 0x04
#define SCI_SCR_MPIE 0x08
#define SCI_SCR_RE   0x10
#define SCI_SCR_TE   0x20
#define SCI_SCR_RIE  0x40
#define SCI_SCR_TIE  0x80

#define SCI_SSR_MPBT 0x01
#define SCI_SSR_MPB  0x02
#define SCI_SSR_TEND 0x04
#define SCI_SSR_PER  0x08
#define SCI_SSR_FER  0x10
#define SCI_SSR_ORER 0x20
#define SCI_SSR_RDRF 0x40
#define SCI_SSR_TDRE 0x80

#define BPS_2400 11
#define BPS_4800 12
#define BPS_9600 13
#define BPS_19200 14
#define BPS_38400 15
#define BPS_57600 16
#define BPS_115200 17
