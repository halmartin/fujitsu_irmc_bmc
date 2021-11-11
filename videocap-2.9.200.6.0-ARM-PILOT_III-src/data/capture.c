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
 * @authors	Bakka Ravinder Reddy	<bakkar@ami.com
 * @brief   	Capture functions for different modes
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

#include "capture.h"
#include "dma90xx.h"
#include "videodata.h"	
#include "cap90xx.h"
#include "fge.h"

static int capture_tiles_only(unsigned short tm_cnt); 
static unsigned short get_tile_map(void);
static unsigned int Compress(unsigned int xpect_offset, unsigned short tm_cnt);

static unsigned char TextBuffOffsetArray[8] = {1, 3, 5, 7, 2, 4, 6, 8};
extern unsigned char g_low_bandwidth_mode;
extern unsigned char g_video_bandwidth_mode;
DWORD GetPhyFBStartAddress( void ){
	return  (fgb_tse_regs->TSFBSA + FRAMEBUFFER);
}

int capture_text_screen(unsigned int cap_flags) 
{
	unsigned long tfe_dest_addr;
	unsigned char TextBufferOffset;

	current_video_mode.text_snoop_status = g_tse_sts; 
	current_video_mode.captured_byte_count = 0 ;

	if (cap_flags & CAPTURE_CLEAR_BUFFER) 
	{
		// indicate ascii, attr, and fonts have been modified
		current_video_mode.text_snoop_status = 0x1C;
	}

	if ((current_video_mode.text_snoop_status & 0x1C) == 0)
	{ 
		// no screen change
		current_video_mode.captured_byte_count = 0 ; 
		current_video_mode.capture_status = NO_SCREEN_CHANGE;
		return CAPTURE_NO_CHANGE;
	}

	tfe_dest_addr = tfedest_bus ;
	if (current_video_mode.text_snoop_status & ATTR_FIELD_MODIFIED)
	{
		// must fetch both ascii & attr
		PrepareTFEDescriptor(current_video_mode.width * 16,current_video_mode.height / current_video_mode.char_height, 
				current_video_mode.width * 16, FRAMEBUFFER + (current_video_mode.video_buffer_offset * 16), 
				tfe_dest_addr, 0, TFE_FETCH_ATTRASCII, 1);
		tfe_dma();

		current_video_mode.captured_byte_count += (current_video_mode.width * (current_video_mode.height / current_video_mode.char_height)) * 2 ;
		tfe_dest_addr += (current_video_mode.width * (current_video_mode.height / current_video_mode.char_height)) * 2 ;

	} else if (current_video_mode.text_snoop_status & ASCII_FIELD_MODIFIED)
	{ 
		// fetch ascii only
		PrepareTFEDescriptor(current_video_mode.width * 16,current_video_mode.height / current_video_mode.char_height, 
				current_video_mode.width * 16, FRAMEBUFFER + (current_video_mode.video_buffer_offset * 16), 
				tfe_dest_addr, 0, TFE_FETCH_ASCII, 1);
		tfe_dma();

		current_video_mode.captured_byte_count += current_video_mode.width * (current_video_mode.height / current_video_mode.char_height) ;
		tfe_dest_addr += current_video_mode.width * (current_video_mode.height / current_video_mode.char_height) ;
	}

	if (current_video_mode.text_snoop_status & FONT_FIELD_MODIFIED)
	{
		PrepareTFEDescriptor(current_video_mode.width * 16, current_video_mode.height / current_video_mode.char_height + 256, 
			current_video_mode.width * 16, FRAMEBUFFER, tfe_dest_addr, 0, TFE_FETCH_FONT, 1);
		tfe_dma();

		current_video_mode.captured_byte_count += (8 * 8192) ;
		TextBufferOffset = ( ((crtcext->SEQ3 & 0xF0) >> 3) | ((crtcext->SEQ3 & 0x0F) >> 2) );
		current_video_mode.TextBuffOffset = TextBuffOffsetArray[(int)(TextBufferOffset)]; 
	}

	current_video_mode.capture_status = 0 ;

	return CAPTURE_HANDLED;
}

/**
 * capture_mode12_screen :MODE 12 and MODE 2F
 * capture screen in mode 12
 */
