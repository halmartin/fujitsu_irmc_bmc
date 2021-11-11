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

#ifndef AMI_VIDEODATA_H
#define AMI_VIDEODATA_H

#include <linux/wait.h>
/* Internal Data Structures and defines. Not exposed to user mode app */

#include "videocap.h"
#include "dbgout.h"
#include "dbglvl.h"
#include "fge.h"

#ifdef  DEFINE_VIDEODATA
#define EXTERN
#else
#define EXTERN extern
#endif

#define TRUE						1
#define FALSE						0

#define IS_IT_PAL              				12
#define IS_IT_TSE              				34
#define IS_IT_XCURPOS					56
#define IS_IT_XCURCOL					65
#define IS_IT_XCURCTL					78
#define IS_IT_XCUR					87
#define IS_IT_TFE					90
#define IS_IT_BSE					91
#define BIGPHYS_SIZE					(8192 * 1024) //(5632 * 1024) //(4096 * 1024)

// Low Bandwidth mode definitions
#define LOW_BANDWIDTH_MODE_NORMAL			0
#define LOW_BANDWIDTH_MODE_8BPP				1
#define LOW_BANDWIDTH_MODE_16BPP			2	
#define LOW_BANDWIDTH_MODE_8BPP_BW			3

typedef struct {
	unsigned char row;
	unsigned char col;
} tm_entry_t;

EXTERN tm_entry_t *tile_map;						/* tile map */

EXTERN unsigned long  	pilot_fg_base;				/* Frame Grabber Base Address	*/
EXTERN u8 *tfedest_base;				/* TFE Destination Base Address	*/ 
EXTERN dma_addr_t	tfedest_bus;
EXTERN u8 *tmpbuffer_base;				/* TFE Destination Base Address	*/ 
EXTERN dma_addr_t	tmpbuffer_bus;
EXTERN unsigned short 	pilot_fgb_irq;				/* Frame Grabber IRQ No	*/
EXTERN unsigned short 	mg908X_dma; 				/* FPGA DMA Channel  			*/
EXTERN unsigned char 	UseDMA;   					/* To Use or Not to use DMA 	*/
EXTERN unsigned char 	PollMode;  					/* To Poll or Not to poll 		*/	
EXTERN unsigned char		bpp4_mode;

EXTERN volatile FGB_TOP_REG_STRUCT *fgb_top_regs;	/* Frame Grabber Top		*/
EXTERN volatile TSE_REG_STRUCT *fgb_tse_regs;		/* Tile Snoop Engine		*/
EXTERN volatile TFE_REG_STRUCT *fgb_tfe_regs;		/* Tile Fetch Engine		*/
EXTERN volatile unsigned long *fgb_tse_rr_base;		/* Tile Snoop Row Reg Base  */
EXTERN volatile BSE_REG_STRUCT *fgb_bse_regs;		/* Bit Slice Engine			*/
EXTERN volatile GRC_STRUCT *grc_ctl_regs;               /* GRC CTL based Regs */ 
EXTERN volatile GRC_REGS_STRUCT *grc_regs;		/* GRC CUR based Regs */
EXTERN volatile unsigned long *display_cntrl;		/* Display Control Regs */
EXTERN volatile unsigned long *card_presence_cntrl;	/* Control visiblity of P3 Graphics to Host */
EXTERN volatile unsigned long *PCISID_regs;
//EXTERN volatile unsigned long *seq1_reg;


EXTERN volatile CRTC0_REG_STRUCT *crtc ;
EXTERN volatile CRTCEXT0_TO_7_REG_STRUCT *crtcext ;
EXTERN volatile ATTR_REG_STRUCT *attr ;
EXTERN volatile unsigned char *XMULCTRL ;
EXTERN long framebuffer_base ;
EXTERN long tfe_descriptor_base ;
EXTERN long sysclk_base;
EXTERN long tfe_dest ;
EXTERN unsigned long TileColumnSize ;
EXTERN unsigned long TileRowSize ;
EXTERN unsigned int MaxRectangles ;
EXTERN unsigned long descaddr;

EXTERN VIDEO_MODE_PARAMS ioctl_video_mode ;
EXTERN VIDEO_MODE_PARAMS before_video_capture ;
EXTERN VIDEO_MODE_PARAMS after_video_capture ;
EXTERN VIDEO_MODE_PARAMS current_video_mode ;
EXTERN VIDEO_MODE_PARAMS status_video_mode ;

/* Tuneable Parameters */
#ifdef  DEFINE_VIDEODATA
TDBG_DECLARE_DBGVAR(videocap)
#else
TDBG_DECLARE_DBGVAR_EXTERN(videocap)
#endif

/* Statistics  */
EXTERN unsigned long 	CaptureCount;
EXTERN unsigned long	FpgaIntrCount;
EXTERN unsigned long	FpgaTimeoutCount;
EXTERN unsigned long	InvalidScreenCount;
EXTERN unsigned long	BlankScreenCount;
EXTERN unsigned long	NoChangeScreenCount;
EXTERN unsigned long	StartTime;



#endif

