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
 *		Baskar Parthiban 		<bparthiban@ami.com>,
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


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <asm/delay.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include "cap90xx.h"
#include "dbgout.h"
#include "videocap.h"
#include "fge.h"
#include "capture.h"
#include "videodata.h"	
#include "profile.h"

#define MG9080_CONTROL_FLAGS		(                                                   \
		MG9080_CCR_ENABLE  | MG9080_ALOVICA_ENABLE		\
		)

// globals
unsigned char 						g_IsModeChange 	= FALSE;
volatile unsigned int					g_tse_sts = 0;
unsigned int						g_bpp15 = 0;
unsigned int						g_xcur_all = 1;
unsigned char						g_video_bandwidth_mode = LOW_BANDWIDTH_MODE_NORMAL;
unsigned char						g_low_bandwidth_mode = LOW_BANDWIDTH_MODE_NORMAL;

// locals
volatile static int					m_cap_ready = IS_IT_TSE;
volatile static int 					m_pal_ready = IS_IT_PAL;
volatile static int					m_xcur_ready = IS_IT_XCURPOS;
volatile static int					g_tfe_ready = 0;
volatile static int					g_bse_ready = 0;
volatile unsigned int					g_ScreenBlank = 0;

unsigned char tse_rr_base_copy[512];
unsigned long rowsts[2], colsts[2]; 
unsigned int AllRows = 0;
unsigned int AllCols = 0;

// local functions
static void copy_video_mode_params(VIDEO_MODE_PARAMS *src, VIDEO_MODE_PARAMS *dest);
static int compare_video_mode_params( VIDEO_MODE_PARAMS *vm1, VIDEO_MODE_PARAMS *vm2);
static void disable_bse_intr(void);
static void enable_tse_intr(void);
static void disable_tse_intr(void);
static void disable_tfe_intr(void);
static void enable_pal_intr(void);
static void disable_pal_intr(void);
static void enable_xcursor_intr(void);
static void disable_xcursor_intr(void);
static int check_blank_screen(void);
static int Get_TS_Map_Info(void);
static void check_for_new_session(unsigned int cap_flags);

/* Wait queue */
DECLARE_WAIT_QUEUE_HEAD(intr_wq);
DECLARE_WAIT_QUEUE_HEAD(pal_intr_wq);
DECLARE_WAIT_QUEUE_HEAD(tfe_intr_wq);
DECLARE_WAIT_QUEUE_HEAD(bse_intr_wq);
DECLARE_WAIT_QUEUE_HEAD(xcur_intr_wq);

/* 
   FGB interrupt handler  
   */
	irqreturn_t
pilot_fgb_intr(int irq, void *dev_id)
{
	volatile unsigned int grc_ctl = 0;
	volatile unsigned int grc_sts = 0;

	/*
	 * BIT SLICE ENGINE INTERRUPT HANDLING SECTION
	 */
	if (fgb_bse_regs->BSCMD & BSE_IRQ_ENABLE)
	{
		if (fgb_bse_regs->BSSTS & BSE_IRQ_ENABLE)
		{
			disable_bse_intr();

			g_bse_ready = IS_IT_BSE;
			wake_up_interruptible(&bse_intr_wq);
		}
	}

	/* 
	 * TILE FETCH ENGINE INTERRUPT HANDLING SECTION
	 */
	if (fgb_tfe_regs->TFCTL & TFE_IRQ_ENABLE)
	{
		if (fgb_tfe_regs->TFSTS & TFE_IRQ_ENABLE)
		{
			disable_tfe_intr();

			g_tfe_ready = IS_IT_TFE;
			wake_up_interruptible(&tfe_intr_wq);
		}
	}

	/* 
	 * PALETTE & XCURSOR INTERRUPT HANDLING SECTION
	 */
	grc_ctl = grc_ctl_regs->GRCCTL1;
	if (grc_ctl & (GRC_INTR | XCURSOR_INTR)) 
	{
		grc_sts = grc_ctl_regs->GRCSTS;
		
		// This is the case for Palette
		if (grc_sts & GRC_INTR) 
		{
			disable_pal_intr();

			m_pal_ready = IS_IT_PAL;
			wake_up_interruptible(&pal_intr_wq);
		}

		// This is the case for Hw Cursor
		if (grc_sts & XCURSOR_INTR)
		{
			disable_xcursor_intr();

			if (grc_sts & XCURPOS_INTR)
			{
				m_xcur_ready = IS_IT_XCURPOS;
			}
			
			if (grc_sts & XCURCTL_INTR)
			{
				m_xcur_ready = IS_IT_XCURCTL;
			}

			if (grc_sts & XCURCOL_INTR)
			{
				m_xcur_ready = IS_IT_XCURCOL;
			}

			wake_up_interruptible(&xcur_intr_wq);
		}
	}

	/*
	 * TSE INTERRUPT HANDLING SECTION
	 */
	if (fgb_tse_regs->TSCMD & TSE_IRQ_ENABLE) 
	{
		if (fgb_top_regs->TOPSTS & 0x0000000f) 
		{
			disable_tse_intr();
			g_tse_sts = fgb_tse_regs->TSSTS;

			if (g_tse_sts & 0x00000001) 
			{
				fgb_tse_regs->TSSTS |= (1 << 0);
			}

			if (g_tse_sts & 0x00000002) 
			{
				fgb_tse_regs->TSSTS |= (1 << 1);
			}

			m_cap_ready = IS_IT_TSE;
			wake_up_interruptible(&intr_wq);
		}
	}


	return IRQ_HANDLED;
}

