DEBUG := n
TARGET := i2c_mux_PILOT
OBJS := PILOT.o 

# Kernel build switch
EXTRA_CFLAGS += -DLINUX_KERNEL_BUILD

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/i2c_mux
EXTRA_CFLAGS += -I${SPXINC}/helper

KBUILD_EXTRA_SYMBOLS += ${SPXINC}/i2c_mux/Module.symvers
KBUILD_EXTRA_SYMBOLS += ${SPXINC}/helper/Module.symvers

-include ${TOOLDIR}/rules/Rules.make.drivers