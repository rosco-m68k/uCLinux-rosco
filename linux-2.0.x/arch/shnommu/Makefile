#
# shnommu/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1998,1999  D. Jeff Dionne <jeff@ryeham.ee.ryerson.ca>,
#                          Kenneth Albanowski <kjahds@kjahds.com>,
#                          The Silver Hammer Group, Ltd.
#
# Based on arch/m68k/Makefile:
# Copyright (C) 1994 by Hamish Macdonald
#

# test for cross compiling
COMPILE_ARCH = $(shell uname -m)

# Find out which board we are compiling for
include arch/$(ARCH)/Boards.mk


# Set up the memory model.  RAM or ROM.
ifdef CONFIG_RAMKERNEL
MODEL = ram
else
MODEL = rom
endif

# get the compiler, flags and targets from the platform
include arch/$(ARCH)/platform/$(PLATFORM)/Rules.make

arch/$(ARCH)/empty.o:
	$(CROSS_COMPILE)gcc -o arch/$(ARCH)/empty.o -c arch/$(ARCH)/empty.c

bootstrap:
	@$(MAKEBOOT) bootstrap

archdep:

