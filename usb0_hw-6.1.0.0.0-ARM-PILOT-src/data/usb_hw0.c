/****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
****************************************************************/

#include <linux/init.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,14,17))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>	
#include <asm/io.h>
#include <mach/platform.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <coreTypes.h>
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
#include <linux/kthread.h>
#endif
#include "board.h"
#include "seusb11.h"
#include "usb_hw.h"
#include "usb_core.h"
#include "helper.h"
#include "dbgout.h"
#include "mod_reg.h"
#include "reset.h"

/* Local Types */
typedef int (*DPC)(void *arg);

typedef struct
{
	DPC DPC;
	int Inst;
} DPC_T;

typedef struct
{
	unsigned long	action;
	int	device_num;
}kthread_data_usb_t;

typedef int (*HostResetHandler) (void);

#define HW_MODULE_DPC_NAME	"USB11_DPC"


/* Global Variables */
TDBG_DECLARE_DBGVAR(usbhw11);

/* Internal Functions. Not Exposed outside */
static void SEUSB11_SoftDisconnect(uint8 DeviceNo);
static int  SEUSB11_RegisterIsr		(uint8 DevNo);
static void SEUSB11_Write_FifoReg(uint8 DeviceNo,uint8 Reg, uint8 *Buffer, uint16 Len);
static int  SEUSB11_InitHardware(uint8 DeviceNo);
/* Functions Exposed via USB_HW structure */
static void		SEUSB11_Enable			(uint8 DeviceNo, uint8 Speed);
static void		SEUSB11_Disable			(uint8 DeviceNo, uint8 Speed);
static int 	SEUSB11_IntrHandler		(uint8 DeviceNo,uint8 ep);
static void		SEUSB11_SetAddress		(uint8 DeviceNo,uint8 Addr, uint8 Enable);
static void		SEUSB11_GetAddress		(uint8 DeviceNo,uint8 *Addr, uint8 *Enable);
static int 		SEUSB11_WriteFifoBuffer (uint8 DeviceNo,uint8 ep, uint8 *Buffer, uint16 Len);
static int 		SEUSB11_ReadFifoBuffer	(uint8 DeviceNo,uint8 ep, uint8 *Buffer,uint16 *Len, uint8 Setup);
static int		SEUSB11_AllocBuffer		(uint8 DeviceNo,uint8 ep, uint16 Fifo, uint16 MaxPkt, uint8 Dir, uint8 Type);
static int		SEUSB11_EnableEp		(uint8 DeviceNo,uint8 ep, uint8 EpDir, uint8 EpType);
static int		SEUSB11_DisableEp		(uint8 DeviceNo,uint8 ep, uint8 EpDir, uint8 EpType);
static int		SEUSB11_GetEpStatus		(uint8 DeviceNo,uint8 ep, uint8 EpDir, uint8 *Enable, uint8 *Stall);
static int		SEUSB11_StallEp			(uint8 DeviceNo,uint8 ep, uint8 EpDir);
static int		SEUSB11_UnstallEp		(uint8 DeviceNo,uint8 ep, uint8 EpDir);
static void	 	SEUSB11_RemoteWakeup 	(uint8 DeviceNo);
static void		SEUSB11_DeviceDisconnect(uint8 DevNo);
static void		SEUSB11_DeviceReconnect (uint8 DevNo);
static void    	SEUSB11_CompleteRequest	(uint8 DevNo,uint8 status,DEVICE_REQUEST *Req);
static int	 	SEUSB11_GetEps (uint8 DevNo, uint8* pdata, uint8* num_eps);
static int      SEUSB11_Init(uint8 DeviceNo, USB_HW *UsbDev, void **DevConf);
static void		SEUSB11_Exit(uint8 DeviceNo);
static uint8    SEUSB11_ReadReg8(uint8 DeviceNo, uint16 RegNo);
static void 	SEUSB11_WriteReg8(uint8 DeviceNo, uint16 RegNo,uint8 Data);
static void     SEUSB11_SoftConnect(uint8 DeviceNo);
static int 		notify_reboot_instance0(struct notifier_block *this, unsigned long code, void *unused);
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
static int SEUSB11_Upstream_disable(uint8 DeviceNo);
static int SEUSB11_Upstream_enable(uint8 DeviceNo);
int IsUsb_Upstream_host = 0;
#endif
static int      UsbDeferProcess0(void *arg);
static void     Usb_OsInitDPC(DPC_T *pDPC);
static void     Usb_OsKillDPC(uint8 DeviceNo);
static void     Usb_OsWakeupDPC(uint8 DeviceNo);
/* Static Variables */
static struct completion DPC_Semaphore[MAX_NUM_SEUSB11];
static struct completion DPC_Init[MAX_NUM_SEUSB11];
static struct completion DPC_Exited[MAX_NUM_SEUSB11];
static DPC_T DPCInst [MAX_NUM_SEUSB11] = {
											{
												UsbDeferProcess0,
												0,
											},
										};

static int DPCThreadRunning [MAX_NUM_SEUSB11] = { 1 };
static kthread_data_usb_t kthread_data_usb [MAX_NUM_SEUSB11] = {
															{ 0, 0 },
														};	

static char *ModuleName = "P2USB11";
static USB_CORE UsbCoreFunc;
static int NumInst = 1;
static int SetAddrEnable [MAX_NUM_SEUSB11] = { 0 };


static struct notifier_block seusb11_notifier [MAX_NUM_SEUSB11] =
{
       { .notifier_call = notify_reboot_instance0,},
};

static int SEUSB11_HostResetHandler0 (void);
static HostResetHandler SEUSB11_HostResetHandlers[MAX_NUM_SEUSB11] = {
												SEUSB11_HostResetHandler0,
											};

/* SEUSB11 Hardware functions structure */
static USB_HW SEUSB11_Dev =
{
	.UsbHwEnable				= SEUSB11_Enable,
	.UsbHwDisable	 			= SEUSB11_Disable,
	.UsbHwIntr 					= SEUSB11_IntrHandler,
	.UsbHwSetAddr 				= SEUSB11_SetAddress,
	.UsbHwGetAddr 				= SEUSB11_GetAddress,
	.UsbHwWrite					= SEUSB11_WriteFifoBuffer,
	.UsbTxComplete				= NULL,
	.UsbHwRead					= SEUSB11_ReadFifoBuffer,
	.UsbHwAllocBuffer			= SEUSB11_AllocBuffer,
	.UsbHwEnableEp				= SEUSB11_EnableEp,
	.UsbHwDisableEp				= SEUSB11_DisableEp,
	.UsbHwGetEpStatus			= SEUSB11_GetEpStatus,
	.UsbHwStallEp				= SEUSB11_StallEp,
	.UsbHwUnstallEp				= SEUSB11_UnstallEp,
	.UsbHwSetRemoteWakeup		= NULL,
	.UsbHwGetRemoteWakeup		= NULL,
 	.UsbHwCompleteRequest		= SEUSB11_CompleteRequest,
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	.UsbHwUpstreamDisable		= SEUSB11_Upstream_disable,
	.UsbHwUpstreamEnable		= SEUSB11_Upstream_enable,
#endif
	.UsbHwDeviceDisconnect		= SEUSB11_DeviceDisconnect,
	.UsbHwDeviceReconnect		= SEUSB11_DeviceReconnect,
 	.UsbHwRemoteWakeup			= SEUSB11_RemoteWakeup,
	.UsbHwGetEps				= SEUSB11_GetEps,
	.NumEndpoints				= SEUSB11_MAX_EP * 2,	/* Useable Endpoints */
	.BigEndian					= 0,		/* Little Endian Device */
    .WriteFifoLock				= 0,        /* Lock WriteFifo routine , so that write to fifo are not interleaved */
	.EP0Size					= SEUSB11_EP0_SIZE,
	.SupportedSpeeds			= SUPPORT_FULL_SPEED,
};

