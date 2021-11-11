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

# ifndef __AMI_VIDEOCTL_H_
# define __AMI_VIDEOCTL_H_

#ifdef __KERNEL__         
#include <linux/version.h>
#if (LINUX_VERSION_CODE !=  KERNEL_VERSION(3,14,17))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif
#endif
//#include "capture.h"

/* Capture Modes */
#define MODE_FULLSCREEN  	0	/* CAP E/A */
#define MODE_CAPXORRLE_B  	1	/* CAP O/B*/
#define MODE_CAPXORRLE_A  	2	/* CAP E/A*/

// tile arrangement in captured frame
#define TILE_CAP_TILES_ONLY		0x0001
#define TILE_CAP_RECTANGLE		0x0002

/* Character device IOCTL's */
#define VIDEO_MAGIC  		'a'
#define VIDEO_IOCCMD     	_IOWR(VIDEO_MAGIC, 0,VIDEO_Ioctl_AVICAA)

typedef enum VIDEO_opcode
{
	VIDEO_IOCTL_START_CAPTURE = 0,
	VIDEO_IOCTL_STOP_CAPTURE,
	VIDEO_IOCTL_GET_SCREEN,
	VIDEO_IOCTL_DONE_SCREEN,
	VIDEO_IOCTL_INIT_VIDEO,
	VIDEO_IOCTL_RESET_CAPTURE_ENGINE,
	VIDEO_IOCTL_INIT_VIDEO_BUFFER,
	VIDEO_IOCTL_SET_VIDEO_PARAMS,
	VIDEO_IOCTL_GET_VIDEO_PARAMS,
	VIDEO_IOCTL_GET_VIDEO_RESOLUTION,
	VIDEO_IOCTL_SET_VIDEO_RESOLUTION,
	VIDEO_IOCTL_CAPTURE_VIDEO,
	VIDEO_IOCTL_CAPTURE_RAW,
	VIDEO_IOCTL_GET_COLOR_GAIN,
	VIDEO_IOCTL_SET_COLOR_GAIN,
	VIDEO_IOCTL_SET_DEFAULT_COLOR_GAIN,
	VIDEO_IOCTL_SHIFT_IMAGE_LEFT,
	VIDEO_IOCTL_SHIFT_IMAGE_RIGHT,
	VIDEO_IOCTL_SHIFT_IMAGE_UP,
	VIDEO_IOCTL_SHIFT_IMAGE_DOWN,
	VIDEO_IOCTL_AUTO_CALIBRATE,
	VIDEO_IOCTL_PROG_VIDEO,
	VIDEO_IOCTL_DEPROG_VIDEO,
	VIDEO_IOCTL_GET_PALETTE,
	VIDEO_IOCTL_GET_PALETTE_WO_WAIT,
	VIDEO_IOCTL_GET_CURPOS,
	VIDEO_IOCTL_RESUME_VIDEO,
	VIDEO_IOCTL_CAPTURE_XCURSOR,
	VIDEO_IOCTL_DISPLAY_CONTROL,
	VIDEO_IOCTL_CTRL_CARD_PRESENCE,
	VIDEO_IOCTL_LOW_BANDWIDTH_MODE,
	VIDEO_IOCTL_GET_BANDWIDTH_MODE,
	VIDEO_IOCTL_GET_CARD_PRESENCE,
	VIDEO_IOCTL_SET_PCISID,
	VIDEO_IOCTL_GET_PCISID	
} VIDEO_OpCode;

typedef enum
{
	VIDEO_IOCTL_SUCCESS = 0,
	VIDEO_IOCTL_INTERNAL_ERROR,
	VIDEO_IOCTL_CAPTURE_ALREADY_ON,
	VIDEO_IOCTL_CAPTURE_ALREADY_OFF,
	VIDEO_IOCTL_SEQNO_MISMATCH,
	VIDEO_IOCTL_NO_SCREENS,
	VIDEO_IOCTL_NO_INPUT,
	VIDEO_IOCTL_INVALID_INPUT,
	VIDEO_IOCTL_NO_VIDEO_CHANGE,
	VIDEO_IOCTL_BLANK_SCREEN,
	VIDEO_IOCTL_HOST_POWERED_OFF,
	VIDEO_IOCTL_VIDEO_PROG_ERROR,
	VIDEO_IOCTL_PALETTE_CHANGE,
	VIDEO_IOCTL_XCURSOR_DISABLED
} VIDEO_ErrCode;
/* cursor mode */
#define XCURSOR_MODE_DISABLED		0
#define XCURSOR_MODE_3COLOR		1
#define XCURSOR_MODE_XGA		2
#define XCURSOR_MODE_XWINDOWS		3
#define XCURSOR_MODE_16COLOR		4

#define XCURSOR_COL			0x81
#define XCURSOR_POSCTL			0x82
#define XCURSOR_ALL			0x83
#define XCURSOR_NONE			0x84
#define XCURSOR_CTL			0x85

#define XCURSOR_MAX_COLORS		16
#define XCURSOR_MAP_SIZE		384 /* 384 for 16colors, 128 for others */


