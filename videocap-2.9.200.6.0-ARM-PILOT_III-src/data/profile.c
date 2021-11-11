/*****************************************************************
 **                                                             **
 **     (C) Copyright 2009-2015, American Megatrends Inc.       **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                             **
 **         Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                             **
 ****************************************************************/

/****************************************************************
* @file   	capmain.c
* @author 	Varadachari Sudan Ayanam <varadacharia@ami.com>
* @brief  	capture driver main file
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
#include <linux/jiffies.h>
#include "dbgout.h"			
#include "videodata.h"

void
ReInitializeDriver(void)
{
#if 0
	TDBG ("Reinitializing Driver\n");
#endif

	/* Reset Profiles and counters */
	CaptureCount 			= 0;		/* Total Commands issued to FPGA */
	FpgaIntrCount			= 0;		/* Fpga Interrupt Count*/
	FpgaTimeoutCount		= 0;		/* Fpga TimeOut Count */

	InvalidScreenCount		= 0;		/* Invalid Screens */
	BlankScreenCount		= 0;		/* Number of Blank Screen Sent */
	NoChangeScreenCount		= 0;		/* No Changes Screens */

	StartTime				= jiffies;	/* Start Time  of Capture */

	return;
}
