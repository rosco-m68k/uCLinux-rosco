#ifndef _FTAPE_IO_H
#define _FTAPE_IO_H

/*
 * Copyright (C) 1993-1995 Bas Laarhoven.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *
 $Source: /cvs/sw/linux-2.0.x/drivers/char/ftape/ftape-io.h,v $
 $Author: christ $
 *
 $Revision: 1.1.1.1 $
 $Date: 1999/11/22 03:47:17 $
 $State: Exp $
 *
 *      This file contains definitions for the glue part
 *      of the QIC-40/80 floppy-tape driver for Linux.
 */

#include "vendors.h"

typedef struct {
	unsigned seek;
	unsigned reset;
	unsigned rewind;
	unsigned head_seek;
	unsigned stop;
	unsigned pause;
} timeout_table;

/*
 *      ftape-io.c defined global vars.
 */
extern timeout_table timeout;
extern int qic_std;
extern int tape_len;
extern volatile int current_command;
extern const struct qic117_command_table qic117_cmds[];
extern int might_be_off_track;

/*
 *      ftape-io.c defined global functions.
 */
extern void udelay(int usecs);
extern int udelay_calibrate(void);
extern void ftape_sleep(unsigned int time);
extern void ftape_report_vendor_id(unsigned int *id);
extern int ftape_command(int command);
extern int ftape_command_wait(int command, int timeout, int *status);
extern int ftape_report_drive_status(int *status);
extern int ftape_report_raw_drive_status(int *status);
extern int ftape_report_status(int *status);
extern int ftape_interrupt_wait(int time);
extern int ftape_ready_wait(int timeout, int *status);
extern int ftape_seek_head_to_track(int track);
extern int ftape_parameter(int command);
extern int ftape_in_error_state(int status);
extern int ftape_set_data_rate(int rate);
extern int ftape_report_error(int *error, int *command, int report);
extern int ftape_reset_drive(void);
extern int ftape_put_drive_to_sleep(vendor_struct drive_type);
extern int ftape_wakeup_drive(wake_up_types method);

#endif
