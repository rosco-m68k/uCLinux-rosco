# Put definitions for platforms and boards in here for i960 platforms.

ifdef CONFIG_SX6
PLATFORM := SX6
BOARD := SuperTrak100
endif

ifdef CONFIG_IQ80303
PLATFORM := 80303
BOARD := IQ80303
endif

ifdef CONFIG_CYVH
PLATFORM := I960VH
BOARD := CycloneVH
endif