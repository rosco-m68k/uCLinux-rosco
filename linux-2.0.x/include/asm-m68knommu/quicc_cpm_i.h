/*
 *  Internal Interfaces for the management of the 68360
 *  Communication Processor Module (CPM). The only source files that should need
 *  to include this file are those in this directory.
 *
 *  Copyright (c) 2002 SED Systems, a Division of Calian Ltd.
 *          <hamilton@sedsystems.ca>
 */

#ifndef __QUICC_CPM_I_H__
#define __QUICC_CPM_I_H__

#include <linux/config.h>
#ifdef __KERNEL__
#if defined(CONFIG_M68360)
#include <asm/m68360.h>
#include <asm/quicc_cpm.h>

/*Defines for issuing commands to the CPM Command Register (CR) */
#define CPM_RESET           ((unsigned short)(0x8000))
#define CPM_COMMAND_BUSY    ((unsigned short)(0x0001))

/*Suggested value for the CPM DMA Serial DMA Control Register. See Page 7-60 */
#define CPM_DMA_SDCR        ((unsigned short)(0x0740))

/*Constants used in configuring the baudrate generators.*/
#define BRGC_DIV_16     ((unsigned int)(0x00000001))
#define BRG_RESET       ((unsigned int)(0x00000001 << 17))
#define BRG_ENABLE      ((unsigned int)(0x00000001 << 16))
#define BRG_RATE_MASK   ((unsigned int)(0x00001fff))
#define BRG_MAX_COUNT 4096

/*CPM Interrupt Controller (CPIC) constants */
// Interrupt priority 4, moved to its proper bit positions.
#define CPM_CPIC_INT_LEVEL  ((unsigned int)(0x00000004))
#define CPM_CPIC_IRL_MASK   ((unsigned int)(0x0000F000))

//Set Highest priority interrupt source to Parallele I/O - PC0 (default)
#define CPM_CPIC_HP         ((unsigned int)(0x0000001f))

//Specify order of priorities of for SCC interrupts
//SCC1 is in highest priority slot (SCaP), SCC4 is lowest priority (SCdP)
#define CPM_CPIC_SCAP       ((unsigned int)(0x00000000))
#define CPM_CPIC_SCBP       ((unsigned int)(0x00000001))
#define CPM_CPIC_SCCP       ((unsigned int)(0x00000002))
#define CPM_CPIC_SCDP       ((unsigned int)(0x00000003))

//Specify if the SCC priorities should be spread through the table(1) or
// grouped(0) together bellow highest priority an PC0 interrupt
#define CPM_CPIC_SCS        ((unsigned int)(0x00000000))

//Mask to get the VBA field from the CPIC error interrupt vector number
#define CPM_CPIC_VBA_MASK   ((unsigned int)(0x0000000E0))


/*Communication processor commands: */
typedef enum
{
        INIT_RX_TX_PARAMS   =   0x0000,
        INIT_RX_PARAMS      =   0x0100,
        INIT_TX_PARAMS      =   0x0200,
        ENTER_HUNT_MODE     =   0x0300,
        HARD_STOP_TX        =   0x0400,
        GRACEFULE_STOP_TX   =   0x0500,
        RESTART_TX          =   0x0600,
        CLOSE_RX_BD         =   0x0700
} cpm_command_t; /*Note: these values are the command op-codes. */
typedef enum
{
        BRG_LB  = -1,
        BRG_1   = 0,
        BRG_2   = 1,
        BRG_3   = 2,
        BRG_4   = 3,
        BRG_UB  = 4
} baudrate_generator_t;
/*Note: these enumerations are used as indexes to a private arrray. */
typedef enum
{
        SCC1 = 0x0000,
        SCC2 = 0x0040,
        SCC3 = 0x0080,
        SCC4 = 0x00C0,
        SMC1 = 0x0090,
        SMC2 = 0x00D0
} sxc_list_t;
/*Note: these values are not selected randomly, they are Channel numbers for
        the cpm command register. */

int cpm_command(sxc_list_t, cpm_command_t);
int sitsa_init(void);
int sitsa_config(sxc_list_t, baudrate_generator_t);
int cpic_init(void);
int sdma_init(void);
int sxc_list_to_vector(sxc_list_t);
int brg_mem_init(void);

/*
 *  Try to use a baudrate generator - returns the baudrate actually generates, 0
 *  if the baudrate generator cannot generate the desired datarate.
 */
unsigned long set_baudrate_generator(baudrate_generator_t, unsigned long);
/*Stop using the baudrate generator */
int clear_baudrate_generator(baudrate_generator_t);

/*Structure for holding the status of each baudrate generator*/
struct baudrate_generator_status_t
{
        baudrate_generator_t    name;
        unsigned int            usage_count;
        unsigned long volatile  *configuration_register;
};
extern struct baudrate_generator_status_t baudrate_generator_status[];
#endif
#endif
#endif
