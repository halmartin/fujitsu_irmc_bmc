/*****************************************************************
 **                                                             **
 **     (C) Copyright 2009-2015, American Megatrends Inc.       **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                             **
 **         Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                             **
 ****************************************************************/

#ifndef __KCS_HW_H__
#define __KCS_HW_H__
#include "icc_what.h"

#define LPC_IRQ_ADDR                       IRQ_KCS_BT_SMIC_MAIL

#define MAX_KCS_CHANNELS                3
#define ERROR_STATE						0xC0

#ifdef ICC_PLATFORM_LITTLE_ENDIAN
u8	STATUS[MAX_KCS_CHANNELS]   = { 0x03, 0x03, 0x03 };
u8	DATA_IN[MAX_KCS_CHANNELS]  = { 0x00, 0x00, 0x00 };
u8	CMD_IN[MAX_KCS_CHANNELS]   = { 0x02, 0x02, 0x02 };
u8 	DATA_OUT[MAX_KCS_CHANNELS] = { 0x00, 0x00, 0x00 };
#else
u8	STATUS[MAX_KCS_CHANNELS]   = 	{ (0x0C + 3), (0x14 + 3), (0x1C + 3) };
u8	DATA_IN[MAX_KCS_CHANNELS]  = 	{ (0x08 + 3), (0x10 + 3), (0x18 + 3) };
u8	CMD_IN[MAX_KCS_CHANNELS]   = 	{ (0x20 + 3), (0x24 + 3), (0x28 + 3) };
u8 	DATA_OUT[MAX_KCS_CHANNELS] = 	{ (0x08 + 3), (0x10 + 3), (0x18 + 3) };
#endif

volatile u8 * kcs_vadd[MAX_KCS_CHANNELS] = 
	{ (u8*) SE_KCS3_VA_BASE, (u8*) SE_KCS4_VA_BASE, (u8*) SE_KCS5_BT_SMIC_MAIL_VA_BASE } ;

#endif /* __KCS_HW_H__ */