void disable_bse_intr()
{
	fgb_bse_regs->BSCMD &= ~(BSE_IRQ_ENABLE);
	fgb_bse_regs->BSSTS |= BSE_IRQ_ENABLE;
}

void disable_tfe_intr()
{
	fgb_tfe_regs->TFCTL &= ~(TFE_IRQ_ENABLE);
	fgb_tfe_regs->TFSTS |= TFE_IRQ_ENABLE;
}

void enable_tse_intr() 
{
	fgb_tse_regs->TSCMD |= TSE_IRQ_ENABLE;
}

void disable_tse_intr() 
{
	fgb_tse_regs->TSCMD &= (~TSE_IRQ_ENABLE);	
}

void enable_pal_intr()
{
	grc_ctl_regs->GRCSTS |= GRC_INTR;
	grc_ctl_regs->GRCCTL1 |= GRC_INTR;
}

void disable_pal_intr()
{
	grc_ctl_regs->GRCCTL1 &= ~(GRC_INTR);
}

void enable_xcursor_intr()
{
	grc_ctl_regs->GRCCTL1 |= XCURSOR_INTR;
}

void disable_xcursor_intr()
{
	grc_ctl_regs->GRCCTL1 &= ~(XCURSOR_INTR);
} 

void get_current_palette(u8 *pal_address)
{
	unsigned char *palette ;

	palette = (unsigned char *)(pilot_fg_base + GRC_BASE_OFFSET) ;

	/**
	 * Verifying for the access of the user-space area thats passed here
	 * Copying the palette data from the pilot-fg base to the allocated region
	 **/
	if ( access_ok(VERIFY_WRITE, (char *)pal_address, 1024) )
	{
		if ( copy_to_user((char *)pal_address, (char *)palette, 1024) )
		{
			printk("Cannot copy palette data to the user end\n");
		}
	}	
}

/* This fn is for Waiting and capturing the Palette
 * 1.  Enabling the Interrupt and Waiting until interrupt arrives and makes the condition TRUE
 * 2.  if condition is made TRUE, then call the Palette handling function
 */
int capture_palt(unsigned int cap_flags, u8 *palt_address)
{
	int lcl_ready;
	//int status;

	/* Enable Pal Interrupt and then wait indefinitely until a Palette interrupt arrives */
	/* First time alone, palette will be obtained unconditionally. Then on, only interrupt based */
	enable_pal_intr();

	/**
	 * Going to wait for an interrupt here
	 * Setting appropriate flags
	 **/
	wait_event_interruptible(pal_intr_wq, (m_pal_ready == IS_IT_PAL));
	lcl_ready = m_pal_ready;
	m_pal_ready = 0;

	if (lcl_ready != IS_IT_PAL)
	{
		return lcl_ready;
	}

	/**
	 * Going to get the current Palette 
	 * Passing the user-space address that we allocated in adviser
	 * Will be copying the palette data from pilot-fg to the allocated area
	 * This was done to avoid the junk color displayed occasionally
	 **/
	get_current_palette(palt_address);

	return lcl_ready;
}


/**
 * capture_screen
 * Tile Column Size support. Because fractional number of columns are difficult to deal with,
 * the following restrictions apply to the valid vesa modes.
 * Horizontal Resolution	Column Size supported
 * 640			16
 * 720			16
 * 800			16-32
 * 848			16
 * 1024			All
 * 1152			All
 * 1280			All
 * 1360			16
 * 1400			Not Supported
 * 1440			16-32
 * 1600			16-32-64
 * Get current screen resolution and if changed, reinit snoop engine. and return
 * Here flip ownership of screen.
 * Read Tile Row and Column registers and Process.
 * Do Tile Fetch
 * Get current screen resolution and if changed, discard data and reinit snoop engine
 **/

