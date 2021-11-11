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

#ifndef AMI_VIDEOCAP_H
#define AMI_VIDOECAP_H

/* Include the corresponding FPGA header file */
#include "fge.h"

/* Ioctl Definition File */
#include "videoctl.h"

/* Major Number of our driver */ /* Compatible with G2 */
#define VIDEOCAP_DEVICE_MAJOR 15	

/* Minimum data bytes for which DMA has to be used */
#define MIN_DMA_TRANSFER	64

/* FPGA Capture Timeout Value (sec).Should be greater than the worst case */
#define FPGA_TIMEOUT_VALUE	1

/* Maximum Pending Screens to Keep*/
#define MAX_SCREENS 1

/* Default Tile Threshold for 9066 */
#define TILE_THRESHOLD 10

/* Default Luminance Threshold for 9066 */
#define LUMINANCE_THRESHOLD 0x90

/* Default PLL Phase Delay for the Xicor98014 ADC */
#define ADC_PHASE_DELAY	(0x40 >> 1)

#define ODD_RAW_BUFFER	0
#define EVEN_RAW_BUFFER	1
#define NO_VIDEO_CHANGE	7
#define INVALID_BUFFER  8
#define NO_VIDEO_INPUT  9

/* Read Ahead Cache Enable */
//#define READ_AHEAD_CACHE

#endif