static usb_ctrl_driver seusb11_driver [MAX_NUM_SEUSB11]=
{
	{
		.module 		= THIS_MODULE,
		.usb_driver_init	= SEUSB11_Init,
		.usb_driver_exit	= SEUSB11_Exit,
		.devnum         	= 0xFF,
	}
};

/* SEUSB11 Device Configuration Struture */
static USB_DEV_CONF SEUSB11_Conf[MAX_NUM_SEUSB11] =
{
	{
		SEUSB11_0_CMD_REG,
		0,
		0xFF,				/* 0xFF = Unconfigured/Not Used */
		0,
		SEUSB11_0_BIG_ENDIAN_DEVICE,
		1,
		{
			{SEUSB11_0_INT_PRI,
			SEUSB11_0_INTR,
			SEUSB11_0_VECTOR,
			IRQF_DISABLED}
			
		}
	}
};

static uint8 SEUSB11_BusState[MAX_NUM_SEUSB11] = {1};
static char* SEUSB11_IrqName[MAX_NUM_SEUSB11] ={"SEUSB11"};
static ep_config_t	ep_config [MAX_NUM_SEUSB11][16] = 
{
					{
						{ 1,	 IN,	64,	CONFIGURABLE,	 0 },
						{ 1,	 OUT,	64,	CONFIGURABLE,	 0 },
						{ 2,	 IN,	64,	CONFIGURABLE,	 0 },
						{ 2,	 OUT,	64,	CONFIGURABLE,	 0 },
						{ 3,	 IN,	64,	CONFIGURABLE,	 0 },
						{ 3,	 OUT,	64,	CONFIGURABLE,	 0 },
						{ 4,	 IN,	64,	CONFIGURABLE,	 0 },
						{ 4,	 OUT,	64,	CONFIGURABLE,	 0 },
						{ 5,	 IN,	64,	CONFIGURABLE,	 0 },
						{ 5,	 OUT,	64,	CONFIGURABLE,	 0 },
						{ 6,	 IN,	64,	CONFIGURABLE,	 0 },
						{ 6,	 OUT,	64,	CONFIGURABLE,	 0 },
						{ 7,	 IN,	64,	CONFIGURABLE,	 0 },
						{ 7,	 OUT,	64,	CONFIGURABLE,	 0 },
						{ 0xFF,  0xFF,  0xFF, 0xFF,          0xFF},  
					},
};
static int InitDone[MAX_NUM_SEUSB11] = {0};


static struct ctl_table_header *my_sys 	= NULL;
static struct ctl_table UsbHw0ctlTable [] =
{
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
	{CTL_UNNUMBERED,"DebugLevel",&(TDBG_FORM_VAR_NAME(usbhw11)),sizeof(int),0644,NULL,NULL,&proc_dointvec}, 
#else
	{"DebugLevel",&(TDBG_FORM_VAR_NAME(usbhw11)),sizeof(int),0644,NULL,&proc_dointvec}, 
#endif
	{0} 
};

module_param (ModuleName, charp, S_IRUGO);
MODULE_PARM_DESC(ModuleName, "Pilot-II USB1.1 hardware module name");
module_param (NumInst, int, S_IRUGO);
MODULE_PARM_DESC(NumInst, "Instances of Pilot-II USB1.1 hardware module");

static uint8  
SEUSB11_ConvertDeviceToIndex(uint8 DeviceNo)
{
	uint8 Index;

	for (Index = 0;Index < MAX_NUM_SEUSB11;Index++) 
	{
		if (SEUSB11_Conf[Index].DeviceNo == DeviceNo)
			return Index;
	}
	TCRIT("SEUSB11_ConvertDeviceToIndex(): Failed for Device No 0x%x\n",DeviceNo); 
	return 0xFF;	
}

static int 
SEUSB11_RegisterIsr(uint8 DevNo)
{
	uint8 Index;	

	Index = SEUSB11_ConvertDeviceToIndex(DevNo);
	if (Index == 0xFF) return 1;

	if (UsbCoreFunc.CoreUsb_OsRegisterIsr(SEUSB11_IrqName[Index],SEUSB11_Conf[Index].IntrConf[0].Vector,
							DevNo,0, SEUSB11_Conf[Index].IntrConf[0].IntrFlags) != 0)
	{
		TCRIT("SEUSB11_RegisterIsr(): Error in Registering IRQ for Device %d\n",DevNo);
		return 1;
	}

	return 0;
}



static int 
SEUSB11_HostResetHandler (uint8 DeviceNo)
{
	uint8 Index;
	uint8 Speed;

	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 0;

	/* If Init is not done, means USB port is not enabled. Skip the reset handling */
	if (!InitDone[Index]){
		printk("Skipping USB port reset handling because USB port is not enabled\n");
		return 0;
	}
	Speed = SEUSB11_Conf[Index].DevSpeed;
	//printk("Reinitializing USB Hardware on Device %d\n",DeviceNo);
	kthread_data_usb[Index].action = 0;
	SEUSB11_InitHardware(DeviceNo);
	SEUSB11_Enable(DeviceNo, Speed);
	if  (UsbCoreFunc.CoreUsbGetDeviceConnectState( DeviceNo))
	{
		SEUSB11_SoftConnect(DeviceNo);	
		TDBG_FLAGGED(usbhw11, DEBUG_HW0,"Device %d is connected\n",DeviceNo);
	}
	else
		TDBG_FLAGGED(usbhw11, DEBUG_HW0,"Device %d is not connected (AutoAttach Mode)\n",DeviceNo);

	return 0;
}

static int 
SEUSB11_HostResetHandler0(void)
{
	uint8 DeviceNo;

	if (SEUSB11_Conf[0].DeviceNo == 0xFF)
			return 1;
	DeviceNo = SEUSB11_Conf[0].DeviceNo;
	return SEUSB11_HostResetHandler (DeviceNo);
}

static uint8  
SEUSB11_GetBusState	(uint8 DevNo)
{
	uint8 Index;
	Index = SEUSB11_ConvertDeviceToIndex(DevNo);
	if (Index == 0xFF)
		return 0;
	return SEUSB11_BusState[Index];
}

static uint8 
SEUSB11_ReadReg8(uint8 DeviceNo, uint16 RegNo)
{ 
	uint8 Index;
	
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return 0;
		
	return READ_SEUSB11_BYTE(SEUSB11_Conf[Index].DataReg+RegNo);
}

