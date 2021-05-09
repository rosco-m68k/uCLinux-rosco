************************************************************
* ADS_RAM-INIT.B -- Initialization file for M68EZ328ADS 
*
* The following memory map is assumed:
*	00000000 -- 00400000  DRAM
*       10000000 -- 10200000  FLASH
*
* We do not use SRAM.
*
* (C) Vladimir Gurevich <vgurevic@cisco.com>
*
***********************************************************

* Initialize ADS *

FFFFF1180130  emucs init
FFFFF0000114  SCR init
FFFFFB0B0100  Disable WD
FFFFF40B01C3  enable RAS[01]/CAS[01]
FFFFF42301C7  enable *DWE, RXD, TXD
FFFFF42B017B  enable CSA1 and CLKO
FFFFFD0D0108  disable hardmap  
FFFFFD0E0107  clear level 7 interrupt
FFFFF100028000 CSA0/CSA1 256M -- 258M FLASH
FFFFF1100201A7 enable FLASH
FFFFFC00028F00 DRAM Config
FFFFFC02029667 DRAM Control
FFFFF106020000 CSD init -- RAS0 0-2M RAS1 2M-4M
FFFFF1160202DD enable DRAM cs                                                   FFFFF3000140   IVR
FFFFF30404007FFFFF IMR

*
* This is an optional part that initializes the on-board 68681
* and sends out 'Ready'
*
FFFD00050110	Reset pointer to MR1A
FFFD00010113	Initialize 68681 Channel A (MR1A) cs8 -parenb
FFFD00010100    Initialize 68681 Channel A (MR2A) -cstopb -echo -crtscts
FFFD000301BB	Speed 9600/9600
FFFD00050105	Enable both Tx & Rx
FFFD0007010D	\r
FFFD0007010A	\n
FFFD0007010A	\n
FFFD0007010A	\n
FFFD0007010A	\n
FFFD0007010A	\n
FFFD0007010A	\n
FFFD0007010A	\n
FFFD00070152	R
FFFD00070165	e
FFFD00070161	a
FFFD00070164    d
FFFD00070179	y
FFFD0007010D	\r
FFFD0007010A	\n