int capture_screen(unsigned int cap_flags,capture_param_t* cap_param)
{
//	int lcl_ready = 0;
	int status;

	enable_tse_intr();

	/**
	 * This condition occurs whenever a new session comes in
	 * Hence, for New Sessions alone, we are directly fetching the data
	 **/
	check_for_new_session(cap_flags);

	wait_event_interruptible_timeout(intr_wq, (m_cap_ready == IS_IT_TSE), (5 * HZ));
//	lcl_ready = m_cap_ready;
	m_cap_ready = 0;

	get_video_mode_params(&before_video_capture);

	if (cap_flags & CAPTURE_NEW_SESSION)
	{
		copy_video_mode_params(&before_video_capture, &current_video_mode) ; // make current
		copy_video_mode_params(&before_video_capture, &status_video_mode);
	}

	cap_flags &= ~(CAPTURE_NEW_SESSION);

	if(compare_video_mode_params(&before_video_capture, &status_video_mode)) 
	{
		g_IsModeChange = TRUE ;
		copy_video_mode_params(&before_video_capture, &current_video_mode) ; // make current
		copy_video_mode_params(&before_video_capture, &status_video_mode);
		current_video_mode.captured_byte_count = 0 ;
		current_video_mode.capture_status = MODE_CHANGE ;
		return 1 ;
	}
	copy_video_mode_params(&before_video_capture, &status_video_mode);
	copy_video_mode_params(&before_video_capture, &current_video_mode) ; // make current
	g_IsModeChange = FALSE;
	
	if ( check_blank_screen() )
	{
		return CAPTURE_ERROR;
	}

	//Max supported resolution width is 1920 for 8 and 16 BPP only 
	//If condition is not satisfied Just return "novideo" to avoid video driver crash 
	if(((current_video_mode.width >= MAX_SUPPORTED_WIDTH) && ((current_video_mode.depth != BPP_8) && (current_video_mode.depth != BPP_16)))
		|| (current_video_mode.width > MAX_SUPPORTED_WIDTH))
	{
		current_video_mode.capture_status = NO_VIDEO ;
		return CAPTURE_ERROR;
	}

	if (cap_flags & CAPTURE_CLEAR_BUFFER) 
	{
		if (current_video_mode.width <= 1) 
		{ 
			// possible hard reset of PILOTII
			current_video_mode.capture_status = NO_VIDEO ;
			return CAPTURE_ERROR;
		}
	} 

	// get column info
	AllRows = (current_video_mode.height / TileColumnSize);
	AllCols = (current_video_mode.width / TileRowSize);

	if ((current_video_mode.height % TileColumnSize) > 0) AllRows += 1;
	if ((current_video_mode.width % TileRowSize) > 0) AllCols += 1;

	if (Get_TS_Map_Info()) 
	{
		return CAPTURE_ERROR;
	}

	if(current_video_mode.video_mode_flags & TEXT_MODE) {		
		status = capture_text_screen(cap_flags);
	} 
	else if(current_video_mode.video_mode_flags & MODE13) {	
		status = capture_mode13_screen(cap_flags);
	}
	else {
		status = capture_tile_screen(cap_flags);
	}

	get_video_mode_params(&after_video_capture);
	if(compare_video_mode_params(&before_video_capture, &after_video_capture)) 
	{
		g_IsModeChange = TRUE ;
		copy_video_mode_params(&after_video_capture, &current_video_mode) ; // make current
		current_video_mode.captured_byte_count = 0 ;
		current_video_mode.capture_status = MODE_CHANGE ;
		return 1 ;
	}

	if ( check_blank_screen() )
	{
		return CAPTURE_ERROR;
	}

	if (CAPTURE_HANDLED != status) 
	{
		return status;
	} 

	current_video_mode.capture_status = 0 ;

	return CAPTURE_HANDLED;
}

void check_for_new_session(unsigned int cap_flags)
{
	if ( cap_flags & CAPTURE_NEW_SESSION )
	{
		m_cap_ready = IS_IT_TSE;
		m_pal_ready = IS_IT_PAL;
		m_xcur_ready = IS_IT_XCURCTL;

		g_xcur_all = 1;
	}

	return;
}

int Get_TS_Map_Info(void)
{
	u8 byTSCMDBytesPerPixel = 0;
	u8 bySnoopBit;
	u8 byBytesPerPixels = 0;
	u16 wStride = 0;
	u32 TSFBSA = 0;
	u32 color_depth = 0;// convert bbp value(1,2,3,4) to 8, 16, 24, 32.

	if((current_video_mode.video_mode_flags & TEXT_MODE) || (current_video_mode.video_mode_flags & MODE13) ){
		fgb_tse_regs->TSCMD = 0x1; // Bit 0(1): TSE enabled; Bit 1(0): VGAText mode	
		current_video_mode.text_snoop_status = fgb_tse_regs->TSSTS ; // clear 
		fgb_tse_regs->TSTMUL = (1024 * 1024);
		return 0;
	}


	byBytesPerPixels = current_video_mode.depth;   
	switch ( byBytesPerPixels ) {
		case 1: byTSCMDBytesPerPixel = 0;
			break;
		case 2:
		case 3: // for tiles capture only
			byTSCMDBytesPerPixel = 1;
			break;
		case 4: byTSCMDBytesPerPixel = 2;
			break;
	}

	// Capture 3BytesPP as 4BytesPP
	if( byBytesPerPixels == 3 ) {
		// for capture tile rectangle we capture as 32bpp
		//wStride = (current_video_mode.stride * 3 )>>2;
		// for capture tiles only we capture as 2bpp
		wStride = (current_video_mode.stride * 3 ) >> byTSCMDBytesPerPixel;

		if ((wStride >> 5) > 64)
		{
			byTSCMDBytesPerPixel = 2;
			wStride = (current_video_mode.stride * 3) >> byTSCMDBytesPerPixel;
		}
	}
	else {
		wStride = current_video_mode.stride;
	}

	/* EIP 317646.
	* ref from pilot4SDK2016_1130 or later.
	* file: hardwareEngines.c line:427.
	* update value stored in TSFBSA register in TSE.
	*/
	
	TSFBSA = current_video_mode.video_buffer_offset;
	switch(byBytesPerPixels)
	{
		case 1: // 8bbp.
			TSFBSA <<= 3;
			color_depth = 8;
			break;

		case 2: // 16bbp.
			TSFBSA <<=2;
			color_depth = 16; 
			break;

		case 3: // 24bbp.
			TSFBSA <<= 3;
			color_depth = 24;
			break;
		case 4: // 32bbp.
			TSFBSA <<= 1;
			color_depth = 32;
			break;
	}
	TSFBSA *= (color_depth >> 3);
	if(TSFBSA != fgb_tse_regs->TSFBSA)
	{
		fgb_tse_regs->TSFBSA = TSFBSA;
	}

	// set upper limit based on screen
	fgb_tse_regs->TSTMUL = 0x500000;

	// enable TSE
	fgb_tse_regs->TSCMD |= TILE_SNOOP_ENABLE;

	// enable TSE interrupt
	fgb_tse_regs->TSCMD |= TSE_IRQ_ENABLE;

	// flip the bit
	if( fgb_tse_regs->TSCMD & 0x00008000 )
		bySnoopBit = 0;
	else
		bySnoopBit = 1; 

	// set up TSE

	if ( bpp4_mode == 0x12){
		current_video_mode.stride = 640;
		wStride = 320;
		fgb_tse_regs->TSCMD = (wStride<< 16) | (bySnoopBit << 15) | (1<<6) | (1<<4) |(1<<2) | (0x3);		
	}
	else if(bpp4_mode == 0x2F){// 2f
		current_video_mode.stride = 800;
		wStride = 400;
		fgb_tse_regs->TSCMD = (wStride<< 16) | (bySnoopBit << 15) | (1<<6) | (1<<4)|( 1<<2) | (0x3);		
	}
	else {
		fgb_tse_regs->TSCMD = (wStride << 16) | (bySnoopBit << 15) | (1<<6) | (1<<4) |( byTSCMDBytesPerPixel<<2) | (0x3);
	}

	// after snoop engine is done

	// for Rectangle capture
	rowsts[0] = fgb_tse_regs->TSRSTS0;
	rowsts[1] = fgb_tse_regs->TSRSTS1;
	colsts[0] = fgb_tse_regs->TSCSTS0;
	colsts[1] = fgb_tse_regs->TSCSTS1;

	// for tiles only capture
	// yet another quick work around
	if ((byTSCMDBytesPerPixel == 2) && (byBytesPerPixels == 3))
	{}
	else {
		memcpy((char *)&tse_rr_base_copy[0], (char *)fgb_tse_rr_base, 512);
	}

	if ((byBytesPerPixels == 3) && (byTSCMDBytesPerPixel == 1)) { 
		Convert2Bppto3Bpp((unsigned long *)&tse_rr_base_copy[0], ((current_video_mode.stride * 3) >> byTSCMDBytesPerPixel), current_video_mode.vdispend);
	}	

	return 0;
}



