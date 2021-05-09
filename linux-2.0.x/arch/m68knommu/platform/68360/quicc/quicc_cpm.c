/*
 *      Functions for management of the 68360 Communication Processor Module
 *      (CPM).
 *
 *      Copyright (c) 2002 SED Systems, a Division of Calian Ltd.
 *              <hamilton@sedsystems.ca>
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/system.h>
#include <asm/quicc_cpm.h>
#include <asm/quicc_cpm_i.h>
#include <asm/m68360_regs.h>
#include <asm/quicc_smc_i.h>
#include <asm/quicc_scc_i.h>

QUICC *pquicc;
extern void *_quicc_base;
unsigned long int system_clock;

struct baudrate_generator_status_t baudrate_generator_status[] =
{
        { BRG_1, 0, NULL},
        { BRG_2, 0, NULL},
        { BRG_3, 0, NULL},
        { BRG_4, 0, NULL}
};


QUICC *m68360_get_pquicc(void)
{
        return((struct quicc *)(_quicc_base));
}

int quicc_cpm_init(void)
{
        register int counter = 0;

        pquicc = m68360_get_pquicc();
        /* compute real system clock value */
        {
                unsigned int local_pllcr = (unsigned int)(pquicc->sim_pllcr);
                /* If the prescaler is dividing by 128 */
                if(local_pllcr & MCU_PREEN)
                {
                        int mf = (int)(pquicc->sim_pllcr & 0x0fff);
                        system_clock = (OSCILLATOR / 128) * (mf + 1);
                }
                else
                {
                        int mf = (int)(pquicc->sim_pllcr & 0x0fff);
                        system_clock = (OSCILLATOR) * (mf + 1);
                }
        }
        pquicc->cp_cr = CPM_RESET | CPM_COMMAND_BUSY;

        /* wait for reset to finish */
        while(pquicc->cp_cr & (CPM_COMMAND_BUSY | CPM_RESET))
        {
                if(counter > 2)
                {
                        /*THe panic interface is not initialized yet */
                        cli();
                        while(1);
                }
                ++counter;
                /*It should take at most 2 cycles to reset the cpm*/
        }
              
        if(sdma_init())
                return(-1);
        
        if(sitsa_init())
                return(-1);
        
        if(brg_mem_init())
                return(-1);

#if defined(CONFIG_M68360_SMC_UART)
        if(smc_mem_init())
                return(-1);
#endif

#if defined(CONFIG_M68360_SCC_UART)
        if(scc_mem_init())
                return(-1);
#endif
        
        if(cpic_init())
                return(-1);
       
        return(0);
}

int brg_mem_init()
{
        baudrate_generator_status[0].configuration_register = &(pquicc->brgc1);
        baudrate_generator_status[1].configuration_register = &(pquicc->brgc2);
        baudrate_generator_status[2].configuration_register = &(pquicc->brgc3);
        baudrate_generator_status[3].configuration_register = &(pquicc->brgc4);

        return(0);
}
int cpic_init()
{
        /*
         *  Disable all interrupts from the CPIC before starting - note
         *  devices need to get themselves added before seeing interrupts.
         */
        pquicc->intr_cimr = 0;

        pquicc->intr_cicr =  0;
        pquicc->intr_cicr |= (CPM_CPIC_SCDP         << 22);
        pquicc->intr_cicr |= (CPM_CPIC_SCCP         << 20);
        pquicc->intr_cicr |= (CPM_CPIC_SCBP         << 18);
        pquicc->intr_cicr |= (CPM_CPIC_SCAP         << 16);
        pquicc->intr_cicr |= (CPM_CPIC_INT_LEVEL    << 13);
        pquicc->intr_cicr |= (CPM_CPIC_HP           <<  8);
        pquicc->intr_cicr |= (CPMVEC_ERROR & CPM_CPIC_VBA_MASK);
        pquicc->intr_cicr |= (CPM_CPIC_SCS);

        return(0);
}
int cpm_interrupt_init()
{
        return(cpic_init());
}

int sdma_init()
{
        pquicc->sdma_sdcr = CPM_DMA_SDCR;
        return(0);
}
int sitsa_init()
{
        pquicc->si_sigmr   = 0;
        pquicc->si_simode  = 0;
        pquicc->si_sicmr   = 0;
        pquicc->si_sicr    = 0;

        return(0);
}

