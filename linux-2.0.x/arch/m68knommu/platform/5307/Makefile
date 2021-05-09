#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

ifdef CONFIG_FULLDEBUG
	AFLAGS += -DDEBUGGER_COMPATIBLE_CACHE=1
endif

#
#
# If you want to play with the HW breakpoints then you will
# need to add define this,  which will give you a stack backtrace
# on the console port whenever a DBG interrupt occurs.  You have to
# set up you HW breakpoints to trigger a DBG interrupt:
#
# AFLAGS += -DTRAP_DBG_INTERRUPT
# CFLAGS += -DTRAP_DBG_INTERRUPT
#

.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -c $< -o $*.o

all: $(BOARD)/crt0_$(MODEL).o entry.o platform.o
O_TARGET := platform.o
O_OBJS := entry.o config.o signal.o ints.o bios32.o

$(BOARD)/crt0_$(MODEL).o: $(BOARD)/crt0_$(MODEL).S

entry.o: entry.S

include $(TOPDIR)/Rules.make