static void 
SEUSB11_WriteReg8(uint8 DeviceNo, uint16 RegNo,uint8 Data)
{ 
	uint8 Index;
	
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;

	WRITE_SEUSB11_BYTE(SEUSB11_Conf[Index].DataReg+RegNo,Data);
	return;
}

static void 
SEUSB11_SoftConnect(uint8 DeviceNo)
{

	uint8 Reg8;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	Reg8 = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_USBCS);
	Reg8 &= ~(SEUSB11_DISCON);
	SEUSB11_WriteReg8 (DeviceNo, SEUSB11_USBCS, Reg8);
	
	Reg8 = READ_SEUSB11_BYTE(SE_CONNECT_VA); 
	Reg8 |= (1<< 5);
	WRITE_SEUSB11_BYTE(SE_CONNECT_VA , Reg8);
	return;

}

static void 
SEUSB11_SoftDisconnect(uint8 DeviceNo)
{

	uint8 Reg8;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	Reg8 = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_USBCS);
	Reg8 |= (SEUSB11_DISCON);
	SEUSB11_WriteReg8 (DeviceNo, SEUSB11_USBCS, Reg8);

	Reg8 = READ_SEUSB11_BYTE(SE_CONNECT_VA );
	Reg8 &= 0xDF;
	WRITE_SEUSB11_BYTE(SE_CONNECT_VA,Reg8);
	return;
}

static int
SEUSB11_UnregisterIsr(uint8 DevNo)
{
	uint8 Index;

	Index = SEUSB11_ConvertDeviceToIndex(DevNo);
	if (Index == 0xFF)
	{
		TCRIT("Error in UnRegistering IRQ for Device %d\n",DevNo);
		return 1;
	}

	UsbCoreFunc.CoreUsb_OsUnregisterIsr(SEUSB11_IrqName[Index],SEUSB11_Conf[Index].IntrConf[0].Vector,
							DevNo,0, SEUSB11_Conf[Index].IntrConf[0].IntrFlags); 
	return 0;
}


/* Exit function called by USBCore directly */
static void 
SEUSB11_Exit(uint8 DeviceNo)
{
	uint8 Status;

	uint8 Index;

	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
	{
		return;
	}

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"Unloading SEUSB11 Hardware Layer\n");

	uninstall_reset_handler(SEUSB11_HostResetHandlers[Index]);
	/* Disable all Interrupts */

   	/* Stop Deffered Interrupt Processing */
	Usb_OsKillDPC(DeviceNo);
	
	Status = SEUSB11_ReadReg8(DeviceNo,USB_INTR_ENABLE );
	Status &= ~(SEUSB11_WUINTREQ | SEUSB11_DMAINTREQ |SEUSB11_USBINTREQ	);
	SEUSB11_WriteReg8(DeviceNo, USB_INTR_ENABLE ,Status);

	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IEN,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IEN ,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIEN,SEUSB11_INT_DISABLE_ALL);

	/* Disable all endpoits */
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07VAL,0);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07VAL,0);

	iounmap((void *)SEUSB11_Conf[Index].DataReg);
	unregister_reboot_notifier(&seusb11_notifier[Index]);
	return;
}

static int  
SEUSB11_InitHardware(uint8 DeviceNo)
{
	uint8 Status;
	uint8 Index;

	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Disable all Interrupts */
	Status = SEUSB11_ReadReg8(DeviceNo,USB_INTR_ENABLE );
	Status &= ~(SEUSB11_WUINTREQ | SEUSB11_DMAINTREQ |SEUSB11_USBINTREQ	);
	SEUSB11_WriteReg8(DeviceNo, USB_INTR_ENABLE ,Status);

	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IEN,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IEN ,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIEN,SEUSB11_INT_DISABLE_ALL);

	/* Unplug (Softconnect) device */
 	SEUSB11_SoftDisconnect(DeviceNo);

 	/* Clear any pending Interrupts */
	SEUSB11_WriteReg8(DeviceNo,USB_INTR_REGISTER,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IRQ,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IRQ ,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIRQ,SEUSB11_INT_CLEAR_ALL);

	/* Enable only EndPoint 0 IN and OUT /*/
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07VAL,0x01);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07VAL,0x01);

	/* Clear EP0 control and status Reg */
 	Status = SEUSB11_ReadReg8(DeviceNo,SEUSB11_EP0CS);
	Status &= ~(SEUSB11_HSNAK | SEUSB11_EP0STALL);
	SEUSB11_WriteReg8(DeviceNo, SEUSB11_EP0CS,Status);

	return 0;
}

static int
SEUSB11_Init(uint8 DeviceNo, USB_HW *UsbDev, void **DevConf)
{
	static int Num_SEUSB11_Devices = 0;
	uint32 seusb11_base;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_Init: Called for SEUSB11 Device %d with DeviceNo %d\n",
				Num_SEUSB11_Devices,DeviceNo);
	*DevConf = NULL;

	if (Num_SEUSB11_Devices > MAX_NUM_SEUSB11)
	{
		TCRIT("SEUSB11_Init():ERROR:SEUSB11 Devices Exceding MAX_NUM_SEUSB11");
		return 1;
	}

	seusb11_base  = (unsigned long)ioremap(SE_USB_0_BASE,0x100000);

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11(%d):Mapped Base is 0x%x\n",DeviceNo,seusb11_base);

	SEUSB11_Conf[Num_SEUSB11_Devices].DataReg	= seusb11_base;

	/*
       Initialize Device Number in Device Conf Structure. This should
       be done here as all SEUSB11 Access Functions uses the function
       SEUSB11_ConvertDeviceToIndex() funciton which internally uses this
       structure value
    */
	SEUSB11_Conf[Num_SEUSB11_Devices].DeviceNo = DeviceNo;

	SEUSB11_InitHardware(DeviceNo);

	/* Fill in the USB_HW Structure */
	memcpy(UsbDev,&SEUSB11_Dev,sizeof(USB_HW));

	/* Save Device Specific Info to Core */
	*DevConf = &SEUSB11_Conf[Num_SEUSB11_Devices];

	/* Wait for 1 seconds for the host to detect soft removal */
	if (!UsbCoreFunc.CoreUsbGlobalConnectSupport)
	 	UsbCoreFunc.CoreUsb_OsDelay(1000);

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_Init: Device %d Initialized with DeviceNo %d\n",
						Num_SEUSB11_Devices,DeviceNo);

	install_reset_handler(SEUSB11_HostResetHandlers[Num_SEUSB11_Devices]);
	/* Initialize Deffered Interrupt Processing */
	init_completion (&DPC_Semaphore[Num_SEUSB11_Devices]);
	init_completion (&DPC_Init[Num_SEUSB11_Devices]);
	init_completion (&DPC_Exited[Num_SEUSB11_Devices]);
	Usb_OsInitDPC(&DPCInst [Num_SEUSB11_Devices]);

	register_reboot_notifier(&seusb11_notifier[Num_SEUSB11_Devices]);

	/* Increment Number of SEUSB11 Devices */
	Num_SEUSB11_Devices++;

	return 0;
}

static int
IsUsbCableConnected(uint8 DeviceNo)
{
	return 1;
}

