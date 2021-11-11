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

#ifndef AMI_IOHNDLR_H
#define AMI_IOHNDLR_H

#include "videoctl.h"

void ioctl_init_fpga (VIDEO_Ioctl *ioc);
void ioctl_reset_capture_engine (VIDEO_Ioctl *ioc);
void ioctl_init_video_buffer (VIDEO_Ioctl *ioc);
void ioctl_get_fpga_params (VIDEO_Ioctl *ioc);
void ioctl_set_fpga_params (VIDEO_Ioctl *ioc);
void ioctl_get_video_resolution (VIDEO_Ioctl *ioc);
void ioctl_capture_video (VIDEO_Ioctl *ioc);
void ioctl_get_palette( VIDEO_Ioctl *ioc);
void ioctl_get_palette_wo_wait( VIDEO_Ioctl *ioc);
void ioctl_get_curpos( VIDEO_Ioctl *ioc);
void ioctl_capture_xcursor( VIDEO_Ioctl *ioc);
void ioctl_resume_video( VIDEO_Ioctl *ioc);
void ioctl_control_display( VIDEO_Ioctl *ioc);
void ioctl_ctrl_card_presence( VIDEO_Ioctl *ioc);
void ioctl_set_bandwidth_mode( VIDEO_Ioctl *ioc);
void ioctl_get_bandwidth_mode( VIDEO_Ioctl *ioc);
void ioctl_get_card_presence( VIDEO_Ioctl *ioc);
void ioctl_set_pci_subsystemID( VIDEO_Ioctl *ioc);
void ioctl_get_pci_subsystemID( VIDEO_Ioctl *ioc);

#endif // AMI_IOHNDLR_H