int capture_mode12_screen(unsigned int cap_flags)
{
	unsigned char bpp;

	// Capture the entire screen for now
	// Need to use TSE later
	current_video_mode.left_x = 0 ; 
	current_video_mode.top_y = 0 ;
	current_video_mode.right_x = AllCols  ;
	current_video_mode.bottom_y = AllRows ;

	bpp = current_video_mode.depth;
	if (bpp == 0xFF) 
	{
		current_video_mode.captured_byte_count = 0 ;
		current_video_mode.capture_status = BAD_COLOR_DEPTH;
		return CAPTURE_ERROR; // can't handle 
	}

	if (((cap_flags & CAPTURE_CLEAR_BUFFER) == 0) &&
			((current_video_mode.text_snoop_status & 0x1C) == 0))
	{ 
		// no screen change
		current_video_mode.captured_byte_count = 0 ; 
		current_video_mode.capture_status = NO_SCREEN_CHANGE;
		return CAPTURE_NO_CHANGE;
	}

	// Fetch entire screen
	PrepareTFEDescriptor(current_video_mode.width, current_video_mode.height, current_video_mode.stride , 
				FRAMEBUFFER, tfedest_bus, 0, TFE_FETCH_4BIT_PLANAR, 1);
	tfe_dma();

	// 2 pixels per byte
	current_video_mode.captured_byte_count = (current_video_mode.width * current_video_mode.height) / 2; 
	current_video_mode.capture_status = 0;

	return CAPTURE_HANDLED;
}

/**
 * capture_tile_screen
 * capture screen in mode13
 */
int capture_mode13_screen(unsigned int cap_flags) 
{ 
	// Fetch entire screen
	current_video_mode.left_x = 0 ; 
	current_video_mode.top_y = 0 ;
	current_video_mode.right_x = AllCols  ;
	current_video_mode.bottom_y = AllRows ;

	if (((cap_flags & CAPTURE_CLEAR_BUFFER) == 0) &&
			((current_video_mode.text_snoop_status & 0x1C) == 0))
	{ 
		// no screen change
		current_video_mode.captured_byte_count = 0; 
		current_video_mode.capture_status = NO_SCREEN_CHANGE;
		return CAPTURE_NO_CHANGE;
	}
	PrepareTFEDescriptor((320 * 32)/4, 200, (320 * 32)/4 , FRAMEBUFFER, tfedest_bus, 0, 
							TFE_FETCH_8_16_JUMP | TFE_FETCH_4BIT_PACKED, 1);
	tfe_dma();

	PrepareTFEDescriptor((320 * 8)/4, 200, (320 * 8)/4 , tfedest_bus, tfedest_bus, 0, TFE_FETCH_4BIT_PACKED, 1);
	tfe_dma();

	current_video_mode.captured_byte_count = 320 * 200;
	current_video_mode.capture_status = 0;

	return CAPTURE_HANDLED;
}

/**
 * capture_tile_screen
 * capture modified tiles in vesa mode. 
 */
int capture_tile_screen(unsigned int cap_flags)
{
	unsigned short tm_cnt;   

	if(current_video_mode.depth == (0xFF)) { 
		current_video_mode.captured_byte_count = 0 ;
		current_video_mode.capture_status = BAD_COLOR_DEPTH ;
		return CAPTURE_ERROR ; 
	}

	/**
	 * In some of the cases, when matrox driver is not loaded in linux host
	 * The width calculating register ends up with a 0 stored in it, and hence width = 0
	 * This was causing a error / hang when doing a TFE_DMA for capturing the video
	 * Handled this by checking for the width = 0 condition.
	 **/
	if (current_video_mode.width == 0 || current_video_mode.height == 0) {
		current_video_mode.captured_byte_count = 0;
		current_video_mode.capture_status = NO_SCREEN_CHANGE;
		return CAPTURE_NO_CHANGE;
	}

	if (cap_flags & CAPTURE_CLEAR_BUFFER)	{
		memset((char *)&tse_rr_base_copy[0], 0xff, 512);
		rowsts[0] = 0xffffffff;
		rowsts[1] = 0xffffffff;
		colsts[0] = 0xffffffff;
		colsts[1] = 0xffffffff;
		if (current_video_mode.depth == 5) {
			return capture_mode12_screen(cap_flags);
		}	 
	}

	// quick work around
	if( current_video_mode.depth == 5 ){
		if(rowsts[0] == 0x00){
			current_video_mode.captured_byte_count = 0;
			current_video_mode.capture_status = NO_SCREEN_CHANGE;
			return CAPTURE_NO_CHANGE;
		}
		else {
			memset((char *)&tse_rr_base_copy[0], 0xff, 4);
			rowsts[0] |= 0x1;
		}
	}

	tm_cnt = get_tile_map();

	if (tm_cnt == 0) {
		// no screen changes. 
		current_video_mode.captured_byte_count = 0;
		current_video_mode.capture_status = NO_SCREEN_CHANGE;
		return CAPTURE_NO_CHANGE;
	} 
	current_video_mode.tile_cap_mode = TILE_CAP_TILES_ONLY;	

	return capture_tiles_only(tm_cnt);
}

