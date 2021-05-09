#ifndef  _SH7615_SERIAL_H
#define  _SH7615_SERIAL_H
typedef volatile unsigned char  *IOptr;
typedef volatile unsigned short *WIOptr;


/* 	SCIF Channel 1 - Internal Registers - Address Locations 		     */

#define SD_MODE_REG        ((IOptr)0xFFFFFCC0)  /*Serial Mode Register        SCSMR1 */
#define SD_BAUD_REG        ((IOptr)0xFFFFFCC2)  /* Bit Rate Register          SCBRR1 */
#define SD_CONTROL_REG     ((IOptr)0xFFFFFCC4)  /*Serial Control Register     SCSCR1 */
#define SD_TRANSMIT_DATA   ((IOptr)0xFFFFFCC6)  /*Transmit FIFO Data Register SCFTDR1*/
#define SD_STATUS1_REG     ((WIOptr)0xFFFFFCC8) /*Serial Status Register 1    SC1SSR1*/
#define SD_STATUS2_REG     ((IOptr)0xFFFFFCCA)  /*Serial Status Register 2    SC2SSR1*/
#define SD_RECEIVE_DATA    ((IOptr)0xFFFFFCCC)  /*Receive FIFO Register       SCFRDR1*/
#define SCFCR1             ((IOptr)0XFFFFFCCE)  /*FIFO Control Register       SCFCR1 */
#define SCFDR1             ((WIOptr)0XFFFFFCD0) /*FIFO Data Register          SCFDR1 */
#define SCFER1             ((WIOptr)0XFFFFFCD2) /*FIFO Error Register         SCFER1 */

#define SD_PRIORITY_REG    ((WIOptr)0xFFFFFE40) /*IPRD*/
#define SD_VECTOR_L_REG    ((WIOptr)0xFFFFFE50) /*VCRL*/
#define SD_VECTOR_M_REG    ((WIOptr)0xFFFFFE52) /*VCRM*/

#define SD_IPR                     0x000A
#define SD_IPR_SCI_MASK            0xFFF0


/*************************************************************************************/
/* 			SCIF Register Values                                         */
/*************************************************************************************/

/*                      Serial Control Register Values                               */
#define SD_RECEIVE_INTR_ENABLE	   0x40	    
#define SD_TRANSMIT_ENABLE         0x20
#define SD_RECEIVE_ENABLE          0x10
#define SD_CLOCK_ENABLE_1          0x00
#define SD_CLOCK_ENABLE_0          0x00
#define SD_RXERROR_VECTOR      0x4900 /*73*/
#define SD_RXDATAFULL_VECTOR   0x0049 /*73*/
#define SD_RXBRK_VECTOR      0x4900 /*  73 all recieve interrupts pointing to the same vector*/
#define SD_RXI_INTR_PRIORITY   0x0004 /*priority of 4*/

/*                      FIFO Control Reg                                             */

#define SD_FIFO_CLR_ENABLE         0x06
#define SD_FIFO_CLR_DISABLE        0xF9
#define SD_FIFO_INIT               0x00

/*************************************************************************************/
/*  			 Data Definitions                                            */
/*************************************************************************************/
/* 			 FPGA 1 						     */

#define SD1_CLK_ENABLE              0x0001
#define SD2_CLK_ENABLE              0x0100
#define SD1_BAUD_9600               0x0008
#define SD_BIT_RATE_REG             4

#define SD_BAUD_9600_25             81          /* For a 25Mhz Clock                 */
#define SD_OUTPUT_BUFF_SIZE         128
#define TDFE_CHECK                   32
#define TEND_CHECK		0x0040


/*mask to check receive errors*/
 
#define ER_MASK   0x0080 
#define BRK_MASK  0x0010
#define FER_MASK  0x0008
#define PER_MASK  0x0004
#define DR_MASK   0x0001
#define ORER_MASK 0x0001
#define RDF_MASK  0x0002
#define RDF_CLR   0xfffd   
#define RE_CLR    0xef 
#define DR_CLR    0xfffe                 
#define BRK_CLR   0xffef 
#define ORER_CLR  0xfe 
#define ER_CLR    0xff7f
#define RFRST     0x02 /*for resetting receive fifo */ 

/* 			Function Declarations                                        */

int serial_port(void);
int serial_enable(void);
int serial_transmit(void);
int serial_receive(void);
extern void console_receive_SH7615();
extern void read_data();
extern void do_error_handler();
extern int get_character();
extern void put_character( char data);
/*			Global Variables					     */

#endif
