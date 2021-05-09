/*
 * File: nios_map.h
 *
 * This file is a machine generated address map
 * for a Nios hardware design.
 * ./cpu32.ptf
 *
 * Generated: 2001.07.31 14:04:02
 */

#ifndef _nios_map_
#define _nios_map_

#ifdef __cplusplus
extern "C" {
#endif

#define na_null                ((void *)          0x00000000) 
#define na_cpu32_cpu           ((void *)          0x00000000) 
#define na_cpu32_cpu_end       ((void *)          0x08000000) 
#define na_mon                 ((void *)          0x00000000) 
#define na_uart0               ((np_uart *)       0x00000400) 
#define na_uart0_irq                              17          
#define na_sram                ((void *)          0x00040000) 
#define na_sram_end            ((void *)          0x00080000) 
#define na_flash               ((void *)          0x00100000) 
#define na_flash_end           ((void *)          0x00200000) 
#define na_timer0              ((np_timer *)      0x00000440) 
#define na_timer0_irq                             16          
#define na_uart1               ((np_uart *)       0x000004a0) 
#define na_uart1_irq                              18          
#define na_seven_seg_pio       ((np_pio *)        0x00000420) 
#define na_spi                 ((np_spi *)        0x00000480) 
#define na_spi_irq                                19          
#define na_sdram               ((void *)          0x01000000) 
#define na_sdram_end           ((void *)          0x02000000) 
#define na_led_pio             ((np_pio *)        0x00000460) 
#define na_button_pio          ((np_pio *)        0x00000470) 
#define na_ide_interface       ((np_usersocket *) 0x00000500) 
#define na_ide_ctl_in          ((np_pio *)        0x00000580) 
#define na_flash_kernel        ((void *)          0x00800000) 
#define na_flash_kernel_end    ((void *)          0x01000000) 
#define na_enet                ((void *)          0x00004000) 
#define na_enet_irq                               35          
#define na_enet_reset          ((np_pio *)        0x00004020) 
#define nasys_printf_uart      ((np_uart *)       0x00000400) 
#define nasys_printf_uart_irq                     17          
#define nasys_gdb_uart         ((np_uart *)       0x000004a0) 
#define nasys_gdb_uart_irq                        18          
#define nasys_main_flash       ((void *)          0x00100000) 
#define nasys_main_flash_end   ((void *)          0x00200000) 
#define nasys_program_mem      ((void *)          0x01000000) 
#define nasys_program_mem_end  ((void *)          0x02000000) 
#define nasys_data_mem         ((void *)          0x01000000) 
#define nasys_data_mem_end     ((void *)          0x02000000) 
#define nasys_stack_top        ((void *)          0x02000000) 
#define nasys_vector_table     ((void *)          0x00040000) 
#define nasys_vector_table_end ((void *)          0x00040100) 
#define nasys_reset_address    ((void *)          0x00000000) 
#define nasys_clock_freq       ((long)            33333000)   
#define nasys_clock_freq_1000  ((long)            33333)      

// Parameters for each peripheral.


// Parameters for altera_nios named na_cpu32_cpu at 0x00008235:
//         num_regs = 512
//       shift_size = 7
//            mstep = 1
//         multiply = 0
//        wvalid_wr = 0
//      rom_decoder = 1
//   mainmem_module = 
//   datamem_module = 
//  maincomm_module = uart0
//   gdbcomm_module = uart1
// germs_monitor_id = MDL
//     reset_offset = 0x0
//     reset_module = mon
//   vecbase_offset = 0x0
//   vecbase_module = sram

// Parameters for altera_avalon_onchip_memory named na_mon at 0x00008235:
//     Writeable = 0
//      Contents = user_file
//      Initfile = cpu32_sdk\\Custom_Germs\\nios_germs_monitor.srec
// Size_In_Bytes = 1K

// Parameters for altera_avalon_uart named na_uart0 at 0x00008235:
//       baud = 115200
//  data_bits = 8
// fixed_baud = 1
//     parity = N
//  stop_bits = 2
// clock_freq = 33333000

// Parameters for altera_avalon_timer named na_timer0 at 0x00008235:

// Parameters for altera_avalon_uart named na_uart1 at 0x00008235:
//       baud = 115200
//  data_bits = 8
// fixed_baud = 0
//     parity = N
//  stop_bits = 1

// Parameters for altera_avalon_pio named na_seven_seg_pio at 0x00008235:
//   has_tri = 0
//   has_out = 1
//    has_in = 0
// edge_type = NONE
//  irq_type = NONE

// Parameters for altera_avalon_spi named na_spi at 0x00008235:
//      databits = 16
//   targetclock = 250000.0
//   clock_units = KHz
//     numslaves = 2
//      ismaster = 1
// clockpolarity = 0
//    clockphase = 1
//      lsbfirst = 0
//    extradelay = 1
// targetssdelay = 5.0E-6
//   delay_units = us

// Parameters for altera_avalon_sdram_controller named na_sdram at 0x00008235:
//      sdram_data_width = 32
//      sdram_addr_width = 11
//      sdram_bank_width = 2
//       sdram_row_width = 11
//       sdram_col_width = 8
// sdram_num_chipselects = 2
//        refresh_period = 15.625us
//         powerup_delay = 100us
//           cas_latency = 1
// precharge_control_bit = 10
//                 t_rfc = 70ns
//                  t_rp = 20ns
//                 t_mrd = 2clocks
//                 t_rcd = 20ns
//                  t_ac = 17ns
//   t_wr_auto_precharge = 1clock + 7ns
//        t_wr_precharge = 14ns
// init_refresh_commands = 2
//        init_nop_delay = 0
//           shared_data = 0
//         enable_ifetch = 1
//              highperf = 1

// Parameters for altera_avalon_pio named na_led_pio at 0x00008235:
//   has_tri = 0
//   has_out = 1
//    has_in = 0
// edge_type = NONE
//  irq_type = NONE

// Parameters for altera_avalon_pio named na_button_pio at 0x00008235:
//   has_tri = 0
//   has_out = 0
//    has_in = 1
// edge_type = NONE
//  irq_type = NONE

// Parameters for altera_avalon_user_defined_interface named na_ide_interface at 0x00008235:

// Parameters for altera_avalon_pio named na_ide_ctl_in at 0x00008235:
//   has_tri = 0
//   has_out = 0
//    has_in = 1
// edge_type = NONE
//  irq_type = NONE

// Parameters for altera_avalon_pio named na_enet_reset at 0x00008235:
//   has_tri = 0
//   has_out = 1
//    has_in = 0
// edge_type = NONE
//  irq_type = NONE

#ifdef __cplusplus
}
#endif