static int capture_tiles_only( unsigned short tm_cnt) 
{
	unsigned short tfwidth, tfheight, tfframewidth;
	unsigned long tfsrcaddr;
	unsigned long tfdescaddr = descaddr;
	unsigned int tile_count = 0, fb_offset = 0;
	unsigned int hdr_offset = 0, xpect_offset = 0;
	unsigned char *dest = (unsigned char *)(tfedest_base);
	unsigned long dest_bus;
	unsigned char bpp = current_video_mode.depth;
	tm_entry_t *tm_entry;
	unsigned int OutDataSize = 0;

	if ((bpp4_mode == 0x12) || (bpp4_mode == 0x2F))
	{
		dest = (unsigned char *)(tfedest_base);
		dest_bus = tfedest_bus;
	}
	else
	{
		dest = (unsigned char *)(tmpbuffer_base);
		dest_bus = tmpbuffer_bus;
	}

	// First byte of destination Buffer = total tiles modified = tm_cnt
	*((unsigned short *)(dest + hdr_offset)) = tm_cnt;
	hdr_offset += sizeof(unsigned short);

	// Calculate the Xpected header size and align it to 4byte boundary
	xpect_offset = (hdr_offset + (tm_cnt * sizeof(tm_entry_t)));
	if ((xpect_offset % 4) != 0) {
		xpect_offset += (4 - (xpect_offset % 4));
	}

	// Set the video data offset to the xpected header offset
	fb_offset = xpect_offset;

	// If Host bpp is 8bpp, then we cannot downscale any further
	// If host is 16bpp, and bandwidth mode is also set to 16bpp, then capture as normal.
	if (bpp == g_low_bandwidth_mode)
	{
		g_video_bandwidth_mode = g_low_bandwidth_mode = LOW_BANDWIDTH_MODE_NORMAL;
	}
	
	for (tile_count = 0; tile_count < tm_cnt; tile_count++) 
	{
		tm_entry = (tile_map + tile_count);
		
		// Copy the header data to the destination Buffer
		*((unsigned char *)(dest + hdr_offset++)) = tm_entry->row;
		*((unsigned char *)(dest + hdr_offset++)) = tm_entry->col;
	
		// copy tile data to destination buffer
		tfframewidth = current_video_mode.stride;

		/* This case is ONLY for 4bpp case. */
		/* If mode is 0x12 and 0x2F,640x480x4bpp and 800x600x4bpp case resp. */
		if ((bpp4_mode == 0x12) || (bpp4_mode == 0x2F))
		{
			tfwidth = TileColumnSize << 1;
			tfheight = TileRowSize;
			tfsrcaddr = GetPhyFBStartAddress() + ((current_video_mode.stride>>1) * tm_entry->row * (TileRowSize <<1)) 
				+ (tm_entry->col * (TileColumnSize <<1));

			// Chain the descriptors till the last modified tile.
			// Last argument in PrepareTFEDescriptor(), if set to 1, stops the Chain.
			if ( tile_count == (tm_cnt - 1) )
			{
				PrepareTFEDescriptor(tfwidth, tfheight, tfframewidth, tfsrcaddr, dest_bus + fb_offset, 
											tfdescaddr, TFE_FETCH_4BIT_PLANAR, 1);
			}
			else
			{
				PrepareTFEDescriptor(tfwidth, tfheight, tfframewidth, tfsrcaddr, dest_bus + fb_offset, 
											tfdescaddr, TFE_FETCH_4BIT_PLANAR, 0);
			}

			fb_offset += (TileRowSize * TileColumnSize);
		}
		else
		{
			tfwidth = TileColumnSize * bpp;
			tfheight = TileRowSize;
			tfframewidth *= bpp;
			tfsrcaddr = GetPhyFBStartAddress() + (tm_entry->col * TileColumnSize * bpp)
				+ (tm_entry->row * TileRowSize * current_video_mode.stride * bpp);

			if (g_low_bandwidth_mode > LOW_BANDWIDTH_MODE_NORMAL)
			{  
				//if descaddr has already shifted for SSP reserved size, minus the size to avoid exceeding allocated size
				tfdescaddr = descaddr + 0x5000 - LMEM_SSP_RESERVED_SIZE; 

				PrepareBSEDescriptor(tfwidth, tfheight, tfframewidth, bpp, tfsrcaddr, dest_bus + fb_offset, tfdescaddr, 0, 1);
				bse_dma();

				fb_offset += (TileRowSize * TileColumnSize * g_low_bandwidth_mode);

				continue;
			}	
			
			//if descaddr has already shifted for SSP reserved size, minus the size to avoid exceeding allocated size
			if ( tfdescaddr == (descaddr + 0x7FF0 - LMEM_SSP_RESERVED_SIZE) )
			{
				PrepareTFEDescriptor(tfwidth, tfheight, tfframewidth, tfsrcaddr, dest_bus + fb_offset, tfdescaddr, 0, 1);
				tfe_dma();
				tfdescaddr = descaddr;
				fb_offset += (tfwidth * tfheight);
				continue;
			}

			// Chain the descriptors till the last modified tile.
			// Last argument in PrepareTFEDescriptor(), if set to 1, stops the Chain.
			if ( tile_count == (tm_cnt - 1) )
			{
				PrepareTFEDescriptor(tfwidth, tfheight, tfframewidth, tfsrcaddr, dest_bus + fb_offset, tfdescaddr, 0, 1);
			}
			else
			{	
				PrepareTFEDescriptor(tfwidth, tfheight, tfframewidth, tfsrcaddr, dest_bus + fb_offset, tfdescaddr, 0, 0);
			}

			fb_offset += (tfwidth * tfheight);
		}

		// Increment the TFE Descriptor address in offset 0x10 to chain the descriptors.
		tfdescaddr += sizeof(Dma_Desc_ptr);
	}// for

	current_video_mode.act_depth = current_video_mode.depth;
	current_video_mode.bandwidth_mode = g_video_bandwidth_mode;

	// This single DMA call with perform for all the chained descriptors
	if (g_low_bandwidth_mode == LOW_BANDWIDTH_MODE_NORMAL)
	{
		tfe_dma();
	}
	else
	{
		current_video_mode.depth = g_low_bandwidth_mode;
	}

	current_video_mode.captured_byte_count = fb_offset;

	if ((bpp4_mode == 0x12) || (bpp4_mode == 0x2F))
	{
		return CAPTURE_HANDLED;
	}
	
	OutDataSize = Compress(xpect_offset, tm_cnt);
	if (OutDataSize == 0)
	{
		memcpy(tfedest_base, tmpbuffer_base, fb_offset);
		current_video_mode.captured_byte_count = fb_offset;
		current_video_mode.capture_status = BAD_COMPRESSION;
		return CAPTURE_ERROR;
	}	

	memcpy(tfedest_base, tmpbuffer_base, xpect_offset);
	current_video_mode.captured_byte_count = (unsigned long)(OutDataSize + xpect_offset);

	return CAPTURE_HANDLED;
}

