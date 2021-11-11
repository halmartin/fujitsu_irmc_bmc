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

#ifndef AMI_DMA90XX_H
#define AMI_DMA90XX_H

void PrepareBSEDescriptor(unsigned short rect_width, unsigned short rect_height, unsigned short frame_width, unsigned int bpp,
			unsigned long src_addr, unsigned long dest_addr, unsigned long desc_addr, unsigned char flags, unsigned char LastEntry);
void PrepareTFEDescriptor(unsigned short rect_width, unsigned short rect_height, unsigned short frame_width,
			unsigned long src_addr, unsigned long dest_addr, unsigned long desc_addr, unsigned char flags, unsigned char LastEntry);
void PrepareTFEDescriptor_Compress(unsigned short rect_width, unsigned short rect_height, unsigned short frame_width,
			unsigned long src_addr, unsigned long dest_addr, unsigned long desc_addr, unsigned char flags, 
			unsigned char DPM_StartByte, unsigned char DPM_SkipByte, unsigned char LastEntry);
void tfe_dma(void);
void bse_dma(void);

#endif // AMI_DMA90XX_H
