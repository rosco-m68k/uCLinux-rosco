/*
 *  quicc_smc.c -- serial driver for the 68360 Serial Managment Controller
 *
 *  Copyright (C) 2002 SED Systems, a Division of Calian Ltd.
 *      (hamilton@sedsystems.ca)
 *
 *  Based on and copied from mcfserial.c which was:
 *
 *  Copyright (C) 1999 Greg Ungerer (gerg@snapgear.com)
 *  Copyright (C) 2000, Lineo Inc. (www.lineo.com)
 *
 *  Based on code from 68332serial.c
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/segment.h>
#include <asm/bitops.h>

#include <asm/quicc_cpm.h>
#include <asm/quicc_cpm_i.h>
#include <asm/quicc_smc_i.h>

static struct serial_buffer_t smc2_tx_buffers[NUMBER_TX_BUFFERS];
static struct serial_buffer_t smc2_rx_buffers[NUMBER_RX_BUFFERS];
static struct serial_buffer_t smc1_tx_buffers[NUMBER_TX_BUFFERS];
static struct serial_buffer_t smc1_rx_buffers[NUMBER_RX_BUFFERS];

enum { NR_SMC_PORTS = 2 };

enum { SERIAL_TYPE_NORMAL = 1, SERIAL_TYPE_CALLOUT = 2 };

static struct quicc_smc_serial_t smc_table[NR_SMC_PORTS];

static const char DRIVER_NAME[] = "SED QUICC SMC SERIAL DRIVER V0.01";
DECLARE_TASK_QUEUE(quicc_smc_serial_task_queue);

#ifdef SERIAL_DEBUGGING
#undef SERIAL_DEBUGGING
#endif

//#define SERIAL_DEBUGGING

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

int smc_mem_init()
{
        /* ttyS0 is SMC 2, ttyS1 is SMC 1 */
        smc_table[0].magic = 0;
        smc_table[0].smc_uart_ram =
                &(pquicc->pram[3].scc.pothers.idma_smc.psmc.u);
        smc_table[0].smc_registers = &(pquicc->smc_regs[1]);
        smc_table[0].smc = SMC2;
        smc_table[0].tx_buffer = smc2_tx_buffers;
        smc_table[0].rx_buffer = smc2_rx_buffers;
        smc_table[0].tx_buffer_descriptor =
                pquicc->ch_or_u.mem_config.smc2_tx_bd;
        smc_table[0].rx_buffer_descriptor =
            pquicc->ch_or_u.mem_config.smc2_rx_bd;
        smc_table[0].current_tx_buffer = 0;
        smc_table[0].current_rx_buffer = 0;
        smc_table[0].irq = CPMVEC_SMC2;
        smc_table[0].flags = ASYNC_BOOT_AUTOCONF;
        smc_table[0].brg_aquired = 0;
        smc_table[0].irq_aquired = 0;

        smc_table[1].magic = 0;
        smc_table[1].smc_uart_ram =
                &(pquicc->pram[2].scc.pothers.idma_smc.psmc.u);
        smc_table[1].smc_registers = &(pquicc->smc_regs[0]);
        smc_table[1].smc = SMC1;
        smc_table[1].tx_buffer = smc1_tx_buffers;
        smc_table[1].rx_buffer = smc1_rx_buffers;
        smc_table[1].tx_buffer_descriptor =
                pquicc->ch_or_u.mem_config.smc1_tx_bd;
        smc_table[1].rx_buffer_descriptor =
            pquicc->ch_or_u.mem_config.smc1_rx_bd;
        smc_table[1].current_tx_buffer = 0;
        smc_table[1].current_rx_buffer = 0;
        smc_table[1].irq = CPMVEC_SMC1;
        smc_table[1].flags = ASYNC_BOOT_AUTOCONF;
        smc_table[1].brg_aquired = 0;
        smc_table[1].irq_aquired = 0;

        return(0);
};

/************************************************************************
*                              Serial Console                           *
************************************************************************/
static int smc_console_inited = 0;
int smc_console_baud_rate = DEFAULT_CONSOLE_DATARATE;
int smc_console_port      = DEFAULT_SERIAL_CONSOLE;

static void rs_quicc_smc_console_print(char const *);

console_print_function_t rs_quicc_smc_console_init()
{
        struct quicc_smc_serial_t *private_data;
        struct smc_regs *smc_registers;
        struct smc_uart_pram *smc_uart_ram;
        unsigned int i;

        if((smc_console_port >= NR_SMC_PORTS) || (smc_console_port < 0))
               return(NULL);
       
        /*Eliminate a few pointer de-references.*/
        private_data =&(smc_table[smc_console_port]);
        smc_registers = private_data->smc_registers;
        smc_uart_ram = private_data->smc_uart_ram;
    
        /*Diable the transmitter and receiver if they are being used. */
        if(cpm_command(private_data->smc, HARD_STOP_TX) != 0)
                return(NULL);

        smc_registers->smcmr &= ~(SMCMR_TX_ENABLE | SMCMR_RX_ENABLE);

        if(cpm_command(private_data->smc, CLOSE_RX_BD) != 0)
                return(NULL);

        /*Set up receive buffer descriptors */
        for(i = 0; i < NUMBER_RX_BUFFERS; ++i)
        {
                if(i == (NUMBER_RX_BUFFERS - 1))
                        private_data->rx_buffer_descriptor[i].status =
                                RX_BD_EMPTY | RX_BD_INTERRUPT | RX_BD_WRAP;
                else
                        private_data->rx_buffer_descriptor[i].status =
                                RX_BD_EMPTY | RX_BD_INTERRUPT;
           
                private_data->rx_buffer_descriptor[i].buf =
                        (private_data->rx_buffer[i].buffer);
               
                private_data->rx_buffer[i].current_character = 0;
        }
        private_data->current_rx_buffer = 0;

        /*Set up transmit buffer descriptors for write function call */
        for(i = 0; i < NUMBER_TX_BUFFERS; ++i)
        {
                if(i == (NUMBER_TX_BUFFERS - 1))
                        private_data->tx_buffer_descriptor[i].status =
                                TX_BD_WRAP;
                else
                        private_data->tx_buffer_descriptor[i].status = 0;

                private_data->tx_buffer_descriptor[i].length  = 0;
                private_data->tx_buffer_descriptor[i].buf =
                        (private_data->tx_buffer[i].buffer);
        }
        private_data->current_tx_buffer = 0;
       
        /*Configure the port B pins for the SMC transmit and receive */
        if(private_data->smc == SMC1)
        {
                pquicc->pip_pbpar |=  PORTB_SMC1_SET_32;
                pquicc->pip_pbdir &=  PORTB_SMC1_CLEAR_32;
                pquicc->pip_pbodr &=  PORTB_SMC1_CLEAR_16;
        }
        else
        {
                pquicc->pip_pbpar |=  PORTB_SMC2_SET_32;
                pquicc->pip_pbdir &=  PORTB_SMC2_CLEAR_32;
                pquicc->pip_pbodr &=  PORTB_SMC2_CLEAR_16;
        }
       
        /* Configure a BRG */
        {
                unsigned long baudrate = smc_console_baud_rate * 16;
                unsigned long ret_val;
                baudrate_generator_t brg = BRG_1;
                do
                {
                        ret_val = set_baudrate_generator(brg, baudrate);
                       
                        if(ret_val == 0)
                                brg = (baudrate_generator_t)(brg + 1);
                } while((ret_val == 0) && (brg < BRG_UB));

                /* Unable to get a baudrate generator */
                if(ret_val == 0)
                        return(NULL);

                private_data->baudrate_generator = brg;
                /*
                 *  DO NOT flag the BRG as aquired. The BRG will be locked
                 *  generating the baudrate for the console and will always be
                 *  available to generate the console baudrate.
                 */

                if(sitsa_config(private_data->smc, brg) != 0)
                        return(NULL);
        }
        smc_uart_ram->rbase = (unsigned short)
                (0x0000ffff &
                 (unsigned long)private_data->rx_buffer_descriptor);

        smc_uart_ram->tbase =
                (unsigned short)
                (0x0000ffff &
                 (unsigned long)private_data->tx_buffer_descriptor);
       
        /*Execute init params */
        if(cpm_command(private_data->smc, INIT_RX_TX_PARAMS) != 0)
                return(NULL);

        smc_uart_ram->rfcr = 0x18;
        smc_uart_ram->tfcr = 0x18;
        smc_uart_ram->mrblr = BUFFER_SIZE;
        smc_uart_ram->max_idl = 0;
        smc_uart_ram->brkln = 0;
        smc_uart_ram->brkec = 0;
        smc_uart_ram->brkcr = 0;

        /* The console is driven off of polling, not interrupts. */
        smc_registers->smce = 0xFF;
        smc_registers->smcm = 0;

        /* Start the transmitter */
        {
                unsigned short reg_val = 9;
                /* 8 databits + 1 start bit + 1 stop bit -1 */

                reg_val = reg_val << 11;
                reg_val |= SMCMR_UART_MODE;
                smc_registers->smcmr = reg_val;
                reg_val |= SMCMR_TX_ENABLE;
                smc_registers->smcmr = reg_val;
        }
        private_data->tx_disabled = 0;
        ++smc_console_inited;
        return(rs_quicc_smc_console_print);
}