static unsigned int Compress(unsigned int fb_offset, unsigned short tm_cnt)
{
	unsigned short tfwidth, tfheight, tfframewidth;
	unsigned long tfsrcaddr;
	unsigned long tfdescaddr;
	int InDataSize = 0, OutDataSize = 0;
	unsigned char bpp = current_video_mode.depth;
	unsigned char DPM_StartByte = 0, DPM_SkipByte = 0;
	unsigned char Index = 0;
	unsigned int Components = 0;
	unsigned int data_offset = 0;
	
	InDataSize = (current_video_mode.captured_byte_count - fb_offset);
	tfwidth = (TileColumnSize * TileRowSize) * (unsigned int)bpp;
	tfheight = tm_cnt;
	tfframewidth = tfwidth;
	tfsrcaddr = (unsigned long)(tmpbuffer_bus + fb_offset);
	Components = (InDataSize / bpp);

	if (bpp == 0x01)
	{
		tfdescaddr = descaddr;
		goto SkipSplit;
	}

	data_offset = fb_offset;
	DPM_StartByte = 0x00;
	DPM_SkipByte = (unsigned char)(bpp - 1);
	tfdescaddr = descaddr;

	for (Index = 0; Index < bpp; Index++)
	{
		PrepareTFEDescriptor_Compress(tfwidth, tfheight, tfframewidth, tfsrcaddr, tfedest_bus + data_offset, 
							tfdescaddr, TFE_SPLIT_BYTES, DPM_StartByte, DPM_SkipByte, 0);
		DPM_StartByte++;
		data_offset += Components;	
		tfdescaddr += 0x10;
	}

	tfsrcaddr = (unsigned long)(tfedest_bus + fb_offset);

SkipSplit:
	DPM_StartByte = 0x00;
	DPM_SkipByte = 0x00;
	fgb_tfe_regs->RLELMT = InDataSize;
	PrepareTFEDescriptor_Compress(tfwidth, tfheight, tfframewidth, tfsrcaddr, tfedest_bus + fb_offset, 
							tfdescaddr, TFE_RLE_EN, DPM_StartByte, DPM_SkipByte, 1);

	tfe_dma();

	OutDataSize = (fgb_tfe_regs->TFRBC);
	tfdescaddr = descaddr;

	if (OutDataSize > InDataSize)
	{
		OutDataSize = 0;
	}

	return OutDataSize;
}

