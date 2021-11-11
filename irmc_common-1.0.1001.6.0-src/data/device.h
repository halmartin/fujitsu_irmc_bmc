/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2010 - 2018 FUJITSU LIMITED
*                         ALL RIGHTS RESERVED
*
* Any reproduction of this program without the express written consent
* of Fujitsu Limited is a violation of the copyright laws and may
* subject you to civil liability and criminal prosecution.
*
* This computer program is CONFIDENTIAL and contains TRADE SECRETS of
* Fujitsu Technology Solutions and/or Fujitsu Limited which must not be disclosed.
* The receipt or possession of this program does not convey any rights to reproduce
* or disclose its contents, or to manufacture, use, or sell anything that it may
* describe, in whole or in part.
*
*****************************************************************************/
/**
 @file device.h
 @brief /dev/irmc-cl Character device operations
 @date 2010-01-20
 @author Vlad Shakhov
 */

#ifndef _CL_DEVICE_H_
#define _CL_DEVICE_H_

/// kernel part of interface
#ifdef __KERNEL__

#include <linux/types.h>

/// @{
/// @name Allowed ttyS* devices to logging
#define CL_VIRTUAL_UART0 3
#define CL_VIRTUAL_UART1 4

// ttyS* device for UART 2
#define CL_BMC_UART2 0

#define CL_DEFAULT_UART CL_VIRTUAL_UART1
#define CL_IRMC_DEBUG_UART CL_BMC_UART2

/// @}

/// Store the kernel module parameter tty_line
extern uint consolelog_tty_line;

int consolelog_device_init(void);
void consolelog_device_shutdown(void);
#endif //__KERNEL__

#endif // _CL_DEVICE_H_
