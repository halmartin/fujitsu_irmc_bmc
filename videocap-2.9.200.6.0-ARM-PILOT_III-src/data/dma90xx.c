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
* @file 	cap90xx.c
* @authors	Vinesh Christoper 		<vineshc@ami.com>,
		Baskar Parthiban 		<bparthiban@ami.com>,
*		Varadachari Sudan Ayanam 	<varadacharia@ami.com>
* @brief   	9080 Fpga Capture screen logic core function
*			definitions
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

#include "cap90xx.h"
#include "videodata.h"
#include <linux/jiffies.h>
#include <linux/delay.h>
#include "dma90xx.h"

extern unsigned char g_low_bandwidth_mode;
void PrepareTFEDescriptor_Compress(unsigned short rect_width, unsigned short rect_Lines, unsigned short StrideinBytes, 
			 unsigned long src_addr, unsigned long dest_addr, unsigned long desc_addr,unsigned char Data_Process_Mode, 
			unsigned char DPM_StartByte, unsigned char DPM_SkipByte, unsigned char LastEntry )
{
	volatile Dma_Desc_ptr* tfe_descriptor;

	// Descriptor has to be set in Lmem
	tfe_descriptor  = (volatile Dma_Desc_ptr *)(desc_addr);
	if (!tfe_descriptor)
	{
		printk("TFE Descriptor is not proper. Cancelling the DMA\n");
		tfe_descriptor = NULL;
		fgb_tfe_regs->TFCTL = 0 ;
		return;
	}

	// Enable the TFE and Set the LMEM Source                                                                    
	fgb_tfe_regs->TFCTL |= (TFE_ENABLE | TFE_IRQ_ENABLE | 1 << 7);

	// If We are splitting the bytes, then set appropriate descriptor and params
	if (Data_Process_Mode & TFE_SPLIT_BYTES)
	{
		tfe_descriptor->Control=(TFE_MODE6_SPLITBYTES | (StrideinBytes<<16) | (DPM_StartByte << 10) | (DPM_SkipByte << 8));
	}
	else
	{
		tfe_descriptor->Control=(TFE_MODE0_NORMAL | (StrideinBytes<<16)); 
	}

	// If This is the last descriptor, then set descriptor accordingly.
	// Also enable the TFE Interrupt bit
	if (LastEntry)
	{
		tfe_descriptor->Control |= (TFE_LAST | TFE_I);
	}
	else
	{
		tfe_descriptor->Control |= TFE_NOTLAST;
	}

	// If RLE should be enabled, then set the params and enable RLE along with Overflow control
	if (Data_Process_Mode & TFE_RLE_EN)
	{
	        fgb_tfe_regs->TFCTL |= (0x55 << 16) | (0xAA << 24);
		tfe_descriptor->Control |= ((TFE_RLE_EN) | TFE_RLE_OVRFLW);
	}

	// Set the Descriptor params like Width, Height etc.
	tfe_descriptor->Width_Height_Reg=((rect_width-1) | ((rect_Lines-1)<<16)); // descriptor offset 0x4
	tfe_descriptor->Srcaddrs = src_addr;					 // descriptor offset 0x8
	tfe_descriptor->Dstaddrs = dest_addr;					 // descriptor offset 0xC
                                                                                    
	return;
}

