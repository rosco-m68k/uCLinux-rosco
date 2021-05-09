/*
 *   FILE: i960.h
 * AUTHOR: kma
 *  DESCR: generic definitions of i960 data structures
 */

#ifndef I960_H
#define I960_H

#ident "$Id: i960.h,v 1.1 1999/12/03 06:02:34 gerg Exp $"

#ifdef __ASSEMBLY__
/*
 * register saving/restoring code. used by both syscall and interrupt handlers
 */

#define current	SYMBOL_NAME(current_set)
#define SAVE_ALL(freereg) \
	ldconst	64, freereg; \
	addo	freereg, sp, sp;	\
	stq	g0, -64(sp);	\
	stq	g4, -48(sp);	\
	stq	g8, -32(sp);	\
	stq	g12, -16(sp)

#define RESTORE_ALL(freereg) \
	ldq	-16(sp), g12;	\
	ldq	-32(sp), g8;	\
	ldq	-48(sp), g4;	\
	ldq	-64(sp), g0;	\
	ldconst	64, freereg;	\
	subo	freereg, sp, sp

#else
typedef unsigned long reg_t;

static inline unsigned long get_ipl(void)
{
	unsigned long retval;
	
	__asm__ __volatile__("modpc	0, 0, %0" : "=r" (retval));
	return ( (retval >> 16) & 0x1f);
}

static inline unsigned long get_pc(void)
{
	unsigned long retval;
	__asm__ __volatile__("modpc	0, 0, %0" : "=r" (retval));
	return retval;
}
/*
 * Interrupt record. Every interrupt handler has one located at fp-16.
 * We'll ultimately use the saved PC to determine whether we interrupted 
 * supervisor or user mode.
 */
typedef struct {
	reg_t		ir_pc;
	reg_t		ir_ac;
	unsigned char	ir_vec;	/* vector number of interrupt; only low-order
				   byte is meaningful */
} irec_t;	
typedef void(*isr_t)(irec_t*);

typedef struct {
	reg_t	i_pending_pris;		/* priorities w/ pending intrs */
	reg_t	i_pending_ints[8];	/* pending interrupts */
	isr_t	i_vectors[248];		/* pointers to handlers */
} itab_t;

#endif

#endif
