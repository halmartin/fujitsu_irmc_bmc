/****************************************************************
 **                                                            **
 **    (C)Copyright 2010-2011, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 * phyctrl_mod.c
 * 
 * Author: Gokulakannan S <gokulakannans@amiindia.co.in>
 ***************************************************************/

#include <linux/version.h>
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>   /* printk()         */
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/ide.h>
#include <asm/delay.h>
#include "phyctrl.h"
#include <linux/poll.h>

#define PHYCTRL_DRIVER_MAJOR   103
#define PHYCTRL_DRIVER_MINOR   0
#ifdef HAVE_UNLOCKED_IOCTL  
  #if HAVE_UNLOCKED_IOCTL  
        #define USE_UNLOCKED_IOCTL  
  #endif
#endif
static int phyctrl_drv_registered = 0;

static int  init_phyctrl_module(void);
static void exit_phyctrl_module(void);

static int phyctrl_open(struct inode * inode, struct file * filp);
static int phyctrl_close(struct inode * inode, struct file * filp);
#ifdef USE_UNLOCKED_IOCTL 
static long phyctrl_ioctlUnlocked (struct file *file, unsigned int cmd, unsigned long arg);
#else
static int phyctrl_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg);
#endif
static struct file_operations phyctrl_fops = 
{
    owner:      THIS_MODULE,
    read:       NULL,
    write:      NULL,
#ifdef USE_UNLOCKED_IOCTL
    unlocked_ioctl:       phyctrl_ioctlUnlocked,
#else
    ioctl:      phyctrl_ioctl,
#endif
    open:       phyctrl_open,
    release:    phyctrl_close,
    poll:       NULL
};

static int 
phyctrl_close(struct inode * inode, struct file * filp)
{
    //printk("PHYCTRL: device close \n");
    return 0;
}

static int 
phyctrl_open(struct inode * inode, struct file * filp)
{
    //printk("PHYCTRL: device open \n");
    return 0;
}


static int
init_phyctrl_module(void)
{
    int err, ret = 0;

    printk("PILOT-II PHYCTRL Driver Version %d.%d\n",PHYCTRL_DRIVER_MAJOR,PHYCTRL_DRIVER_MINOR);
    printk("Copyright (c) 2011 American Megatrends Inc.\n");

    /* ----  Register the character device ------------------- */
    if ((err = register_chrdev(PHYCTRL_DEVICE_MAJOR, PHYCTRL_DEVNAME, &phyctrl_fops)) < 0)
    {
        printk ("Phyctrl: Failed to register PHYCTRL device (err %d)\n", err);
        ret = -ENODEV;
        goto fail;
    }
    phyctrl_drv_registered = 1;
    return ret; 

fail:
    exit_phyctrl_module ();
    return ret;
}

#ifdef USE_UNLOCKED_IOCTL
static long phyctrl_ioctlUnlocked (struct file *file, unsigned int cmd, unsigned long arg)
#else
static int 
phyctrl_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
    unsigned char phy_dev = 0;

    if (__copy_from_user((void *)&phy_dev ,(void *)arg, sizeof(unsigned char)))
    { 
        printk("Phyctrl: Error copying data from user \n"); 
        return -EFAULT; 
    }
    
    if (INIT_PHY_DEVICE != cmd)
    {
        printk("Phyctrl: Error Unknown IOCTL Command\n"); 
        return -EINVAL;
    }
    switch (phy_dev)
    {

    case FSC_PHY_BCM_5221 :
        printk ("Phyctrl: Phy Device is Broadcom BCM5221(%xh)\n", phy_dev);
        break;
        
    case FSC_PHY_SMSC_LAN8700:
        printk ("Phyctrl: Phy Device is SMSC LAN8700(%xh)\n", phy_dev);
        break;
        
    case FSC_PHY_REALTEK_RTL8211:
        printk ("Phyctrl: Phy Device is RTL 8211E(%xh)\n", phy_dev);
        break;
        
    case FSC_NIC_INTEL_82575:
        printk ("Phyctrl: Phy Device is Intel Zoar 82575(%xh)\n", phy_dev);
        break;
        
    case FSC_NIC_INTEL_KAWELA:
        printk ("Phyctrl: Phy Device is Intel Kawela(%xh)\n", phy_dev);
        break;
        
    case FSC_NIC_INTEL_HARTWELL :
        printk ("Phyctrl: Phy Device is Intel Hartwell(%xh)\n", phy_dev);
        break;
        
    default :
        printk ( "Phyctrl: Invalid PHY Device(%xh)\n", phy_dev);
        return -EINVAL;
    }
    return 0;
}



static void
exit_phyctrl_module(void)
{
    printk("Unloading PILOT-II PHYCTRL Module ..............\n");

    /* unregister the device */
    if (phyctrl_drv_registered)
    {
        unregister_chrdev(PHYCTRL_DEVICE_MAJOR, PHYCTRL_DEVNAME);
        phyctrl_drv_registered = 0;
    }

    printk("PILOT-II PHYCTRL module unloaded sucessfully.\n");

    return;
}

module_init (init_phyctrl_module);
module_exit (exit_phyctrl_module);

MODULE_AUTHOR("Anurag Bhatia- American Megatrends Inc");
MODULE_DESCRIPTION("Pilot-II SOC PHYCTRL Driver.");

