DEBUG     		:= n
TARGET	 	  	:= usb1_hw
OBJS			:= usb_hw1.o

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/helper
EXTRA_CFLAGS += -I${SPXINC}/dbgout
EXTRA_CFLAGS += -I${SPXINC}/usbe
EXTRA_CFLAGS += -I${SPXINC}/reset

KBUILD_EXTRA_SYMBOLS += ${SPXINC}/reset/Module.symvers
KBUILD_EXTRA_SYMBOLS += ${SPXINC}/usbe/Module.symvers


-include ${TOOLDIR}/rules/Rules.make.drivers



