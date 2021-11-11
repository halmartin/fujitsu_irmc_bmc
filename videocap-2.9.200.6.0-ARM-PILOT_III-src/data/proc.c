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
* @file 	proc.c
* @authors	Vinesh Christoper 		<vineshc@ami.com>,
*		Baskar Parthiban 		<bparthiban@ami.com>,
*		Varadachari Sudan Ayanam 	<varadacharia@ami.com>
* @brief   	functions exposed through proc file system
*			are defined here
****************************************************************/

#ifndef MODULE				/* If not defined in Makefile */
#define MODULE
#endif

#ifndef __KERNEL__			/* If not defined in Makefile */
#define __KERNEL__
#endif

#ifndef EXPORT_SYMTAB		/* If not defined in Makefile */
#define EXPORT_SYMTAB
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>

#include "dbgout.h"			/* debugging macros */
#include "videocap.h"
#include "cap90xx.h"
#include "videodata.h"
extern unsigned char doDecrement;
extern unsigned char doIncrement;


#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int ReadDriverInfo(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int ReadDriverInfo(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{

	int len=0;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif

	/* Fill the buffer with the data */
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Video Capture Driver Information\n");
	len += sprintf(buf+len,"Copyright (c) 2009-2015 American Megatrends Inc.\n\n");
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Tuneable Parameter's Values :\n");
	len += sprintf(buf+len,"\tDebug Level = 0x%lx\n",TDBG_FORM_VAR_NAME(videocap));

	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Resources Used:\n");
	len += sprintf(buf+len,"\t Frame Grabber Base = 0x%lx\n",pilot_fg_base);
	len += sprintf(buf+len,"\t Frame Grabber IRQ = 0x%d (%s)\n", pilot_fgb_irq,(PollMode)?"Polling":"Used");
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Debug Option for \"DebugLevel\" :\n");

#ifdef DEBUG
	len += sprintf(buf+len,"\t Bit 0 = Module Main and Driver Open and Close\n");
	len += sprintf(buf+len,"\t Bit 1 = FPGA Capture, Interrupt and Timeouts \n");
	len += sprintf(buf+len,"\t Bit 2 = Ioctl - Start and Stop Redirection\n");
	len += sprintf(buf+len,"\t Bit 3 = Capture Screen Validations\n");
	len += sprintf(buf+len,"\t Bit 4 = MMAP Operations \n");
#else
	len += sprintf(buf+len,"\t Module is compiled without DEBUG option. Cannot use DebugLevel\n");
#endif

	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif

	/* Return length of data */
	return len;
}


#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int ReadDriverStatus(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int ReadDriverStatus(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len=0;
	int bpp;
	int width;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
	/* This is a requirement. So don't change it */
	*start = buf;

  if (offset != 0)
	{
		*eof = 1;
		return 0;
	}
#else
  if (*offset != 0)
	{
		return 0;
	}