void PrepareTFEDescriptor(unsigned short rect_width, unsigned short rect_Lines, unsigned short StrideinBytes, 
			 unsigned long src_addr, unsigned long dest_addr, unsigned long desc_addr,unsigned char Data_Process_Mode, unsigned char LastEntry )
{
	volatile Dma_Desc_ptr* tfe_descriptor;

	// Descriptor has to be set in Lmem
	if( desc_addr ){
		tfe_descriptor  = (volatile Dma_Desc_ptr *)(desc_addr);
		}
	else {
		tfe_descriptor  = (volatile Dma_Desc_ptr *)(descaddr);
		}

	if (!tfe_descriptor)
	{
		printk("TFE Descriptor is not proper. Cancelling the DMA\n");
		tfe_descriptor = NULL;
		fgb_tfe_regs->TFCTL = 0 ;
		return;
	}
                                                                    
	// destination and scr are all in DDR
	// only need to set stride in bytes and last entry
	if(current_video_mode.video_mode_flags & TEXT_MODE) {
		if(Data_Process_Mode & TFE_FETCH_ASCII) {
			fgb_tfe_regs->TFCTL = (TFE_ENABLE | TFE_IRQ_ENABLE | TFE_8_16_JUMP | 1 << 7) ;
			tfe_descriptor->Control=(TFE_MODE4_ASCIIONLY | (StrideinBytes<<16));   // descriptor offset 0
		}
		if(Data_Process_Mode & TFE_FETCH_ATTRASCII) { // fetch ascii and attr
			fgb_tfe_regs->TFCTL = (TFE_ENABLE | TFE_IRQ_ENABLE | TFE_8_16_JUMP | 1 << 7 ) ;
			tfe_descriptor->Control=(TFE_MODE3_ATTRASCII | (StrideinBytes<<16));   // descriptor offset 0
		}
		if(Data_Process_Mode & TFE_FETCH_FONT) {
			fgb_tfe_regs->TFCTL = (TFE_ENABLE | TFE_IRQ_ENABLE | 1 << 7)  ;
			tfe_descriptor->Control=(TFE_MODE5_FONTMODE | (StrideinBytes<<16));   // descriptor offset 0
		}
	}
	else { // bitmapped
		if(Data_Process_Mode & TFE_FETCH_8_16_JUMP) {
			fgb_tfe_regs->TFCTL = (TFE_ENABLE | TFE_IRQ_ENABLE | TFE_8_16_JUMP | 1 << 7) ;
		} 
		else {
			fgb_tfe_regs->TFCTL = (TFE_ENABLE | TFE_IRQ_ENABLE | 1 << 7);
		}
		if(Data_Process_Mode & TFE_FETCH_4BIT_PACKED) {
			tfe_descriptor->Control = (TFE_MODE2_4BITPKD | (StrideinBytes<<16));
		}
		else if (Data_Process_Mode & TFE_FETCH_4BIT_PLANAR) {
			tfe_descriptor->Control = (TFE_MODE1_4BITPLNR | (StrideinBytes<<16));
		}
		else {			
			//tfe_descriptor->Control=(0<<1 | 0<<13 | (StrideinBytes<<16)); 
			tfe_descriptor->Control = (TFE_MODE0_NORMAL | (StrideinBytes<<16)); //disable RLE at hardware-level
		}
	}

	if (LastEntry)
	{
		tfe_descriptor->Control |= (TFE_LAST | TFE_I);
	}
	else
	{
		tfe_descriptor->Control |= TFE_NOTLAST;
	}

	tfe_descriptor->Width_Height_Reg=((rect_width-1) | ((rect_Lines-1)<<16)); // descriptor offset 0x4
	tfe_descriptor->Srcaddrs = src_addr;					 // descriptor offset 0x8
	tfe_descriptor->Dstaddrs = dest_addr;					 // descriptor offset 0xC
                                                                                    
	return;
}

void tfe_dma(void)
{
	// Set up the descriptor base address in TFE 0x8h
	// TFE keys off this write to actually start
	fgb_tfe_regs->TFDTB =  (unsigned long)(LMEMSTART + LMEM_SSP_RESERVED_SIZE);

	WaitForTFE();

	// Disable TFE
	fgb_tfe_regs->TFCTL = 0 ;
	
	return;
}

