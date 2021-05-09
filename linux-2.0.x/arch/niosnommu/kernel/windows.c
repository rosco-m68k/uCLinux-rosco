/* Vic - make note of the compiler bug in "flush_user_windows" */
/* windows.c: Routines to deal with register window management
 *            at the C-code level.
 *
 * Copyright (C) 1995 David S. Miller (davem@caip.rutgers.edu)
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/mm.h>

/* Do save's until all user register windows are out of the cpu. */
void flush_user_windows(void)
{
  //kh	if(current->tss.uwinmask)
  //kh		flush_user_windows();
//xxxx Warning - This is optimized by the nios compiler to branch to the start of the routine
//xxxx           avoiding the "save " instruction, instead of doing a call.
}