#endif // _nios_map_

/* end of file */
/*
 * File: nios_peripherals.h
 *
 * This file is a machine peripherals definition
 * for a Nios hardware design.
 * ./cpu32.ptf
 *
 * Generated: 2001.07.31 14:04:03
 */

#ifndef _nios_peripherals_
#define _nios_peripherals_

#ifdef __cplusplus
extern "C" {
#endif

// : 1 present 

// altera_avalon_cs8900: 1 present 

// altera_avalon_onchip_memory: 1 present 

// altera_avalon_pio: 5 present 
// PIO Peripheral

// PIO Registers
typedef volatile struct
	{
	int np_piodata;          // read/write, up to 32 bits
	int np_piodirection;     // write/readable, up to 32 bits, 1->output bit
	int np_piointerruptmask; // write/readable, up to 32 bits, 1->enable interrupt
	int np_pioedgecapture;   // read, up to 32 bits, cleared by any write
	} np_pio;

// PIO Routines
void nr_pio_showhex(int value); // shows low byte on pio named na_seven_seg_pio


// altera_avalon_sdram_controller: 1 present 

// altera_avalon_spi: 1 present 
// SPI Registers
typedef volatile struct
	{
	int np_spirxdata;       // Read-only, 1-16 bit
	int np_spitxdata;       // Write-only, 1-16 bit
	int np_spistatus;       // Read-only, 9-bit
	int np_spicontrol;      // Read/Write, 9-bit
	int np_spireserved;     // reserved
	int np_spislaveselect;  // Read/Write, 1-16 bit, master only
	} np_spi;

// SPI Status Register Bits
enum
{
	np_spistatus_e_bit    = 8,
	np_spistatus_rrdy_bit = 7,
	np_spistatus_trdy_bit = 6,
	np_spistatus_tmt_bit  = 5,
	np_spistatus_toe_bit  = 4,
	np_spistatus_roe_bit  = 3,

	np_spistatus_e_mask    = (1 << 8),
	np_spistatus_rrdy_mask = (1 << 7),
	np_spistatus_trdy_mask = (1 << 6),
	np_spistatus_tmt_mask  = (1 << 5),
	np_spistatus_toe_mask  = (1 << 4),
	np_spistatus_roe_mask  = (1 << 3),
	};

// SPI Control Register Bits
enum
	{
	np_spicontrol_ie_bit    = 8,
	np_spicontrol_irrdy_bit = 7,
	np_spicontrol_itrdy_bit = 6,
	np_spicontrol_itmt_bit  = 5,
	np_spicontrol_itoe_bit  = 4,
	np_spicontrol_iroe_bit  = 3,

	np_spicontrol_ie_mask    = (1 << 8),
	np_spicontrol_irrdy_mask = (1 << 7),
	np_spicontrol_itrdy_mask = (1 << 6),

	np_spicontrol_itoe_mask  = (1 << 4),
	np_spicontrol_iroe_mask  = (1 << 3),
	};

// SPI Routines.
int spi_rxchar(np_spi *spiBase);
int spi_txchar(int i, np_spi *spiBase);



// altera_avalon_timer: 1 present 

// ----------------------------------------------
// Timer Peripheral

// Timer Registers
typedef volatile struct
	{
	int np_timerstatus;  // read only, 2 bits (any write to clear TO)
	int np_timercontrol; // write/readable, 4 bits
	int np_timerperiodl; // write/readable, 16 bits
	int np_timerperiodh; // write/readable, 16 bits
	int np_timersnapl;   // read only, 16 bits
	int np_timersnaph;   // read only, 16 bits
	} np_timer;

// Timer Register Bits
enum
	{
	np_timerstatus_run_bit    = 1, // timer is running
	np_timerstatus_to_bit     = 0, // timer has timed out

	np_timercontrol_stop_bit  = 3, // stop the timer
	np_timercontrol_start_bit = 2, // start the timer
	np_timercontrol_cont_bit  = 1, // continous mode
	np_timercontrol_ito_bit   = 0, // enable time out interrupt

	np_timerstatus_run_mask    = (1<<1), // timer is running
	np_timerstatus_to_mask     = (1<<0), // timer has timed out

	np_timercontrol_stop_mask  = (1<<3), // stop the timer
	np_timercontrol_start_mask = (1<<2), // start the timer
	np_timercontrol_cont_mask  = (1<<1), // continous mode
	np_timercontrol_ito_mask   = (1<<0)  // enable time out interrupt
	};

// Timer Routines
int nr_timer_milliseconds(void);	// Starts on first call, hogs timer1.


// altera_avalon_uart: 2 present 

// UART Registers
typedef volatile struct
	{
	int np_uartrxdata;  // Read-only, 8-bit
	int np_uarttxdata;  // Write-only, 8-bit
	int np_uartstatus;  // Read-only, 8-bit
	int np_uartcontrol; // Read/Write, 9-bit
	int np_uartdivisor; // Read/Write, 16-bit, optional
	} np_uart;

// UART Status Register Bits
enum
	{
	np_uartstatus_e_bit    = 8,
	np_uartstatus_rrdy_bit = 7,
	np_uartstatus_trdy_bit = 6,
	np_uartstatus_tmt_bit  = 5,
	np_uartstatus_toe_bit  = 4,
	np_uartstatus_roe_bit  = 3,
	np_uartstatus_brk_bit  = 2,
	np_uartstatus_fe_bit   = 1,
	np_uartstatus_pe_bit   = 0,

	np_uartstatus_e_mask    = (1<<8),
	np_uartstatus_rrdy_mask = (1<<7),
	np_uartstatus_trdy_mask = (1<<6),
	np_uartstatus_tmt_mask  = (1<<5),
	np_uartstatus_toe_mask  = (1<<4),
	np_uartstatus_roe_mask  = (1<<3),
	np_uartstatus_brk_mask  = (1<<2),
	np_uartstatus_fe_mask   = (1<<1),
	np_uartstatus_pe_mask   = (1<<0)
	};

// UART Status Register Bits
enum
	{
	np_uartcontrol_tbrk_bit  = 9,
	np_uartcontrol_ie_bit    = 8,
	np_uartcontrol_irrdy_bit = 7,
	np_uartcontrol_itrdy_bit = 6,
	np_uartcontrol_itmt_bit  = 5,
	np_uartcontrol_itoe_bit  = 4,
	np_uartcontrol_iroe_bit  = 3,
	np_uartcontrol_ibrk_bit  = 2,
	np_uartcontrol_ife_bit   = 1,
	np_uartcontrol_ipe_bit   = 0,

	np_uartcontrol_tbrk_mask  = (1<<9),
	np_uartcontrol_ie_mask    = (1<<8),
	np_uartcontrol_irrdy_mask = (1<<7),
	np_uartcontrol_itrdy_mask = (1<<6),
	np_uartcontrol_itmt_mask  = (1<<5),
	np_uartcontrol_itoe_mask  = (1<<4),
	np_uartcontrol_iroe_mask  = (1<<3),
	np_uartcontrol_ibrk_mask  = (1<<2),
	np_uartcontrol_ife_mask   = (1<<1),
	np_uartcontrol_ipe_mask   = (1<<0)
	};

// UART Routines
int nr_uart_rxchar(np_uart *uartBase);        // 0 for default UART
int nr_uart_txcr(void);
int nr_uart_txchar(int c,np_uart *uartBase); // 0 for default UART
int nr_uart_txhex(int x);                     // 16 or 32 bits
int nr_uart_txhex16(short x);
int nr_uart_txhex32(long x);
int nr_uart_txstring(char *s);


// altera_avalon_user_defined_interface: 1 present 

// ----------------------------------------------
// User Socket

typedef void *np_usersocket;


// altera_nios: 1 present 

// Nios CPU Routines
void nr_installcwpmanager(void);	// called automatically at by nr_setup.s
void nr_delay(int milliseconds);	// approximate timing based on clock speed
void nr_zerorange(char *rangeStart,int rangeByteCount);

// Nios ISR Manager Routines
typedef void (*nios_isrhandlerproc)(int context);
void nr_installuserisr(int trapNumber,nios_isrhandlerproc handlerProc,int context);

// Nios GDB Stub Routines
void nios_gdb_install(int active);
#define nios_gdb_breakpoint() asm("TRAP 5")

#if NIOS_GDB
	#define NIOS_GDB_SETUP 		\
		nios_gdb_install(1);	\
		nios_gdb_breakpoint();
#else
	#define NIOS_GDB_SETUP
#endif

// Nios setjmp/longjmp support
// (This will collide with <setjmp.h>
// if you include it! These are the
// nios-correct versions, however.)

typedef int jmp_buf[2];

int nr_setjmp(jmp_buf env);
int nr_longjmp(jmp_buf env,int value);

#define setjmp(a,b) nr_setjmp((a),(b))
#define longjmp(a,b) nr_longjmp((a),(b))


// altera_nios_dev_board_flash: 1 present 

// altera_nios_dev_board_sram32: 1 present 

// mtx_sodimm_flash_controller: 1 present 


#ifdef __cplusplus
}
#endif

#endif // _nios_peripherals_

/* end of file */