static void rs_quicc_smc_console_print(char const *p)
{
        unsigned long flags;
        struct quicc_smc_serial_t *private_data;
        struct smc_regs *smc_registers;
        struct smc_uart_pram *smc_uart_ram;
        QUICC_BD *current_tx_bd = NULL;

        if((p == NULL) || (*p == '\0'))
                return;

        save_flags(flags); cli();
       
        private_data =&(smc_table[smc_console_port]);
        smc_registers = private_data->smc_registers;
        smc_uart_ram = private_data->smc_uart_ram;
       
        while(*p != '\0')
        {
                current_tx_bd =
                        &(private_data->tx_buffer_descriptor
                                        [private_data->current_tx_buffer]);

                /* Wait for an available buffer descriptor */
                if(private_data->tx_disabled)
                        cpm_command(private_data->smc, RESTART_TX);
                while(current_tx_bd->status & TX_BD_FULL);
                current_tx_bd->length = 0;

                while((*p != '\0') &&
                                (current_tx_bd->length < (BUFFER_SIZE - 1)))
                {
                        current_tx_bd->buf[current_tx_bd->length] = (*p);
                        ++current_tx_bd->length;
                        if(*p == '\n')
                        {
                                current_tx_bd->buf[current_tx_bd->length] =
                                        '\r';
                                ++current_tx_bd->length;
                        }
                        ++p;
                }

                /* Send the buffer. */
                if(private_data->tx_disabled)
                {
                    /*
                     *  don't generate an interrupt if the transmitter is
                     *  disabled.
                     */
                    unsigned int tmp = current_tx_bd->status;
                    tmp |= TX_BD_FULL;
                    tmp &= ~(TX_BD_INTERRUPT);
                    current_tx_bd->status = tmp;
                }
                else
                {
                    current_tx_bd->status |= (TX_BD_FULL | TX_BD_INTERRUPT);
                }

                if(*p != '\0')
                {
                        /* 
                         *  If there is more data to send, advance to the next
                         *  buffer.
                         */
                        ++private_data->current_tx_buffer;
                        if(private_data->current_tx_buffer == NUMBER_TX_BUFFERS)
                                private_data->current_tx_buffer = 0;
                }
        }
        /*Wait for the last buffer to be sent then advance to the next buffer */
        current_tx_bd =
            &(private_data->tx_buffer_descriptor
                    [private_data->current_tx_buffer]);
        while(current_tx_bd->status & TX_BD_FULL);

        ++private_data->current_tx_buffer;
        if(private_data->current_tx_buffer == NUMBER_TX_BUFFERS)
                private_data->current_tx_buffer = 0;

        /* If the transmitter was disabled, re-disable it */
        if(private_data->tx_disabled)
                cpm_command(private_data->smc, HARD_STOP_TX);
        restore_flags(flags);
        return;
}

/************************************************************************
*                       End of Serial Console                           *
************************************************************************/

static inline int smc_serial_paranoia_check(struct quicc_smc_serial_t *info,
                dev_t device, const char *routine)
{
        static const char *badmagic =
                "Warning: bad magic number for serial struct (%d, %d) in"
                " %s\n";
        static const char *badinfo =
                "Warning: null quicc_smc_serial_t for (%d, %d) in %s\n";

        if((info != &smc_table[0]) && (info != &smc_table[1]))
        {
                printk(badinfo, MAJOR(device), MINOR(device), routine);
                return(1);
        }
        if(info->magic != SERIAL_MAGIC)
        {
                printk(badmagic, MAJOR(device), MINOR(device), routine);
                return(1);
        }
        return(0);
}

/*
 *  Register the serial port.
 */
static struct tty_driver        quicc_smc_serial_driver,
                                quicc_smc_callout_driver;
static int                      quicc_smc_serial_refcount;
static struct tty_struct        *quicc_smc_tty_table[NR_SMC_PORTS];
static struct termios           *quicc_smc_termios[NR_SMC_PORTS];
static struct termios           *quicc_smc_termios_locked[NR_SMC_PORTS];

static int  quicc_smc_open(struct tty_struct *, struct file *);
static void quicc_smc_close(struct tty_struct *, struct file *);
static int  quicc_smc_write(struct tty_struct *, int,
                                const unsigned char *, int);
static void quicc_smc_flush_buffer(struct tty_struct *);
static void quicc_smc_set_termios(struct tty_struct *, struct termios *);
static void quicc_smc_stop(struct tty_struct *);
static void quicc_smc_start(struct tty_struct *);
static void quicc_smc_hangup(struct tty_struct *);
static void quicc_smc_interrupt(int, void *, struct pt_regs *);
static void quicc_smc_do_serial_hangup(void *);
static void quicc_smc_do_rx_softint(void *);
static void quicc_smc_do_tx_softint(void *);
static void quicc_smc_do_serial_bh(void);
static int  quicc_smc_write_room(struct tty_struct *);
static int  quicc_smc_startup(struct quicc_smc_serial_t *);
static void quicc_smc_shutdown(struct quicc_smc_serial_t *);
static int  quicc_smc_change_speed(struct quicc_smc_serial_t *);
static int  quicc_smc_ioctl(struct tty_struct *tty, struct file *filp,
                unsigned int cmd, unsigned long arg);
static void quicc_smc_put_char(struct tty_struct *, unsigned char);
static void quicc_smc_flush_chars(struct tty_struct *);

/* A really simple stub to simplify error handling. */
inline int  quicc_smc_restore_shutdown(struct quicc_smc_serial_t *info,
                unsigned long flags, int retval)
{
        quicc_smc_shutdown(info);
        restore_flags(flags);
        return(retval);
}