void get_video_mode_params(VIDEO_MODE_PARAMS *vm)
{
	unsigned short temp16 ;
	unsigned char temp8 ;
	unsigned long video_buffer_offset = 0 ;

	// video flags
	vm->video_mode_flags = 0 ;
	if((crtcext->CRTCEXT3) & REG_BIT7) {
		vm->video_mode_flags |= MGA_MODE ;
	}

	if((vm->video_mode_flags & MGA_MODE) == 0) {
		if(((crtcext->GCTL6) & REG_BIT0) == 0) {
			vm->video_mode_flags |= TEXT_MODE ;
		}
	}
	if(((crtcext->GCTL5) & REG_BIT6) == 0) {
		vm->video_mode_flags |= TWO56_COLOR ;
	}

	// get bytes per pixel 
	// vm->depth  
	vm->depth = get_bpp(*XMULCTRL & 0x7);
	//vm->depth = *XMULCTRL & 0x7 ;

	// get height
	temp8 = crtc->CRTC12 ;
	temp16 = (unsigned short)temp8 ;
	if(crtc->CRTC7 & REG_BIT1)
		temp16 |= REG_BIT8 ;
	if(crtc->CRTC7 & REG_BIT6)
		temp16 |= REG_BIT9 ;
	if(crtcext->CRTCEXT2 & REG_BIT2)
		temp16 |= REG_BIT10 ;
	vm->height = (temp16 + 1) ; // height in lines
	current_video_mode.vdispend = vm->height;
	/***
	//SLES OS 800*600 75Hz return dowuuble the height size,n
	 * Matrox has pointed out that when determining the screen height conv2t4( CRTC09 bit 7 ) should also be checked.
		After calculating the screen height, if conv2t4 is 0 then it is correct and if it is 1 then you will have to divide the height by 2.
	 * 
	 */
	if(crtc->CRTC9 & REG_BIT7)
	{
		current_video_mode.vdispend = vm->height/2;
	}

	// get width
	temp8 = crtc->CRTC1 ;
	temp16 = (unsigned short)temp8 ;
	if((vm->video_mode_flags) & MGA_MODE) {
		/**
		 * This FrameWidth is calculated using the CRTC1 register which is for 'hdispend' 
		 * This hdispend will give the exact 'Displayed' pixels per line.
		 **/
		vm->width = ((temp16 + 1) * 8) ; // width in pixels
		current_video_mode.hdispend = vm->width;

		/**
		 * This calculates the Frame Width based on the 'Offset' param in the Matrox spec.
		 * This will give the total pixels per line.
		 * Hence we would have to refer with the 'hdispend' param for displaying the 
		 * exact number of pixels each line. 
		 **/
		//calc_resol(vm);
		if (crtc->CRTC11 & 0x80) {
			current_video_mode.hdispend = vm->width;
			current_video_mode.video_mode_flags |= NON_MATROX;
		}
	} 
	else { // VGA Mode 
		if(vm->video_mode_flags & TEXT_MODE) {
			vm->width = crtc->CRTC13 ; // 
			vm->width |= (crtcext->CRTCEXT0 & 0x30) << 4 ;
			vm->width *= 2;
			current_video_mode.hdispend = vm->width;
		} 
		else { // graphics mode
			if(crtc->CRTC9 == 0x41) { // CRTC9, ATTR10, GCTL5 are unique in Mode 13
				vm->width = 320 ;
				vm->height = 200 ;
				vm->video_mode_flags |= MODE13 ;
				if (crtc->CRTC11 & 0x80) {
					current_video_mode.hdispend = vm->width;
				}
			}
			else if ((vm->depth == 5) && (crtc->CRTC17 == 0xE3)) {
				if (bpp4_mode == 0x12) {
					vm->width = vm->hdispend = 640;
					vm->height = vm->vdispend = 480;
					current_video_mode.width = current_video_mode.hdispend = 640;
					current_video_mode.height = current_video_mode.vdispend = 480;
				}
				else if (bpp4_mode == 0x2F) {
					vm->width = vm->hdispend = 800;
					vm->height = vm->vdispend = 600;
					current_video_mode.width = current_video_mode.hdispend = 800;
					current_video_mode.height = current_video_mode.vdispend = 600;
				}
				vm->video_mode_flags |= MGA_MODE;	
			}  
			else {
				vm->width = ((temp16 + 1) * 8) ; // width in pixels
			}
		}
	}

	// get stride
	temp16 = (unsigned short)(crtc->CRTC13);
	if (crtcext->CRTCEXT0 & 0x10)
		temp16 |= REG_BIT8;
	if (crtcext->CRTCEXT0 & 0x20)
		temp16 |= REG_BIT9;
	if ( (vm->depth != 0xFF) && (temp16 != 0) ) {
		vm->stride = ( (temp16 * 128) / (vm->depth * 8) );
	}
	else
		vm->stride = vm->width;


	vm->char_height = (crtc->CRTC9 & 0x1F) + 1 ;

	video_buffer_offset = crtc->CRTCD ;
	video_buffer_offset |= (crtc->CRTCC << 8) ;
	video_buffer_offset |= (crtcext->CRTCEXT0 & 0xf) << 16 ;
	if(crtcext->CRTCEXT0 & 0x40) {
		video_buffer_offset |= 0x100000 ;
	}

	vm->video_buffer_offset = video_buffer_offset ;

}

