/****************************************************************
 **                                                            **
 **    (C)Copyright 2011-2012, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
*****************************************************************/
/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2012 - 2018 FUJITSU LIMITED
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

#ifndef __LOGBUFFER_H__
#define __LOGBUFFER_H__

#define LOGBUFFER_CTL_FILE "/dev/logbuffer"
#define MAX_LOGBUF_READ_SIZE		254	/* Max entry length including size byte is 255 therefore max data is 254 */

/* IOCTL commands and structure: */
typedef enum {
	LOGBUFFER_IOC_CLEAR = 0x100,		/* IOCTL cmd to clear the log buffer */
	LOGBUFFER_IOC_ENABLED_LOGGING,		/* IOCTL cmd to enable writing to log buffer */
	LOGBUFFER_IOC_DISABLED_LOGGING,		/* IOCTL cmd to disable writing to log buffer */
	LOGBUFFER_IOC_GET_STATUS,			/* IOCTL cmd to get enable/disable status */
	LOGBUFFER_IOC_GET_RWP,				/* IOCTL cmd to get current read/write offset */
	LOGBUFFER_IOC_READ_OFFSET,			/* IOCTL cmd to read with offset */
	LOGBUFFER_IOC_GET_ERROR             /* IOCTL cmd to get error code */
}eLogbufferFunc;

typedef struct {
	unsigned int read_p;
	unsigned int write_p;
}__attribute__((packed)) logbuffer_ioc_cmd_t;

typedef struct {
	unsigned int offset;
	unsigned int size;
	char buf[MAX_LOGBUF_READ_SIZE];
}__attribute__((packed)) logbuffer_ioc_read_t;

#endif

