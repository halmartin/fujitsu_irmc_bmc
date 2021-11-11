/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2014 Fujitsu Technology Solutions
*                         ALL RIGHTS RESERVED
*
* This computer program is CONFIDENTIAL and contains TRADE SECRETS of
* Fujitsu Technology Solutions. The receipt or possession of this program does
* not convey any rights to reproduce or disclose its contents, or to
* manufacture, use, or sell anything that it may describe, in whole or
* in part, without the specific written consent of Fujitsu Technology Solutions.
* Any reproduction of this program without the express written consent
* of Fujitsu Technology Solutions is a violation of the copyright laws and may
* subject you to civil liability and criminal prosecution.
*
*****************************************************************************/

#ifndef _LINUX_PROC_I2C_PCA954X_H
#define _LINUX_PROC_I2C_PCA954X_H

#include "pca954x.h"

tPca954xChip *getPcaData(unsigned char id);
int pca954x_createProc(void);
int pca954x_removeProc(void);

#endif //_LINUX_PROC_I2C_PCA954X_H
