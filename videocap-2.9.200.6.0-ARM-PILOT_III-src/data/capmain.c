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
* @file   	capmain.c
* @author 	Varadachari Sudan Ayanam <varadacharia@ami.com>
* @brief  	capture driver main file
****************************************************************/

/* Video Capture Module Version */
#define VIDEOCAP_MAJOR	1
#define VIDEOCAP_MINOR	1	/* 0 for 2.4 and 1 for 2.6 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <asm/uaccess.h>	/* copyin and copyout   */
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/bigphysarea.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <linux/interrupt.h>


#include "helper.h"			/* symbols from helper module */
#include "dbgout.h"

#include "videocap.h"
#include "iohndlr.h"
#include "ioctl.h"
#include "proc.h"
#include "mmap.h"
#include "cap90xx.h"
#include "profile.h"

#define DEFINE_VIDEODATA	1

/* Function Prototypes */
static int videocap_open (struct inode *inode, struct file *file);
static int videocap_release (struct inode *inode, struct file *file);

/* Data Structures */
#include "videodata.h"

extern irqreturn_t pilot_fgb_intr(int irq, void *dev_id);

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
struct proc_info *statisticsproc;
struct proc_info *statusproc;
struct proc_info *infoproc;
struct proc_info *regproc;
struct proc_info *paletteproc;
struct proc_info *videoproc;
struct proc_info *tfebuffproc;
struct proc_info *deccolproc;
struct proc_info *decrowproc;
struct proc_info *decmaxproc;
#endif

/* Sysctl Table */
static struct ctl_table VideoTable [] =
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
	{1,"DebugLevel",&(TDBG_FORM_VAR_NAME(videocap)),sizeof(int),0644,NULL,NULL,&proc_dointvec},
	{2,"TileColumnSize",&TileColumnSize,sizeof(unsigned long),0644,NULL,NULL,&proc_doulongvec_minmax},
	{3,"TileRowSize",&TileRowSize,sizeof(unsigned long),0644,NULL,NULL,&proc_doulongvec_minmax},
	{4,"MaxRectangles",&MaxRectangles,sizeof(unsigned long),0644,NULL,NULL,&proc_doulongvec_minmax},
#else
	{"DebugLevel",&(TDBG_FORM_VAR_NAME(videocap)),sizeof(int),0644,NULL,&proc_dointvec},
	{"TileColumnSize",&TileColumnSize,sizeof(unsigned long),0644,NULL,&proc_doulongvec_minmax},
	{"TileRowSize",&TileRowSize,sizeof(unsigned long),0644,NULL,&proc_doulongvec_minmax},
	{"MaxRectangles",&MaxRectangles,sizeof(unsigned long),0644,NULL,&proc_doulongvec_minmax},
#endif
	{0}
};

/* Proc and Sysctl entries */
static struct proc_dir_entry *moduledir = NULL;
static struct ctl_table_header *my_sys 	= NULL;


/* Driver Information Table */
static struct file_operations videocap_fops =
{
	owner:		THIS_MODULE,
#ifdef USE_UNLOCKED_IOCTL
	unlocked_ioctl:		videocap_ioctl,
#else
	ioctl:		videocap_ioctl,
#endif
	open:		videocap_open,
	release:	videocap_release,
	mmap:		videocap_mmap,
};


static int opencount = 0;


static
int
videocap_open (struct inode *inode, struct file *file)
{
	// MOD_INC_USE_COUNT;
	/* First Open, Initialize everything */
	if (opencount == 0) {
		// just clears some unused variables
		ReInitializeDriver(); 
	}

	opencount++;
	return 0;
}

static
int
videocap_release (struct inode *inode, struct file *file)
{
	// MOD_DEC_USE_COUNT;
	opencount --;

	/* Last Release. Clear EveryThing */
	if (opencount == 0) {
		ReInitializeDriver();
	}

	return 0;
}


