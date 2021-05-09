# Put definitions for platforms and boards in here.

ifdef CONFIG_M68000
#2002-05-14 gc: 
PLATFORM := 68000
endif

ifdef CONFIG_SM2010
BOARD := SM2010
endif

ifdef CONFIG_M68328
PLATFORM := 68328
ifdef CONFIG_PILOT
BOARD := pilot
endif
endif

ifdef CONFIG_M68EZ328
PLATFORM := 68EZ328
ifdef CONFIG_PILOT
BOARD := PalmV
endif
ifdef CONFIG_UCSIMM
BOARD := ucsimm
endif
ifdef CONFIG_CWEZ328
BOARD := ucsimm
endif
ifdef CONFIG_ALMA_ANS
BOARD := alma_ans
endif
ifdef CONFIG_ADS
BOARD := alma_ads
endif
endif

ifdef CONFIG_M68332
PLATFORM := 68332
ifdef CONFIG_MWI
BOARD := mwi
endif
endif

ifdef CONFIG_M68EN302
PLATFORM := 68EN302
BOARD := generic
endif

ifdef CONFIG_M68360
PLATFORM := 68360
ifdef CONFIG_UCQUICC
BOARD := uCquicc
endif
ifdef CONFIG_SED_SIOS_MASTER
BOARD := sed_sios_master
endif
ifdef CONFIG_SED_SIOS_REMOTE
BOARD := sed_sios_remote
endif
endif

ifdef CONFIG_M68376
PLATFORM := 68376
ifdef CONFIG_FR1000
BOARD := FR1000
endif
endif

ifdef CONFIG_M5204
PLATFORM := 5204
BOARD := SBC5204
endif

ifdef CONFIG_M5206
PLATFORM := 5206
BOARD := ARNEWSH
endif

ifdef CONFIG_M5206e
PLATFORM := 5206e
ifdef CONFIG_MOTOROLA
BOARD := MOTOROLA
endif
ifdef CONFIG_ELITE
BOARD := eLITE
endif
ifdef CONFIG_NETtel
BOARD := NETtel
endif
ifdef CONFIG_TELOS
BOARD := toolvox
endif
ifdef CONFIG_CFV240
BOARD := CFV240
endif
endif

ifdef CONFIG_M5249
PLATFORM := 5249
ifdef CONFIG_MOTOROLA
BOARD := MOTOROLA
endif
endif

ifdef CONFIG_M5272
PLATFORM := 5272
ifdef CONFIG_MOTOROLA
BOARD := MOTOROLA
endif
ifdef CONFIG_NETtel
BOARD := NETtel
endif
ifdef CONFIG_GILBARCONAP
BOARD := NAP
endif
ifdef CONFIG_COBRA5272
BOARD := senTec
endif
ifdef CONFIG_CANCam
BOARD := CANCam
endif
ifdef CONFIG_SCALES
BOARD := SCALES
endif
endif

ifdef CONFIG_M5307
PLATFORM := 5307
ifdef CONFIG_ARNEWSH
BOARD := ARNEWSH
endif
ifdef CONFIG_NETtel
BOARD := NETtel
endif
ifdef CONFIG_eLIA
BOARD := eLIA
endif
ifdef CONFIG_DISKtel
BOARD := DISKtel
endif
ifdef CONFIG_SECUREEDGEMP3
BOARD := MP3
endif
ifdef CONFIG_MOTOROLA
BOARD := MOTOROLA
endif
ifdef CONFIG_CLEOPATRA
BOARD := CLEOPATRA
endif
endif

ifdef CONFIG_M5407
PLATFORM := 5407
ifdef CONFIG_MOTOROLA
BOARD := MOTOROLA
endif
ifdef CONFIG_CLEOPATRA
BOARD := CLEOPATRA
endif
endif
