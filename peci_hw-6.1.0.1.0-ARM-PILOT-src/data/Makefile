#----------------------------------------------------------------------
#                         Makefile to build peci
#                 Copyright (c) 2008 American Megatrends Inc 
#-----------------------------------------------------------------------

DEBUG     := n
TARGET    := peci_hw
OBJS      := pecimain_hw.o 

EXTRA_CFLAGS += -I${SPXINC}/linux/linux

EXTRA_CFLAGS += -I${SPXINC}/global 
EXTRA_CFLAGS += -I${SPXINC}/peci
EXTRA_CFLAGS += -I${SPXINC}/reset
EXTRA_CFLAGS += -I${SPXINC}/helper

KBUILD_EXTRA_SYMBOLS +=${SPXINC}/reset/Module.symvers

-include ${TOOLDIR}/rules/Rules.make.drivers