int rs_quicc_smc_init(void)
{
        struct quicc_smc_serial_t *info;
        unsigned long flags;
        int i;

#ifdef SERIAL_DEBUGGING
        printk("%s\n", __FUNCTION__);
#endif

        save_flags(flags); cli();

        init_bh(CM206_BH, quicc_smc_do_serial_bh);

        memset(&quicc_smc_serial_driver, 0, sizeof(struct tty_driver));
        quicc_smc_serial_driver.magic           = TTY_DRIVER_MAGIC;
        quicc_smc_serial_driver.name            = "ttyS";
        quicc_smc_serial_driver.major           = TTY_MAJOR;
        quicc_smc_serial_driver.minor_start     = 64;
        quicc_smc_serial_driver.num             = NR_SMC_PORTS;
        quicc_smc_serial_driver.type            = TTY_DRIVER_TYPE_SERIAL;
        quicc_smc_serial_driver.subtype         = SERIAL_TYPE_NORMAL;
        quicc_smc_serial_driver.init_termios    = tty_std_termios;
        quicc_smc_serial_driver.init_termios.c_cflag
                                        =B9600 | CS8 | CREAD | HUPCL | CLOCAL;

        cfsetospeed(&quicc_smc_serial_driver.init_termios,
                        smc_console_baud_rate);

        quicc_smc_serial_driver.flags           = TTY_DRIVER_REAL_RAW;
        quicc_smc_serial_driver.refcount        = &quicc_smc_serial_refcount;
        quicc_smc_serial_driver.table           = quicc_smc_tty_table;
        quicc_smc_serial_driver.termios         = quicc_smc_termios;
        quicc_smc_serial_driver.termios_locked  = quicc_smc_termios_locked;

        quicc_smc_serial_driver.open            = quicc_smc_open;
        quicc_smc_serial_driver.close           = quicc_smc_close;
        quicc_smc_serial_driver.write           = quicc_smc_write;
        quicc_smc_serial_driver.flush_chars     = quicc_smc_flush_chars;
        quicc_smc_serial_driver.put_char        = quicc_smc_put_char;
        quicc_smc_serial_driver.write_room      = quicc_smc_write_room;
        quicc_smc_serial_driver.flush_buffer    = quicc_smc_flush_buffer;
        quicc_smc_serial_driver.throttle        = NULL;
        quicc_smc_serial_driver.unthrottle      = NULL;
        quicc_smc_serial_driver.set_termios     = quicc_smc_set_termios;
        quicc_smc_serial_driver.stop            = quicc_smc_stop;
        quicc_smc_serial_driver.start           = quicc_smc_start;
        quicc_smc_serial_driver.hangup          = quicc_smc_hangup;
        quicc_smc_serial_driver.ioctl           = quicc_smc_ioctl;

        memcpy(&quicc_smc_callout_driver, &quicc_smc_serial_driver,
                        sizeof(struct tty_driver));
        quicc_smc_callout_driver.name           = "cua";
        quicc_smc_callout_driver.major          = TTYAUX_MAJOR;
        quicc_smc_callout_driver.subtype        = SERIAL_TYPE_CALLOUT;

        if(tty_register_driver(&quicc_smc_serial_driver))
                printk("Could not register QUICC SMC Serial Device.");
        else
                printk("Registered ttyS %s Driver.\n", DRIVER_NAME);

        if(tty_register_driver(&quicc_smc_callout_driver))
                printk("Could not register QUICC SMC Serial Callout Device.");
        else
                printk("Registered  cua %s Driver.\n", DRIVER_NAME);

        info = smc_table;
        for(i = 0; i < NR_SMC_PORTS; ++i)
        {
                info->magic                     = SERIAL_MAGIC;
                info->line                      = i;
                info->tty                       = NULL;
                info->close_delay               = 50;
                info->closing_wait              = 3000;
                info->event                     = 0;
                info->count                     = 0;
                info->blocked_open              = 0;
                info->baud                      = 16*smc_console_baud_rate;
                info->tqueue_receive.routine    = quicc_smc_do_rx_softint;
                info->tqueue_receive.data       = info;
                info->tqueue_transmit.routine   = quicc_smc_do_tx_softint;
                info->tqueue_transmit.data      = info;
                info->tqueue_hangup.routine     = quicc_smc_do_serial_hangup;
                info->tqueue_hangup.data        = info;
                info->callout_termios           =
                        quicc_smc_callout_driver.init_termios;
                info->normal_termios            =
                        quicc_smc_serial_driver.init_termios;
                info->open_wait                 = NULL;
                info->close_wait                = NULL;
                info->irq_aquired               = 0;
                info->tx_disabled               = 0;

                /*
                 *  Don't aquire the interrupt here, aquire it when the deivce
                 *  is opened for the first time. This will allow the SMC to
                 *  be used for different functions.
                 */
                ++info;
        }
        restore_flags(flags);
        return 0;
}

/*
 *  On open, if the serial port is already being used the open blocks or returns
 *  -EAGAIN if the open is non-blocking.
 */
static int quicc_smc_block_while_used(struct tty_struct *tty, struct file *filp,
                struct quicc_smc_serial_t *info)
{
        struct wait_queue       wait = { current, NULL };
        int                     retval;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(info->flags & ASYNC_CLOSING)
        {
                interruptible_sleep_on(&(info->close_wait));
#ifdef SERIAL_DO_RESTART
                if(info->flags & ASYNC_HUP_NOTIFY)
                        return(-EAGAIN);
                else
                        return(-ERESTARTSYS);
#else
                return(-EAGAIN);
#endif
        }

        /*
         *  If this is a callout device, then make sure the normal device is
         *  not being used.
         */
        if(tty->driver.subtype == SERIAL_TYPE_CALLOUT)
        {
                if(info->flags & ASYNC_NORMAL_ACTIVE)
                        return(-EBUSY);

                if((info->flags & ASYNC_CALLOUT_ACTIVE) &&
                   (info->flags & ASYNC_SESSION_LOCKOUT) &&
                   (info->session != current->session))
                        return(-EBUSY);

                if((info->flags & ASYNC_CALLOUT_ACTIVE) &&
                   (info->flags & ASYNC_PGRP_LOCKOUT) &&
                   (info->pgrp != current->pgrp))
                        return(-EBUSY);

                info->flags |= ASYNC_CALLOUT_ACTIVE;
                return(0);
        }

        if((filp->f_flags & O_NONBLOCK) ||
           (tty->flags & (1 << TTY_IO_ERROR)))
        {
                if(info->flags & ASYNC_CALLOUT_ACTIVE)
                        return(-EBUSY);

                info->flags |= ASYNC_NORMAL_ACTIVE;
                return(0);
        }

        retval = 0;
        add_wait_queue(&info->open_wait, &wait);
        /*
         *  info->count has been incremented but we are not opening the
         *  device yet so decrement it.
         */
        --info->count;
        ++info->blocked_open;
        while(1)
        {
                current->state = TASK_INTERRUPTIBLE;
                if((tty_hung_up_p(filp)) ||
                   (!info->flags & ASYNC_INITIALIZED))
                {
#ifdef SERIAL_DO_RESTART
                        if(info->flags & ASYNC_HUP_NOTIFY)
                                retval = -EAGAIN;
                        else
                                retval = -ERESTARTSYS;
#else
                        retval = -EAGAIN;
#endif
                        break;
                }

                if((!(info->flags & ASYNC_CALLOUT_ACTIVE)) &&
                   (!(info->flags & ASYNC_CLOSING)))
                        break;

                if(current->signal & ~current->blocked)
                {
                        retval = -ERESTARTSYS;
                        break;
                }

                schedule();
        }
        current->state = TASK_RUNNING;
        remove_wait_queue(&(info->open_wait), &wait);

        if(!tty_hung_up_p(filp))
                ++info->count;
        --info->blocked_open;
        if(retval)
                return(retval);

        info->flags |= ASYNC_NORMAL_ACTIVE;
        return(0);
}

static int quicc_smc_open(struct tty_struct *tty, struct file *filp)
{
        struct quicc_smc_serial_t       *info;
        int                             retval, line;

        line = MINOR(tty->device) - tty->driver.minor_start;
#ifdef SERIAL_DBUGGING
        printk("Trying to open ttyS%d as a quicc SMC.\n", line);
#endif
        if((line < 0) || (line >= NR_SMC_PORTS))
                return(-ENODEV);

        info = &(smc_table[line]);
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif

        ++info->count;
        tty->driver_data = info;
        info->tty = tty;

        retval = quicc_smc_block_while_used(tty, filp, info);
        if(retval)
        {
#ifdef SERIAL_DBUGGING
                printk("QUICC SMC BLOCK WHILE USED returned %d\n", retval);
#endif
                return(retval);
        }
        
        if((info->count == 1) && (info->flags & ASYNC_SPLIT_TERMIOS))
        {
                if(tty->driver.subtype == SERIAL_TYPE_NORMAL)
                        *tty->termios = info->normal_termios;
                else
                        *tty->termios = info->callout_termios;
        }

        if(info->count == 1)
        {
                info->baud = 0;
                info->put_char_buffer.current_character = 0;

                info->pip_pbdir = pquicc->pip_pbdir;
                info->pip_pbpar = pquicc->pip_pbpar;
                info->pip_pbodr = pquicc->pip_pbodr;

                retval = quicc_smc_startup(info);
                if(retval)
                {
#ifdef SERIAL_DBUGGING
                        printk("QUICC SMC STARTUP returned %d\n", retval);
#endif
                        --info->count;

                        pquicc->pip_pbdir = info->pip_pbdir;
                        pquicc->pip_pbpar = info->pip_pbpar;
                        pquicc->pip_pbodr = info->pip_pbodr;
                        
                        return(retval);
                }
        }

        info->session = current->session;
        info->pgrp = current->pgrp;

#ifdef SERIAL_DBUGGING
        printk("QUICC SMC OPEN succeeded.\n");
#endif
                        
        return(0);
}

