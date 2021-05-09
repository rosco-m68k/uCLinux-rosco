#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -c $< -o $*.o

all: $(BOARD)/crt0_$(MODEL).o entry.o platform.o quicc

#ifeq ($(CONFIG_M68EN360_ETHERNET),y)
#QUICC_OBJS += quicc/enet.o
#endif

QUICC_OBJS	= quicc/quicc_cpm.o 

ifeq ($(CONFIG_M68360_SMC_UART),y)
	QUICC_OBJS += quicc/quicc_smc.o
endif
ifeq ($(CONFIG_M68360_SCC_UART),y)
	QUICC_OBJS += quicc/quicc_scc.o
endif
ifeq ($(CONFIG_M68360_SIM_WDT),y)
	QUICC_OBJS += quicc/quicc_wdt.o
endif

quicc:		$(QUICC_OBJS)



O_TARGET := platform.o
O_OBJS := entry.o config.o signal.o quicc_traps.o quicc_ints.o $(QUICC_OBJS)


$(BOARD)/crt0_$(MODEL).o: $(BOARD)/crt0_$(MODEL).S

entry.o: entry.S

include $(TOPDIR)/Rules.make