unsigned char get_bpp ( unsigned char XMulCtrl)
{
	unsigned char bpp = 0xFF;

	bpp = 0xFF ;
	switch(XMulCtrl) {
		case 0x0: 
			if (crtc->CRTC17 == 0xE3) {
				bpp = 5; // 4bpp

				if (crtc->CRTC3 == 0x82) 
					bpp4_mode = 0x12;
				else if (crtc->CRTC3 == 0x80)
					bpp4_mode = 0x2F;
			}
			else 
			{
				bpp4_mode = 0x00;
				bpp = 1; // 8bpp
			}
			break;

		case 0x1: // 15 bpp
			/**
			 * 15bpp was set the same case as 16bpp
			 * The RGB model for 15bpp is different from 16bpp in client end.
			 * Had to set the same bpp = 2 value for 15bpp as well for capturing purposes
			 * Hence, have set a flag to indicate 15bpp. If true, then sending flag to client
			 * Client will set the RGB model according to flag
			 **/
			bpp = 2;
			g_bpp15 = 1;
			bpp4_mode = 0x00;
			break;

		case 0x2: // sixteen bits
			bpp = 2;
			bpp4_mode = 0x00;
			break;

		case 0x3: // 24Bpp - now supported
			bpp = 3;
			bpp4_mode = 0x00;
			break;

		case 0x4: // 32Bpp
			bpp = 4;
			bpp4_mode = 0x00;
			break;

		case 0x5:
		case 0x6: // these are reserved codes
			bpp = 0xFF;
			bpp4_mode = 0x00;
			break;
		case 0x7:
			bpp = 4;
			bpp4_mode = 0x00;
			break;
	}
	return bpp;
}


static int compare_video_mode_params( VIDEO_MODE_PARAMS *vm1, VIDEO_MODE_PARAMS *vm2)
{
	if(vm1->video_mode_flags != vm2->video_mode_flags) {
		return 1 ;
	}
	if(vm1->width != vm2->width) {
		return 1;
	}
	if(vm1->height != vm2->height) {
		return 1;
	}
	if(vm1->depth != vm2->depth) {
		return 1;
	}
	if(vm1->char_height != vm2->char_height) {
		return 1;
	}
	if(vm1->video_buffer_offset != vm2->video_buffer_offset) {
		copy_video_mode_params(vm1, vm2) ; // make current			
	}
	return 0 ;
}

static void copy_video_mode_params( VIDEO_MODE_PARAMS *src,  VIDEO_MODE_PARAMS *dest)
{
	dest->width = src->width ;
	dest->height = src->height ;
	dest->depth = src->depth ;
	dest->video_mode_flags = src->video_mode_flags ;
	dest->columns_per_tile = src->columns_per_tile ;
	dest->rows_per_tile = src->rows_per_tile ;
	dest->char_height = src->char_height ;
	dest->video_buffer_offset = src->video_buffer_offset ;
	dest->stride = src->stride ;
}


