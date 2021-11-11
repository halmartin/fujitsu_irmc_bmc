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
* @file 	mmap.c
* @authors	Vinesh Christoper 	    <vineshc@ami.com>,
*		Baskar Parthiban 	    <bparthiban@ami.com>,
*		Varadachari Sudan Ayanam    <varadacharia@ami.com>
* @brief   	contains function that is used to map device memory
*			address to application
****************************************************************/

#ifndef MODULE			/* If not defined in Makefile */
#define MODULE
#endif

#ifndef __KERNEL__		/* If not defined in Makefile */
#define __KERNEL__
#endif

#ifndef EXPORT_SYMTAB		/* If not defined in Makefile */
#define EXPORT_SYMTAB
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <asm/pgtable.h>
#include <asm/io.h>

#include "videocap.h"
#include "videodata.h"
#include "dbgout.h"

#define VMA_OFFSET(vma) 	((vma)->vm_pgoff << PAGE_SHIFT)


void
videocap_vma_open(struct vm_area_struct *vma)
{
	TDBG_FLAGGED(videocap,DBG_MMAP,"vma_open() Called\n");
//    MOD_INC_USE_COUNT;
}

void
videocap_vma_close(struct vm_area_struct *vma)
{
	TDBG_FLAGGED(videocap,DBG_MMAP,"vma_close() Called\n");
 //  MOD_DEC_USE_COUNT;
}

struct
vm_operations_struct videocap_vm_ops =
{
    open	:  videocap_vma_open,
    close	:  videocap_vma_close,

/* Don't use nopage, because no page will work correctly only if we have a single page.
 * Since we are allocating a link of pages using __get_free_pages, we cannot use this.			   */
/*
    nopage	:  videocap_vma_nopage,
 */
};


int
videocap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long pos = 0;
	unsigned long start = (unsigned long)(vma->vm_start);
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	if (tfedest_base)	
		pos = tfedest_bus;
	else
	{
		TDBG("The Big Phys is not Allocated at all. Not Mapping\n");
		return -1;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	if (remap_page_range(start, pos, size, vma->vm_page_prot)) {
        	return -EAGAIN;
    	}
#else
    	vma->vm_flags |= VM_IO;
    	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    	if (io_remap_pfn_range(vma, start, pos >> PAGE_SHIFT, size, vma->vm_page_prot)) 
    	{
		TDBG("Could not map the Phys area\n");
       		return -EAGAIN;
    	}
#endif

    vma->vm_ops = &videocap_vm_ops;
    videocap_vma_open(vma);

    return 0;
}