static
int
AllocateResources (void)
{
	unsigned long addr;
	unsigned long size;
	
	// framebuffer_base is used by cpu related accesses
	framebuffer_base = (unsigned long)ioremap(FRAMEBUFFER, FRAMEBUFFER_SIZE);
	pilot_fg_base = (unsigned long)ioremap(0x40900000, 0x00A00000);
	tfe_descriptor_base = (unsigned long)ioremap((LMEMSTART + LMEM_SSP_RESERVED_SIZE), (LMEM_SIZE - LMEM_SSP_RESERVED_SIZE));
	descaddr = (unsigned long)(tfe_descriptor_base);
	sysclk_base = (unsigned long)ioremap(SYS_CLK_CNTRL, 0x00000200);

	size = BIGPHYS_SIZE;
	tfedest_base = NULL;
	tfedest_base = bigphysarea_alloc_pages(size/PAGE_SIZE, 0, GFP_KERNEL);
	
        if (tfedest_base) 
	{
            tfedest_bus = virt_to_phys(tfedest_base);
            addr = (unsigned long)tfedest_base;
            while (size > 0) 
	    {
                SetPageReserved(virt_to_page(addr));
                addr += PAGE_SIZE;
                size -= PAGE_SIZE;
	    }
	memset(tfedest_base, 0x00, size);
	} else {
	    TCRIT("Could not allocate the Physical memory pool for Videocap\n");
	    return 1;
	}

	size = BIGPHYS_SIZE;
	tmpbuffer_base = NULL;
	tmpbuffer_base = bigphysarea_alloc_pages(size/PAGE_SIZE, 0, GFP_KERNEL);
	
	
        if (tmpbuffer_base) 
	{
            tmpbuffer_bus = virt_to_phys(tmpbuffer_base);
            addr = (unsigned long)tmpbuffer_base;
            while (size > 0) 
	    {
                SetPageReserved(virt_to_page(addr));
                addr += PAGE_SIZE;
                size -= PAGE_SIZE;
	    }
	memset(tmpbuffer_base, 0x00, size);
	} else {
	    TCRIT("Could not allocate the Physical memory pool for Videocap\n");
	    return 1;
	}

	/* Initialize the Register Locations */
	fgb_top_regs = (volatile FGB_TOP_REG_STRUCT *)(pilot_fg_base);
	fgb_tse_regs = (volatile TSE_REG_STRUCT *)(pilot_fg_base + 0x400);
	fgb_tfe_regs = (volatile TFE_REG_STRUCT *)(pilot_fg_base + 0x100);
	fgb_tse_rr_base = (volatile unsigned long *)(pilot_fg_base + 0x600);
	fgb_bse_regs = (volatile BSE_REG_STRUCT *)(pilot_fg_base + 0x200);
	grc_ctl_regs = (volatile GRC_STRUCT *)(pilot_fg_base + GRC_BASE_OFFSET + GRC_OFFSET);
	grc_regs = (volatile GRC_REGS_STRUCT *)(pilot_fg_base + GRC_BASE_OFFSET + GRC_REGS_STRUCT_OFFSET);
	display_cntrl = (volatile unsigned long *)(sysclk_base + DISP_CNTRL_OFFSET);
	card_presence_cntrl = (volatile unsigned long *)(sysclk_base + PCIDID_OFFSET);
	PCISID_regs = (volatile unsigned long *)(sysclk_base + PCISID_OFFSET);

	crtc = (volatile CRTC0_REG_STRUCT * )(pilot_fg_base + GRC_BASE_OFFSET + CRTC0_OFFSET);
    	crtcext =  (volatile CRTCEXT0_TO_7_REG_STRUCT *)(pilot_fg_base + GRC_BASE_OFFSET + CRTCEXT_OFFSET);
    	attr =  (volatile ATTR_REG_STRUCT *)(pilot_fg_base + GRC_BASE_OFFSET + ATTR0_OFFSET);
    	XMULCTRL = (volatile unsigned char *)(pilot_fg_base + GRC_BASE_OFFSET + 0x450);
	tile_map = (tm_entry_t *)kmalloc(64 * 64 * sizeof(tm_entry_t), GFP_KERNEL);
	if (tile_map == NULL) {
		return 1;
	}

	pilot_fgb_irq = IRQ_FGE; 
	PollMode = 0;

	/* Request IRQ */
	if (!PollMode)
	{
		if (request_irq(pilot_fgb_irq, pilot_fgb_intr, 0, "PILOTFGB",NULL))
		{
			TWARN("Request IRQ of PILOT FRAME GRABBER IRQ Failed. Running in Poll Mode\n");
			PollMode = 1;
		}
	}

	

	return 0;
}