static int  quicc_smc_startup(struct quicc_smc_serial_t *info)
{
        unsigned long   flags;
        int             retval;

        struct smc_uart_pram      *smc_uart_ram  = NULL;
        struct smc_regs           *smc_registers = NULL;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        smc_registers = info->smc_registers;
        smc_uart_ram = info->smc_uart_ram;

        if(info->flags & ASYNC_INITIALIZED)
                return(0);

        save_flags(flags); cli();

        /*
         *  If the port is not being used for the printk port, then
         *  the serial port buffers need setup.
         */
        if((smc_console_port != info->line) || (smc_console_inited == 0))
        {
                unsigned int              i;

                /*Disable the transmitter and receiver if they are being used.*/
                if(cpm_command(info->smc, HARD_STOP_TX) != 0)
                        return(quicc_smc_restore_shutdown(info, flags, -EIO));
                smc_registers->smcmr &= ~(SMCMR_TX_ENABLE | SMCMR_RX_ENABLE);
                if(cpm_command(info->smc, CLOSE_RX_BD) != 0)
                        return(quicc_smc_restore_shutdown(info, flags, -EIO));
                /*Set up receive buffer descriptors */
                for(i = 0; i < NUMBER_RX_BUFFERS; ++i)
                {
                        if(i == (NUMBER_RX_BUFFERS - 1))
                                info->rx_buffer_descriptor[i].status =
                                        RX_BD_EMPTY | RX_BD_INTERRUPT |
                                        RX_BD_WRAP;
                        else
                                info->rx_buffer_descriptor[i].status =
                                        RX_BD_EMPTY | RX_BD_INTERRUPT;
                        info->rx_buffer_descriptor[i].buf =
                                (info->rx_buffer[i].buffer);
                        info->rx_buffer[i].current_character = 0;
                }
                info->current_rx_buffer = 0;
               
                /*Set up transmit buffer descriptors for write function call */
                for(i = 0; i < NUMBER_TX_BUFFERS; ++i)
                {
                        if(i == (NUMBER_TX_BUFFERS - 1))
                                info->tx_buffer_descriptor[i].status =
                                        TX_BD_WRAP;
                        else
                                info->tx_buffer_descriptor[i].status = 0;
                       
                        info->tx_buffer_descriptor[i].length  = 0;
                        info->tx_buffer_descriptor[i].buf =
                                (info->tx_buffer[i].buffer);
                }
                info->current_tx_buffer = 0;
        }

        if(info->tty)
                info->tty->flags &= ~(1 << TTY_IO_ERROR);

        retval = quicc_smc_change_speed(info);
        if(retval)
                return(quicc_smc_restore_shutdown(info, flags, retval));

        info->flags |= ASYNC_INITIALIZED;
        restore_flags(flags);
        return(0);
}
/*
 *      Configure the serial port (datarate, stopbits,...)
 */
static int quicc_smc_change_speed(struct quicc_smc_serial_t *info)
{
        unsigned long   flags;
        int             retval;
        unsigned int    cflag;
        int             i;

        struct smc_uart_pram      *smc_uart_ram  = NULL;
        struct smc_regs           *smc_registers = NULL;
        QUICC_BD                  *current_tx_bd = NULL;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n\n", __FUNCTION__, info);
#endif
        if((info->tty == NULL) || (info->tty->termios == NULL))
                return(-EIO);

        smc_registers = info->smc_registers;
        smc_uart_ram = info->smc_uart_ram;

        /*
         *  The SMC does not have modem control lines so make sure they are
         *  are not being used.
         */
        info->tty->termios->c_cflag |= CLOCAL;

        cflag = info->tty->termios->c_cflag;

        /*
         *  Disable the receiver - leave interrupts in there current state so
         *  any outstanding data can be received.
         */
        smc_registers->smcmr &= ~(SMCMR_RX_ENABLE);
        save_flags(flags);

        if(cpm_command(info->smc, CLOSE_RX_BD) != 0)
                return(quicc_smc_restore_shutdown(info, flags, -EIO));

        save_flags(flags); cli();
        /*
         *  Wait for any outstanding data to be sent.
         */
        quicc_smc_flush_chars(info->tty);

        current_tx_bd = info->tx_buffer_descriptor;
        for(i = 0; i < NUMBER_TX_BUFFERS; ++i)
        {
                while(current_tx_bd->status & TX_BD_FULL);
                ++current_tx_bd;
        }

        if(cpm_command(info->smc, HARD_STOP_TX) != 0)
                return(quicc_smc_restore_shutdown(info, flags, -EIO));
        smc_registers->smcmr &= ~(SMCMR_TX_ENABLE);

        /*Set up receive buffer descriptors */
        for(i = 0; i < NUMBER_RX_BUFFERS; ++i)
        {
                if(i == (NUMBER_RX_BUFFERS - 1))
                        info->rx_buffer_descriptor[i].status =
                                RX_BD_EMPTY | RX_BD_INTERRUPT | RX_BD_WRAP;
                else
                        info->rx_buffer_descriptor[i].status =
                                RX_BD_EMPTY | RX_BD_INTERRUPT;
                info->rx_buffer_descriptor[i].buf =
                        (info->rx_buffer[i].buffer);
                info->rx_buffer[i].current_character = 0;
        }
        info->current_rx_buffer = 0;
       
        /*Set up transmit buffer descriptors for write function call */
        for(i = 0; i < NUMBER_TX_BUFFERS; ++i)
        {
                if(i == (NUMBER_TX_BUFFERS - 1))
                        info->tx_buffer_descriptor[i].status =
                                TX_BD_WRAP;
                else
                        info->tx_buffer_descriptor[i].status = 0;
                info->tx_buffer_descriptor[i].length = 0;
                info->tx_buffer_descriptor[i].buf =
                        (info->tx_buffer[i].buffer);
        }
        info->current_tx_buffer = 0;

        /*Configure the port B pins for the SMC transmit and receive */
        if(info->smc == SMC1)
        {
                pquicc->pip_pbpar |=  PORTB_SMC1_SET_32;
                pquicc->pip_pbdir &=  PORTB_SMC1_CLEAR_32;
                pquicc->pip_pbodr &=  PORTB_SMC1_CLEAR_16;
        }
        else
        {
                pquicc->pip_pbpar |=  PORTB_SMC2_SET_32;
                pquicc->pip_pbdir &=  PORTB_SMC2_CLEAR_32;
                pquicc->pip_pbodr &=  PORTB_SMC2_CLEAR_16;
        } 
        /* Configure a BRG */
        {
                unsigned long baudrate = cfgetospeed(info->tty->termios) * 16;
                unsigned long ret_val;
                baudrate_generator_t brg = BRG_1;

                if(info->baud)
                        baudrate = info->baud;
                
                if(info->brg_aquired)
                {
                        clear_baudrate_generator(info->baudrate_generator);
                        info->brg_aquired = 0;
                }

                do
                {
                        ret_val = set_baudrate_generator(brg, baudrate);

                        if(ret_val == 0)
                        {
                                brg = (baudrate_generator_t)(brg + 1);
                        }
                        else
                        {
                                info->brg_aquired = 1;
                                info->baud = baudrate;
                        }
                } while((ret_val == 0) && (brg < BRG_UB));

                /* Unable to get a baudrate generator */
                if(ret_val == 0)
                        return(quicc_smc_restore_shutdown(info, flags, -EIO));
               
                info->baudrate_generator = brg;
                if(sitsa_config(info->smc, brg) != 0)
                        return(quicc_smc_restore_shutdown(info, flags, -EIO));
        }
        smc_uart_ram->rbase = (unsigned short)
                (0x0000ffff&(unsigned long)info->rx_buffer_descriptor);
       
        smc_uart_ram->tbase =
                (unsigned short)(0x0000ffff&
                 (unsigned long)info->tx_buffer_descriptor);
                                                        
        /*Execute init params */
        if(cpm_command(info->smc, INIT_RX_TX_PARAMS) != 0)
                return(quicc_smc_restore_shutdown(info, flags, -EIO));
       
        smc_uart_ram->rfcr = 0x18;
        smc_uart_ram->tfcr = 0x18;
        smc_uart_ram->mrblr = BUFFER_SIZE;
        smc_uart_ram->max_idl = SXC_MAX_IDL;
        smc_uart_ram->brkln = 0;
        smc_uart_ram->brkec = 0;
        smc_uart_ram->brkcr = 0; 

        /* Configure the interrupts from the SMC.*/
        smc_registers->smce = 0xFF;

        smc_registers->smcm =
            (SMC_INT_RX_DATA | SMC_INT_BREAK_RECEIVED | SMC_INT_TX_DONE);

        if(info->irq_aquired == 0)
        {
                retval = request_irq(IRQ_MACHSPEC |info->irq,
                                quicc_smc_interrupt,
                                SA_INTERRUPT, DRIVER_NAME, info);

                if(retval)
                        return(quicc_smc_restore_shutdown(info, flags, retval));

                info->irq_aquired = 1;
        }

        /* Start the transmitter */
        {
                unsigned short reg_val;
                switch(cflag & CSIZE)
                {
                        case CS5: reg_val = 5; break;
                        case CS6: reg_val = 6; break;
                        case CS7: reg_val = 7; break;
                        default:  reg_val = 8; break;
                }

                if(cflag & PARENB)
                        ++reg_val;

                ++reg_val; /*There is at least 1 stop bit.*/
                if(cflag & CSTOPB)
                        ++reg_val; /* Two stop bits.*/

                reg_val = reg_val << 11;
                reg_val |= SMCMR_UART_MODE;

                if(cflag & CSTOPB)
                        reg_val |= SMCMR_TWO_STOP_BITS;

                if(cflag & PARENB)
                {
                        reg_val |= SMCMR_PARITY_ENABLE;
                        if((cflag & PARODD) == 0)
                                reg_val |= SMCMR_EVEN_PARITY;

                }
                smc_registers->smcmr = reg_val;
                reg_val |= (SMCMR_RX_ENABLE | SMCMR_TX_ENABLE);
                smc_registers->smcmr = reg_val;
        }

        restore_flags(flags);
        return(0);
}

