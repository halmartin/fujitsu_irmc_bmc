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
 @file irmc_common.c
 @brief iRMC common infrastructure (/proc/irmc/ entry)
*/

#ifndef _FTS_IRMC_COMMON_H_
#define _FTS_IRMC_COMMON_H_

#ifdef LINUX_KERNEL_BUILD

/// used for logging purpose
#define CONSOLELOG_LOG_NAME 	"iRMC Console Log device"

//the name is exported only for driver developing
#include <linux/proc_fs.h>
extern struct proc_dir_entry *irmc_proc_basedir;

int consolelog_started(void);
void consolelog_start(unsigned int tty_line);
void consolelog_stop(void);
void consolelog_lock(void);
void consolelog_unlock(void);
void irmc_debug_uart_consolelog_start(void);
void irmc_debug_uart_consolelog_stop(void);
#else // LINUX_KERNEL_BUILD

#include <time.h>

#endif // LINUX_KERNEL_BUILD

#define IRMC_PROC_BASEDIR	"irmc"

#ifdef PRAM_PROC_FILE
#define PRAM_PROCFS_NAME	"pram"
#define PROC_PRAM		"/proc/irmc/pram"
#endif

#ifdef JIFFIES_PROC_FILE
#define JIFFIES_PROCFS_NAME	"SystemTicks"
#define PROC_SYSTEM_TICKS	"/proc/irmc/SystemTicks"
#endif


#define PWR_PRESS_DURING_BOOT_PROCFS_NAME	"pwb"
#define PROC_SYSTEM_PWR_PRESS_DURING_BOOT	"/proc/irmc/pwb"


#define CONSOLELOG_CHRDEV_NAME	"irmc-cl"
#define CONSOLE_LOG_CHARDEV "/dev/irmc-cl"
#define CONSOLE_LOG_DEBUG_CHARDEV	"/dev/irmc-debug-uart-cl"

//@{
/// @name IOCTL's list to /dev/irmc-cl

/// (unsigned int *) argument, tty line number
#define CONSOLELOG_IOC_GET_START	3
/// no arguments
#define CONSOLELOG_IOC_GET_STOP		4
/// return 0 or 1 depend of logging status
#define CONSOLELOG_IOC_GET_STATUS	5
/// (unsigned int *) argument. send 1 to filter all sequences, 0 to filter some
#define CONSOLELOG_IOC_SET_FILTER	6
/// (unsigned int *) argument. return 1 to filter all sequences, 0 to filter some
#define CONSOLELOG_IOC_GET_FILTER	7
/// no arguments
#define CONSOLELOG_IOC_CLEAR_LOG	8
//  (char *) argument, buffer. return bytes read
#define CONSOLELOG_IOC_DEBUG_UART_READ	9
//@}

#endif // _FTS_IRMC_COMMON_H_