static int
SEUSB11_BusSuspend(uint8 DeviceNo)
{
	uint8 Index;

	if (IsUsbCableConnected(DeviceNo) == 0)
	{
		UsbCoreFunc.CoreUsbBusDisconnect(DeviceNo);
		return 1;
	}

	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return 0;

	/* Set we are Suspended */
	SEUSB11_BusState[Index] = 0;

	/* Call core routines */
	UsbCoreFunc.CoreUsbBusSuspend(DeviceNo);

	return 0;
}

static void
SEUSB11_BusResume(uint8 DeviceNo)
{
	uint8 Index;

	/* Call Core Routine */
	UsbCoreFunc.CoreUsbBusResume(DeviceNo);

	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;

	/* Set we are running */
	SEUSB11_BusState[Index] = 1;

	return;
}

static void
SEUSB11_BusReset(uint8 DeviceNo)	
{
	uint8 Index,Status;

	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;

	/* Clear any pending Interrupts */
	SEUSB11_WriteReg8(DeviceNo,USB_INTR_REGISTER,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IRQ,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IRQ ,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIRQ,SEUSB11_INT_CLEAR_ALL);

	/* Clear EP0 control and status Reg */
 	Status =SEUSB11_ReadReg8(DeviceNo,SEUSB11_EP0CS);
	Status &= ~(SEUSB11_HSNAK | SEUSB11_EP0STALL);
	SEUSB11_WriteReg8(DeviceNo, SEUSB11_EP0CS,Status);

	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07VAL,0x1);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07VAL,0x1);

	/* Enable Interrupt */
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IEN,0x1);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IEN ,0x1);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIEN,0x19);  // Don't enable SOF and SutoKir, 

	Status = SEUSB11_ReadReg8(DeviceNo,USB_INTR_ENABLE );
	Status |= (SEUSB11_WUINTEN | SEUSB11_USBINTEN	);
	SEUSB11_WriteReg8(DeviceNo, USB_INTR_ENABLE ,Status);

	/* Call Core Routine */
	UsbCoreFunc.CoreUsbBusReset(DeviceNo);	
//	SEUSB11_DeviceReconnect(DeviceNo);
	return;
}

static void
SEUSB11_EP0_RxDone(uint8 DeviceNo, int Setup)
{
	uint16 Len;
	uint8 *RxBuffer=NULL;
	uint8 Index;

	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;
	
	if (!Setup)
		TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11(%d): Non Setup Packet Recevied in EP 0\n",DeviceNo);
		
	/* Get Endpoint 0 Buffer */
	RxBuffer = UsbCoreFunc.CoreUsbGetRxData(DeviceNo,0);
	if (RxBuffer == NULL)
	{
		printk("Got NULL pointer, so returning ...\n");
		return;
	}

	/* Read the FIFO of Endpoint 0 OUT */
	SEUSB11_ReadFifoBuffer(DeviceNo,0,RxBuffer,&Len,Setup);
	
	/* Write Read Len to Endpoint 0 RxDataLen */
	UsbCoreFunc.CoreUsbSetRxDataLen(DeviceNo,0,Len);

	/* Call Core Routine */
	UsbCoreFunc.CoreUsbRxHandler0(DeviceNo,RxBuffer,(uint32)Len,Setup);
	return;	
}

static void
SEUSB11_EP0_TxDone(uint8 DeviceNo)		
{
	/* Call Core Routine */
	UsbCoreFunc.CoreUsbTxHandler(DeviceNo,0);
	return;
}

static void
SEUSB11_Tx_Intr(uint8 DeviceNo, uint8 ep)
{
	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"Tx Intr for Device %d Ep %d\n",DeviceNo,ep);
	UsbCoreFunc.CoreUsbTxHandler(DeviceNo,ep);		
	return;
}

static void
SEUSB11_Rx_Intr(uint8 DeviceNo, uint8 ep)
{

	uint8* RxData = NULL;
	uint16 Len;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"Rx Intr for Device %d Ep %d\n",DeviceNo,ep);

	/* It is an OUT Endpoint , Get Endpoints RxData*/
	RxData = UsbCoreFunc.CoreUsbGetRxData(DeviceNo,ep);
	if(RxData == NULL)
	{
		printk("Got NULL pointer, so returning ...\n");
		return;
	}
	
	/* Read the Fifo to Endpoint's RxData and Set RxDataLen*/
	SEUSB11_ReadFifoBuffer(DeviceNo,ep,RxData,&Len,0);
	UsbCoreFunc.CoreUsbSetRxDataLen(DeviceNo,ep,Len);
	
	/* Call Core Rx Handler*/
	UsbCoreFunc.CoreUsbRxHandler(DeviceNo,ep);
	
	return;
}	


static 
void
SEUSB11_Write_FifoReg(uint8 DeviceNo,uint8 ep , uint8 *InputBuffer, uint16 Len)
{
	int16 i = 0;
	uint16 Index;
	uint8  *Buffer8;
	uint16 Reg;
	int Counter = 0x1000;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11 (%d) Write Fifo Reg for Ep %d with Length %d\n",DeviceNo,ep,Len);

	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	if (ep == 0)
	{
		while (Counter)
		{
			Reg = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_EP0CS);
			if (0 == (Reg & SEUSB11_IN0BSY))
				break;
			else
				Counter -= 1;
		}
		if (Counter == 0)
		{
			TCRIT ("IN0 Buffer busy bit is set\n");
		}
	}
   	Reg = (SEUSB11_IN0BUF - (ep*0x200));
	Index = 0;
	/* Write data in  bytes */
	Buffer8 = InputBuffer;
	for (i=0;i<Len;i++)
	{
		SEUSB11_WriteReg8(DeviceNo,(Reg + (i*4)),Buffer8[i]);
	}
	/* Load BC register to send the data to host */
	SEUSB11_WriteReg8(DeviceNo,(SEUSB11_IN0BC + (ep*8)),Len);

	return;	
}

static int 
init_SEUSB11_module (void)
{
	int i;
	int ret_val = 0;

	printk ("Loading usb-11 module...\n");
	if (0 != get_usb_core_funcs (&UsbCoreFunc))
	{
		TCRIT ("init_SEUSB11_module(): Unable to get Core Module Functions...\n");
		return -1;
	}

	for (i = 0; i < NumInst; i++)
	{
        if ((ModuleName != NULL) && (sizeof(seusb11_driver[i].name) > strlen(ModuleName)))
		{
			strcpy (seusb11_driver[i].name, ModuleName);
		}
		else
		{
			TCRIT ("init_SEUSB11_module(): Invalid Module Name (too big or NULL)\n");
			return -1;
		}

		ret_val = register_usb_chip_driver_module (&seusb11_driver[i]);
		switch (ret_val)
		{
		case -1:
			unregister_usb_chip_driver_module (&seusb11_driver[i]);
			break;
		}
	}
	if (!ret_val)
	{
		my_sys = AddSysctlTable("usbhw11",&UsbHw0ctlTable[0]);
	}
	else
	{
		TCRIT ("Error: SEUSB11 modules could not be registered with USB core\n");
	}
	return ret_val;
}

static void
exit_SEUSB11_module (void)
{
	int i;

	printk ("Unloading usb-11 module...\n");	
    for (i = 0; i < NumInst; i++)
		unregister_usb_chip_driver_module (&seusb11_driver[i]);

	if (my_sys) RemoveSysctlTable(my_sys);
    return;
}

