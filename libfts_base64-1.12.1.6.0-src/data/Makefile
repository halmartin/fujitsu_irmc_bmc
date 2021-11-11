#----------------------------------------------------------------------
#                         Makefile to build libfts_base64
#                 COPYRIGHT 2011 - 2018 FUJITSU LIMITED
#-----------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging------
DEBUG 		= n
#------------------------------------------------------------------------------

#---------------------- Change according to your files ------------------------
LIBRARY_NAME	= libfts_base64

SRC += fts_base64.c

CFLAGS += -D_FTS_LINUX_MIGRATION_
#CFLAGS += -D_GNU_SOURCE
CFLAGS += -I${SPXINC}/global
CFLAGS += -I.
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/fts
CFLAGS += -I${SPXINC}/encryption
#CFLAGS += -DTOOL_CHAIN_TYPE_AMI


include ${TOOLDIR}/rules/Rules.make.libs
#---------------------------------------------------------