// Called when insmod is invoked
int
init_videocap (void)
{
	/* Initialize Data */
	pilot_fg_base = 0;
	pilot_fgb_irq  = 0;
	PollMode    = 0;

	/* Tuneable Parameters */
	TileColumnSize = 32 ; // 32 pixels
	TileRowSize = 32 ; // 32 lines
	MaxRectangles = 1 ;

	/* Register the driver */
	if (register_chrdev(VIDEOCAP_DEVICE_MAJOR, "videocap",  &videocap_fops) < 0)
	{
		TEMERG("Video Capture Driver can't get Major %d\n",VIDEOCAP_DEVICE_MAJOR);
	 	goto RegisterFailed;
	}

	/* Create this module's directory entry in proc and add a file "DriverStatus" */
	moduledir = proc_mkdir("videocap",rootdir);


#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
	statisticsproc = AddProcEntry(moduledir,"Statistics",ReadDriverStatistics,NULL,NULL);
	statusproc = AddProcEntry(moduledir,"Status",ReadDriverStatus,NULL,NULL);
	infoproc = AddProcEntry(moduledir,"Info",ReadDriverInfo,NULL,NULL);
	regproc = AddProcEntry(moduledir,"Registers",DumpRegisters,NULL,NULL);
	paletteproc = AddProcEntry(moduledir,"Palette",DumpPalette,NULL,NULL);
	videoproc = AddProcEntry(moduledir,"VideoBuff",DumpVideo,NULL,NULL);
	tfebuffproc = AddProcEntry(moduledir,"TfeBuff",DumpTfeBuff,NULL,NULL);
	deccolproc = AddProcEntry(moduledir,"DecColSize",DECTSEColumnSize,NULL,NULL);
	decrowproc = AddProcEntry(moduledir,"DecRowSize",DECTSERowSize,NULL,NULL);
	decmaxproc = AddProcEntry(moduledir,"DecMaxRect",DECTSEMaxRectangles,NULL,NULL);
#else
	AddProcEntry(moduledir,"Statistics",ReadDriverStatistics,NULL,NULL);
	AddProcEntry(moduledir,"Status",ReadDriverStatus,NULL,NULL);
	AddProcEntry(moduledir,"Info",ReadDriverInfo,NULL,NULL);
	AddProcEntry(moduledir,"Registers",DumpRegisters,NULL,NULL);
	AddProcEntry(moduledir,"Palette",DumpPalette,NULL,NULL);
	AddProcEntry(moduledir,"VideoBuff",DumpVideo,NULL,NULL);
	AddProcEntry(moduledir,"TfeBuff",DumpTfeBuff,NULL,NULL);
	AddProcEntry(moduledir,"DecColSize",DECTSEColumnSize,NULL,NULL);
	AddProcEntry(moduledir,"DecRowSize",DECTSERowSize,NULL,NULL);
	AddProcEntry(moduledir,"DecMaxRect",DECTSEMaxRectangles,NULL,NULL);
#endif

	/* Add sysctl to access the videocap */
	my_sys  = AddSysctlTable("videocap",&VideoTable[0]);

	/* Request Resources - IRQ */
	if (AllocateResources() != 0)
		goto AllocResFailed;

	return 0;

AllocResFailed:
	/* Remove driver related sysctl entries */
	RemoveSysctlTable(my_sys);

	/* Remove the driver's proc entries */
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
	RemoveProcEntry(statisticsproc);
	RemoveProcEntry(statusproc);
	RemoveProcEntry(infoproc);
	RemoveProcEntry(regproc);
	RemoveProcEntry(paletteproc);
	RemoveProcEntry(videoproc);
	RemoveProcEntry(tfebuffproc);
	RemoveProcEntry(deccolproc);
	RemoveProcEntry(decrowproc);
	RemoveProcEntry(decmaxproc);
#else
	RemoveProcEntry(moduledir,"Info");
	RemoveProcEntry(moduledir,"Status");
	RemoveProcEntry(moduledir,"Statistics");
	RemoveProcEntry(moduledir,"Registers");
	RemoveProcEntry(moduledir,"Palette");
	RemoveProcEntry(moduledir,"VideoBuff");
	RemoveProcEntry(moduledir,"TfeBuff");
	RemoveProcEntry(moduledir,"DecColSize");
	RemoveProcEntry(moduledir,"DecRowSize");
	RemoveProcEntry(moduledir,"DecMaxRect");
#endif

	remove_proc_entry("videocap",rootdir);

	/* Unregister the driver */
	unregister_chrdev(VIDEOCAP_DEVICE_MAJOR,"videocap");

RegisterFailed:
	return 1;
}


