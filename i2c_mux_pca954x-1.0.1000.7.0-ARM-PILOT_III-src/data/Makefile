DEBUG := n
TARGET := i2c_mux_pca954x
OBJS := pca954x.o pca954x_proc.o

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/i2c_mux
EXTRA_CFLAGS += -I${SPXINC}/helper

KBUILD_EXTRA_SYMBOLS += ${SPXINC}/i2c_mux/Module.symvers
KBUILD_EXTRA_SYMBOLS += ${SPXINC}/helper/Module.symvers

-include ${TOOLDIR}/rules/Rules.make.drivers