int sitsa_config(sxc_list_t sxc, baudrate_generator_t brg)
{
        unsigned int mask = 0xffffffff;
        unsigned int reg_val= 0;

        if((brg <= BRG_LB) || (brg >= BRG_UB))
                return(-ENODEV);

        switch(sxc)
        {
                case SMC1:
                case SMC2:
                        reg_val = brg;
                        if(sxc == SMC1)
                        {
                                reg_val = reg_val << 12;
                                mask = 0xffff0000;
                        }
                        else
                        {
                                reg_val = reg_val << 28;
                                mask = 0x0000ffff;
                        }
                        pquicc->si_simode &= mask;
                        pquicc->si_simode |= reg_val;
                        return(0);
                        break;
                case SCC1:
                case SCC2:
                case SCC3:
                case SCC4:
                        reg_val = brg;
                        reg_val = reg_val << 3;
                        reg_val = reg_val | brg;

                        switch(sxc)
                        {
                                case SCC1:
                                        mask = 0xffffff00;
                                        break;
                                case SCC2:
                                        mask = 0xffff00ff;
                                        reg_val = reg_val << 8;
                                        break;
                                case SCC3:
                                        mask = 0xff00ffff;
                                        reg_val = reg_val << 16;
                                        break;
                                case SCC4:
                                        mask = 0x00ffffff;
                                        reg_val = reg_val << 24;
                                        break;
                                default:
                                        break;
                        }
                        pquicc->si_sicr &= mask;
                        pquicc->si_sicr |= reg_val;
                        return(0);
                        break;

                default:
                        return(-ENODEV);
                        break;
        }
}

int cpm_command(sxc_list_t sxc, cpm_command_t command)
{
        unsigned short reg_val;
        register int counter = 0;
        switch(sxc)
        {
                case SCC1:
                case SCC2:
                case SCC3:
                case SCC4:
                case SMC1:
                case SMC2:
                        break;
                default:
                        return(-ENODEV);
                        break;
        }
        switch(command)
        {
                case INIT_RX_TX_PARAMS:
                case INIT_RX_PARAMS:
                case INIT_TX_PARAMS:
                case ENTER_HUNT_MODE:
                case HARD_STOP_TX:
                case GRACEFULE_STOP_TX:
                case RESTART_TX:
                case CLOSE_RX_BD:
                        break;
                default:
                        return(-ENODEV);
        }

        if(pquicc->cp_cr & CPM_COMMAND_BUSY)
                panic("CPM is busy before a command is issued.\n");

        reg_val = sxc | command | CPM_COMMAND_BUSY;

        pquicc->cp_cr = reg_val;
        while(pquicc->cp_cr & CPM_COMMAND_BUSY)
        {
                if(counter > 120)
                        panic("CPM mot completing commands.\n");
                ++counter;
        }

        return(0);
}

unsigned long set_baudrate_generator(baudrate_generator_t brg,
                        unsigned long baud_rate)
{
        unsigned long prescaler = 1;
        unsigned long brg_divisor;
        unsigned long try_2;
        unsigned long prescale_try_2;
        unsigned long error_try_1;
        unsigned long error_try_2;
        unsigned long brg_reg_val;
        unsigned long volatile *brgc;

        if((brg <= BRG_LB) || (brg >= BRG_UB))
                return(0);
        /*
         *  503 is minimum rate for 33MHz crystal
         *  25000000 is maximum for 25Mhz crystal
         *  TODO: CHECK THESE VALUES
         */
        if((baud_rate < 503) || (baud_rate > 25000000))
                return(0);
       
        /*
         *  This divides the clock too little (round off error) and is faster
         *  then the desired rate.
         */
        brg_divisor = system_clock/baud_rate;
        /* Divide the clock by one more and test what happens.*/
        try_2 = brg_divisor + 1;
        prescale_try_2 = 1;
       
        if(try_2 > BRG_MAX_COUNT)
        {
                prescale_try_2 = 16;
                try_2 = try_2 >> 4;
        }
       
        if(brg_divisor > BRG_MAX_COUNT)
        {
                brg_divisor = brg_divisor >> 4; /* divide by 16 */
                prescaler = 16;
        }
        /* If more division is needed then can be generated, cause an error. */
        if(brg_divisor > BRG_MAX_COUNT)
                return(0);
       
        error_try_1 = (system_clock/prescaler/brg_divisor) - baud_rate;
        error_try_2 = baud_rate - (system_clock/prescale_try_2/try_2);

        /*
         *  If try 2 takes us over the max count, force the system to use try 1.
         */
        if(try_2 > BRG_MAX_COUNT)
                error_try_2 = error_try_1;

        if(error_try_2 < error_try_1)
        {
                brg_divisor = try_2;
                prescaler = prescale_try_2;
        }

        brg_reg_val = (brg_divisor - 1) << 1;
       
        if(prescaler == 16)
                brg_reg_val = brg_reg_val | BRGC_DIV_16;

        brgc = baudrate_generator_status[brg].configuration_register;
        if(baudrate_generator_status[brg].usage_count == 0)
        {
                *brgc = BRG_RESET;
                *brgc = 0;
                brg_reg_val = brg_reg_val | BRG_ENABLE;
                *brgc = brg_reg_val;
        }
        else
        {
                if(brg_reg_val != ((*brgc) & BRG_RATE_MASK))
                        return(0);
        }

        ++baudrate_generator_status[brg].usage_count;
        return(system_clock/prescaler/brg_divisor);
}