/**
 * This fn will re-initialize the driver capture thread
 * The capture thread will be waiting for an interrupt
 * Whenever we receive a resume call from JViewer
 * This will wake up the interruptible_wait call
 **/
void resume_video(void)
{
	// Resume the video thread
	if (m_pal_ready != IS_IT_PAL)
	{
		m_pal_ready = IS_IT_PAL;
		wake_up_interruptible(&pal_intr_wq);
	}

	// Resume the palette thread
	if (m_cap_ready != IS_IT_TSE)
	{
		m_cap_ready = IS_IT_TSE;
		wake_up_interruptible(&intr_wq);
	}
}

/**
 * Checks for a flag
 * If Host is ON, then turns it OFF, else vice versa
**/
void control_display(unsigned char *lockstatus)
{
	unsigned long g_lock_status=0;
	unsigned long display_cntrl_tmp;
	unsigned char lockcmd = *lockstatus;
	// If lockstatus is 1, then lock the monitor else unlock the monitor
	
	if (lockcmd == 1)
	{
		// Turns off the host display
		*display_cntrl |= (CRTC_CYCLE_DISABLE | LCL_DISP_DISABLE);
	}
	else if(lockcmd == 0)
	{
		// Turns on the host display
		*display_cntrl &= ~(CRTC_CYCLE_DISABLE | LCL_DISP_DISABLE);
	}
	
	display_cntrl_tmp = *display_cntrl;
	g_lock_status = display_cntrl_tmp & (CRTC_CYCLE_DISABLE | LCL_DISP_DISABLE);
	if(g_lock_status == 0)
	{
		*lockstatus = 0x04;
	}
	else
	{
		*lockstatus = 0x05;
	}
	
	return;	
}

int get_bandwidth_mode(void)
{
	return g_video_bandwidth_mode;
}

void set_bandwidth_mode(unsigned char Bandwidth_Mode)
{
	// Set the global flag to indicate the bandwidth mode
	// 0 = Normal mode
	// 1 = Low Bandwidth 8bpp
	// 2 = Low Bandwidth 16bpp
	// 3 = Low Bandwidth 8bpp (B&W)
	g_video_bandwidth_mode = g_low_bandwidth_mode = Bandwidth_Mode;

	// 3 is 8bpp B&W.  Hence, we set as 1 to indicate 8bpp capture
	if (g_video_bandwidth_mode == LOW_BANDWIDTH_MODE_8BPP_BW)
		g_low_bandwidth_mode = LOW_BANDWIDTH_MODE_8BPP;

	// If the current screen BPP is same as our low bandwidth mode setting, then we reset it to normal mode
	if ((current_video_mode.depth == g_low_bandwidth_mode) || (g_low_bandwidth_mode == 0))
		g_video_bandwidth_mode = g_low_bandwidth_mode = LOW_BANDWIDTH_MODE_NORMAL;

	current_video_mode.bandwidth_mode = g_video_bandwidth_mode;
	
	return;
}

#define FTS_PCI_HIDE_MASK       0x080019a2              //FTSCHG_NZ Jan 31, 2013 change for VGA disable 

/* 
 * This call will return whether the Pilot-III PCI Card will be visible to host or not
 * The PCI Vendor ID and PCI Device ID will be masked to hide the Pilot-III PCI from host
 * These IDs will be checked to identify and return if the card is available or not
*/
int get_card_presence(void)
{
	//if (*card_presence_cntrl == P3_PCI_HIDE_MASK)
	if (*card_presence_cntrl == FTS_PCI_HIDE_MASK)  // FTSCHG_NZ Jan 31, 2013
	{
		return 0;
	}
	else
	{
		return 1;
	}		
}

void control_card_presence(unsigned char CardPresence)
{
	volatile unsigned long *PIDB0_Reg;      // Pilot Debugregister
	unsigned short PIDBG0_offset = 0x54;

	// Check for CardPresence flag and act accordingly
	// If CardPresence is 1, then enable the Card else Hide the card's PCI IDs
	if (CardPresence)
	{
		// Sets the actual Device ID & Vendor ID for P3 PCI
		*card_presence_cntrl = ((P3_PCI_DEVICE_ID << 16) | (P3_PCI_VENDOR_ID));

		PIDB0_Reg = (volatile unsigned long *)(sysclk_base + PIDBG0_offset);
//              TCRIT("--->NZ enable: PIDB0_Reg %08x: %08x\n",(UINT32)PIDB0_Reg, (UINT32) *PIDB0_Reg);
		*PIDB0_Reg = (UINT32) *PIDB0_Reg & ~(REG_BIT8); //clear Bit 8
//              TCRIT("--->NZ enable: PIDB0_Reg %08x: %08x\n",(UINT32)PIDB0_Reg, (UINT32) *PIDB0_Reg); 
	}
	else
	{
		// Hides the P3 PCI by resetting the PCI Device ID & Vendor ID with 0xFFFFFFFF
		// NZ_FTS: we can not hide the VGA device in this way, than we also
		// hide PCIE Function1
		// To solve this problem, we fake the VGA device for the OS to be
		// a "non compatible VGA device".
		//
//              *card_presence_cntrl = (P3_PCI_HIDE_MASK);      // FTSCHG_NZ Jan 31, 2013
		*card_presence_cntrl = (FTS_PCI_HIDE_MASK);     // FTSCHG_NZ Jan 31, 2013 

		PIDB0_Reg = (volatile unsigned long *)(sysclk_base + PIDBG0_offset);
		*PIDB0_Reg = (UINT32) *PIDB0_Reg | REG_BIT8; //set Bit 8
		TCRIT("--->NZ disable: PIDB0_Reg %08x: %08x\n",(UINT32)PIDB0_Reg, (UINT32) *PIDB0_Reg);
	}

	return;
} 