void
exit_videocap (void)
{
	unsigned long size; 
	unsigned long addr;

	TDBG_FLAGGED(videocap,DBG_MAIN,"Unloading Video Capture Driver ..............\n");

	printk("videocap exiting\n");
	/* Release Resources */
	if (!PollMode)
		free_irq(pilot_fgb_irq, NULL);
	iounmap((void *)pilot_fg_base);
	iounmap((void *)framebuffer_base);
	iounmap((void *)tfe_descriptor_base);

	if (tfedest_base) {
            size = BIGPHYS_SIZE;
            addr = (unsigned long)tfedest_base;
            while (size > 0) {
                ClearPageReserved(virt_to_page(addr));
                addr += PAGE_SIZE;
                size -= PAGE_SIZE;
            }
            bigphysarea_free_pages(tfedest_base);
        }


	if (!tile_map) {
		kfree(tile_map);
	}

	/* Remove driver related sysctl entries */
	RemoveSysctlTable(my_sys);

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
  RemoveProcEntry(statisticsproc);
  RemoveProcEntry(statusproc);
  RemoveProcEntry(infoproc);
  RemoveProcEntry(regproc);
  RemoveProcEntry(paletteproc);
  RemoveProcEntry(videoproc);
  RemoveProcEntry(tfebuffproc);
  RemoveProcEntry(deccolproc);
  RemoveProcEntry(decrowproc);
  RemoveProcEntry(decmaxproc);
#else
  RemoveProcEntry(moduledir,"Info");
  RemoveProcEntry(moduledir,"Status");
  RemoveProcEntry(moduledir,"Statistics");
  RemoveProcEntry(moduledir,"Registers");
  RemoveProcEntry(moduledir,"Palette");
  RemoveProcEntry(moduledir,"VideoBuff");
  RemoveProcEntry(moduledir,"TfeBuff");
  RemoveProcEntry(moduledir,"DecColSize");
  RemoveProcEntry(moduledir,"DecRowSize");
  RemoveProcEntry(moduledir,"DecMaxRect");
#endif
	
  remove_proc_entry("videocap",rootdir);

	/* Unregister the driver */
	unregister_chrdev(VIDEOCAP_DEVICE_MAJOR,"videocap");

	TDBG_FLAGGED(videocap,DBG_MAIN,"Unloading Video Capture Driver Completed \n");
	return;
}

module_init(init_videocap);
module_exit(exit_videocap);

/*--------------------- Module Information Follows ------------------------*/
MODULE_AUTHOR ("Varadachari Sudan - American Megatrends Inc");
MODULE_DESCRIPTION ("Video Capture Driver");
MODULE_LICENSE ("GPL");

