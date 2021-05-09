#
# or32/Makefile
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
# Based on arch/m68k/Makefile:
#

# test for cross compiling
COMPILE_ARCH = $(shell uname -m)

# Find out which board we are compiling for
include arch/$(ARCH)/Boards.mk

HEAD := arch/or32/kernel/head.o

# Set up the memory model.  RAM or ROM.
ifdef CONFIG_RAMKERNEL
MODEL = ram
else
MODEL = rom
endif

# get the compiler, flags and targets from the platform
# include arch/$(ARCH)/platform/$(PLATFORM)/Rules.make
include arch/$(ARCH)/Rules.make

archdep:

