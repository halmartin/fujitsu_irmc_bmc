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

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

#include "dbgout.h"			
#include "videocap.h"
#include "videodata.h"		
#include "videoctl.h"
#include "fge.h"
#include "iohndlr.h"

#ifdef HAVE_UNLOCKED_IOCTL  
  #if HAVE_UNLOCKED_IOCTL  
  #define USE_UNLOCKED_IOCTL  
  #endif
#endif

#ifdef USE_UNLOCKED_IOCTL
long videocap_ioctl(struct file *filep,unsigned int cmd,unsigned long arg)
#else
int videocap_ioctl(struct inode *inode, struct file *filep,unsigned int cmd,unsigned long arg)
#endif
{
    VIDEO_Ioctl ioc;

#ifndef USE_UNLOCKED_IOCTL
    /* Validation : If valid dev */
    if (!(inode) )
		return -EINVAL;
#endif

    /* Validation : Check it is ours */
    if (_IOC_TYPE(cmd) != VIDEO_MAGIC)
		return (-EINVAL);

    /* Copy from user to kernel space */
    if (copy_from_user(&ioc, (char *) arg, sizeof(VIDEO_Ioctl)))
    {
	    printk("videocap_ioctl: Error copying data from user \n");
	    return -EFAULT;
    }

    switch (ioc.OpCode)
    {
		case VIDEO_IOCTL_INIT_VIDEO:
			ioctl_init_fpga (&ioc);				// currently does nothing
			break;

		case VIDEO_IOCTL_RESET_CAPTURE_ENGINE:
			ioctl_reset_capture_engine (&ioc);	// reset Profiles and counters
			break;

		case VIDEO_IOCTL_INIT_VIDEO_BUFFER:		
			ioctl_init_video_buffer (&ioc);		// zero even or odd raw buffer
			break;

		case VIDEO_IOCTL_SET_VIDEO_PARAMS:
			ioctl_set_fpga_params (&ioc);		// set Snoop Engine Params
			break;

		case VIDEO_IOCTL_GET_VIDEO_PARAMS:
			ioctl_get_fpga_params (&ioc);		// get Snoop Engine Params
			break;

		case VIDEO_IOCTL_GET_VIDEO_RESOLUTION:
			ioctl_get_video_resolution (&ioc);	// get width & height from MATROX
			break;

		case VIDEO_IOCTL_CAPTURE_VIDEO:
			ioctl_capture_video (&ioc);
			break;

		case VIDEO_IOCTL_GET_PALETTE:
			ioctl_get_palette (&ioc);
			break;
			
		case VIDEO_IOCTL_GET_PALETTE_WO_WAIT:
			ioctl_get_palette_wo_wait (&ioc);
			break;
			
		case VIDEO_IOCTL_GET_CURPOS:
			ioctl_get_curpos (&ioc);
			break;

		case VIDEO_IOCTL_CAPTURE_XCURSOR:
			ioctl_capture_xcursor(&ioc);
			break;

		/**
		 * This Ioctl is added as a re-initialization call whenever resume is called from JViewer
		 * The capture thread will be waiting in a interruptible call which will be 
		 * woken up if a new session comes in
		**/
		case VIDEO_IOCTL_RESUME_VIDEO:
			ioctl_resume_video (&ioc);
			break;

		case VIDEO_IOCTL_DISPLAY_CONTROL:
			ioctl_control_display(&ioc);
			break;

		case VIDEO_IOCTL_CTRL_CARD_PRESENCE:
			ioctl_ctrl_card_presence(&ioc);
			break;

		case VIDEO_IOCTL_LOW_BANDWIDTH_MODE:
			ioctl_set_bandwidth_mode(&ioc);
			break;

		case VIDEO_IOCTL_GET_BANDWIDTH_MODE:
			ioctl_get_bandwidth_mode(&ioc);
			break;
	
		case VIDEO_IOCTL_GET_CARD_PRESENCE:
			ioctl_get_card_presence(&ioc);
			break;
			
		case VIDEO_IOCTL_SET_PCISID:
			ioctl_set_pci_subsystemID(&ioc);
			break;

		case VIDEO_IOCTL_GET_PCISID:
			ioctl_get_pci_subsystemID(&ioc);
			break;
			
		default:
			TDBG("Unknown Ioctl\n");
			return -EINVAL;
			break;
    }

    /* Copy back the results to user space */
    if (copy_to_user((char *) arg, &ioc, sizeof(VIDEO_Ioctl)))
    {
	    printk("videocap_ioctl: Error copying data to user \n");
	    return -EFAULT;
    }

    return 0;
}



