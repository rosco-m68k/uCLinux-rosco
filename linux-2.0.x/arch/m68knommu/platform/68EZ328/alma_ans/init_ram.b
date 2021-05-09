************************************************************
* ALMA_ANS_RAM-INIT.B 
*
* Init registers on ALMA_ANS board to reasonable values
*
* (C) Vladimir Gurevich, 1999
* Date: 9/9/1999
*
***********************************************************

* Initialize ALMA_ANS board *

FFFFF1180130  emucs init
FFFFF0000114  SCR init
FFFFFB0B0100  Disable WD
FFFFF40B01C3  enable RAS[01]/CAS[01]
FFFFF4230107  enable *DWE, RXD, TXD, CTS and RTS
FFFFF42B01F7  enable A20
FFFFFD0D0108  disable hardmap
FFFFFD0E0107  clear level 7 interrupt
FFFFF100028000 CSA0 256M -- 258M FLASH + CSA1 258M -- 260M Ethernet
FFFFF1100201A9
FFFFFC00028F00 DRAM Config
FFFFFC02029667 DRAM Control
FFFFF106020000 CSD init -- RAS0 0-1M RAS1 1M-2M
FFFFF1160202DB enable DRAM cs 
FFFFF3000140   IVR
FFFFF30404007FFFFF IMR
