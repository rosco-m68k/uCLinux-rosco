#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -traditional -c $< -o $*.o

all: kernel.o
O_TARGET := kernel.o
O_OBJS := setup.o process.o traps.o ptrace.o sys_m68k.o time.o ksyms.o
ifdef CONFIG_CONSOLE
O_OBJS += console.o
endif

include $(TOPDIR)/Rules.make
