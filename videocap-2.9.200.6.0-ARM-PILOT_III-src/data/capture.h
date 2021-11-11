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

#ifndef CAPTURE_H
#define CAPTURE_H

#include "coreTypes.h"
#include "icc_what.h"

#ifdef ICC_OS_WINDOWS
#pragma pack(1)
#endif


#ifdef ICC_OS_LINUX
#define PACK __attribute__((packed))
#else
#define PACK
#endif

/**
 * Capture parameters
**/
typedef struct
{
	u8	BytesPerPixel; 
	u8	video_mode_flags;
	u8	char_height;
	u8	left_x ;
	u8	right_x ;
	u8	top_y ;
	u8	bottom_y ;
	u8	text_snoop_status ;
	u8	Actual_Bpp;
} PACK capture_param_t;

typedef struct
{
	unsigned int CRTCA;
	unsigned int CRTCB;
	unsigned int CRTCC;
	unsigned int CRTCD;
	unsigned int CRTCE;
	unsigned int CRTCF;
} PACK curpos_param_t;

typedef struct
{
	unsigned int CURSEN;
	unsigned int STRTADDR;
	unsigned int ENDADDR;
	unsigned int CURPOSX;
	unsigned int CURPOSY;
} PACK curpos_t;

typedef struct
{
	unsigned char LowBandwidthMode;
} PACK bandwidth_t;

/**
 * The Text mode colors are all passed through the internal Attribute palette first
 * This value got from attribute palette is then used as index to the 1024byte color palette
 * Hence sending all 16 attribute colors 
**/
typedef struct
{
	u8 ATTR0;
	u8 ATTR1;
	u8 ATTR2;
	u8 ATTR3;
	u8 ATTR4;
	u8 ATTR5;
	u8 ATTR6;
	u8 ATTR7;
	u8 ATTR8;
	u8 ATTR9;
	u8 ATTRA;
	u8 ATTRB;
	u8 ATTRC;
	u8 ATTRD;
	u8 ATTRE;
	u8 ATTRF;
} PACK attr_t;


#define CAPTURE_HANDLED                         0
#define CAPTURE_ERROR                           1
#define CAPTURE_NO_CHANGE                       2

#define MAX_SUPPORTED_WIDTH	1920
#define BPP_8					1
#define BPP_16					2
#define BPP_24					3
#define BPP_32					4
#define BPP_4					5

int capture_text_screen(unsigned int cap_flags);
int capture_mode12_screen(unsigned int cap_flags);
int capture_mode13_screen(unsigned int cap_flags);
int capture_tile_screen(unsigned int cap_flags);


#ifdef ICC_OS_WINDOWS
#pragma pack()
#endif

#undef PACK

#endif /* CAPTURE_H */

