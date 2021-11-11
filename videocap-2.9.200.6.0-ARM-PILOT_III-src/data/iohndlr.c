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

/****************************************************************
* @file   	ioctl.c
* @author 	Varadachari Sudan Ayanam <varadacharia@ami.com>
* @brief  	Ioctl calls that are exposed to application are
*			define here
****************************************************************/

#ifndef MODULE			
#define MODULE
#endif

#ifndef __KERNEL__		
#define __KERNEL__
#endif

#ifndef EXPORT_SYMTAB		
#define EXPORT_SYMTAB
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

#include "dbgout.h"			
#include "videocap.h"
#include "cap90xx.h"
#include "videodata.h"		
#include "videoctl.h"
#include "fge.h"
#include "profile.h"

// local functions
static void get_screen(VIDEO_Ioctl *ioc);
static void get_attribute(VIDEO_Ioctl *ioc);

// <<TBD>> not doing anything
void ioctl_init_fpga (VIDEO_Ioctl *ioc)
{
	// Do init fpga stuff here
	TDBG ("Init Fpga IOCTL\n");

	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

// <<TBD>> check the significance
void ioctl_reset_capture_engine (VIDEO_Ioctl *ioc)
{
	// Do reset fpga stuff here
	TDBG ("Reset capture engine IOCTL\n");

	ReInitializeDriver();

	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

// <<TBD>> not doing anything
void ioctl_init_video_buffer (VIDEO_Ioctl *ioc)
{
	// Do init video buffer stuff here
	TDBG ("\nInit video buffer IOCTL\n");

	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

// <<TBD>> check the significance
void ioctl_get_fpga_params (VIDEO_Ioctl *ioc)
{
	TDBG ("Get fpga params IOCTL\n");

	// Get params for the most recently captured frame directly
	// from the Tile Snoop registers
	ioc->uni.Get_Screen.TileColumnSize = (fgb_tse_regs->TSCMD >> 6) & 0x3  ;
	ioc->uni.Get_Screen.TileRowSize = (fgb_tse_regs->TSCMD >> 4) & 0x3  ;

	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

// <<TBD>> not doing anything
void ioctl_set_fpga_params (VIDEO_Ioctl *ioc)
{
	TDBG ("Set fpga params IOCTL\n");

	if(fgb_tse_regs->TSCMD & TILE_SNOOP_ENABLE) { // can't set parameters now
		ioc->ErrCode = VIDEO_IOCTL_CAPTURE_ALREADY_ON ;
		return ;
	}	
	
	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

// <<TBD>> video_mode_flags has video mode.. change name
void ioctl_get_video_resolution (VIDEO_Ioctl *ioc)
{

	get_video_mode_params(&ioctl_video_mode) ;
	ioc->uni.Get_Screen.video_mode_flags = ioctl_video_mode.video_mode_flags;
	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

// <<TBD>> capture_screen parameters are not used.
void ioctl_capture_video (VIDEO_Ioctl *ioc)
{
	capture_param_t cap_param;		// capture parameters filled in by driver

	TileColumnSize = ioc->uni.Get_Screen.TileColumnSize;
	TileRowSize = ioc->uni.Get_Screen.TileRowSize;

	capture_screen(ioc->uni.Capture_Param.Cap_Flags, &cap_param);
	// copy data into the ioctl structure
	get_screen(ioc); 

	return;
}

void ioctl_get_palette_wo_wait (VIDEO_Ioctl *ioc)
{
    get_current_palette(ioc->Palt_Address);
    /**
     * Once the external 256 color palette changes., we capture the Attribute palette also.
     **/
    get_attribute(ioc);
    return;
}

void ioctl_get_palette( VIDEO_Ioctl *ioc)
{
	int 			status;
	/**
	 * Added the 2nd param to the capture_palt 
	 * This is to pass the address allocated in the adviser to the driver
	**/
	status = capture_palt(ioc->uni.Capture_Param.Cap_Flags, ioc->Palt_Address);
	
      	/**
	 * Once the external 256 color palette changes., we capture the Attribute palette also.
	**/
	get_attribute(ioc);

	if (status == IS_IT_PAL) ioc->ErrCode = VIDEO_IOCTL_PALETTE_CHANGE; 
	
	return ;
}

void ioctl_get_curpos( VIDEO_Ioctl *ioc)
{
	ioc->uni.Get_Curpos.CRTCA = (int)crtc->CRTCA;
	ioc->uni.Get_Curpos.CRTCB = (int)crtc->CRTCB;
	ioc->uni.Get_Curpos.CRTCC = (int)crtc->CRTCC;
	ioc->uni.Get_Curpos.CRTCD = (int)crtc->CRTCD;
	ioc->uni.Get_Curpos.CRTCE = (int)crtc->CRTCE;
	ioc->uni.Get_Curpos.CRTCF = (int)crtc->CRTCF;
	return;
}	

void ioctl_capture_xcursor(VIDEO_Ioctl *ioc)
{
        //int status;

        capture_xcursor(ioc->uni.xcursor);
	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

void ioctl_resume_video( VIDEO_Ioctl *ioc)
{
	resume_video();
	return;
}

void ioctl_control_display( VIDEO_Ioctl *ioc)
{
	unsigned char lockstatus = 0;
	// Get the status from the video app
	lockstatus = ioc->HostDispLock; 
	
	// Call this to finish the lock/unlock task
	control_display(&lockstatus);
	ioc->HostDispLock = lockstatus;
	
	return;
}

void ioctl_get_bandwidth_mode( VIDEO_Ioctl *ioc)
{
	ioc->LowBandwidthMode = get_bandwidth_mode();
	
	return;
}

void ioctl_set_bandwidth_mode( VIDEO_Ioctl *ioc)
{
	// Call this to finish setting the global flag
	set_bandwidth_mode(ioc->LowBandwidthMode);

	return;
}

void ioctl_ctrl_card_presence( VIDEO_Ioctl *ioc)
{
	unsigned char CardPresence = 0;

	// Get the Card Presence status flag from video app
	CardPresence = ioc->CardPresence;

	// Call this to finish the hide/show P3 card task
	control_card_presence(CardPresence);

	return;
}

void ioctl_get_card_presence( VIDEO_Ioctl *ioc)
{
	ioc->CardPresence = get_card_presence();
	
	return;
}

void ioctl_set_pci_subsystemID( VIDEO_Ioctl *ioc)
{

	set_pci_subsystem_Id(ioc->uni.PCIsubsysID);

	return;
}

void ioctl_get_pci_subsystemID( VIDEO_Ioctl *ioc)
{
	unsigned long pciSubsysId;
	pciSubsysId = get_pci_subsystem_Id();
	ioc->uni.PCIsubsysID.devID = pciSubsysId >> 16;
	ioc->uni.PCIsubsysID.venID = pciSubsysId & 0xFFFF;
	return;
}

static void get_screen (VIDEO_Ioctl *ioc)
{
	ioc->uni.Get_Screen.FrameHeight 	=	current_video_mode.height;
	ioc->uni.Get_Screen.FrameWidth  	=	current_video_mode.width;
	ioc->uni.Get_Screen.VDispEnd 		= 	current_video_mode.vdispend;
	ioc->uni.Get_Screen.HDispEnd 		=	current_video_mode.hdispend;
	ioc->uni.Get_Screen.FrameSize		= 	current_video_mode.captured_byte_count ;  
	ioc->uni.Get_Screen.ModeChange     	= 	FALSE;
	ioc->uni.Get_Screen.BytesPerPixel	= 	current_video_mode.depth;//get_bpp(&current_video_mode) ;
	ioc->uni.Get_Screen.Actual_Bpp		=	current_video_mode.act_depth;
	ioc->uni.Get_Screen.char_height		= 	current_video_mode.char_height ;
	ioc->uni.Get_Screen.video_mode_flags	= 	current_video_mode.video_mode_flags ;
	ioc->uni.Get_Screen.text_snoop_status	= 	current_video_mode.text_snoop_status ;
	ioc->uni.Get_Screen.left_x		= 	current_video_mode.left_x ;
	ioc->uni.Get_Screen.right_x		= 	current_video_mode.right_x ;
	ioc->uni.Get_Screen.top_y		= 	current_video_mode.top_y ;
	ioc->uni.Get_Screen.bottom_y		=	current_video_mode.bottom_y ;
	ioc->uni.Get_Screen.tile_cap_mode	=	current_video_mode.tile_cap_mode;
	ioc->uni.Get_Screen.SyncLossInvalid 	= 	FALSE;
	ioc->uni.Get_Screen.TextBuffOffset	= 	current_video_mode.TextBuffOffset;	
	ioc->LowBandwidthMode			=	current_video_mode.bandwidth_mode;

	/**
	 * 15bpp was not handled before
	 * Had to set the same bpp = 2 value for 15bpp as well for capturing purposes
	 * Hence, have set a flag to indicate 15bpp. If true, then sending flag to client
	 * Client will set the RGB model according to flag
	**/
	if (g_bpp15 == 1) {
		ioc->uni.Get_Screen.video_mode_flags |= (1 << 3);
		g_bpp15 = 0;
	}

	// Updated the 4bpp code since it is common to Pilot2 and Pilot3
	if (current_video_mode.depth == 0x05)
	{
		ioc->uni.Get_Screen.video_mode_flags |= BAD_COMPRESSION;
		ioc->ErrCode = VIDEO_IOCTL_SUCCESS;
		return;
	}
	
	if (current_video_mode.capture_status & BAD_COMPRESSION)
	{
		TDBG ("Bad Compression....\n");
		ioc->uni.Get_Screen.video_mode_flags |= BAD_COMPRESSION;
		ioc->ErrCode = VIDEO_IOCTL_SUCCESS;
		return;
	}

	// To indicate Syncloss/InvalidMode to videod application
	if(current_video_mode.capture_status & NO_VIDEO)
	{
		TDBG ("INPUT LOST... \n");
		ioc->uni.Get_Screen.SyncLossInvalid = MG9080_NO_INPUT;
		ioc->ErrCode = VIDEO_IOCTL_NO_INPUT ;
		return ;
	}

	if (current_video_mode.capture_status & UNSUPPORTED_MODE)
	{
		ioc->uni.Get_Screen.SyncLossInvalid = MG9080_INVALID_INPUT;
		TDBG ("INVALID MODE... \n");
		
	}
	if (current_video_mode.capture_status & BAD_COLOR_DEPTH)
	{
		ioc->uni.Get_Screen.SyncLossInvalid = MG9080_INVALID_INPUT;
		TDBG ("INVALID MODE... \n");
		
	}

	if (current_video_mode.capture_status & MODE_CHANGE) {
		
		ioc->uni.Get_Screen.ModeChange 	= TRUE;
		ioc->ErrCode 				= VIDEO_IOCTL_BLANK_SCREEN;
		return;
	}

	if (current_video_mode.capture_status & NO_SCREEN_CHANGE) 
	{
		if (crtcext->SEQ1 & SEQ1_POWOFF)
		{
			ioc->ErrCode = VIDEO_IOCTL_BLANK_SCREEN;
			
		}
		else
		{
			ioc->ErrCode = VIDEO_IOCTL_NO_VIDEO_CHANGE ;
		
		}
		return;	
	}
	ioc->ErrCode = VIDEO_IOCTL_SUCCESS;

	return;
}

static void get_attribute(VIDEO_Ioctl *ioc)
{
	/**
	 * This is the attribute palette
	 * Attribute palette covers 16 colors which is internally built.
	**/
	ioc->uni.Get_Attr.ATTR0 = attr->ATTR0;
	ioc->uni.Get_Attr.ATTR1 = attr->ATTR1;
	ioc->uni.Get_Attr.ATTR2 = attr->ATTR2;
	ioc->uni.Get_Attr.ATTR3 = attr->ATTR3;
	ioc->uni.Get_Attr.ATTR4 = attr->ATTR4;
	ioc->uni.Get_Attr.ATTR5 = attr->ATTR5;
	ioc->uni.Get_Attr.ATTR6 = attr->ATTR6;
	ioc->uni.Get_Attr.ATTR7 = attr->ATTR7;
	ioc->uni.Get_Attr.ATTR8 = attr->ATTR8;
	ioc->uni.Get_Attr.ATTR9 = attr->ATTR9;
	ioc->uni.Get_Attr.ATTRA = attr->ATTRA;
	ioc->uni.Get_Attr.ATTRB = attr->ATTRB;
	ioc->uni.Get_Attr.ATTRC = attr->ATTRC;
	ioc->uni.Get_Attr.ATTRD = attr->ATTRD;
	ioc->uni.Get_Attr.ATTRE = attr->ATTRE;
	ioc->uni.Get_Attr.ATTRF = attr->ATTRF;

	return;
}