static void
reboot_action(uint8 DeviceNo)
{
	uint8 Index;
	uint8 Speed;

	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	Speed = SEUSB11_Conf[Index].DevSpeed;
	SEUSB11_Disable (DeviceNo, Speed);
}

static int
notify_reboot_instance0(struct notifier_block *this, unsigned long code, void *unused)
{
	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11 Instance0 Reboot Notifier...\n");
	if (code == SYS_DOWN || code == SYS_HALT)
    {
		if (SEUSB11_Conf[0].DeviceNo != 0xFF)
			reboot_action (SEUSB11_Conf[0].DeviceNo);
	}
    return NOTIFY_DONE;
}

static void
ClearIntrStatus(uint8 DeviceNo,uint16 RegNo, uint8 BitNo)
{
	uint8 Value;
	Value = SEUSB11_ReadReg8(DeviceNo,RegNo);
	Value |= (BitNo);
	SEUSB11_WriteReg8(DeviceNo,RegNo,Value);
	return;
}

static void
SEUSB11_Enable(uint8 DeviceNo, uint8 Speed)
{
	uint8 Index;
	uint8 Status;
	
	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_Enable() is called for Device %d\n", DeviceNo);
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	SEUSB11_Conf[Index].DevSpeed = Speed;

    /* Clear any pending Interrupts */
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IRQ,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IRQ ,SEUSB11_INT_CLEAR_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIRQ,SEUSB11_INT_CLEAR_ALL);

	if (!InitDone[Index])
	{
		/* Register the ISR */
		TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_Enable(): Before register ISR for device %d\n", DeviceNo);
		SEUSB11_RegisterIsr(DeviceNo);
		/* Set Intr Initialization Done */
		InitDone[Index] = 1;
	}

	/* Enable EP0 Interrupts */
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IEN,SEUSB11_IN0IEN);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IEN ,SEUSB11_OUT0IEN);
	/* Enable reset,Suspend and Setup data valid interrupts
		Don't enable SOF and Setup Token interrupts. */
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIEN,(SEUSB11_URESIE | SEUSB11_SUSPIE | SEUSB11_SUDAVIE)); 
	/* Enable USB interrupt and wakeup Interrupt */
	Status = SEUSB11_ReadReg8(DeviceNo,USB_INTR_ENABLE );
	Status |= (SEUSB11_WUINTEN|SEUSB11_USBINTEN);
	SEUSB11_WriteReg8(DeviceNo, USB_INTR_ENABLE ,Status);

	return;
}

static void
SEUSB11_Disable(uint8 DeviceNo, uint8 Speed)
{
	uint8 Index,Status;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_Disable() is called for Device %d\n", DeviceNo);
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;
	
	/* Disable all Interrupts */
	Status = SEUSB11_ReadReg8(DeviceNo,USB_INTR_ENABLE );
	Status &= ~(SEUSB11_WUINTREQ | SEUSB11_DMAINTREQ |SEUSB11_USBINTREQ	);
	SEUSB11_WriteReg8(DeviceNo, USB_INTR_ENABLE ,Status);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IEN,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IEN ,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_USBIEN,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07VAL,SEUSB11_INT_DISABLE_ALL);
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07VAL ,SEUSB11_INT_DISABLE_ALL);

	/* Disconnect Device from Host */
	SEUSB11_SoftDisconnect(DeviceNo);

	if (InitDone[Index])
	{
		TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_Disable(): Freeing Interrupt for Device %d\n", DeviceNo);
		SEUSB11_UnregisterIsr(DeviceNo);
		/* Set Intr Initialization Done */
		InitDone[Index] = 0;
	}
	return;
}

#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
static int SEUSB11_Upstream_enable(uint8 DevNo )
{
	IsUsb_Upstream_host = 1;
	SEUSB11_DeviceReconnect(DevNo);
	return 0;
}

static int SEUSB11_Upstream_disable(uint8 DevNo)
{	
	SEUSB11_DeviceDisconnect(DevNo);
	IsUsb_Upstream_host = 0;
	return 0;
}
#endif

static int
SEUSB11_IntrHandler(uint8 DeviceNo,uint8 InEp)
{
	uint8 IvecReg;
	uint8 SuspendGot=0;
	uint8 Index,ep;
	uint8 USBIntrReg,Out07irq,IN07irq ;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"SEUSB11_IntrHandler(): Intr Entry -----------> for Device %d\n",DeviceNo);

	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return IRQ_HANDLED;

	kthread_data_usb[Index].device_num = DeviceNo;

	USBIntrReg = SEUSB11_ReadReg8(DeviceNo,USB_INTR_REGISTER);
	IvecReg = SEUSB11_ReadReg8(DeviceNo,SEUSB11_IVEC);
