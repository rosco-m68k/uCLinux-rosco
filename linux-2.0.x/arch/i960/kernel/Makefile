#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

.S.o:
	$(CC) -D__ASSEMBLY__ -traditional $(EXTRA_CFLAGS) -c $< -o $*.o

all: head.o entry.o kernel.o
O_TARGET := kernel.o
O_OBJS := entry.o process.o traps.o ints.o signal.o ptrace.o \
	  setup.o bios32.o time.o ksyms.o semaphore.o syscall.o

ifdef CONFIG_MON960
O_OBJS += mon960_calls.o
ifdef CONFIG_MON960_CONSOLE
O_OBJS += mon960_console.o
endif
endif

include $(TOPDIR)/Rules.make
