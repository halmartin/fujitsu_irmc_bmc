/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */
#ifndef __PS2_H__
#define __PS2_H__
#include "icc_what.h"



/* IOCTL Commands and structure */
typedef enum {
	ENABLE_KEYBOARD,
	DISABLE_KEYBOARD,
	ENABLE_MOUSE,
	DISABLE_MOUSE,

	RESET_PS2,
	READ_KB_STATUS,
	READ_MS_STATUS,
	INFORM_LPC_RESET
}ePS2Func;

#endif