//	USBirqReg = SEUSB11_ReadReg8(DeviceNo,SEUSB11_USBIRQ);
	Out07irq  = SEUSB11_ReadReg8(DeviceNo,SEUSB11_OUT07IRQ);
	IN07irq =  SEUSB11_ReadReg8(DeviceNo,SEUSB11_IN07IRQ);


	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"\n Int %x %x ",USBIntrReg ,IvecReg );
	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"OUT07irq = %x ", SEUSB11_ReadReg8(DeviceNo,SEUSB11_OUT07IRQ));
	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"IN07irq = %x ", SEUSB11_ReadReg8(DeviceNo,SEUSB11_IN07IRQ));
	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"OUTXCS = %x ", SEUSB11_ReadReg8(DeviceNo,SEUSB11_OUT1CS ));
	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"EP0CS = %x ", SEUSB11_ReadReg8(DeviceNo,SEUSB11_EP0CS));
	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"USBIRQ = %x\n", SEUSB11_ReadReg8(DeviceNo,SEUSB11_USBIRQ));


	if (!(USBIntrReg & (SEUSB11_WUINTREQ | SEUSB11_USBINTREQ ) ))
	{
#if 0
		TWARN("SEUSB11_IntrHandler(%d): Spurious Intr\n",DeviceNo);
#endif
		return IRQ_HANDLED;
	}
	if (USBIntrReg  & SEUSB11_WUINTEN )
	{
		SuspendGot = 0;
		if (SEUSB11_GetBusState(DeviceNo) == 0)
			SEUSB11_BusResume(DeviceNo);

		ClearIntrStatus(DeviceNo,USB_INTR_REGISTER,SEUSB11_WUINTEN);
	}
	if (USBIntrReg  & SEUSB11_USBINTREQ )
	{
		IvecReg = (IvecReg >> 2);
		ClearIntrStatus(DeviceNo,USB_INTR_REGISTER,SEUSB11_USBINTREQ);
		/* Check for Spurious Interrupt */
		if (IvecReg > 0x14)
		{
			TWARN("SEUSB11_IntrHandler(%d): Spurious Intr\n",DeviceNo);
			return IRQ_HANDLED;
		}

		/************************ Check for Bus Conditions  ************************/
		if (IvecReg == SEUSB11_SUSPEND)
		{	
			ClearIntrStatus(DeviceNo,SEUSB11_USBIRQ,SEUSB11_SUSPIR);
			SuspendGot = 1;					/* This will be processed at end */
		}
		if (IvecReg == SEUSB11_USBRESET	)
		{
			kthread_data_usb[Index].action = 0;
			if (test_and_set_bit (ACTION_USB_BUS_RESET,&kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB11_IntrHandler(%d): Bus Reset Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			ClearIntrStatus(DeviceNo,SEUSB11_USBIRQ,SEUSB11_URESIR);
		}
		if (IvecReg == SEUSB11_SUDAV)
		{
			if (test_and_set_bit (ACTION_USB_SETUP_PACKET,&kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB11_IntrHandler(%d): Setup Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			ClearIntrStatus(DeviceNo,SEUSB11_USBIRQ,SEUSB11_SUDAVIR	);
		}
		if ( IvecReg == SEUSB11_EP0IN)
		{
			if (test_and_set_bit (ACTION_USB_EP0_TX_DONE,&kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB11_IntrHandler(%d): EPO IN Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			ClearIntrStatus(DeviceNo,SEUSB11_IN07IRQ,SEUSB11_IN0IR);
		}
		if ( IvecReg == SEUSB11_EP0OUT	)
		{
			if (test_and_set_bit (ACTION_USB_EP0_RX_DONE,&kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB11_IntrHandler(%d): EP0 OUT Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			ClearIntrStatus(DeviceNo,SEUSB11_OUT07IRQ,SEUSB11_OUT0IR);
		}

		for (ep = 1; ep < SEUSB11_MAX_EP; ep++)
		{
			if (IN07irq & (1 << ep))
			{
				ClearIntrStatus(DeviceNo,SEUSB11_IN07IRQ, (1 << ep));
				SEUSB11_Tx_Intr(DeviceNo, ep);
			}
			if (Out07irq & (1 << ep))
			{
				ClearIntrStatus(DeviceNo,SEUSB11_OUT07IRQ, (1 << ep));
				SEUSB11_Rx_Intr(DeviceNo, ep);
			}

		}

		/************************** Suspend Interrupt processed at end ***************/
		if (SuspendGot)
		{
			if (test_and_set_bit (ACTION_USB_BUS_SUSPEND,&kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB11_IntrHandler(%d): Suspend Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
            Usb_OsWakeupDPC (DeviceNo);
		}
	}


	TDBG_FLAGGED(usbhw11, DEBUG_HW0_INTR,"<-------------------------- SEUSB11 (%d) Intr Exit\n",DeviceNo);	

	return IRQ_HANDLED;
}

static void
SEUSB11_SetAddress(uint8 DeviceNo,uint8 Addr, uint8 Enable)
{
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_SetAddress() is called for Device %d\n",DeviceNo);	
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	SetAddrEnable [Index] = Enable;
	return;
}

static void
SEUSB11_GetAddress(uint8 DeviceNo,uint8 *Addr, uint8 *Enable)
{
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_GetAddress() is called for Device %d\n",DeviceNo);
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	*Addr = SEUSB11_ReadReg8(DeviceNo, SEUSB11_FNADDR);
	*Enable = SetAddrEnable[Index];
	return;

}

static int
SEUSB11_WriteFifoBuffer(uint8 DeviceNo,uint8 ep, uint8 *Buffer, uint16 Len)
{

	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_WriteFifoBuffer() is called for device %d ep %d Len %d\n",
										DeviceNo,ep, Len);

	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint number */
	if (ep > SEUSB11_MAX_EP)
		return 1;
//	if (Len == 0)
//    {
//		TWARN("SEUSB11_WriteFifoBuffer(): received zero ??? lenght buffer\n");
//		return 0;
//	}
	 SEUSB11_Write_FifoReg(DeviceNo,ep,Buffer,Len);

	 return 0;
}

static int
SEUSB11_ReadFifoBuffer(uint8 DeviceNo,uint8 ep, uint8 *Buffer, uint16 *Len,uint8 Setup)
{
	uint16 i;
	uint16 ReadLen, OutBuff_Reg; 
	uint8 Index;
	
	if(Buffer == NULL)
	{
		printk("Got NULL pointer, so returning ...\n");
		return 1;
	}

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_ReadFifoBuffer() is called for device %d and Setup=%d\n",
										DeviceNo,Setup);

	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_ReadFifoBuffer() is called for Device %d\n", DeviceNo);
	/* Validate Endpoint Value */
	if (ep > SEUSB11_MAX_EP)
		return 1;

	/* Possibly a zero length packet ? */
	/* Not pssible for setup packet as int is only gen when 8 bytes are received,*/
	/* ??? */
	
	if ((ep == 0) && (Setup))
	{
		for (i=0;i<8;i++)
		{	
			Buffer[i]= SEUSB11_ReadReg8(DeviceNo,SEUSB11_SETUPBUF + (i*4));
		}
		*Len = 8;
		return 0;
	}

	*Len = 0;
	/* Read the Length  */
	ReadLen = 	SEUSB11_ReadReg8(DeviceNo,(SEUSB11_OUT0BC + (ep*8))  );
	/* Increment total read count */
	(*Len) = ReadLen;
    /* Read the buffer */

	OutBuff_Reg = (SEUSB11_OUT0BUF - (ep * 0x200)); 
	for (i=0;i<ReadLen;i++)
	{		
		Buffer[i]= SEUSB11_ReadReg8(DeviceNo,(OutBuff_Reg + (i*4))  );
	}
	/* Re-arm Byte Count register to receive next Out packet 
	Write any value to BC register */
	SEUSB11_WriteReg8(DeviceNo,(SEUSB11_OUT0BC+(ep*8)),ReadLen);


	return 0;
}

static int	
SEUSB11_AllocBuffer(uint8 DeviceNo,uint8 Ep, uint16 Fifo, uint16 MaxPkt, uint8 Dir, uint8 Type)
{
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_AllocBuffer() is called for device %d endpoint %d MaxPkt %d Dir %d Type %d\n",
										DeviceNo,Ep, MaxPkt, Dir, Type);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Argument Fifo Size is not used at all for SEUSB11. It is always set to 512 */
	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_AllocBuffer() called ...\n");
	/* Validate Endpoint Value */
	if (Ep > SEUSB11_MAX_EP)
		return 1;		
	else 
		return 0;
} 		

static int
SEUSB11_EnableEp(uint8 DeviceNo,uint8 Ep, uint8 EpDir, uint8 EpType)
{
	uint8 temp;
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_EnableEp() is called for device %d endpoint %d epdir %d eptype %d\n",
										DeviceNo,Ep,EpDir,EpType);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;


	/* Validate Endpoint Value */
	if (Ep > SEUSB11_MAX_EP)
		return 1;		

	/* For SEUSB11 we should not enable EP 0*/
	if (Ep == 0)
		return 0;
	if (EpDir == DIR_IN)
	{
		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_IN07IEN);
		temp |= (1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IEN, temp);

		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_IN07VAL);
		temp |= (1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07VAL, temp);
	}
	else if (EpDir == DIR_OUT)
	{
		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_OUT07IEN);
		temp |= (1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IEN, temp);

		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_OUT07VAL);
		temp |= (1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07VAL, temp);
	}

	return 0;
}



static int
SEUSB11_DisableEp(uint8 DeviceNo,uint8 Ep, uint8 EpDir, uint8 EpType)
{
	uint8 temp;
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_DisableEp() is called for device %d endpoint %d epdir %d eptype %d\n",
										DeviceNo,Ep,EpDir,EpType);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint Value */
	if (Ep > SEUSB11_MAX_EP)
		return 1;		

	/* For SEUSB11 we should not disable EP 0*/
	if (Ep == 0)
		return 0;
	if (EpDir == DIR_IN)
	{
		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_IN07IEN);
		temp &= ~(1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07IEN, temp);

		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_IN07VAL);
		temp &= ~(1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_IN07VAL, temp);
	}
	else if (EpDir == DIR_OUT)
	{
		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_OUT07IEN);
		temp &= ~(1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07IEN, temp);

		temp = SEUSB11_ReadReg8 (DeviceNo, SEUSB11_OUT07VAL);
		temp &= ~(1 << Ep);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_OUT07VAL, temp);
	}
	return 0;
}