void set_pci_subsystem_Id(PCI_DEVID pcisubsysID)
{

      *PCISID_regs = ( (pcisubsysID.devID <<16) | (pcisubsysID.venID));

	return;
} 

unsigned long get_pci_subsystem_Id(void)
{
    return(*PCISID_regs);
}

void WaitForBSE()
{
	// Waits for BSE DMA Event to complete
	wait_event_interruptible_timeout(bse_intr_wq, (g_bse_ready == IS_IT_BSE), (100*HZ));
	g_bse_ready = 0;

	return;
}

void WaitForTFE()
{
	//unsigned int status = 0;

	// Waits for TFE DMA Event to complete : Waiting time is: 250 ms
	wait_event_interruptible_timeout(tfe_intr_wq, (g_tfe_ready == IS_IT_TFE), (100*HZ));
        g_tfe_ready = 0;

	return;
}

/**
 * Convert2Bppto3Bpp
 * for 24bpp capture, we capture it as a 2Bpp screen using TFE
 * Convert that 2Bpp data into 3Bpp data using this call.
 **/
void Convert2Bppto3Bpp(unsigned long *BaseSnoopMap, unsigned long Stride, unsigned long Height)
{
	unsigned int iLAllCols = 0;
	unsigned int iLAllRows = 0;
	unsigned int Row, Col;
	unsigned char SnoopMapBkup[512];
	unsigned char *CopyofSnoopMap;
	unsigned long *SnoopMapRow = NULL;
	unsigned char bySnoopBitMap = 0x00;
	unsigned char byIter = 0;
	unsigned long Bits = 63;
	unsigned char bySnoopBit;
	unsigned char ToggleSnoopBits = 0;

	/**
	iLAllCols = Stride >> 5;
	iLAllRows = Height >> 5;

	if (Height == 600)
		iLAllRows += 1;**/
	
	/** fixes **/
	/*Fix done by Emulex for RHEL black box Issue*/
	
	iLAllCols = (Stride+31) >> 5; // dwStride/32
	iLAllRows = (Height+31) >>5;
	iLAllCols = 3*((iLAllCols+2)/3);
	
	memset(SnoopMapBkup, 0x00, 512);

	for (Row = 0; Row < iLAllRows ; Row++)
	{
		CopyofSnoopMap = SnoopMapBkup;
		CopyofSnoopMap += (Row * 8);
		bySnoopBitMap = 0x00;
		byIter = 0;

		for (Col = 0; Col < iLAllCols ; Col++)
		{
			switch (Col)
			{
				case 0:
					SnoopMapRow = (BaseSnoopMap + (Row * 2));
					Bits = 31;
					break;
				case 32:
					SnoopMapRow = (BaseSnoopMap + ((Row * 2) + 1));
					Bits = 31;
					break;
			}
			bySnoopBit = (unsigned char)((*SnoopMapRow) << Bits >> 31);

			switch (ToggleSnoopBits)
			{
				case 0:
					ToggleSnoopBits = 1;
					if (bySnoopBit)
						bySnoopBitMap = 0x01;
					break;
				case 1:
					ToggleSnoopBits = 2;
					if (bySnoopBit)
						bySnoopBitMap |= 0x03;
					break;
				case 2:
					ToggleSnoopBits = 0;
					if (bySnoopBit)
						bySnoopBitMap |= 0x02;
					*CopyofSnoopMap |= (bySnoopBitMap << byIter);
					byIter += 2;
					if (byIter == 8)
					{
						CopyofSnoopMap++;
						byIter = 0x00;
					}
					bySnoopBitMap = 0x00;
					break;
			}
			Bits--;
		}
	}
	memcpy(BaseSnoopMap, SnoopMapBkup, 512);

	return;
}

/**
 * This fn will check for blank screen condition
 * The Combo of SEQ1 & CRTCEXT1 register is for BlankScreen and PowerSave conditions
 * The Combo of TSSTS and XMULCTRL is for PowerOFF and PowerON status conditions
 * The Combo of SEQ4, CRTC24, CRTC26 and TSSTS is for Power control conditions
 * This is based on the register updations done by the mga driver in Linux OS.
 * Also, the register conditions is different under windows when normally powered off and on and when powered off and on with standby state enabled.
 **/
static int check_blank_screen(void)
{
	if ( (((crtcext->SEQ1) & SEQ1_POWOFF) || ((crtcext->CRTCEXT1) & SEQ1_BLNKSCRN))
			|| (((!(fgb_tse_regs->TSSTS)) && ((*XMULCTRL & 0x7) == 0x7)))
			//|| ((!((crtc->CRTC24) & 0x80)) && (!((crtc->CRTC26) & 0x20)) && ((crtcext->SEQ4) & 0x04))
			|| ((!((crtc->CRTC24) & 0x80)) && (!((crtc->CRTC26) & 0x20)) && ((fgb_tse_regs->TSSTS) <= 0x00A)) )   

	{
		g_IsModeChange = TRUE;
		current_video_mode.capture_status = MODE_CHANGE;
		g_ScreenBlank = 0;

		return 1;
	}
	return 0;
}	

