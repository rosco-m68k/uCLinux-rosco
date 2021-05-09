#
# linux/arch/arm/lib/Makefile
#
# Copyright (C) 1995, 1996 Russell King
#

L_TARGET := lib.a
L_OBJS   := backtrace.o bitops.o checksum.o delay.o fp_support.o \
	    loaders.o memfastset.o segment.o system.o string.o \
            longjmp.o

ifdef CONFIG_ARCH_ACORN
  L_OBJS += ll_char_wr.o io-acorn.o irqs-acorn.o
  ifdef CONFIG_ARCH_A5K
    L_OBJS += floppydma.o
  endif
  ifdef CONFIG_ARCH_RPC
    L_OBJS += floppydma.o
  endif
endif

ifeq ($(MACHINE),ebsa110)
L_OBJS	 += io-ebsa.o irqs-ebsa.o
endif

ifeq ($(MACHINE),trio)
L_OBJS	 += irqs-trio.o io-trio.o
endif

ifeq ($(MACHINE),atmel)
L_OBJS	 += irqs-atmel.o io-atmel.o
endif

ifeq ($(MACHINE),netarm)
L_OBJS	 += irqs-netarm.o io-netarm.o
endif

ifeq ($(MACHINE),gba)
L_OBJS	 += irqs-gba.o io-gba.o
endif

include $(TOPDIR)/Rules.make

constants.h: getconstants
	./getconstants > constants.h

getconstants: getconstants.c getconstants.h
	$(HOSTCC) -D__KERNEL__ -o getconstants getconstants.c

getconstants.h: getconsdata.c
	$(CC) $(CFLAGS) -c getconsdata.c
	$(PERL) extractinfo.perl $(OBJDUMP) > $@

ifneq ($(CONFIG_BINUTILS_NEW),y)
%.o: %.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -E $< | tr ';$$' '\n#' > ..tmp.$<.s
	$(CC) $(CFLAGS:-pipe=) -c -o $@ ..tmp.$<.s
	$(RM) ..tmp.$<.s
else
%.o: %.S
	$(CC) $(CFLAGS) -D__ASSEMBLY__ -c -o $@ $<
endif

clean:
	$(RM) getconstants constants.h getconstants.h