static void quicc_smc_shutdown(struct quicc_smc_serial_t *info)
{
        unsigned long           flags;
        struct smc_regs         *smc_registers = info->smc_registers;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(!(info->flags & ASYNC_INITIALIZED))
                return;

        save_flags(flags); cli();

        /*
         *  Since we may be stopping the console port during this operation,
         *  printk's should not be tried. TODO: Make this stop and restart 
         *  cleaner (mabye flag the port is being temporarily stopped).
         */

        /* Disable transmit and receive. */
        smc_registers->smcmr = 0;
        cpm_command(info->smc, CLOSE_RX_BD);
        cpm_command(info->smc, HARD_STOP_TX);

        /* Remove the interrupt. */
        smc_registers->smce = 0xFF;
        smc_registers->smcm = 0;
        if(info->irq_aquired)
        {
                free_irq(IRQ_MACHSPEC | info->irq, info);
                info->irq_aquired = 0;
        }

        /* Stop using the baudrate generator. */
        if(info->brg_aquired)
        {
                clear_baudrate_generator(info->baudrate_generator);
                info->brg_aquired = 0;
        }
        /* Free the serial line ports. */
        if(info->smc == SMC1)
        {
                pquicc->pip_pbdir &= PORTB_SMC1_CLEAR_32;
                pquicc->pip_pbpar &= PORTB_SMC1_CLEAR_32;
        }
        else
        {
                pquicc->pip_pbdir &= PORTB_SMC2_CLEAR_32;
                pquicc->pip_pbpar &= PORTB_SMC2_CLEAR_32;
        }

        if(info->tty)
                info->tty->flags |= (1 << TTY_IO_ERROR);
        
        info->flags &= ~ASYNC_INITIALIZED;

        /* If we clobbered the printk port, restore it. */
        if((smc_console_inited) && (smc_console_port == info->line))
                if(rs_quicc_smc_console_init() == NULL)
                        panic("Unable to restore printk console.\n");
        restore_flags(flags);
        return;
}

static int quicc_smc_write(struct tty_struct *tty, int from_user, 
               const unsigned char *buffer, int count)
{
        struct quicc_smc_serial_t *info = NULL;
        unsigned long flags;
        struct smc_regs *smc_registers = NULL;
        struct smc_uart_pram *smc_uart_ram = NULL;
        int actual_sent;
        QUICC_BD *current_tx_bd = NULL;

        if(tty == NULL)
        {
#ifdef SERIAL_DEBUGGING
                printk("%s with tty = NULL\n", __FUNCTION__);
#endif
                return(0);
        }

        if((buffer == NULL) || (count == 0))
        {
#ifdef SERIAL_DEBUGGING
                printk("%s with buffer = 0x%p and count = %d\n",
                                __FUNCTION__, buffer, count);
#endif
                return(0);
        }

        info = (struct quicc_smc_serial_t *)tty->driver_data;
        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return(0);

        save_flags(flags); cli();
      
        if(info->put_char_buffer.current_character)
               quicc_smc_flush_chars(tty);

        smc_registers = info->smc_registers;
        smc_uart_ram = info->smc_uart_ram;

        current_tx_bd = &(info->tx_buffer_descriptor[info->current_tx_buffer]);

        actual_sent = 0;
        while(((current_tx_bd->status & TX_BD_FULL) == 0) &&
                        (actual_sent < count))
        {
                int amount_sending = MIN((count - actual_sent), BUFFER_SIZE);

                if(from_user)
                        memcpy_fromfs((void *)current_tx_bd->buf, buffer,
                                                amount_sending);
                else
                        memcpy((void *)current_tx_bd->buf, buffer,
                                        amount_sending);

                current_tx_bd->length = amount_sending;
                current_tx_bd->status |= (TX_BD_FULL | TX_BD_INTERRUPT);

                actual_sent += amount_sending;
                buffer += amount_sending;
                info->stats.tx += amount_sending;

                 ++info->current_tx_buffer;
                 if(info->current_tx_buffer == NUMBER_TX_BUFFERS)
                         info->current_tx_buffer = 0;

                 current_tx_bd =
                         &(info->tx_buffer_descriptor[info->current_tx_buffer]);
        }
        restore_flags(flags);

        info->stats.tx+=actual_sent;

        return(actual_sent);
}

static int  quicc_smc_write_room(struct tty_struct *tty)
{
        struct quicc_smc_serial_t *info = NULL;
        int free_buffers = 0;
        int i = 0;
       
        if(tty == NULL)
        {
#ifdef SERIAL_DEBUGGING
                printk("%s returning with 0 because tty==NULL\n", __FUNCTION__);
#endif
                return(0);
        }
       
        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__ ))
                return(0);

        /*
         *  Worst case is that each buffer is used to send one character
         *  (very wasteful but needed when a person is typing).
         */
        for(i = 0; i < NUMBER_TX_BUFFERS; ++i)
        {
                QUICC_BD *current_tx_bd = &(info->tx_buffer_descriptor[i]);
                if((current_tx_bd->status & TX_BD_FULL) == 0)
                        ++free_buffers;
        }

#ifdef SERIAL_DEBUGGING
        printk("%s free_buffers = %d\n", __FUNCTION__, free_buffers);
#endif
        return(free_buffers);
}