static int
SEUSB11_GetEpStatus(uint8 DeviceNo,uint8 Ep, uint8 EpDir, uint8 *Enable, uint8 *Stall)
{
	uint8 Value8;
	uint16 Reg8;
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_GetEpStatus(): is called for device %d endpoint %d\n",
										DeviceNo,Ep);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	*Enable = 0;
	*Stall = 0;

	/* Validate Endpoint Value */
	if (Ep > SEUSB11_MAX_EP)
		return 1;

	/* For Endpoint 0 */
	if (Ep == 0)
	{
		*Enable = 1;  /* Always Enabled */
		Value8 = SEUSB11_ReadReg8(DeviceNo,SEUSB11_EP0CS);
		*Stall 	= (Value8  & SEUSB11_EP0STALL)?1:0;
		return 0;
	}

	if (EpDir == DIR_IN)
	{
		Reg8 = (SEUSB11_IN1CS + ((Ep-1)*8));
		Value8 = SEUSB11_ReadReg8(DeviceNo, Reg8);
		if (Value8 & SEUSB11_INXSTL)
			*Stall = 1;
	}
	else if (EpDir == DIR_OUT)
	{
		Reg8 = (SEUSB11_OUT1CS + ((Ep-1)*8));
		Value8 = SEUSB11_ReadReg8(DeviceNo,Reg8  );
		if (Value8 & SEUSB11_OUTXSTL)
			*Stall = 1;
	}
	return 0;
}

static int
SEUSB11_StallEp(uint8 DeviceNo,uint8 Ep, uint8 EpDir)
{
	uint8 Value8;
	uint16 Reg8;
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_StallEp(): is called for device %d endpoint %d\n",
										DeviceNo,Ep);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint Value */
	if (Ep > SEUSB11_MAX_EP)
		return 1;		

	if (Ep == 0)
	{
		
		Value8 = SEUSB11_ReadReg8(DeviceNo,SEUSB11_EP0CS);
		Value8 |= (SEUSB11_HSNAK | SEUSB11_EP0STALL);
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_EP0CS, Value8 );
		
	}

	else
	{
		if (EpDir == DIR_IN)
		{
			Reg8 = (SEUSB11_IN1CS + ((Ep-1)*8));
			Value8 = SEUSB11_ReadReg8(DeviceNo, Reg8);
			Value8 |= SEUSB11_INXSTL;
			SEUSB11_WriteReg8(DeviceNo,Reg8,Value8 );
		}
		else if (EpDir == DIR_OUT)
		{
			Reg8 = (SEUSB11_OUT1CS + ((Ep-1)*8));
			Value8 = SEUSB11_ReadReg8(DeviceNo,Reg8  );
			Value8 |= SEUSB11_OUTXSTL;
			SEUSB11_WriteReg8(DeviceNo,Reg8, Value8 );
		}

	}

	/* Call Core Handler */
	UsbCoreFunc.CoreUsbHaltHandler(DeviceNo,Ep, EpDir);	
    
	return 0;
	
}

static int		
SEUSB11_UnstallEp(uint8 DeviceNo,uint8 Ep, uint8 EpDir)
{

	uint8 Value8;
	uint16 Reg8;
	uint8  togctl_value;
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_UnstallEp(): is called for device %d endpoint %d\n",
										DeviceNo,Ep);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Lets clear tagctl for EP*/
	togctl_value = 0;
	togctl_value |= Ep;
	if(EpDir == DIR_IN)
	   togctl_value |= BIT4;
	
	/* Clear toggle bit  S=0 R=1*/
	togctl_value &= ~SEUSB11_TOGCTL_S_BIT;
	togctl_value |= SEUSB11_TOGCTL_R_BIT;

   	SEUSB11_WriteReg8(DeviceNo,SEUSB11_TOGCTL, togctl_value  ); 

	if (Ep == 0)
	{
		
		Value8 = SEUSB11_ReadReg8(DeviceNo,SEUSB11_EP0CS);
		Value8 |= SEUSB11_HSNAK ;
		Value8 &=  ~SEUSB11_EP0STALL;
		SEUSB11_WriteReg8(DeviceNo,SEUSB11_EP0CS, Value8 );
		
	}
	else
	{
		if (EpDir == DIR_IN)
		{
			Reg8 = (SEUSB11_IN1CS + ((Ep-1)*8));
			Value8 = SEUSB11_ReadReg8(DeviceNo, Reg8);
			Value8 &= ~SEUSB11_INXSTL;
			SEUSB11_WriteReg8(DeviceNo,Reg8,Value8 );
		}
				
		else if (EpDir == DIR_OUT)
		{
			
			Reg8 = (SEUSB11_OUT1CS + ((Ep-1)*8));
			
			Value8 = SEUSB11_ReadReg8(DeviceNo,Reg8  );
			Value8 &= ~SEUSB11_OUTXSTL;
			SEUSB11_WriteReg8(DeviceNo,Reg8, Value8 );

		}

	}

	/* Call Core Handler */
	UsbCoreFunc.CoreUsbUnHaltHandler(DeviceNo,Ep, EpDir);	
	return 0;

}


static void
SEUSB11_CompleteRequest(uint8 DeviceNo,uint8 status,DEVICE_REQUEST *Req)
{
	uint8 Ctrl;
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_CompleteRequest(): is called for device %d with status %d\n",
										DeviceNo,status);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	if (status == 1 )	/* Error */
	{
		TWARN("SEUSB11_CompleteRequest: Sending Error Status\n");
		SEUSB11_StallEp(DeviceNo,0,DIR_IN);
		return;
	}
	/* Clear HSNAK */
	Ctrl = SEUSB11_ReadReg8(DeviceNo,SEUSB11_EP0CS);
	Ctrl |= SEUSB11_HSNAK;
	SEUSB11_WriteReg8(DeviceNo,SEUSB11_EP0CS ,Ctrl);
	return;
}

/*
   Why we are using SoftConnect:
   When the USB cable is already connected to host and when we initialize
   SEUSB11, the host has no way to detect the change in the configuration
   and will not find the devices exposed by SEUSB11. Disabling and Enabling
   Softconnect simulates removal and insertion of cable, which the host USB
   software uses to reconfigure the devices.
 */

