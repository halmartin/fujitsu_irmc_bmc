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

/*
 * PwmTach hardware Driver Header File
 *
 * Copyright (C) 2009-2015 American Megatrends Inc.
 *
 * This driver provides hardware layer header file for PWMTACH driver 
 * properties to set and get.
 */


#ifndef __PWMTACH_HW_PROP_H__
#define __PWMTACH_HW_PROP_H__



/* PWM TACH PROPERTY */
#define TACH_FILTERSELECT            0x01
#define TACH_CLOCKSELECT             0x02
#define TACH_ERROR                   0x03
#define TACH_OVERFLOW                0x04
#define TACH_SPEEDREADY              0x05

#define PWM_DIVISION_128_64          0x06
#define PWM_CLOCKSELECT              0x07
#define PWM_PRESCALEVALUE            0x08
#define PWM_COUNTERRESOLUTIONVALUE   0x09

/* ******************** */

#endif