static void quicc_smc_flush_buffer(struct tty_struct *tty)
{
        struct quicc_smc_serial_t *info = NULL;
        unsigned int i;
        struct smc_regs *smc_registers = NULL;
        unsigned long flags;

        if(tty == NULL)
                return;

        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return;
        smc_registers = info->smc_registers;

        save_flags(flags);cli();

        /* Stop transmitting so we can clear the buffers. */
        cpm_command(info->smc, HARD_STOP_TX);
        smc_registers->smcmr &= ~(SMCMR_TX_ENABLE);

        /*Set up transmit buffer descriptors for write function call */
        for(i = 0; i < NUMBER_TX_BUFFERS; ++i)
        {
                if(i == (NUMBER_TX_BUFFERS - 1))
                        info->tx_buffer_descriptor[i].status =
                                TX_BD_WRAP;
                else
                        info->tx_buffer_descriptor[i].status = 0;
                info->tx_buffer_descriptor[i].length = 0;
                info->tx_buffer_descriptor[i].buf =
                        (info->tx_buffer[i].buffer);
        }
        info->current_tx_buffer = 0;

        /* re-initialize transmit buffers and start transmitter. */
        cpm_command(info->smc, INIT_TX_PARAMS);
        smc_registers->smcmr |= SMCMR_TX_ENABLE;

        restore_flags(flags);

        wake_up_interruptible(&tty->write_wait);
        if((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
                        (tty->ldisc.write_wakeup))
                (tty->ldisc.write_wakeup)(tty);
}

static void quicc_smc_set_termios(struct tty_struct *tty,
                struct termios *old_termios)
{
        struct quicc_smc_serial_t *info = NULL;
        if(tty == NULL)
                return;

        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return;

        if(old_termios == NULL)
        {
                info->baud = 0;
                quicc_smc_change_speed(info);
                return;
        }

        if(tty->termios->c_cflag == old_termios->c_cflag)
                return;

        info->baud = 0;
        if(quicc_smc_change_speed(info) != 0)
                panic("Error changing the serial port configuration.\n");

        return;
}

static void quicc_smc_close(struct tty_struct *tty, struct file *filp)
{
        struct quicc_smc_serial_t *info = NULL;
        unsigned long flags;

        if(tty == NULL)
                return;
       
        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif

        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return;

        save_flags(flags); cli();

        if(tty_hung_up_p(filp))
        {
                restore_flags(flags);
                return;
        }

        if((tty->count == 1) && (info->count != 1))
        {
                printk("quicc_smc_close: bad serial port count; tty->count"
                       " is 1, info->count is %d\n", info->count);
                info->count = 1;
        }
        
        if(info->count <= 0)
        {
                printk("quicc_smc_close: bad serial port count for ttyS%d"
                        " : %d\n", info->line, info->count);
                info->count = 1;
        }
        --(info->count);

        if(info->count)
        {
                restore_flags(flags);
                return;
        }

        info->flags |= ASYNC_CLOSING;

        if(info->flags & ASYNC_NORMAL_ACTIVE)
                info->normal_termios = *tty->termios;
        if(info->flags & ASYNC_CALLOUT_ACTIVE)
                info->callout_termios = *tty->termios;

        tty->closing = 1;
        if(info->closing_wait != ASYNC_CLOSING_WAIT_NONE)
                tty_wait_until_sent(tty, info->closing_wait);

        quicc_smc_shutdown(info);

        if(tty->driver.flush_buffer)
                tty->driver.flush_buffer(tty);
        if(tty->ldisc.flush_buffer)
                tty->ldisc.flush_buffer(tty);
        tty->closing = 0;
        info->event = 0;
        info->tty = 0;

        if (tty->ldisc.num != ldiscs[N_TTY].num)
        {
                if (tty->ldisc.close)
                        (tty->ldisc.close)(tty);
                tty->ldisc = ldiscs[N_TTY];
                tty->termios->c_line = N_TTY;
                if (tty->ldisc.open)
                        (tty->ldisc.open)(tty);
        }
        if (info->blocked_open)
        {
                if (info->close_delay)
                {
                        current->state = TASK_INTERRUPTIBLE;
                        current->timeout = jiffies + info->close_delay;
                        schedule();
                }
                wake_up_interruptible(&info->open_wait);
        }
        info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE|
                        ASYNC_CLOSING);
        wake_up_interruptible(&info->close_wait);
        restore_flags(flags);
        return;
}

static void quicc_smc_hangup(struct tty_struct *tty)
{
        struct quicc_smc_serial_t *info = NULL;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(tty == NULL)
                return;

        info = (struct quicc_smc_serial_t *)tty->driver_data;
        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return;

        quicc_smc_flush_buffer(tty);
        quicc_smc_shutdown(info);

        info->event = 0;
        info->count = 0;
        info->flags &= ~(ASYNC_NORMAL_ACTIVE | ASYNC_CALLOUT_ACTIVE);
        info->tty   = 0;
        wake_up_interruptible(&info->open_wait);
}

static void quicc_smc_do_serial_hangup(void *p)
{               
        struct quicc_smc_serial_t *info = (struct quicc_smc_serial_t *)p;
        struct tty_struct         *tty;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(info == NULL)
                return;

        tty = info->tty;
        if (!tty)
                return;

        tty_hangup(tty);
}
/* IOCTL and friends */
static void quicc_smc_send_break(struct quicc_smc_serial_t *, int);
static int  quicc_smc_get_serial_info(struct quicc_smc_serial_t*,
                                      struct serial_struct *);
static int  quicc_smc_set_serial_info(struct quicc_smc_serial_t*,
                                      struct serial_struct *);
static int quicc_smc_ioctl(struct tty_struct *tty, struct file *filp,
                unsigned int cmd, unsigned long arg)
{
        struct quicc_smc_serial_t *info = NULL;
        int retval, error;

        if(tty == NULL)
                return(-ENOTTY);

        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return(-ENODEV);

        if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
            (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD)  &&
            (cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT))
        {
                if (tty->flags & (1 << TTY_IO_ERROR))
                        return(-EIO);
        }
        switch (cmd)
        {
                case TCSBRK:    /* SVID version: non-zero arg --> no break */
                        retval = tty_check_change(tty);
                        if (retval)
                                return retval;
                        tty_wait_until_sent(tty, 0);
                        if (!arg)
                                quicc_smc_send_break(info, HZ/4);
                                /* 1/4 second */
                        return(0);

                case TCSBRKP:   /* support for POSIX tcsendbreak() */
                        retval = tty_check_change(tty);
                        if (retval)
                                return retval;
                        tty_wait_until_sent(tty, 0);
                        quicc_smc_send_break(info, arg ? arg*(HZ/10) : HZ/4);
                        return(0);

                case TIOCGSOFTCAR:
                        error = verify_area(VERIFY_WRITE,
                                        (void *)arg, sizeof(long));
                        if (error)
                                return error;
                        put_fs_long(C_CLOCAL(tty) ? 1 : 0,
                                        (unsigned long *) arg);
                        return(0);

                case TIOCSSOFTCAR:
                        arg = get_fs_long((unsigned long *) arg);
                        /*
                         * SMCs don't have modem control lines so CLOCAL should
                         * always be set.
                         */
                        if(arg)
                        {
                                tty->termios->c_cflag |= CLOCAL;
                                return(0);
                        }
                        else
                        {
                                return(-EINVAL);
                        }

                case TIOCGSERIAL:
                        error = verify_area(VERIFY_WRITE, (void *) arg,
                                        sizeof(struct serial_struct));
                        if (error)
                                return(error);
                        else
                                return(quicc_smc_get_serial_info(info,
                                                (struct serial_struct *)arg));
                case TIOCSSERIAL:
                        return(quicc_smc_set_serial_info(info,
                                        (struct serial_struct *)arg));

                case TIOCSERGSTRUCT:
                        error = verify_area(VERIFY_WRITE, (void *)arg,
                                        sizeof(struct quicc_smc_serial_t));
                        if (error)
                                return(error);

                        memcpy_tofs((struct quicc_smc_serial_t *)arg, info,
                                        sizeof(struct quicc_smc_serial_t));
                        return(0); 

                case TIOCSERGETLSR: /* Get line status register */
                        /*
                           This is for a user space 485 driver, the 68360
                           would be better to have this a kernel driver.
                         */
                        return(-ENOIOCTLCMD);

                        /*These are for modem control signals */
                case TIOCMGET:
                case TIOCMBIS:
                case TIOCMBIC:
                case TIOCMSET:
                default:
                        return(-ENOIOCTLCMD);
        }
        return 0;
}
static void quicc_smc_send_break(struct quicc_smc_serial_t *info, int duration)
{
        struct smc_uart_pram *smc_uart_ram;
        unsigned short brkcr;
        unsigned long flags;
        unsigned long brkLength;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        save_flags(flags); cli();

        smc_uart_ram = info->smc_uart_ram;
        brkcr = smc_uart_ram->brkcr;

        brkLength = ((info->baud)/(160 * HZ))*(duration);
        /* Hold line in break state for a least three character times */
        if(brkLength < 3)
            brkLength = 3; 
        if(brkLength > 0x0000ffff)
            brkLength = 0x0000ffff;

        smc_uart_ram->brkcr = brkLength;

        /* Stop the transmitter so it sends a break sequence. */
        cpm_command(info->smc, HARD_STOP_TX);
        info->tx_disabled = 1;

        current->state = TASK_INTERRUPTIBLE;
        current->timeout = jiffies + duration;

        schedule();

        cli();
        cpm_command(info->smc, RESTART_TX);
        info->tx_disabled = 0;
        
        smc_uart_ram->brkcr = brkcr;
        restore_flags(flags);
}
static int  quicc_smc_get_serial_info(struct quicc_smc_serial_t *info,
                                      struct serial_struct *retinfo)
{
        struct serial_struct tmp;
        unsigned int i;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if (!retinfo)
                return -EFAULT;

