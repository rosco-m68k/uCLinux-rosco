/* Timer configuration as needed by time.c and ints.c */

#define TRR0	(volatile unsigned long*)0xff000300
#define TCR0	(volatile unsigned long*)0xff000304
#define TMR0	(volatile unsigned long*)0xff000308

#define TRR1	(volatile unsigned long*)0xff000310
#define TCR1	(volatile unsigned long*)0xff000314
#define TMR1	(volatile unsigned long*)0xff000318

#define IPND_REGISTER (volatile unsigned long*)0xFF008500

/* some helper macros for programming the mode register, TMR0 */

/* the clock may decrement once per cpu cycle, once every two cycles, ...
 * up to eight; set the appropriate bits in TMR0
 */
#define CLOCKSHIFT 4
#define CLOCKDIV(x)	(x << CLOCKSHIFT)
#define CLOCKDIV1 CLOCKDIV(0)
#define CLOCKDIV2 CLOCKDIV(1)
#define CLOCKDIV4 CLOCKDIV(2)
#define CLOCKDIV8 CLOCKDIV(3)

#define CLOCK_ENABLE		2	/* set for clock to work */
#define CLOCK_AUTO_RELOAD	4	/* reload from TRR0 when we reach 0 */
#define CLOCK_SUPER_ONLY	8	/* don't let users reprogram clocks */


/* 100 Mhz bus */
#define CYCLES_PER_HZ	(100 * 1000* 1000) / HZ
