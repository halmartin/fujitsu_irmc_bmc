DEBUG     := n
TARGET    := logbuffer
OBJS      := logbuffer_mod.o


# Kernel build switch
EXTRA_CFLAGS += -DLINUX_KERNEL_BUILD

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/dbgout
EXTRA_CFLAGS += -I${SPXINC}/helper
EXTRA_CFLAGS += -I${SPXINC}/fts

#---------------- Do not modify any thing below this line ------------------------
-include ${TOOLDIR}/rules/Rules.make.drivers

