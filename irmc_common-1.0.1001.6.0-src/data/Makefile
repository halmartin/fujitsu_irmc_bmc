DEBUG := n

# Kernel build switch
EXTRA_CFLAGS += -DLINUX_KERNEL_BUILD
# enable/disable for PRAM file support
#EXTRA_CFLAGS += -DPRAM_PROC_FILE
# enable/disable for jiffies file support
EXTRA_CFLAGS += -DJIFFIES_PROC_FILE
# enable/disable for power button press support during boot phase
EXTRA_CFLAGS += -DPWR_PRESS_DURING_BOOT_PROC_FILE

TARGET := irmc_common
OBJS := irmc_common_main.o device.o filter.o proc.o ring_buffer.o

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/ftsdrv
EXTRA_CFLAGS += -I${SPXINC}/fts
EXTRA_CFLAGS += -I${SPXINC}/helper

-include ${TOOLDIR}/rules/Rules.make.drivers