/* hardware cursor color */
struct hwcursor_color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} __attribute__((packed));
typedef struct hwcursor_color XCURSOR_COLOR;

struct hwcursor_pos {
	unsigned char mode;
	unsigned short pos_x;
	unsigned short pos_y;
} __attribute__((packed));
typedef struct hwcursor_pos XCURPOS;
typedef struct hwcursor_pos xcursor_pos_t;

struct hwcursor_col {
	XCURSOR_COLOR color[XCURSOR_MAX_COLORS];
	unsigned long long maps[XCURSOR_MAP_SIZE];
} __attribute__((packed));
typedef struct hwcursor_col XCURCOL;
typedef struct hwcursor_col xcursor_col_t;

struct hwcursor_data {
	XCURPOS		xcurpos;
	XCURCOL		xcurcol;
} __attribute__((packed));
typedef struct hwcursor_data XCURSOR_DATA;
typedef struct hwcursor_data xcursor_data_t;

/* hardware cursor info */
struct hwcursor_info {
	unsigned char	xcursor_mode;
	XCURSOR_DATA	xcursor_data;
} __attribute__((packed));
typedef struct hwcursor_info XCURSOR_INFO;
typedef struct hwcursor_info xcursor_param_t;


typedef struct
{
	unsigned long  FrameSize;
	unsigned short FrameWidth;
	unsigned short FrameHeight;
	unsigned short HDispEnd;
	unsigned short VDispEnd;
	unsigned char  SyncLossInvalid;
	unsigned char  ModeChange;
	unsigned char TileColumnSize;
	unsigned char TileRowSize ;
	unsigned char TextBuffOffset ;
	unsigned char BytesPerPixel ;
	unsigned char left_x ;
	unsigned char right_x ;
	unsigned char top_y ;
	unsigned char bottom_y ;
	unsigned char char_height ;
	unsigned char video_mode_flags ;
	unsigned char text_snoop_status ;
	unsigned short tile_cap_mode;
	unsigned char Actual_Bpp;
} FRAME_INFO;


typedef struct
{
	unsigned int CRTCA;    /* Start Addr */
	unsigned int CRTCB;    /* End Addr */
	unsigned int CRTCC;    /* Row Aide */ 
	unsigned int CRTCD;    /* Column Aide */
	unsigned int CRTCE;    /* Row */
	unsigned int CRTCF;    /* Column */
} CURPOS_INFO;

/**
 * Added a seperate structure of attributes to be used for text mode colors
 * The colors are determined by passing the attribute index to the attribute palette
 * which is then passed as index to the 256 color palette.
 **/ 
typedef struct
{
	unsigned char ATTR0;
	unsigned char ATTR1;
	unsigned char ATTR2;
	unsigned char ATTR3;
	unsigned char ATTR4;
	unsigned char ATTR5;
	unsigned char ATTR6;
	unsigned char ATTR7;
	unsigned char ATTR8;
	unsigned char ATTR9;
	unsigned char ATTRA;
	unsigned char ATTRB;
	unsigned char ATTRC;
	unsigned char ATTRD;
	unsigned char ATTRE;
	unsigned char ATTRF;
} ATTR_INFO;

typedef struct
{
	unsigned short devID;
	unsigned short venID;
}PCI_DEVID;

/* Soft compression types */
#define SOFT_COMPRESSION_NONE 		0x00
#define SOFT_COMPRESSION_MINILZO	0x01
#define SOFT_COMPRESSION_GZIP		0x02
#define SOFT_COMPRESSION_BZIP2		0x03
#define MG9080_NO_INPUT				0x01 // sync loss bit
#define MG9080_INVALID_INPUT		0x02 // Invalid Input

#define CAPTURE_FRAME_BIT			0x0001
#define CAPTURE_EVEN_FRAME			0x0000
#define CAPTURE_ODD_FRAME			0x0001
#define CAPTURE_CLEAR_BUFFER			0x0002
#define CAPTURE_PALETTE_UPDATED			0x0008
#define CAPTURE_NEW_SESSION 			0xFF00

typedef struct 
{
	VIDEO_OpCode		OpCode;
	VIDEO_ErrCode 	ErrCode;
	u8 		*Palt_Address;
	u8 		HostDispLock;
	u8		CardPresence;
	u8		LowBandwidthMode;
	union
	{
		struct
		{
			unsigned int Frame; 		/* Odd or Even Frame */
			unsigned int Cap_Flags;		/* capture mode Full screen, Odd, Even*/
		} Capture_Param;

		struct
		{
			unsigned char RGain;
			unsigned char GGain;
			unsigned char BGain;
			unsigned char Default_RGain;
			unsigned char Default_GGain;
			unsigned char Default_BGain;
		}Color_Gain;

		FRAME_INFO Get_Screen;
		CURPOS_INFO Get_Curpos;
		ATTR_INFO Get_Attr;
		XCURSOR_INFO *xcursor;
		PCI_DEVID PCIsubsysID;
#ifdef __KERNEL__
	} uni;
#else
};
#endif
} __attribute__((packed)) VIDEO_Ioctl_AVICAA;

typedef VIDEO_Ioctl_AVICAA VIDEO_Ioctl;

#endif

