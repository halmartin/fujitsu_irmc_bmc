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
 * phyctrl.h
 * 
 * Author: Gokulakannan S <gokulakannans@amiindia.co.in>
 ***************************************************************/


#ifndef __PHYCTRL_H__
#define __PHYCTRL_H__

/* IOCTL Commands and structure */

typedef struct phyctrlcmd 
{
    unsigned char Index;
    unsigned char data;
} PHYCTRLCMD;

typedef PHYCTRLCMD phyctrl_ioctl_data;



#define PHYCTRL_CTL_FILE   "/dev/phyctrl"


/* Device Properties */
#define PHYCTRL_DEVICE_MAJOR   103
#define PHYCTRL_DEVNAME        "PHYCTRL"

#define INIT_PHY_DEVICE     1

/*
 * Phy Name                 BIT[5:0]            Description
 * 
 * Broadcom BCM5221         (4h)                Dedicated
 * SMSC LAN8700             (ah)                Dedicated
 * RTL 8211E                (ch)                FrontLan and Dedicated
 * Intel Zoar 82575         (10h)               Shared with only one port
 * Intel Kawela             (11h)               Shared with only one port
 * Intel Hartwell           (12h)               Shared with only one port 
 */

#define FSC_PHY_BCM_5221            0x04
#define FSC_PHY_SMSC_LAN8700        0x0A 
#define FSC_PHY_REALTEK_RTL8211     0x0C    /* RealTek RTL8211 subtypes will evaluated by fw! */
#define FSC_NIC_INTEL_82575         0x10    /**** all NCSI-NICs will be set to ZOAR! ***/
#define FSC_NIC_INTEL_KAWELA        0x11
#define FSC_NIC_INTEL_HARTWELL      0x12

#define FSC_NIC_INVALID             0xFF
#define FSC_NIC_UNSPECIFIED         0x00



#endif