void PrepareBSEDescriptor(unsigned short rect_width, unsigned short rect_Lines, unsigned short StrideinBytes, unsigned int bpp, 
			 unsigned long src_addr, unsigned long dest_addr, unsigned long desc_addr,unsigned char Data_Process_Mode, unsigned char LastEntry )
{
	volatile Dma_Desc_ptr *bse_descriptor = NULL;
	unsigned long lmem_bucket_size = 0;
	unsigned long dest_bucket_size = 0; 
	unsigned char dest_stride =0;
	unsigned char num_buckets = 0;

	// Descriptor has to be set in Lmem
	if( desc_addr )
	{
		bse_descriptor  = (volatile Dma_Desc_ptr *)(desc_addr);
	}

	if (!bse_descriptor)
	{
		printk("BSE Descriptor is not proper. Cancelling the DMA\n");
		bse_descriptor = NULL;
		fgb_bse_regs->BSCMD = 0;
		return;
	}

	if (g_low_bandwidth_mode == LOW_BANDWIDTH_MODE_NORMAL)
	{
		printk("Bandwidth mode is NORMAL. Cannot run through BSE\n");
		bse_descriptor = NULL;
		fgb_bse_regs->BSCMD = 0;
		return;
	}

	// This is the maximum value for this. Hence default to this, instead of calculating each time
	lmem_bucket_size = 0x80; 
	dest_bucket_size = ((rect_width/bpp) * rect_Lines) >> 3;
	num_buckets = (g_low_bandwidth_mode * 8) - 1;

	fgb_bse_regs->BSCMD = ((0x5010 << 16) | ((bpp-1) << 4) | (num_buckets << 8) | BSE_IRQ_ENABLE | BSE_ENABLE | BSE_DESC_LMEM);
	fgb_bse_regs->BSDBS = (lmem_bucket_size << 24) | dest_bucket_size;

	switch (bpp)
	{
		// 565 (16bpp) Converted to 323 (8bpp) i.e. 15 14 13 10 9 4 3 2
		case 2:
			if (g_low_bandwidth_mode == LOW_BANDWIDTH_MODE_8BPP)
			{
				fgb_bse_regs->BSBPS0 = 0x1AA49062;
				fgb_bse_regs->BSBPS1 = 0x1EE;
			}
					
			break;

		// 8888 (32bpp) or 888 (24bpp) Converted to 323 (8bpp) i.e. 23 22 21 15 14 7 6 5
		case 3:
		case 4:
			if (g_low_bandwidth_mode == LOW_BANDWIDTH_MODE_8BPP)
			{	
				fgb_bse_regs->BSBPS0 = 0x2AF71CC5;
				fgb_bse_regs->BSBPS1 = 0x2F6;
			}
			else if (g_low_bandwidth_mode == LOW_BANDWIDTH_MODE_16BPP)
			{
				fgb_bse_regs->BSBPS0 = 0x14731483;
				fgb_bse_regs->BSBPS1 = 0x26F7358B;
				fgb_bse_regs->BSBPS2 = 0x000BDAB4;
			}

			break;

		// Default option, in case bpp is none of the above
		default:	
			bse_descriptor = NULL;
			fgb_bse_regs->BSCMD = 0;
			return;
	}

	if (LastEntry)
	{
		bse_descriptor->Control = (BSE_LAST | BSE_ENABLE);
	}
	else
	{
		bse_descriptor->Control = (BSE_NOTLAST);
	}

	dest_stride = (rect_width/bpp) >> 3;
	bse_descriptor->Control |= ((StrideinBytes << 16) | (dest_stride << 8));

	bse_descriptor->Width_Height_Reg = ((rect_width - 1) | ((rect_Lines - 1) << 16)); // descriptor offset 0x4
	bse_descriptor->Srcaddrs = src_addr;					 // descriptor offset 0x8
	bse_descriptor->Dstaddrs = dest_addr;					 // descriptor offset 0xC
                                                                                
	return;
}

void bse_dma(void)
{
	// Set up the descriptor base address in BSE 0x8h
	// BSE keys off this write to actually start
	fgb_bse_regs->BSDTB = (unsigned long)(LMEMSTART + 0x5000);

	WaitForBSE();

	// Disable BSE
	fgb_bse_regs->BSCMD = 0;
	
	return;
}