        memset(&tmp, 0, sizeof(tmp));
        tmp.type = info->type;
        tmp.line = info->line;

        i = 0;
        while((i < NR_SMC_PORTS) && (info != &smc_table[i]))
                ++i;
        tmp.port =  i;
        tmp.irq = info->irq;
        tmp.flags = info->flags;
        tmp.baud_base = info->baud;
        tmp.close_delay = info->close_delay;
        tmp.closing_wait = info->closing_wait;
        tmp.custom_divisor = 16;
        memcpy_tofs(retinfo,&tmp,sizeof(*retinfo));
        return 0;
}

static int  quicc_smc_set_serial_info(struct quicc_smc_serial_t *info,
                                      struct serial_struct *new_info)
{
        struct serial_struct            new_serial;
        struct quicc_smc_serial_t       old_info;
        int                             retval = 0;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if (!new_info)
                return -EFAULT;

        memcpy_fromfs(&new_serial, new_info, sizeof(new_serial));
        old_info = *info;

        if (!suser())
        {
                if((new_serial.baud_base != info->baud) ||
                   (new_serial.type != info->type) ||
                   (new_serial.close_delay != info->close_delay) ||
                   ((new_serial.flags & ~ASYNC_USR_MASK) !=
                    (info->flags & ~ASYNC_USR_MASK)))
                        return(-EPERM);

                if(new_serial.custom_divisor != 16)
                        return(-EIO);

                info->flags = ((info->flags & ~ASYNC_USR_MASK) |
                                (new_serial.flags & ASYNC_USR_MASK));
                goto check_and_exit;
        }

        if (info->count > 1)
                return -EBUSY;

        /*
         * OK, past this point, all the error checking has been done.
         * At this point, we start making changes.....
         */
        info->baud = new_serial.baud_base;
        info->flags = ((info->flags & ~ASYNC_FLAGS) |
                        (new_serial.flags & ASYNC_FLAGS));
        info->type = new_serial.type;
        info->close_delay = new_serial.close_delay;
        info->closing_wait = new_serial.closing_wait;

check_and_exit:
        retval = quicc_smc_startup(info);
        return retval;
}

/* Finished IOCTl and FRIENDS. */

static void quicc_smc_stop(struct tty_struct *tty)
{
        struct quicc_smc_serial_t *info = NULL;
        unsigned long flags;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif

        if(tty == NULL)
                return;
       
        info = (struct quicc_smc_serial_t *)tty->driver_data;

        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return;

        save_flags(flags); cli();

        cpm_command(info->smc, HARD_STOP_TX);
        info->tx_disabled = 1;

        restore_flags(flags);
        return;
}
static void quicc_smc_start(struct tty_struct *tty)
{
        struct quicc_smc_serial_t *info = NULL;
        unsigned long flags;

        if(tty == NULL)
                return;
       
        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
       
        if(smc_serial_paranoia_check(info, tty->device, __FUNCTION__))
                return;
      
        if(info->tx_disabled)
        { 
                save_flags(flags); cli();
                
                cpm_command(info->smc, RESTART_TX);
                info->tx_disabled = 0;
                
                restore_flags(flags);
        }
        return; 
}

/*
 *  Interrupt handling. The interrupt should occur when a receive buffer is
 *  closed.
  */

static void quicc_smc_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
        struct quicc_smc_serial_t *info = (struct quicc_smc_serial_t *)dev_id;
        struct smc_regs *smc_registers = info->smc_registers;
        unsigned char int_cause = 0;
        struct tty_struct *tty;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
#ifdef SERIAL_DEBUGGING
        printk("%s with irq 0x%x and info->irq 0x%x.\n",
                       __FUNCTION__, irq, info->irq);
#endif

        if((info->irq | IRQ_MACHSPEC) != (irq | IRQ_MACHSPEC))
                panic("Bad call to %s", __FUNCTION__);

        /* get and acknowledge the interrupt cause */
        int_cause = smc_registers->smce;
        smc_registers->smce = int_cause;
        
        tty = info->tty;

        /*
         *  There is no terminal device to receive characters so free any 
         *  buffes that are being used.
         */
        if(tty == NULL)
        {
                unsigned int i;
                /* Acknowledge the data to re-use the buffer. */
                for(i = 0; i < NUMBER_RX_BUFFERS; ++i)
                {
                        if((info->rx_buffer_descriptor[i].status & RX_BD_EMPTY)
                                        == 0)
                                info->rx_buffer_descriptor[i].status |=
                                        RX_BD_EMPTY;
                }
                return;
        }

        /* If a break was received but no data received */
        if((int_cause & SMC_INT_BREAK_RECEIVED) &&
                        ((int_cause & SMC_INT_RX_DATA) == 0))
        {
                if(tty->flip.count < TTY_FLIPBUF_SIZE)
                {
                        ++(info->stats.rx);
                        ++(tty->flip.count);
                        ++(info->stats.rxbreak);
                        *tty->flip.flag_buf_ptr++ = TTY_BREAK;
                        *tty->flip.char_buf_ptr++ = '\0';
                }
                queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
                return;
        }

        if(int_cause & SMC_INT_RX_DATA)
                queue_task_irq_off(&info->tqueue_receive,
                                &quicc_smc_serial_task_queue);

        if(int_cause & SMC_INT_TX_DONE)
                queue_task_irq_off(&info->tqueue_transmit,
                                &quicc_smc_serial_task_queue);

        if((int_cause & SMC_INT_RX_DATA) || (int_cause & SMC_INT_TX_DONE))
                mark_bh(CM206_BH);

        return;
}
static void quicc_smc_do_serial_bh(void)
{
#ifdef SERIAL_DEBUGGING
        printk("%s\n", __FUNCTION__);
#endif
        run_task_queue(&quicc_smc_serial_task_queue);
}
/*
 *  This is called when there is data available in a serial port receive buffer.
 *  Copy as much as possible out of the buffer. If all buffers are empty,
 *  return. If all are not empty, reschedule this function after telling
 *  the tty that its buffer is full.
 */
