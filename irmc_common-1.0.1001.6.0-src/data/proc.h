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
 @file proc.h
 @brief /proc/irmc/console-log operations 
 @date 2010-01-20
 @author Vlad Shakhov

API for /proc filesystem interface  
*/

#ifndef _CL_PROC_H_
#define _CL_PROC_H_

int consolelog_proc_init(void);
void consolelog_proc_shutdown(void);

#endif // _CL_PROC_H_
