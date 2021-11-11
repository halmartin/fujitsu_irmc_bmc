/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2014 - 2018 FUJITSU LIMITED
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

#ifndef _I2C_PILOT_H_
#define _I2C_PILOT_H_

#ifdef LINUX_KERNEL_BUILD

extern void *pilot_addChip(u8 bus, u8 addr, u8 mask, u8 rstGpio, int type, u8 *chanNumbering, u8 maxChanIndex, u16 *modifiers, u8 numModifiers );
extern void pilot_removeChip(void *pPlatformDevice);
extern void pilot_resetChip(void *pPlatformDevice);
extern u32 pilot_getType(char *name);
extern void pilot_setMaskChip(void *pPlatformDevice, u8 mask);
extern int pilot_getStatus (void *pPlatformDevice, char *buf);
extern void pilot_notifyDeselectChan(void *pPlatformDevice);

#endif

#endif //_I2C_PILOT_H_