#endif

	/* Fill the buffer with the data */
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Video Capture Driver Status\n");
	len += sprintf(buf+len,"Copyright (c) 2009-2015 American Megatrends Inc.\n\n");
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	
	get_video_mode_params(&status_video_mode) ;
	if(crtcext->GCTL6 & REG_BIT0) { // graphics mode is set
		len += sprintf(buf+len,"\tVideo Resolution = %dx%d\n",
			(int)status_video_mode.width, (int)status_video_mode.height);
		width = crtc->CRTC0 ;
		if(crtcext->CRTCEXT1 & 1) {
			width |= 0x10 ;
		}
		width +=5 ;
		width *= 8 ;
		len += sprintf(buf+len,"\tHTOTAL -  CRTC0 = %d\n",
			width);
	}
	else { // character mode
		len += sprintf(buf+len,"\tVideo Resolution = %dx%d\n",
			(int)status_video_mode.width, (int)status_video_mode.height / status_video_mode.char_height );
		len += sprintf(buf+len,"\tCharacter Height = %d\n", status_video_mode.char_height );
	}

	bpp = status_video_mode.depth;//(int)get_bpp(&status_video_mode);
	switch(bpp) {
		case 1:
			len += sprintf(buf+len,"\tColor Depth is 8\n");
			break ;
		case 2:
			len += sprintf(buf+len,"\tColor Depth is 16\n");
			break ;
		case 3:
			len += sprintf(buf+len,"\tColor Depth is 24\n");
			break ;
		case 4:
			len += sprintf(buf+len,"\tColor Depth is 32\n");
			break ;
		default:
			len += sprintf(buf+len,"\tColor Depth is unknown = %x\n",status_video_mode.depth);
			break ;

	}
	if((status_video_mode.video_mode_flags & MGA_MODE) == 0) {
		len += sprintf(buf+len,"\tVGA Mode is set \n");

	}
	else {
		len += sprintf(buf+len,"\tMGA Mode is set \n");
	}
	if(crtcext->GCTL6 & REG_BIT0) {
		len += sprintf(buf+len,"\tGraphics Mode Set \n");
	}
	else {
		len += sprintf(buf+len,"\tCharacter Mode Set \n");
	}
	if(status_video_mode.video_mode_flags & MODE13) {
		len += sprintf(buf+len,"\tVGA MODE13 is set \n");
	}
	if(status_video_mode.video_mode_flags & MODE12) {
		len += sprintf(buf+len,"\tVGA MODE12 is set \n");
	}
	if(crtcext->GCTL5 & REG_BIT6) {
		len += sprintf(buf+len,"\tVGA 256 Color Mode Set \n");
	}
	else {
		len += sprintf(buf+len,"\tVGA 256 Color Mode NOT Set \n");
	}
	len += sprintf(buf+len,"\tVideo Buffer Address Offset = 0x%lx \n",status_video_mode.video_buffer_offset);
	
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
	/* Set End of File if no more data */
	*eof = 1;