static void quicc_smc_do_rx_softint(void *p)
{
        struct quicc_smc_serial_t *info = (struct quicc_smc_serial_t *)p;

        int current_rx_buffer;
        QUICC_BD *current_descriptor;
        unsigned int current_character;
        unsigned int current_length;
        struct tty_struct *tty;
        int copied_char = 0;

#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(info->tty == NULL)
                return;

        tty = info->tty;

        current_rx_buffer = info->current_rx_buffer;
        current_character =
                info->rx_buffer[current_rx_buffer].current_character;

        current_descriptor = &(info->rx_buffer_descriptor[current_rx_buffer]);

        if(current_descriptor->status & RX_BD_EMPTY)
                return;

        while(((current_descriptor->status & RX_BD_EMPTY) == 0) &&
              (tty->flip.count < TTY_FLIPBUF_SIZE))
        {
                current_length = current_descriptor->length;

                if(current_length <= current_character)
                {
#ifdef SERIAL_DEBUGGING
                        printk("current_rx_buffer = %d\n", current_rx_buffer);
                        printk("current_character = %d\n", current_character);
                        printk("current_length = %d\n", current_length);
                        printk("current_descriptor = 0x%p\n",
                                        current_descriptor);
                        printk("current_descriptor->status = 0x%x\n",
                                        current_descriptor->status);
#else
                        printk("Serial receive buffer pointers are screwed"
                               " up, discarding buffer and resolving.\n");
#endif
                        info->rx_buffer[current_rx_buffer].current_character
                                = 0;
                       
                }
                else
                {
                        /*
                         *  While there is data in the current buffer and
                         *  the tty can accept the data; copy the data to
                         *  the tty's buffer.
                         */
                        while((tty->flip.count < TTY_FLIPBUF_SIZE) &&
                              (current_character < current_length))
                        {
                                ++tty->flip.count;
                                ++info->stats.rx;
                                *tty->flip.char_buf_ptr++ =
                                        info->rx_buffer[current_rx_buffer].
                                                buffer[current_character];
                                ++current_character;

                                copied_char = 1;
                                
                                /*
                                 *  Check for errors, they only occur on the
                                 *  last character.
                                 */
                                if(current_character < current_length)
                                {
                                        *tty->flip.flag_buf_ptr++ = 0;
                                }
                                else if(current_descriptor->status &
                                                RX_BD_RXED_BREAKS)
                                {
                                        ++info->stats.rxbreak;
                                        *tty->flip.flag_buf_ptr++ = TTY_BREAK;
                                }
                                else if(current_descriptor->status &
                                                RX_BD_PARITY_ERROR)
                                {
                                        ++info->stats.rxparity;
                                        *tty->flip.flag_buf_ptr++ = TTY_PARITY;
                                }
                                else if(current_descriptor->status &
                                                RX_BD_OVERRUN)
                                {
                                        ++info->stats.rxoverrun;
                                        *tty->flip.flag_buf_ptr++ = TTY_OVERRUN;
                                }
                                else if(current_descriptor->status &
                                                RX_BD_FRAMING_ERROR)
                                {
                                        ++info->stats.rxframing;
                                        *tty->flip.flag_buf_ptr++ = TTY_FRAME;
                                }
                                else
                                {
                                        *tty->flip.flag_buf_ptr++ = 0;
                                }
                        } /* while characters can be copied from the buffer. */
                } /* If the pointers are valid. */

                /* The buffer current buffer is empty. */
                if(current_character >= current_length)
                {
                        if(current_rx_buffer == (NUMBER_RX_BUFFERS - 1))
                        {
                                current_descriptor->status =
                                        RX_BD_EMPTY | RX_BD_INTERRUPT |
                                        RX_BD_WRAP;
                                current_rx_buffer = 0;
                        }
                        else
                        {
                                current_descriptor->status =
                                        RX_BD_EMPTY | RX_BD_INTERRUPT;
                                ++current_rx_buffer;
                        }
                        /* advance to the next buffer */
                        info->current_rx_buffer = current_rx_buffer;
                        info->rx_buffer[current_rx_buffer].current_character
                                =  current_character = 0;
                        current_descriptor =
                                &(info->rx_buffer_descriptor
                                                [current_rx_buffer]);
                } /*If the current buffer is empty and we advanced to the next
                    buffer*/

        } /*while there are chars and they can be copied to the tty.*/

        if(tty->flip.count >= TTY_FLIPBUF_SIZE)
        {
                /*
                 *  The terminal device is full, tell the terminal that it
                 *  should try to get another buffer. Also, reschedule the
                 *  copy for the next timer tick to avoid discarding the
                 *  received data.
                 */
                //queue_task(&tty->flip.tqueue, &tq_timer);
                queue_task(&info->tqueue_receive, &tq_timer);
        }
        if((copied_char) || (tty->flip.count >= TTY_FLIPBUF_SIZE))
                queue_task(&tty->flip.tqueue, &tq_timer);
}
/*
 *  This function is called after a buffer is available to queue up data to
 *  send out of the serial port.
 */
static void quicc_smc_do_tx_softint(void *p)
{
        struct quicc_smc_serial_t *info = (struct quicc_smc_serial_t *)p;
        struct tty_struct *tty;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif

        if(info->tty == NULL)
                return;

        tty = info->tty;
       
        if((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
                        (tty->ldisc.write_wakeup != NULL))
                (tty->ldisc.write_wakeup)(tty);

        wake_up_interruptible(&tty->write_wait);
}
static void quicc_smc_put_char(struct tty_struct *tty, unsigned char ch)
{
        struct quicc_smc_serial_t *info = NULL;
       
        if(tty == NULL)
        {
#ifdef SERIAL_DEBUGGING
                printk("%s returning with 0 because tty==NULL\n", __FUNCTION__);
#endif
                return;
        }

        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p, ch = '%c'\n", __FUNCTION__, info, ch);
#endif
        if(info->put_char_buffer.current_character < BUFFER_SIZE)
        {
                info->put_char_buffer.buffer[
                        info->put_char_buffer.current_character] = ch;
                ++info->put_char_buffer.current_character;
        }
        if(info->put_char_buffer.current_character == BUFFER_SIZE)
                quicc_smc_flush_chars(tty);
}

static void quicc_smc_flush_chars(struct tty_struct *tty)
{
        struct quicc_smc_serial_t *info = NULL;
       
        if(tty == NULL)
        {
#ifdef SERIAL_DEBUGGING
                printk("%s returning because tty==NULL\n", __FUNCTION__);
#endif
                return;
        }

        info = (struct quicc_smc_serial_t *)tty->driver_data;
#ifdef SERIAL_DEBUGGING
        printk("%s with info = 0x%p\n", __FUNCTION__, info);
#endif
        if(info->put_char_buffer.current_character)
        {
                unsigned int temp;
                unsigned int retval = 0;
                unsigned int tries = 0;

                temp = info->put_char_buffer.current_character;

                /* Zero this before calling write or it will call flush_chars */
                info->put_char_buffer.current_character = 0;

                if(temp > BUFFER_SIZE)
                        panic("Put char buffer overfilled.\n");

#ifdef SERIAL_DEBUGGING
                printk("Calling quicc smc write with temp = %d\n", temp);
#endif
                while((tries < 3) && (retval == 0))
                {
                        retval = quicc_smc_write(tty, 0,
                                        info->put_char_buffer.buffer, temp);
                        ++tries;
                }
        }
}
/* When querying the proc filesystem, return the stats. */
int quicc_smc_readproc(char *buffer)
{           
    struct quicc_smc_serial_t   *info;
    int                         len, i;

    len = sprintf(buffer,"%s\n", DRIVER_NAME);

    info = smc_table;

    for (i = 0; i < NR_SMC_PORTS; ++i)
    {
        len += sprintf((buffer + len),
                "%d: parameter ram:0x%p registers:0x%p irq=%d baud:%d\n",
                i, info->smc_uart_ram, info->smc_registers, info->irq,
                (info->baud/16));

        len += sprintf((buffer + len), "\ttx:%d rx:%d ",
                info->stats.tx, info->stats.rx);
        len += sprintf((buffer + len), "fe:%d ", info->stats.rxframing);
        len += sprintf((buffer + len), "pe:%d ", info->stats.rxparity);
        len += sprintf((buffer + len), "brk:%d ", info->stats.rxbreak);
        len += sprintf((buffer + len), "oe:%d \n", info->stats.rxoverrun);

        ++info;
    }

    return(len);
}
