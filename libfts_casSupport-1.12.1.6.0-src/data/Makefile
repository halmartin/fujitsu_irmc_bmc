#----------------------------------------------------------------------
#                         Makefile to build libfts_casSupport
#                 COPYRIGHT 2011 - 2018 FUJITSU LIMITED
#-----------------------------------------------------------------------

#------- Comment/uncomment the following line to enable/disable debugging------
DEBUG 		= n
#------------------------------------------------------------------------------

#---------------------- Change according to your files ------------------------
LIBRARY_NAME	= libfts_casSupport

SRC += fts_cas.c

CFLAGS += -I.
CFLAGS += -I./include

# Common Includes
CFLAGS  += -I${TARGETDIR}/usr/include
CFLAGS  += -I${TARGETDIR}/usr/include/openssl
CFLAGS  += -I${TARGETDIR}/include

# Project includes
CFLAGS += -I${SPXINC}/global
CFLAGS += -I${SPXINC}/dbgout
CFLAGS += -I${SPXINC}/fts

CFLAGS  += -I${SPXINC}/ipmi
CFLAGS  += -I${SPXINC}/libipmi
CFLAGS  += -I${SPXINC}/libipmiparams
CFLAGS  += -I${SPXINC}/ipmiparams
# Note: needed for AES.h included by libipmi_session.h from June 2012 Release
CFLAGS  += -I${SPXINC}/encryption

# Internationalisation / Localised stuff
CFLAGS  += -I${TARGETDIR}/usr/share/gettext
CFLAGS  += -DENABLE_NLS=1 
CFLAGS  += -DLOCALEDIR=\"/usr/lib/locale\"

include ${TOOLDIR}/rules/Rules.make.libs
#---------------------------------------------------------
