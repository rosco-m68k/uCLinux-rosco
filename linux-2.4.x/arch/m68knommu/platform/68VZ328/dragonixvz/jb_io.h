/******************************************************************/
/*                                                                */
/* Module:       jb_io.h                                          */
/*                                                                */
/* Descriptions: Manages I/O related routines, for the DraginixVZ */
/*                                                                */
/* Revisions:    0.1 02/12/07                                     */
/*                                                                */
/******************************************************************/
/* $Id: jb_io.h,v 1.1.1.3 2003/08/29 01:04:26 davidm Exp $ */

#ifndef JB_IO_H
#define JB_IO_H
void initialize_jtag_hardware(void);
void close_jtag_hardware(void);
__inline__ void DriveSignal(int, int, int, int);
__inline__ int ReadTDO(int, int, int);
int low_printf(const char *, ...);
#endif
