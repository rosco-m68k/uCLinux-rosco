# Put definitions for platforms and boards in here.

ifdef CONFIG_LEON_2
PLATFORM := LEON-2.x
ifdef CONFIG_LEON4TSIM
BOARD := TSIM
endif
endif
