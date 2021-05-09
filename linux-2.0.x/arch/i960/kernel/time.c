/*
 *  linux/arch/i960/kernel/time.c
 *  
 *  Copyright (C) 1999	Keith Adams	<kma@cse.ogi.edu>
 *  			Oregon Graduate Institute
 *  
 *  Based on:
 *  
 *  linux/arch/m68knommu/kernel/time.c
 *
 *  Copyright (C) 1998, 1999    D. Jeff Dionne <jeff@lineo.ca>,
 *                      	Kenneth Albanowski <kjahds@kjahds.com>,
 *
 *  Copied/hacked from:
 *
 *  linux/arch/m68k/kernel/time.c
 *
 *  Copyright (C) 1991, 1992, 1995  Linus Torvalds
 *
 * This file contains the i960-specific time handling details.
 */

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>

#include <asm/machdep.h>
#include <asm/segment.h>
#include <asm/io.h>
#include <asm/board/timer.h>

#include <linux/timex.h>


/*
 * XXX: bloody hell. We need to have figured out our machine's MHZ, and
 * programmed its clock accordingly.
 */
static inline int set_rtc_mmss(unsigned long nowtime)
{
	return -1;
}

int do_reboot = 0;

/*
 * timer_interrupt() needs to keep up the real-time clock,
 * as well as call the "do_timer()" routine every clocktick
 */
void timer_interrupt(struct pt_regs * regs)
{
	/* last time the cmos clock got updated */
	static long last_rtc_update=0;

	do_timer(regs);

	/*
	 * If we have an externally synchronized Linux clock, then update
	 * CMOS clock accordingly every ~11 minutes. Set_rtc_mmss() has to be
	 * called as close as possible to 500 ms before the new second starts.
	 */
	if (time_state != TIME_BAD && xtime.tv_sec > last_rtc_update + 660 &&
	    xtime.tv_usec > 500000 - (tick >> 1) &&
	    xtime.tv_usec < 500000 + (tick >> 1))
		if (set_rtc_mmss(xtime.tv_sec) == 0)
			last_rtc_update = xtime.tv_sec;
		else
			last_rtc_update = xtime.tv_sec - 600; /* do it again in 60 s */

}

/* Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * This algorithm was first published by Gauss (I think).
 *
 * WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines were long is 32-bit! (However, as time_t is signed, we
 * will already get problems at other places on 2038-01-19 03:14:08)
 */
static inline unsigned long mktime(unsigned int year, unsigned int mon,
	unsigned int day, unsigned int hour,
	unsigned int min, unsigned int sec)
{
	if (0 >= (int) (mon -= 2)) {	/* 1..12 -> 11,12,1..10 */
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}
	return (((
	    (unsigned long)(year/4 - year/100 + year/400 + 367*mon/12 + day) +
	      year*365 - 719499
	    )*24 + hour /* now have hours */
	   )*60 + min /* now have minutes */
	  )*60 + sec; /* finally seconds */
}

void time_init(void)
{
	unsigned int year, mon, day, hour, min, sec;

	extern void arch_gettod(int *year, int *mon, int *day, int *hour,
				int *min, int *sec);

	arch_gettod (&year, &mon, &day, &hour, &min, &sec);

	if ((year += 1900) < 1970)
		year += 100;
	xtime.tv_sec = mktime(year, mon, day, hour, min, sec);
	xtime.tv_usec = 0;

#ifdef CONFIG_M68328
	(*((volatile unsigned short*)0xFFFFF610)) = /*0x1000;*/ 0xd7e4; 
	(*((volatile unsigned short*)0xFFFFF60c)) = 0x33; /* Reset */
	(*((volatile unsigned short*)0xFFFFF60e)) = 0x2;  /* divider */
	(*((volatile unsigned long*)0xFFFFF304)) &= ~2;   /* Enable interrupt */
#endif
#ifdef CONFIG_M68332
	*(volatile unsigned short *)0xfffa22 = 0x0140;  /* ipl 6, vec 0x40 */
	*(volatile unsigned short *)0xfffa24 = 0x00a3;  /* 50 Hz */
#endif
}

/***************************************************************************
 *
 * do_gettimeoffset
 *
 * This works correctly at least on the 80303.  I'm not familiar enough
 * with the other i960 CPUs to know if they all have the same timer support,
 * or if this is specific to the io processors only.
 *
 ***************************************************************************/
static inline unsigned long do_gettimeoffset(void)
{
    unsigned long timer_count_register_0 = *TCR0;
    unsigned long ticks_elapsed = CYCLES_PER_HZ - timer_count_register_0;
    unsigned long count = ticks_elapsed / (CYCLES_PER_HZ / 1000000) / HZ;
    unsigned long offset = 0;
    
    /* How do you figure out the underflow probability?  I've just seen these comments
       on other platforms, but I can't say if it's also true for the i960 timer. */
    /* The probability of underflow is less than 2% */
    if (timer_count_register_0 > CYCLES_PER_HZ - CYCLES_PER_HZ / 50)
    {
        /* Check for pending timer interrupt */
        if(*IPND_REGISTER & 0x00001000)
        {   
            offset = CYCLES_PER_HZ / HZ;   
        }
    }

    /* transform to usecs */
    return offset + count;
}


/*
 * This version of gettimeofday now has near microsecond resolution with do_gettimeoffset.
 */
void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;
	
	save_flags(flags);
	cli();
	*tv = xtime;
    tv->tv_usec += do_gettimeoffset();
    
    if (tv->tv_usec >= 1000000) {
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}
    
	restore_flags(flags);
}

void do_settimeofday(struct timeval *tv)
{
	cli();
	/* This is revolting. We need to set the xtime.tv_usec
	 * correctly. However, the value in this location is
	 * is value at the last tick.
	 * Discover what correction gettimeofday
	 * would have done, and then undo it!
	 */
#if 0	 
	tv->tv_usec -= mach_gettimeoffset();
#endif

	if (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}

	xtime = *tv;
	time_state = TIME_BAD;
	time_maxerror = MAXPHASE;
	time_esterror = MAXPHASE;
	sti();
}