/**
 * get_tile_map
 * construct tile map for current screen.
 * tile map resides in tile_map
 */
static unsigned short get_tile_map()
{
	unsigned long *rowreg0 = NULL;
	unsigned int i,j;
	unsigned char tse_local_rr_base[512];
	unsigned long *rr_base = (unsigned long *)&tse_local_rr_base[0];
	unsigned int  row_shift;
	unsigned int  col_shift;
	unsigned long Row_Status;
	unsigned long col_Status;
	unsigned int bits = 63;
	unsigned char snoopbit;
	tm_entry_t *tm_entry = NULL;
	unsigned int tile_cnt = 0;

	// Backup the Snoop Map 
	memcpy((char *)&tse_local_rr_base[0], (char *)&tse_rr_base_copy[0], 512);

	/* Get the Max No. of Rows for each screen. */
	/* Check for the Bit_Set in each register 0 and 1 respectively */
	/* If a bit is set, then check in a similar way for Columns */
	/* Update the TileMap with its respective parameters */

	for (i=0; i<AllRows;i++)
	{
		if (i > 31)
		{
			Row_Status = rowsts[1];
			row_shift = i - 32;
		} 
		else
		{
			Row_Status = rowsts[0];
			row_shift = i;
		}

		if ((Row_Status >> row_shift) & 0x1)
		{
			for (j=0; j<AllCols; j++)
			{
				switch(j)
				{
					case 0: 
						rowreg0 = rr_base + (i * 2);
						bits = 31;
						break;
					case 32:
						rowreg0 = rr_base + ((i*2) + 1);
						bits = 31;
						break;
				}
				snoopbit = (unsigned char)((*rowreg0) << bits >> 31);

				// A Bit is set for a particular Row & Column. Updating the Tile Map 
				if (snoopbit)
				{
					tm_entry = (tile_map + tile_cnt);
					tm_entry->row = i;
					tm_entry->col = j;
					tile_cnt += 1;
				}
				else
				{
					if (j > 31)
					{
						col_Status = colsts[1];
						col_shift = j - 32;
					} 
					else
					{
						col_Status = colsts[0];
						col_shift = j;
					}
					
					if ((col_Status >> col_shift) & 0x1)
					{
						tm_entry = (tile_map + tile_cnt);
						tm_entry->row = i;
						tm_entry->col = j;
						tile_cnt += 1;
					}
				}
				bits--;
			} //j,AllCols
		}//row_status, row_shift
	}//i, Max_Rows						


	return tile_cnt;
}