#endif

	/* Return length of data */
	return len;
}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int ReadDriverStatistics(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int ReadDriverStatistics(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len=0;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif

	/* Fill the buffer with the data */
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Video Capture Driver Statistics\n");
	len += sprintf(buf+len,"Copyright (c) 2009-2015 American Megatrends Inc.\n\n");
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

	len += sprintf(buf+len,"Capture Statistics :\n");
	len += sprintf(buf+len,"\tCapture Count = 0x%lx\n",CaptureCount);
	len += sprintf(buf+len,"\tFpga Interrupts  = 0x%lx\n",FpgaIntrCount);
	len += sprintf(buf+len,"\tFpga TimeOuts    = 0x%lx\n",FpgaTimeoutCount);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

	len += sprintf(buf+len,"Screen  Statistics :\n");
	len += sprintf(buf+len,"\tBlank Screens    = 0x%lx\n",BlankScreenCount);
	len += sprintf(buf+len,"\tInvalid Screens  = 0x%lx\n",InvalidScreenCount);
	len += sprintf(buf+len,"\tNoChange Screens = 0x%lx\n",NoChangeScreenCount);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

	len +=sprintf(buf+len,"Overall Frame Rate Statistics : \n");
	len +=sprintf(buf+len,"Captures Per Second = 0x%ld\n",
									(CaptureCount *HZ)/(jiffies -StartTime));
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif
	

  /* Return length of data */
	return len;
}


#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DumpPalette(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int DumpPalette(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len=0;
	unsigned char *palette ;
	int i ;
	int x ;
	int line ;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif

	palette = (unsigned char *)(pilot_fg_base + GRC_BASE_OFFSET) ;
//	printk("Pointer %lx\n",(unsigned long)palette);

	len += sprintf(buf+len,"\n\nPalette Entries\n");
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

	x = 0 ;
	line = 0 ;
	
       	len += sprintf(buf+len,"%02x\t",line);
	for(i=0; i< 256 ; i++) {
        	len += sprintf(buf+len,"%02x ", *palette++);
        	len += sprintf(buf+len,"%02x ", *palette++);
        	len += sprintf(buf+len,"%02x ", *palette++);
        	len += sprintf(buf+len,"%02x ", *palette++);
        	len += sprintf(buf+len,"\t");
		if(x == 3) {
			x = 0 ;
        		len += sprintf(buf+len,"\n");
			line += 4;
       			len += sprintf(buf+len,"%02x\t",line);
		}
		else {
			x++ ;
		}
	}

	len += sprintf(buf+len,"\n");

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif


	/* Return length of data */
	return len;
}
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DECTSEColumnSize(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int DECTSEColumnSize(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len=0;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif
  
  /* Fill the buffer with the data */
  len += sprintf(buf+len,"Current Column Size = 16 \n");

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif

	/* Return length of data */
	return len;
}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DumpVideo(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int DumpVideo(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{

	unsigned char *buff_ptr ;
	unsigned long video_offset ;
	int i ;
	int cnt ;

	int len=0;

	
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif

 
  video_offset = 0 ;
	video_offset |= (crtcext->CRTCEXT0 & 0xF) << 16 ;
	video_offset |= crtc->CRTCC << 8 ;
	video_offset |= crtc->CRTCD ;

	buff_ptr = (unsigned char *)(framebuffer_base + video_offset) ;

        /* Fill the buffer with the data */
        len += sprintf(buf+len,"Video Buffer Data \n");

	for (i=0; i< 64; i++) {
		for(cnt = 0; cnt < 16; cnt++) {
        		len += sprintf(buf+len,"%02x ", *buff_ptr );
			buff_ptr++ ;
		}
        	len += sprintf(buf+len,"\n");
	}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif

	/* Return length of data */
	return len;
}
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DumpTfeBuff(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int DumpTfeBuff(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{

	unsigned char *buff_ptr ;
	int i ;
	int cnt ;

	int len=0;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif

	buff_ptr = (unsigned char *)tfedest_base;

        /* Fill the buffer with the data */
        len += sprintf(buf+len,"TFE Buffer Data \n");

	for (i=0; i< 32; i++) {
		for(cnt = 0; cnt < 16; cnt++) {
        		len += sprintf(buf+len,"%02x ", *buff_ptr );
			buff_ptr++ ;
		}
        	len += sprintf(buf+len,"\n");
	}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif

	/* Return length of data */
	return len;
}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DECTSERowSize(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int DECTSERowSize(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len=0;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif
        
  /* Fill the buffer with the data */
  len += sprintf(buf+len,"Current Row Size = 16 \n");


#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif

	/* Return length of data */
	return len;
}
#if 0
int INCTSEMaxRectangles(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len=0;

	/* This is a requirement. So don't change it */
	*start = buf;

	if (offset != 0)
	{
		*eof = 1;
		return 0;
	}

        /* Fill the buffer with the data */
	MaxRectangles += 1 ;
        len += sprintf(buf+len,"Current Max Rectangles = %d \n", MaxRectangles);

	/* Set End of File if no more data */
	*eof = 1;

	/* Return length of data */
	return len;
}
#endif
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DECTSEMaxRectangles(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int DECTSEMaxRectangles(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{
	int len=0;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif

  /* Fill the buffer with the data */
	MaxRectangles -= 1 ;
  len += sprintf(buf+len,"Current Max Rectangles = %d \n", MaxRectangles);

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif

	/* Return length of data */
	return len;
}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
int DumpRegisters(struct file *file,  char __user *buf, size_t count, loff_t *offset)
#else
int DumpRegisters(char *buf, char **start, off_t offset, int count, int *eof, void *data)
#endif
{

	volatile GRC_REGS_STRUCT *grc_regs ;
	int len=0;
	unsigned long temp ;

#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
  /* This is a requirement. So don't change it */
  *start = buf;

  if (offset != 0)
  {
    *eof = 1;
    return 0;
  }
#else
  if (*offset != 0)
  {
    return 0;
  }
#endif	

  grc_regs = (volatile GRC_REGS_STRUCT *)
						(pilot_fg_base + GRC_BASE_OFFSET + GRC_REGS_STRUCT_OFFSET);

	/* Fill the buffer with the data */
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"FrameGrabber Registers\n");
	len += sprintf(buf+len,"Copyright (c) 2009-2015 American Megatrends Inc.\n\n");
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

	len += sprintf(buf +len, "BSCMD			= 0x%lx\n", fgb_bse_regs->BSCMD);
	len += sprintf(buf +len, "BSSTS			= 0x%lx\n", fgb_bse_regs->BSSTS);
	len += sprintf(buf +len, "BSDTB			= 0x%lx\n", fgb_bse_regs->BSDTB);
	len += sprintf(buf +len, "BSDBS			= 0x%lx\n", fgb_bse_regs->BSDBS);
	len += sprintf(buf +len, "BSBPS0		= 0x%lx\n", fgb_bse_regs->BSBPS0);
	len += sprintf(buf +len, "BSBPS1		= 0x%lx\n", fgb_bse_regs->BSBPS1);
	len += sprintf(buf +len, "BSBPS2		= 0x%lx\n", fgb_bse_regs->BSBPS2);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

	len += sprintf(buf +len, "Top TOPCTL           = 0x%lx\n", fgb_top_regs->TOPCTL);
	len += sprintf(buf +len, "Top TOPSTS           = 0x%lx\n", fgb_top_regs->TOPSTS);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");

	len += sprintf(buf +len, "Snoop TSCMD          = 0x%lx\n", fgb_tse_regs->TSCMD);
	len += sprintf(buf +len, "Snoop TSSTS          = 0x%lx\n", fgb_tse_regs->TSSTS);
	// Don't Read Row and Column Status because video redirection may be taking place
	// and these registers are autocleared by reading
	len += sprintf(buf +len, "Snoop TSTCSTS        = 0x%lx\n", fgb_tse_regs->TSTCSTS);
	len += sprintf(buf +len, "Snoop TSFBSA         = 0x%lx\n", fgb_tse_regs->TSFBSA);
	len += sprintf(buf +len, "Snoop TSICR          = 0x%lx\n", fgb_tse_regs->TSICR);
	len += sprintf(buf +len, "Snoop TSTMUL         = 0x%lx\n", fgb_tse_regs->TSTMUL);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf +len, "Tile Fetch TFCTL          = 0x%lx\n", fgb_tfe_regs->TFCTL);
	len += sprintf(buf +len, "Tile Fetch TFSTS          = 0x%lx\n", fgb_tfe_regs->TFSTS);
	len += sprintf(buf +len, "Tile Fetch TFSTS          = 0x%lx\n", fgb_tfe_regs->TFSTS);
	len += sprintf(buf +len, "Tile Fetch TFDTB          = 0x%lx\n", fgb_tfe_regs->TFDTB);
	len += sprintf(buf +len, "Tile Fetch TFCHK          = 0x%lx\n", fgb_tfe_regs->TFCHK);
	len += sprintf(buf +len, "Tile Fetch TFRBC          = 0x%lx\n", fgb_tfe_regs->TFRBC);
	
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Matrox Attribute Registers\n");
	len += sprintf(buf +len, "ATTR0\t = 0x%02x\tATTRC\t= 0x%02x\n", attr->ATTR0,attr->ATTRC);
	len += sprintf(buf +len, "ATTR1\t = 0x%02x\tATTRD\t= 0x%02x\n", attr->ATTR1,attr->ATTRD);
	len += sprintf(buf +len, "ATTR2\t = 0x%02x\tATTRE\t= 0x%02x\n", attr->ATTR2,attr->ATTRE);
	len += sprintf(buf +len, "ATTR3\t = 0x%02x\tATTRF\t= 0x%02x\n", attr->ATTR3,attr->ATTRF);
	len += sprintf(buf +len, "ATTR4\t = 0x%02x\tATTR10\t= 0x%02x\n", attr->ATTR4,attr->ATTR10);
	len += sprintf(buf +len, "ATTR5\t = 0x%02x\tATTR11\t= 0x%02x\n", attr->ATTR5,attr->ATTR11);
	len += sprintf(buf +len, "ATTR6\t = 0x%02x\tATTR12\t= 0x%02x\n", attr->ATTR6,attr->ATTR12);
	len += sprintf(buf +len, "ATTR7\t = 0x%02x\tATTR13\t= 0x%02x\n", attr->ATTR7,attr->ATTR13);
	len += sprintf(buf +len, "ATTR8\t = 0x%02x\tATTR14\t= 0x%02x\n", attr->ATTR8,attr->ATTR14);
	len += sprintf(buf +len, "ATTR9\t = 0x%02x\n", attr->ATTR9);
	len += sprintf(buf +len, "ATTRA\t = 0x%02x\n", attr->ATTRA);
	len += sprintf(buf +len, "ATTRB\t = 0x%02x\n", attr->ATTRB);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Matrox Graphics Control Registers\n");
	len += sprintf(buf +len, "GCTL0\t = 0x%02x\n", crtcext->GCTL0);
	len += sprintf(buf +len, "GCTL1\t = 0x%02x\n", crtcext->GCTL1);
	len += sprintf(buf +len, "GCTL2\t = 0x%02x\n", crtcext->GCTL2);
	len += sprintf(buf +len, "GCTL3\t = 0x%02x\n", crtcext->GCTL3);
	len += sprintf(buf +len, "GCTL4\t = 0x%02x\n", crtcext->GCTL4);
	len += sprintf(buf +len, "GCTL5\t = 0x%02x\n", crtcext->GCTL5);
	len += sprintf(buf +len, "GCTL6\t = 0x%02x\n", crtcext->GCTL6);
	len += sprintf(buf +len, "GCTL7\t = 0x%02x\n", crtcext->GCTL7);
	len += sprintf(buf +len, "GCTL8\t = 0x%02x\n", crtcext->GCTL8);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Matrox CRTC Registers\n");
	len += sprintf(buf +len, "CRTC0\t = 0x%02x\tCRTCE  = 0x%02x\n", crtc->CRTC0,crtc->CRTCE);
	len += sprintf(buf +len, "CRTC1\t = 0x%02x\tCRTCF  = 0x%02x\n", crtc->CRTC1,crtc->CRTCF);
	len += sprintf(buf +len, "CRTC2\t = 0x%02x\tCRTC10 = 0x%02x\n", crtc->CRTC2,crtc->CRTC10);
	len += sprintf(buf +len, "CRTC3\t = 0x%02x\tCRTC11 = 0x%02x\n", crtc->CRTC3,crtc->CRTC11);
	len += sprintf(buf +len, "CRTC4\t = 0x%02x\tCRTC12 = 0x%02x\n", crtc->CRTC4,crtc->CRTC12);
	len += sprintf(buf +len, "CRTC5\t = 0x%02x\tCRTC13 = 0x%02x\n", crtc->CRTC5,crtc->CRTC13);
	len += sprintf(buf +len, "CRTC6\t = 0x%02x\tCRTC14 = 0x%02x\n", crtc->CRTC6,crtc->CRTC14);
	len += sprintf(buf +len, "CRTC7\t = 0x%02x\tCRTC15 = 0x%02x\n", crtc->CRTC7,crtc->CRTC15);
	len += sprintf(buf +len, "CRTC8\t = 0x%02x\tCRTC16 = 0x%02x\n", crtc->CRTC8,crtc->CRTC16);
	len += sprintf(buf +len, "CRTC9\t = 0x%02x\tCRTC17 = 0x%02x\n", crtc->CRTC9,crtc->CRTC17);
	len += sprintf(buf +len, "CRTCA\t = 0x%02x\tCRTC18 = 0x%02x\n", crtc->CRTCA,crtc->CRTC18);
	len += sprintf(buf +len, "CRTCB\t = 0x%02x\tCRTC22 = 0x%02x\n", crtc->CRTCB,crtc->CRTC22);
	len += sprintf(buf +len, "CRTCC\t = 0x%02x\tCRTC24 = 0x%02x\n", crtc->CRTCC,crtc->CRTC24);
	len += sprintf(buf +len, "CRTCD\t = 0x%02x\tCRTC26 = 0x%02x\n", crtc->CRTCD,crtc->CRTC26);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Matrox CRTCEXT Registers\n");
	len += sprintf(buf +len, "CRTCEXT0\t = 0x%02x\n", crtcext->CRTCEXT0);
	len += sprintf(buf +len, "CRTCEXT1\t = 0x%02x\n", crtcext->CRTCEXT1);
	len += sprintf(buf +len, "CRTCEXT2\t = 0x%02x\n", crtcext->CRTCEXT2);
	len += sprintf(buf +len, "CRTCEXT3\t = 0x%02x\n", crtcext->CRTCEXT3);
	len += sprintf(buf +len, "CRTCEXT4\t = 0x%02x\n", crtcext->CRTCEXT4);
	len += sprintf(buf +len, "CRTCEXT5\t = 0x%02x\n", crtcext->CRTCEXT5);
	len += sprintf(buf +len, "CRTCEXT6\t = 0x%02x\n", crtcext->CRTCEXT6);
	len += sprintf(buf +len, "CRTCEXT7\t = 0x%02x\n", crtcext->CRTCEXT7);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Matrox SEQ Registers\n");
	len += sprintf(buf +len, "SEQ0\t = 0x%02x\n", crtcext->SEQ0);
	len += sprintf(buf +len, "SEQ1\t = 0x%02x\n", crtcext->SEQ1);
	len += sprintf(buf +len, "SEQ2\t = 0x%02x\n", crtcext->SEQ2);
	len += sprintf(buf +len, "SEQ3\t = 0x%02x\n", crtcext->SEQ3);
	len += sprintf(buf +len, "SEQ4\t = 0x%02x\n", crtcext->SEQ4);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf+len,"Matrox CURPOS Registers\n");
	len += sprintf(buf +len, "CURPOSXL\t = 0x%02x\n", grc_regs->CURPOSXL);
	len += sprintf(buf +len, "CURPOSXH\t = 0x%02x\n", grc_regs->CURPOSXH);
	len += sprintf(buf +len, "CURPOSYL\t = 0x%02x\n", grc_regs->CURPOSYL);
	len += sprintf(buf +len, "CURPOSYH\t = 0x%02x\n", grc_regs->CURPOSYH);
	len += sprintf(buf +len, "XCURCTL\t = 0x%02x\n", grc_regs->XCURCTL);
	len += sprintf(buf+len,"\n----------------------------------------------------------\n");
	len += sprintf(buf +len, "XZOOMCTRL\t = 0x%02x\n", grc_regs->XZOOMCTRL);
	len += sprintf(buf +len, "MISC\t = 0x%02x\n", grc_regs->MISC);
	len += sprintf(buf +len, "FEAT\t = 0x%02x\n", grc_regs->FEAT);
	len += sprintf(buf +len, "PIXRDMSK\t = 0x%02x\n", grc_regs->PIXRDMSK);
	len += sprintf(buf +len, "XMULCTRL\t = 0x%02x\n", grc_regs->XMULCTRL);
	len += sprintf(buf +len, "XGENCTRL\t = 0x%02x\n", grc_regs->XGENCTRL);
	len += sprintf(buf +len, "XMISCCTRL\t = 0x%02x\n", grc_regs->XMISCCTRL);
	len += sprintf(buf +len, "XCOLMSK\t = 0x%02x\n", grc_regs->XCOLMSK);
	len += sprintf(buf +len, "XCOLKEY\t = 0x%02x\n", grc_regs->XCOLKEY);

	temp = (unsigned long)&grc_regs->CURPOSXL ;
	len += sprintf(buf +len, "grc_regs POINTER = 0x%lx\n", temp);
	//len += sprintf(buf +len, "grc_regs POINTER = 0x%lx\n", (unsigned long)grc_regs);

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  *offset+=len;
#else
  /* Set End of File if no more data */
  *eof = 1;
#endif

	/* Return length of data */
	return len;
}