int clear_baudrate_generator(baudrate_generator_t brg)
{
        unsigned long volatile *brgc;
        if((brg <= BRG_LB) || (brg >= BRG_UB))
                return(-ENODEV);

        if(baudrate_generator_status[brg].usage_count == 0)
                return(-ENODEV);

        --baudrate_generator_status[brg].usage_count;
       
        //Baudrate generator is no longer being used so stop it.
        if(baudrate_generator_status[brg].usage_count == 0)
        {
                brgc = baudrate_generator_status[brg].configuration_register;
                *brgc = BRG_RESET;
                *brgc = 0;
        }
        return(0);
}

int sxc_list_to_vector(sxc_list_t device)
{
        switch(device)
        {
                case SCC1: return(CPMVEC_SCC1);
                case SCC2: return(CPMVEC_SCC2);
                case SCC3: return(CPMVEC_SCC3);
                case SCC4: return(CPMVEC_SCC4);
                case SMC1: return(CPMVEC_SMC1);
                case SMC2: return(CPMVEC_SMC2);
                default: return(-1);
        }
}

/*
 *  This routine is copied from mcfserial.c
 */
int rs_quicc_console_setup(const char *arg)
{
        int console_port;
        int console_baud_rate = 0;

        if(strncmp(arg, "/dev/ttyS", 9) == 0)
        {
                console_port = arg[9] - '0';
                arg+= 10;
        }
        else
        {
                if(strncmp(arg, "/dev/cua", 8) == 0)
                {
                        console_port = arg[8] - '0';
                        arg += 9;
                }
                else
                {
                        return(0);
                }
        }
        if(*arg == ',')
                console_baud_rate = simple_strtoul(arg+1, NULL, 0);
#if defined(CONFIG_M68360_SMC_UART)
        smc_console_port = console_port;
#endif
#if defined(CONFIG_M68360_SCC_UART)
        scc_console_port = console_port - 2;
#endif

        if(console_baud_rate)
        {
#if defined(CONFIG_M68360_SMC_UART)
                smc_console_baud_rate = console_baud_rate;
#endif
#if defined(CONFIG_M68360_SCC_UART)
                scc_console_baud_rate = console_baud_rate;
#endif
        }

        return(1);
}
console_print_function_t rs_quicc_console_init(void)
{
        console_print_function_t retval = NULL;

#if defined(CONFIG_M68360_SMC_UART)
        retval = rs_quicc_smc_console_init();
#endif

        if(retval == NULL)
#if defined(CONFIG_M68360_SCC_UART)
            return(rs_quicc_scc_console_init());
#else
            return(NULL);
#endif
        else
            return(retval);
}
int rs_quicc_init(void)
{
        int ret_val = -ENODEV;
#if defined(CONFIG_M68360_SMC_UART)
        ret_val = rs_quicc_smc_init();
        if(ret_val)
        {
                return(ret_val);
        }
#endif
#if defined(CONFIG_M68360_SCC_UART)
        ret_val = rs_quicc_scc_init();
#endif
        return(ret_val);
}


unsigned long cfgetospeed(const struct termios *termios_p)
{
    switch(termios_p->c_cflag & (CBAUD | CBAUDEX))
    {
        case B50:       return(50);
        case B75:       return(75);
        case B110:      return(110);
        case B134:      return(134);
        case B150:      return(150);
        case B200:      return(200);
        case B300:      return(300);
        case B600:      return(600);
        case B1200:     return(1200);
        case B1800:     return(1800);
        case B2400:     return(2400);
        case B4800:     return(4800);
        case B9600:     return(9600);
        case B19200:    return(19200);
        case B38400:    return(38400);
        case B57600:    return(57600);
        case B115200:   return(115200);
        case B230400:   return(230400);
        default:        return(0);
    }
}

speed_t long_to_speed_t(long rate)
{
    switch(rate)
    {
        case 50: return(B50);
        case 75: return(B75);
        case 110: return(B110);
        case 134: return(B134);
        case 150: return(B150);
        case 200: return(B200);
        case 300: return(B300);
        case 600: return(B600);
        case 1200: return(B1200);
        case 1800: return(B1800);
        case 2400: return(B2400);
        case 4800: return(B4800);
        case 9600: return(B9600);
        case 19200: return(B19200);
        case 38400: return(B38400);
        case 57600: return(B57600);
        case 115200: return(B115200);
        case 230400: return(B230400);
        default: return(B0);
    }
}
int cfsetospeed(struct termios *termios_p, long rate)
{
    termios_p->c_cflag &= ~(CBAUD | CBAUDEX);
    termios_p->c_cflag |= long_to_speed_t(rate);
    return(0);
}

void quicc_kick_wdt(void)
{
        pquicc->sim_swsr=0x55;
        pquicc->sim_swsr=0xAA;
}
