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

#ifndef __PECI_HW_H__
#define __PECI_HW_H__


#include "peci.h"

/* Pilot-ii PECI Hardware Instance No.*/
#define PECI_HW_MAX_INST	1

/* Pilot-ii PECI Registers */
#define PECICTL_REG		0x00
#define PECISTS_REG		0x04
#define PECIOPTRATE_REG		0x08
#define PECIPTRS_REG		0x0C
#define PECIHWFCS_REG		0x10
#define PECIRXOFF_REG		0x14
#define PECIBITBANG_REG		0x18
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
#define PECITXQ_REG(x)		(0x20 + (4 * (x)))
#define PECIRXQ_REG(x)		(0x20 + (4 * (x)))
#else
#define PECITXQ_REG(x)		(0x1C + (4 * (x)))
#define PECIRXQ_REG(x)		(0x30 + (4 * (x)))
#endif

/* bits of PECI control register */
#define PECI_ENABLE		0x01
#define PECI_TRIGGER		0x02
#define PECI_INTR_ENABLE	0x04
#define PECI_AWFCS_EN		0x1000

/* bits of PECI status register */
#define PECI_BUSY		0x01
#define PECI_INTR_STATUS	0x02

#define ReadReg8(RegOffset)	((unsigned char*)(SE_PECI_VA_BASE + RegOffset))
#define ReadReg16(RegOffset)	((unsigend short*)(SE_PECI_VA_BASE + RegOffset))
#define ReadReg32(RegOffset)	(unsigned int)((unsigned int*)(SE_PECI_VA_BASE + RegOffset))

#define WriteReg8(RegOffset,Value)	*((volatile unsigned char*)(SE_PECI_VA_BASE) + RegOffset) = Value
#define WriteReg16(RegOffset,Value)	*((volatile unsigned short*)(SE_PECI_VA_BASE) + RegOffset) = Value
#define WriteReg32(RegOffset,Value)	*((volatile unsigned long*)(SE_PECI_VA_BASE) + RegOffset) =Value
	
/* This is to Enable and Disable the debug statement statically  */
#if defined (DEBUG)
#define dbgprint	printk
#else
#define dbgprint(str,...)
#endif

#endif