/**
 * capture_xcur - wait for xcursor interrupt
 */

int capture_xcursor(xcursor_param_t *xcursor)
{
	int status;
	unsigned int lcl_status = 0;

	/* Enable XCursor Interrupt and then wait indefinitely until a XCursor interrupt arrives */
	/* First time alone, XCursor will be obtained unconditionally. Then on, only interrupt based */
	enable_xcursor_intr();

	if (xcursor->xcursor_mode == XCURSOR_ALL)
	{
		m_xcur_ready = IS_IT_XCURPOS;
		g_xcur_all = 1;
	}

	/**
	 * Going to wait for an interrupt here
	 * Setting appropriate flags
	**/
	status = wait_event_interruptible_timeout(xcur_intr_wq, (m_xcur_ready != 0), (1 * HZ));
	if (status == 0)
	{
		xcursor->xcursor_mode = XCURSOR_NONE;
		return status;
	}
	else
	{
		lcl_status = m_xcur_ready;
		m_xcur_ready = 0;
	}

	/* get hardware cursor info */
	status = get_xcursor_info(xcursor, lcl_status);
	
	return status;
}

/**
 * get_hwcursor_info - get hardware cursor infomation
 * Read cursor mode, positions, maps data and colors
 */

int get_xcursor_info(xcursor_param_t *xcursor, unsigned int lcl_status)
{
	unsigned char colIndex = 0;
	volatile unsigned long *xcur_maps_base;
	xcursor_data_t *xcur_data = &(xcursor->xcursor_data);

	memset(xcursor, 0, sizeof(xcursor_param_t));

	switch (lcl_status)
	{
		case IS_IT_XCURPOS:
			xcursor->xcursor_mode = XCURSOR_POSCTL;
			break;
		case IS_IT_XCURCOL:
			xcursor->xcursor_mode = XCURSOR_ALL;
			break;
		case IS_IT_XCURCTL:
			xcursor->xcursor_mode = XCURSOR_CTL;
			break;
	}

	if (g_xcur_all)
	{
		xcursor->xcursor_mode = XCURSOR_ALL;
		g_xcur_all = 0;
	}
	
	/* get cursor mode */
	xcur_data->xcurpos.mode = grc_regs->XCURCTL;

	/* if the cursor is diabled, no need to send cursor info */
	if (xcur_data->xcurpos.mode == XCURSOR_MODE_DISABLED) {
		return 0;
	}

	/* get current cursor position */
	xcur_data->xcurpos.pos_x = grc_regs->CURPOSXL | (grc_regs->CURPOSXH << 8);
	xcur_data->xcurpos.pos_y = grc_regs->CURPOSYL | (grc_regs->CURPOSYH << 8);

	/* copy hw cursor buffer data from frame buffer */
	xcur_maps_base = (unsigned long *) (((grc_regs->XCURADDL << 10) | 
				((grc_regs->XCURADDH & 0x3F) << 18)) + framebuffer_base);
	/* Sometimes XCURADDH and XCURADDL crosses the 8mb buffer limit, 
	   to avoid kernel crash added the below condition. 
	   ToDo : Need to find and fix why the xcur_maps_base crosses the buffer limit. */ 
	if ( (xcur_data->xcurpos.mode == XCURSOR_MODE_16COLOR) && 
		 (((unsigned long int)(xcur_maps_base + (sizeof(unsigned long long) * XCURSOR_MAP_SIZE))) < 
			((unsigned long int)(framebuffer_base + FRAMEBUFFER_SIZE))) ) {
		/* total slice is 6 for 16 color mode */
		if (copy_to_user((void *) &xcur_data->xcurcol.maps, (void *) xcur_maps_base, 
			sizeof(unsigned long long) * XCURSOR_MAP_SIZE)) {
			printk("videocap: cannot copy xcursor map data to user end\n");
		}
	}
	else if ( ((unsigned long int)(xcur_maps_base + (sizeof(unsigned long long) * 128))) < 
			  ((unsigned long int)(framebuffer_base + FRAMEBUFFER_SIZE)) ) {
		/* total slice is 2 for other mode; each slice 64bits */
		if (copy_to_user((void *) &xcur_data->xcurcol.maps, (void *) xcur_maps_base, 
			sizeof(unsigned long long) * 128)) {
			printk("videocap: cannot copy xcursor map data to user end\n");
		}
	}

	/* read hw cursor color register */
	xcur_data->xcurcol.color[colIndex].red = grc_regs->XCURCOL0RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL0GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL0BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL1RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL1GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL1BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL2RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL2GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL2BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL3RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL3GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL3BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL4RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL4GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL4BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL5RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL5GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL5BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL6RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL6GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL6BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL7RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL7GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL7BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL8RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL8GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL8BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL9RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL9GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL9BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL10RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL10GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL10BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL11RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL11GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL11BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL12RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL12GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL12BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL13RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL13GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL13BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL14RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL14GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL14BLUE;
	xcur_data->xcurcol.color[++colIndex].red = grc_regs->XCURCOL15RED;
	xcur_data->xcurcol.color[colIndex].green = grc_regs->XCURCOL15GREEN;
	xcur_data->xcurcol.color[colIndex].blue = grc_regs->XCURCOL15BLUE;

	return 0;
}



