# $Id: Makefile,v 1.30 1996/04/22 10:37:53 davem Exp $
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

.S.s:
	$(CPP) -D__ASSEMBLY__ $(AFLAGS) -ansi $< -o $*.s

.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -ansi -c $< -o $*.o


all: kernel.o

O_TARGET := kernel.o
IRQ_OBJS := irq.o
O_OBJS   := wof.o wuf.o etrap.o rtrap.o switch.o traps.o ${IRQ_OBJS} \
	    process.o signal.o setup.o \
	    sys_sparc.o \
	    time.o windows.o cpu.o \
	    sclow.o ptrace.o


include $(TOPDIR)/Rules.make
