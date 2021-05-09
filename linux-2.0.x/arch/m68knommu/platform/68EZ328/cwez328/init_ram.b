*********************************************
* CWEZ328 68EZ328 DRAM INIT, testing file *
* Jan 10, 2000                              *
*********************************************

FFFFFB0B0100       watchdog off.
FFFFF0000118       SCR reg, Buss timeout, Supervisor, Single Map.
FFFFF200022400     PLLCR.
FFFFF202020123     PLLFSR.
FFFFF40B0102       Enable RAS + CAS lines, CSB0 enabled, CSB1 disabled.
FFFFF4230100       Enable *DWE.
FFFFFD0D0108       Disable Hardware Compare Map.
FFFFFD0E0107       Level 7 interrupt clear.
FFFFF100028000     Flash at 10000000-107FFFFF.
FFFFF11002018B     CSA0: 4Mx16, 0 Wait, enabled.
FFFFF102020800     External at 1000000-101FFFF.
FFFFF42B0187       PF3..PF6 set as A20..A23.
FFFFF112020081     CSBx: 128Kx16, 5 Wait, enabled.
FFFFFC00028F00     DRAM Config.
FFFFFC02029667     DRAM Control. was 9667.
FFFFF106020800     DRAM at 1000000-17FFFFF.
FFFFF11602068F     4Mx16, 0 Wait, enable.
FFFFF3000140       IVR.
FFFFF30404007FFFFF IMR.

