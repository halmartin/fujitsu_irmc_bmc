DEBUG  := n
TARGET := videocap
OBJS   := mmap.o ioctl.o capmain.o proc.o cap90xx.o dma90xx.o iohndlr.o profile.o capture.o

EXTRA_CFLAGS += -I${SPXINC}/global
EXTRA_CFLAGS += -I${SPXINC}/dbgout
EXTRA_CFLAGS += -I${SPXINC}/helper

-include ${TOOLDIR}/rules/Rules.make.drivers
