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

#ifndef AMI_CAP90XX_H
#define AMI_CAP90XX_H

#include <linux/mm.h>
#include "fge.h"
#include "videoctl.h"
#include "capture.h"

extern volatile unsigned int g_tse_sts;				/* tile snoop status */
extern unsigned int g_bpp15;
extern unsigned char tse_rr_base_copy[512];
extern unsigned long rowsts[2], colsts[2];
extern unsigned int AllRows;
extern unsigned int AllCols;

int capture_screen(unsigned int cap_flags, capture_param_t *cap_param);
//int capture_data(unsigned int cap_flags, capture_param_t *cap_param);      
int capture_palt (unsigned int cap_flags, u8 *palt_address);
int capture_curpos (void);
int capture_xcursor(XCURSOR_INFO *xcursor);
void get_video_mode_params(VIDEO_MODE_PARAMS *vm);
unsigned char get_bpp(unsigned char XMulCtrl);
void get_current_palette(u8 *pal_address);
void resume_video(void);
int get_xcursor_info(XCURSOR_INFO *xcursor, unsigned int lcl_status);
void Convert2Bppto3Bpp(unsigned long *BaseSnoopMap, unsigned long Stride, unsigned long Height);
void WaitForTFE(void);
void WaitForBSE(void);
void control_display(unsigned char *lockstatus);
void control_card_presence(unsigned char CardPresence);
void set_bandwidth_mode(unsigned char bandwidth_mode);
int get_bandwidth_mode(void);
int get_card_presence(void);
void set_pci_subsystem_Id(PCI_DEVID PCIsubsysID);
unsigned long get_pci_subsystem_Id(void);
#endif // AMI_CAP90XX_H
