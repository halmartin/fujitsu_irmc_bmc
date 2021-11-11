/***************************************************************
****************************************************************
**                                                            **
**    (C)Copyright 2009-2015, American Megatrends Inc.        **
**                                                            **
**            All Rights Reserved.                            **
**                                                            **
**        6145-F, Northbelt Parkway, Norcross,                **
**                                                            **
**        Georgia - 30071, USA. Phone-(770)-246-8600.         **
**                                                            **
****************************************************************/

#ifndef AMI_IOCTL_H
#define AMI_IOCTL_H

#include<linux/version.h>
#include<linux/fs.h>

#ifdef HAVE_UNLOCKED_IOCTL  
  #if HAVE_UNLOCKED_IOCTL  
  #define USE_UNLOCKED_IOCTL  
  #endif
#endif

#ifdef USE_UNLOCKED_IOCTL
/* Defined in ioctl.c, used in module.c */
long videocap_ioctl(struct file *filep,unsigned int cmd,unsigned long arg);
#else
int videocap_ioctl(struct inode *inode, struct file *filep,unsigned int cmd,unsigned long arg);
#endif

#endif // AMI_IOCTL_H