static void		
SEUSB11_DeviceDisconnect(uint8 DeviceNo)
{
	uint8 Index;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_DeviceDisconnect() is called for device %d\n", DeviceNo);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	/* Disconnect Device from Host */
	SEUSB11_SoftDisconnect(DeviceNo);

	return;
}

static void
SEUSB11_DeviceReconnect (uint8 DeviceNo)
{
	uint8 Index;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_DeviceReconnect() is called for device %d\n", DeviceNo);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	/* Connect Device to Bus */
	SEUSB11_SoftConnect(DeviceNo);

	UsbCoreFunc.CoreUsb_OsDelay(1);

	return;
}

static void	 	
SEUSB11_RemoteWakeup (uint8 DeviceNo)
{
	uint8 Reg8;
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_RemoteWakeup() is called for device %d\n", DeviceNo);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	// Set sigrsume bit (usbcs bit 0) for atlest 1 ms to max 15msec
	Reg8 = SEUSB11_ReadReg8(DeviceNo,SEUSB11_USBCS);
	Reg8 |= SEUSB11_SIGRESUME;
	SEUSB11_WriteReg8(DeviceNo, SEUSB11_USBCS ,Reg8);

	//remote wakeup delay has been increased to 10Ms from 2Ms in order to wakeup device and make it work for kernel2 and kernel 3
	UsbCoreFunc.CoreUsb_OsDelay(10);

	// Clear sigrsume bit
	Reg8 = SEUSB11_ReadReg8(DeviceNo,SEUSB11_USBCS);
	Reg8 &= ~SEUSB11_SIGRESUME;
	SEUSB11_WriteReg8(DeviceNo, SEUSB11_USBCS ,Reg8);

	return;
}

static int
SEUSB11_GetEps (uint8 DeviceNo, uint8* pdata, uint8* num_eps)
{
	uint8 Index;
	uint8 *EpData;
	uint8 Eps;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0,"SEUSB11_GetEps() is called for device %d\n", DeviceNo);

	Index = SEUSB11_ConvertDeviceToIndex (DeviceNo);
	if (Index == 0xFF)
	{
		TCRIT ("SEUSB11_GetEps(): Error Unable the read Endpoint Information for Device %d\n", DeviceNo);
		return -1;
	}
	EpData = (uint8*) ep_config[Index];
	Eps = 0;

	while (Eps < 16)
	{
		if (EpData[0] == 0xFF) break;
		memcpy (pdata, EpData, sizeof(ep_config_t));
		Eps++;
		EpData += sizeof (ep_config_t);
		pdata += sizeof (ep_config_t);
	}
	*num_eps = Eps;
	TDBG_FLAGGED(usbhw11, DEBUG_HW0, "Number of EndPoints = %d\n", *num_eps);
    return 0;
}


static int
UsbDeferProcess0 (void *arg)
{

	TDBG_FLAGGED(usbhw11, DEBUG_HW0_DPC,"UsbDeferProcess0() is called for device %d with action %ld\n",
											kthread_data_usb[0].device_num, kthread_data_usb[0].action);

	while (kthread_data_usb[0].action)
	{
		if (test_and_clear_bit (ACTION_USB_BUS_SUSPEND, &kthread_data_usb[0].action))
		{
			SEUSB11_BusSuspend(kthread_data_usb[0].device_num);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_BUS_RESET, &kthread_data_usb[0].action))
		{
			SEUSB11_BusReset(kthread_data_usb[0].device_num);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_SETUP_PACKET, &kthread_data_usb[0].action))
		{
			SEUSB11_EP0_RxDone(kthread_data_usb[0].device_num,SETUP_PKT);
			SEUSB11_WriteReg8(kthread_data_usb[0].device_num,SEUSB11_OUT0BC,0x0);	/* This should enable OUT0 to receive data */
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_EP0_TX_DONE, &kthread_data_usb[0].action))
		{
			SEUSB11_EP0_TxDone(kthread_data_usb[0].device_num);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_EP0_RX_DONE, &kthread_data_usb[0].action))
		{
			SEUSB11_EP0_RxDone(kthread_data_usb[0].device_num,NOT_SETUP_PKT);
			continue;
		}
	}
    return 0;
}

/* The following code can be taken as it is when porting to other SOCs */

static int
DPCDispatchThread(void *proc)
{
	DPC DPCprocess = NULL;
	int Inst;
	DPC_T *pDPC = (DPC_T*) proc;

#if(LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
	char ProcessName [100];
#endif

	TDBG_FLAGGED(usbhw11, DEBUG_HW0_DPC,"DPCDispatchThread() is called\n");
	Inst = pDPC->Inst;
	DPCprocess = (DPC)pDPC->DPC;

#if(LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
	/* Put in background and release our user space connections */
	sprintf (ProcessName, "%s%d",HW_MODULE_DPC_NAME, Inst);
	daemonize (ProcessName);
#endif

	/* Notify caller that DPC is Initialized */
	complete(&DPC_Init[Inst]);

	/* Main DPC Processing starts here */
    while (DPCThreadRunning[Inst])	
	{	
		wait_for_completion_interruptible(&DPC_Semaphore[Inst]);
		if (!DPCThreadRunning[Inst])
		{
			TWARN("Terminating Kernel Thread\n");
			break;
		}
		if (DPCprocess)
		{
			(*DPCprocess)(NULL);
		}
		else
			TCRIT("WARNING: DPC Process is NULL\n");

	}
	complete(&DPC_Exited[Inst]);
	return 0;
}

static void
DPCStarter(void *proc)
{
	/* Start the local kernel thread which dispatches our DPC */
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
	kernel_thread(DPCDispatchThread, proc,0);
#else
  kthread_run(DPCDispatchThread,proc,"USB0DPC");
#endif
}

static void
Usb_OsInitDPC(DPC_T *pDPC)
{
	int Inst = pDPC->Inst;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0_DPC,"Usb_OsInitDPC() is called\n");
	/* Start the Thread */
	DPCStarter((void *)pDPC);

	/* Wait for DPC to Initialize */
	wait_for_completion_interruptible(&DPC_Init[Inst]);
	
	return;
}

static void
Usb_OsWakeupDPC(uint8 DeviceNo)
{
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0_DPC,"Usb_OsWakeupDPC() is called for device %d\n",DeviceNo);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	complete(&DPC_Semaphore[Index]);
	return;
}

static void
Usb_OsKillDPC(uint8 DeviceNo)
{
	uint8 Index;

	TDBG_FLAGGED(usbhw11, DEBUG_HW0_DPC,"Usb_OsKillDPC() is called for device %d\n",DeviceNo);
	/* Get Device Index */
	Index = SEUSB11_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	DPCThreadRunning [Index] = 0;

	complete(&DPC_Semaphore[Index]);

	/* Wait for DPCDispatchThread to exit */
	wait_for_completion_interruptible(&DPC_Exited[Index]);
	return;
}

module_init(init_SEUSB11_module);
module_exit(exit_SEUSB11_module);

MODULE_AUTHOR("Rama Rao Bisa <RamaB@ami.com>");
MODULE_DESCRIPTION("PILOT-II hardware driver module for USB1.1 Controller");
MODULE_LICENSE("GPL");

