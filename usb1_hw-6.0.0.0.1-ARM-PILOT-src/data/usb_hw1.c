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

#include<linux/version.h>
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,14,17))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>	
#include <asm/io.h>
#include <asm/dma.h>
#include <asm/cacheflush.h>
#include <mach/platform.h>
#include <asm/tlbflush.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <coreTypes.h>
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
#include <linux/kthread.h>
#endif
#include "board.h"
#include "seusb20.h"
#include "usb_hw.h"
#include "usb_core.h"
#include "helper.h"
#include "dbgout.h"
#include "mod_reg.h"
#include "reset.h"

/* Global Variables */
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
DEFINE_SEMAPHORE (DMAWait);
#else
DECLARE_MUTEX (DMAWait);
#endif

/*Local Types */
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

#define HW_MODULE_DPC_NAME 	"USB20_DPC"

/* Global Variables */
TDBG_DECLARE_DBGVAR(usbhw20);


/* Internal Functions. Not Exposed outside */
static int  SEUSB20_RegisterIsr		(uint8 DevNo);
static void SEUSB20_Dma(uint8 DeviceNo,uint8 ep, uint8 *Buffer, uint16 Len);
static int  SEUSB20_InitHardware(uint8 DeviceNo);

/* Functions Exposed via USB_HW structure */
static void		SEUSB20_Enable			(uint8 DeviceNo, uint8 Speed);
static void		SEUSB20_Disable			(uint8 DeviceNo, uint8 Speed);
static int 	SEUSB20_IntrHandler		(uint8 DeviceNo,uint8 ep);
static void		SEUSB20_SetAddress		(uint8 DeviceNo,uint8 Addr, uint8 Enable);
static void		SEUSB20_GetAddress		(uint8 DeviceNo,uint8 *Addr, uint8 *Enable);
static int 		SEUSB20_WriteFifoBuffer (uint8 DeviceNo,uint8 ep, uint8 *Buffer, uint16 Len);
static int 		SEUSB20_ReadFifoBuffer	(uint8 DeviceNo,uint8 ep, uint8 *Buffer,uint16 *Len, uint8 Setup);
static int		SEUSB20_AllocBuffer		(uint8 DeviceNo,uint8 ep, uint16 Fifo, uint16 MaxPkt, uint8 Dir, uint8 Type);
static int		SEUSB20_EnableEp		(uint8 DeviceNo,uint8 ep, uint8 EpDir, uint8 EpType);
static int		SEUSB20_DisableEp		(uint8 DeviceNo,uint8 ep, uint8 EpDir, uint8 EpType);
static int		SEUSB20_GetEpStatus		(uint8 DeviceNo,uint8 ep, uint8 EpDir, uint8 *Enable, uint8 *Stall);
static int		SEUSB20_StallEp			(uint8 DeviceNo,uint8 ep, uint8 EpDir);
static int		SEUSB20_UnstallEp		(uint8 DeviceNo,uint8 ep, uint8 EpDir);
static void		SEUSB20_DeviceDisconnect(uint8 DevNo);
static void		SEUSB20_DeviceReconnect (uint8 DevNo);
static void    	SEUSB20_CompleteRequest	(uint8 DevNo,uint8 status,DEVICE_REQUEST *Req);
static int 		SEUSB20_GetEps (uint8 DevNo, uint8* pdata, uint8* num_eps);
static int      SEUSB20_Init(uint8 DeviceNo, USB_HW *UsbDev, void **DevConf);
static void     SEUSB20_Exit(uint8 DeviceNo);
static uint8    SEUSB20_ReadReg8(uint8 DeviceNo, uint16 RegNo);
static void     SEUSB20_WriteReg8(uint8 DeviceNo, uint16 RegNo,uint8 Data);
static void     SEUSB20_SoftConnect(uint8 DeviceNo);
static void     ClearIntrStatus(uint8 DeviceNo,uint16 RegNo, uint8 BitNo);
static int 		notify_reboot_instance0(struct notifier_block *this, unsigned long code, void *unused);

static int      UsbDeferProcess0(void *arg);
static void     Usb_OsInitDPC(DPC_T *pDPC);
static void     Usb_OsKillDPC(uint8 DeviceNo);
static void     Usb_OsWakeupDPC(uint8 DeviceNo);
static int SEUSB20_Upstream_enable(uint8 DeviceNo);
static int SEUSB20_Upstream_disable(uint8 DeviceNo);
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
int IsUsb_Upstream_host = 0;
#endif
/* Static Variables */
static char *ModuleName = "P2USB20";
static int NumInst = 1;

static struct completion DPC_Semaphore[MAX_NUM_SEUSB20];
static struct completion DPC_Init[MAX_NUM_SEUSB20];
static struct completion DPC_Exited[MAX_NUM_SEUSB20];

static DPC_T DPCInst [MAX_NUM_SEUSB20] = {
											{
												UsbDeferProcess0,
												0,
											},
                                         };

static kthread_data_usb_t	kthread_data_usb [MAX_NUM_SEUSB20] = {
															{ 0, 0},
														 };
static int DPCThreadRunning [MAX_NUM_SEUSB20] = { 1 };

static uint32*	dmaVirtAddr_write [MAX_NUM_SEUSB20];

static USB_CORE UsbCoreFunc;
static dma_data_t  dma_data [MAX_NUM_SEUSB20] = { 
													{ 0 },
												};
static dma_addr_t dmaPhyAddr_write [MAX_NUM_SEUSB20];

static int InitDone[MAX_NUM_SEUSB20] = { 0 };

static struct notifier_block seusb20_notifier [MAX_NUM_SEUSB20] =
{
       { .notifier_call = notify_reboot_instance0,},
};

static int SEUSB20_HostResetHandler0 (void);
static HostResetHandler SEUSB20_HostResetHandlers[MAX_NUM_SEUSB20] = {
												SEUSB20_HostResetHandler0,
											};
/* SEUSB20 Hardware functions structure */
static USB_HW SEUSB20_Dev =
{
	.UsbHwEnable 						= SEUSB20_Enable,
	.UsbHwDisable						= SEUSB20_Disable,
	.UsbHwIntr							= SEUSB20_IntrHandler,
	.UsbHwSetAddr						= SEUSB20_SetAddress,
	.UsbHwGetAddr						= SEUSB20_GetAddress,
	.UsbHwWrite							= SEUSB20_WriteFifoBuffer,
	.UsbTxComplete						= NULL,
	.UsbHwRead							= SEUSB20_ReadFifoBuffer,
	.UsbHwAllocBuffer					= SEUSB20_AllocBuffer,
	.UsbHwEnableEp						= SEUSB20_EnableEp,
	.UsbHwDisableEp						= SEUSB20_DisableEp,
	.UsbHwGetEpStatus					= SEUSB20_GetEpStatus,
	.UsbHwStallEp						= SEUSB20_StallEp,
	.UsbHwUnstallEp						= SEUSB20_UnstallEp,
	.UsbHwSetRemoteWakeup				= NULL,
	.UsbHwGetRemoteWakeup				= NULL,
 	.UsbHwCompleteRequest				= SEUSB20_CompleteRequest,
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	.UsbHwUpstreamDisable		= SEUSB20_Upstream_disable,
	.UsbHwUpstreamEnable		= SEUSB20_Upstream_enable,
#endif
	//enable/disable USB 2.0
	.UsbHwUpstreamMediaDisable			= SEUSB20_Upstream_disable,
	.UsbHwUpstreamMediaEnable			= SEUSB20_Upstream_enable,
	.UsbHwDeviceDisconnect				= SEUSB20_DeviceDisconnect,
	.UsbHwDeviceReconnect				= SEUSB20_DeviceReconnect,
 	.UsbHwRemoteWakeup					= NULL,
	.UsbHwGetEps						= SEUSB20_GetEps,
	.NumEndpoints						= SEUSB20_MAX_EP * 2,				/* Useable Endpoints */
	.BigEndian							= 0,							/* Little Endian Device */
    .WriteFifoLock						= 0,                          /* Lock WriteFifo routine , so that write to fifo are not interleaved */
	.EP0Size							= SEUSB20_EP0_SIZE,
	.SupportedSpeeds					= (SUPPORT_FULL_SPEED | SUPPORT_HIGH_SPEED),
};

static usb_ctrl_driver seusb20_driver [MAX_NUM_SEUSB20] =
{
	{
		.module 			= THIS_MODULE,
		.usb_driver_init	= SEUSB20_Init,
		.usb_driver_exit	= SEUSB20_Exit,
		.devnum         	= 0xFF,
	},
};

/* SEUSB20 Device Configuration Struture */
static USB_DEV_CONF SEUSB20_Conf[MAX_NUM_SEUSB20] =
{
	{
		SEUSB20_0_CMD_REG,
		0,
		0xFF,				/* 0xFF = Unconfigured/Not Used */
		0,
		SEUSB20_0_BIG_ENDIAN_DEVICE,
		1,
		{
			{SEUSB20_0_INT_PRI,
			SEUSB20_0_INTR,
			SEUSB20_0_VECTOR,
			IRQF_DISABLED}
			
		}
	}
};
static int SetAddrEnable [MAX_NUM_SEUSB20] = { 0 };
static uint8 SEUSB20_BusState[MAX_NUM_SEUSB20] = {1};
static char* SEUSB20_IrqName[MAX_NUM_SEUSB20] ={"SEUSB20"};
static ep_config_t	ep_config [MAX_NUM_SEUSB20][16] = 
					{
						{
							{ 1,	IN,		64,		CONFIGURABLE,	 0 },
							{ 1,	OUT,	64,		CONFIGURABLE,	 0 },
							{ 2,	IN, 	64,		CONFIGURABLE,	 0 },
							{ 2,	OUT, 	64,		CONFIGURABLE,	 0 },
							{ 3,	IN, 	512,	CONFIGURABLE,	 0 },
							{ 3,	OUT,	256,	CONFIGURABLE,	 0 },
							{ 4,	OUT,	512,	CONFIGURABLE,	 0 },
							{ 4,	IN, 	512,	CONFIGURABLE,	 0 },
							{ 0xFF, 0xFF,   0xFF,   0xFF,          0xFF},  
						},
					};


static struct ctl_table_header *my_sys 	= NULL;
static struct ctl_table UsbHw1ctlTable [] =
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
	{CTL_UNNUMBERED,"DebugLevel",&(TDBG_FORM_VAR_NAME(usbhw20)),sizeof(int),0644,NULL,NULL,&proc_dointvec}, 
#else
	{"DebugLevel",&(TDBG_FORM_VAR_NAME(usbhw20)),sizeof(int),0644,NULL,&proc_dointvec}, 
#endif
	{0} 
};

module_param (ModuleName, charp, S_IRUGO);
MODULE_PARM_DESC(ModuleName, "Pilot-II hardware module name");
module_param (NumInst, int, S_IRUGO);
MODULE_PARM_DESC(NumInst, "Instances of Pilot-II USB20 hardware module");

static int SEUSB20_Upstream_enable(uint8 DevNo)
{
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	IsUsb_Upstream_host = 1;
#endif
	SEUSB20_DeviceReconnect(DevNo);
	return 0;
}

static int SEUSB20_Upstream_disable(uint8 DevNo)
{	
	SEUSB20_DeviceDisconnect(DevNo);
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	IsUsb_Upstream_host = 0;
#endif
	return 0;
}

static uint8  
SEUSB20_ConvertDeviceToIndex(uint8 DeviceNo)
{
	uint8 Index;
	for (Index = 0;Index < MAX_NUM_SEUSB20;Index++) 
	{
		if (SEUSB20_Conf[Index].DeviceNo == DeviceNo)
			return Index;
	}
	TCRIT("FATAL: Internal Error : SEUSB20_ConvertDeviceToIndex() Failed for Device No 0x%x\n",DeviceNo); 
	return 0xFF;	
}


static void 
SEUSB20_FIFOReset(uint8 DeviceNo , uint8 Dir,uint8 Ep)
{
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, Dir|Ep);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, SEUSB20_FIFORST | Dir | Ep);
	return;
}

static void 
SEUSB20_Toggle(uint8 DeviceNo , uint8 Dir,uint8 Ep)
{

	SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, Dir|Ep);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, SEUSB20_TOGRST| Dir | Ep);
	return;
}

static void 
SEUSB20_FIFOReset_All(uint8 DeviceNo)
{
	int i;
	for(i=0;i<5;i++)
		SEUSB20_FIFOReset( DeviceNo , SEUSB20_DIR_IN, i);
	for(i=0;i<5;i++)
		SEUSB20_FIFOReset( DeviceNo , SEUSB20_DIR_OUT, i);
	return;  
}

static void 
SEUSB20_Toggle_All(uint8 DeviceNo)
{
	int i;
	for(i=0;i<5;i++)
		SEUSB20_Toggle( DeviceNo , SEUSB20_DIR_IN, i);
	for(i=0;i<5;i++)
		SEUSB20_Toggle( DeviceNo , SEUSB20_DIR_OUT, i);
	return;  
}

static int
SEUSB20_HostResetHandler(uint8 DeviceNo)
{
	uint8 Index;
	uint8 Speed;

	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 0;

	Speed = SEUSB20_Conf[Index].DevSpeed;

	//printk("Reinitializing USB Hardware on Device %d\n",DeviceNo);
	kthread_data_usb[Index].action = 0;
	SEUSB20_InitHardware(DeviceNo);
	SEUSB20_Enable(DeviceNo, Speed);
	if  (UsbCoreFunc.CoreUsbGetDeviceConnectState( DeviceNo))
	{
		SEUSB20_SoftConnect(DeviceNo);	
		TDBG_FLAGGED(usbhw20, DEBUG_HW1,"Device %d is connected\n",DeviceNo);
	}
	else
		TDBG_FLAGGED(usbhw20, DEBUG_HW1,"Device %d is not connected (AutoAttach Mode)\n",DeviceNo);

	return 0;

}

static int 
SEUSB20_HostResetHandler0(void)
{
	uint8 DeviceNo;

	if (SEUSB20_Conf[0].DeviceNo == 0xFF)
		return 1;

	DeviceNo = SEUSB20_Conf[0].DeviceNo;
	return SEUSB20_HostResetHandler (DeviceNo);
}

static uint8 
SEUSB20_ReadReg8(uint8 DeviceNo, uint16 RegNo)
{ 
	uint8 Index;
	
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return 0;
		
	return READ_SEUSB20_BYTE(SEUSB20_Conf[Index].DataReg+RegNo);
}

static void 
SEUSB20_WriteReg8(uint8 DeviceNo, uint16 RegNo,uint8 Data)
{ 
	uint8 Index;
	
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;

	WRITE_SEUSB20_BYTE(SEUSB20_Conf[Index].DataReg+RegNo,Data);
	return;
}

static void 
SEUSB20_WriteReg32(uint8 DeviceNo, uint16 RegNo,uint32 Data)
{ 
	uint8 Index;
	
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;

	WRITE_SEUSB20_DWORD(SEUSB20_Conf[Index].DataReg+RegNo,Data);
	return;
}

static void 
SEUSB20_SoftConnect(uint8 DeviceNo)
{
	uint8 Reg8;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	Reg8 = SEUSB20_ReadReg8(DeviceNo,SEUSB20_USBCS );
	Reg8 &= ~(SEUSB20_DISCON);
	SEUSB20_WriteReg8(DeviceNo, SEUSB20_USBCS  ,Reg8);
	return;
}

static void 
SEUSB20_SoftDisconnect(uint8 DeviceNo)
{
	uint8 Reg8;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	Reg8 = SEUSB20_ReadReg8(DeviceNo,SEUSB20_USBCS );
	Reg8 |= (SEUSB20_DISCON);
	SEUSB20_WriteReg8(DeviceNo, SEUSB20_USBCS  ,Reg8);
	return;
}

static int
SEUSB20_RegisterIsr(uint8 DevNo)
{
	uint8 Index;

	Index = SEUSB20_ConvertDeviceToIndex(DevNo);
	if (Index == 0xFF)
	{
		TCRIT("Error in Registering IRQ for Device %d\n",DevNo);
		return 1;
	}
	if (UsbCoreFunc.CoreUsb_OsRegisterIsr(SEUSB20_IrqName[Index],SEUSB20_Conf[Index].IntrConf[0].Vector,
							DevNo,0, SEUSB20_Conf[Index].IntrConf[0].IntrFlags) != 0)
	{
		TCRIT("Error in Registering IRQ for Device %d\n",DevNo);
		return 1;
	}

	return 0;
}

static int
SEUSB20_UnregisterIsr(uint8 DevNo)
{

	uint8 Index;

	Index = SEUSB20_ConvertDeviceToIndex(DevNo);
	if (Index == 0xFF)
	{
		TCRIT("Error in UnRegistering IRQ for Device %d\n",DevNo);
		return 1;
	}

	UsbCoreFunc.CoreUsb_OsUnregisterIsr(SEUSB20_IrqName[Index],SEUSB20_Conf[Index].IntrConf[0].Vector,
							DevNo,0, SEUSB20_Conf[Index].IntrConf[0].IntrFlags); 
	return 0;
}

static void
SEUSB20_Exit(uint8 DeviceNo)
{
	uint8 Reg8 , i;
	uint8 Index;


	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"Unloading SEUSB20 Hardware Layer\n");


	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
	{
		return;
	}
   	/* Stop Deffered Interrupt Processing */
	Usb_OsKillDPC(DeviceNo);
	uninstall_reset_handler(SEUSB20_HostResetHandlers[Index]);

	/* Disable all Interrupts */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBINTS_EN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBIEN,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IEN,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07PNGIEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07FULLIEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07EMPTIEN ,SEUSB20_INT_DISABLE_ALL);


	/* Disable all endpoits */
	
	for (i=0;i<SEUSB20_MAX_EP ;i++)
	{	
		Reg8 = SEUSB20_ReadReg8(DeviceNo,(SEUSB20_IN1CON + (i*8)) );
		Reg8 &= ~(SEUSB20_VAL);
		SEUSB20_WriteReg8(DeviceNo,(SEUSB20_IN1CON + (i*8)),Reg8 );

		Reg8 = SEUSB20_ReadReg8(DeviceNo,(SEUSB20_OUT1CON + (i*8)) );
		Reg8 &= ~(SEUSB20_VAL);
		SEUSB20_WriteReg8(DeviceNo,(SEUSB20_OUT1CON + (i*8)),Reg8 );

	}

	iounmap((void *)SEUSB20_Conf[0].DataReg);
	dma_free_coherent(NULL,0x1000,dmaVirtAddr_write[Index],dmaPhyAddr_write[Index]);

    unregister_reboot_notifier(&seusb20_notifier[Index]);
	return;
}

static int  
SEUSB20_InitHardware(uint8 DeviceNo)
{

	uint8 Index;

	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

#if defined (SOC_PILOT_III)
	*(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+0x54) |= (USB2_DMA_EN);
#endif

	SEUSB20_SoftDisconnect(DeviceNo);

	/* Disable all Interrupts */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBINTS_EN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBIEN,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IEN,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07PNGIEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07FULLIEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07EMPTIEN ,SEUSB20_INT_DISABLE_ALL);

 	/* Clear any pending Interrupts */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBINTS_ST ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBIRQ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IRQ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07PNGIRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07FULLIRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07EMPTIRQ ,SEUSB20_INT_CLEAR_ALL);

	return 0;
}

static int
SEUSB20_Init(uint8 DeviceNo, USB_HW *UsbDev, void **DevConf)
{
	static int Num_SEUSB20_Devices = 0;
	uint32 seusb20_base;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_Init: Called for SEUSB20 Device %d with DeviceNo %d\n",
				Num_SEUSB20_Devices,DeviceNo);

	*DevConf = NULL;

	if (Num_SEUSB20_Devices > MAX_NUM_SEUSB20)
	{
		TCRIT("SEUSB20_Init():ERROR:SEUSB20 Devices Exceding MAX_NUM_SEUSB20");
		return 1;
	}

	seusb20_base  = (unsigned long)ioremap(SE_USB_1_BASE,0x100000);

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20(%d):Mapped Base is 0x%x\n",DeviceNo,seusb20_base);
	/* backward compatibility with Pilot3 */
#if defined SOC_PILOT_IV
	*((volatile unsigned short *)(seusb20_base + 0x1E6)) = 0x200;
	*((volatile unsigned short *)(seusb20_base + 0x1E8)) = 0x200;
	*((volatile unsigned short *)(seusb20_base + 0x3E6)) = 0x200;
	*((volatile unsigned short *)(seusb20_base + 0x3E8)) = 0x200;
	*((volatile unsigned short *)(seusb20_base + 0x30C)) = 0x200;
	*((volatile unsigned short *)(seusb20_base + 0x310)) = 0x200;
	*((volatile unsigned short *)(seusb20_base + 0x34C)) = 0x200;
	*((volatile unsigned short *)(seusb20_base + 0x350)) = 0x200;
#endif 
	
	SEUSB20_Conf[Num_SEUSB20_Devices].DataReg	= seusb20_base;

	/*
       Initialize Device Number in Device Conf Structure. This should
       be done here as all SEUSB20 Access Functions uses the function
       SEUSB20_ConvertDeviceToIndex() funciton which internally uses this
       structure value
    */                                 
	SEUSB20_Conf[Num_SEUSB20_Devices].DeviceNo = DeviceNo;

    dmaVirtAddr_write [Num_SEUSB20_Devices] =(uint32*)dma_alloc_coherent(NULL,0x1000, &dmaPhyAddr_write[Num_SEUSB20_Devices] , GFP_DMA|GFP_KERNEL);
	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"DMA Physical Reg = %x \n", (uint32)dmaPhyAddr_write [Num_SEUSB20_Devices]);
	if (dmaVirtAddr_write [Num_SEUSB20_Devices] == NULL)
	{
		TCRIT(" Error in allocation dmaVirtAddr \n ");
		return 1 ;
	}
	SEUSB20_InitHardware(DeviceNo);

	/* Fill in the USB_HW Structure */
	memcpy(UsbDev,&SEUSB20_Dev,sizeof(USB_HW));

	/* Save Device Specific Info to Core */
	*DevConf = &SEUSB20_Conf[Num_SEUSB20_Devices];

	/* Wait for 1 seconds for the host to detect soft removal */
	if (!UsbCoreFunc.CoreUsbGlobalConnectSupport)
	 	UsbCoreFunc.CoreUsb_OsDelay(1000);

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_Init: Device %d Initialized with DeviceNo %d\n",
						Num_SEUSB20_Devices,DeviceNo);

	install_reset_handler(SEUSB20_HostResetHandlers[Num_SEUSB20_Devices]);

        /* Initialize Deffered Interrupt Processing */
	init_completion (&DPC_Semaphore[Num_SEUSB20_Devices]);
	init_completion (&DPC_Init[Num_SEUSB20_Devices]);
	init_completion (&DPC_Exited[Num_SEUSB20_Devices]);

	Usb_OsInitDPC(&DPCInst [Num_SEUSB20_Devices]);

	register_reboot_notifier(&seusb20_notifier[Num_SEUSB20_Devices]);

	/* Increment Number of SEUSB20 Devices */
	Num_SEUSB20_Devices++;

	return 0;
}

static int
IsUsbCableConnected(uint8 DeviceNo)
{
	return 1;
}

static int
SEUSB20_BusSuspend(uint8 DeviceNo)
{
	uint8 Index;

	if (IsUsbCableConnected(DeviceNo) == 0)
	{
		UsbCoreFunc.CoreUsbBusDisconnect(DeviceNo);
		return 1;
	}

	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return 0;

	/* Set we are Suspended */
	SEUSB20_BusState[Index] = 0;

	/* Call core routines */
	UsbCoreFunc.CoreUsbBusSuspend(DeviceNo);

	return 0;
}

static void
SEUSB20_BusReset(uint8 DeviceNo)	
{
	uint8 Index;
	uint8 Status;


	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;

	    /* Clear any pending Interrupts */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IRQ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07PNGIRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07FULLIRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07EMPTIRQ ,SEUSB20_INT_CLEAR_ALL);

	/* Enable Endpoint zero if needed*/

	/* Enable EP0 Interrupt */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IEN,0x1);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IEN ,0x1);

	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBIEN,
					  SEUSB20_SUDAVIE |SEUSB20_SUSPIE|SEUSB20_URESIE| SEUSB20_HSPIE);  


	Status = SEUSB20_ReadReg8(DeviceNo,SEUSB20_USBINTS_EN  );
	Status |= (SEUSB20_WUINT|SEUSB20_USBINT);
	SEUSB20_WriteReg8(DeviceNo, SEUSB20_USBINTS_EN ,Status);

	SEUSB20_FIFOReset_All(DeviceNo);
	SEUSB20_Toggle_All(DeviceNo);

	/* Call Core Routine */
	UsbCoreFunc.CoreUsbBusReset(DeviceNo);	
	return;
}

static void
SEUSB20_EP0_RxDone(uint8 DeviceNo, int Setup)
{
	uint16 Len;
	uint8 *RxBuffer=NULL;
	uint8 Index;

	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;

	if (!Setup)
		TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20(%d): Non Setup Packet Recevied in EP 0\n",DeviceNo);
		
	/* Get Endpoint 0 Buffer */
	RxBuffer = UsbCoreFunc.CoreUsbGetRxData(DeviceNo,0);

	/* Read the FIFO of Endpoint 0 OUT */
	SEUSB20_ReadFifoBuffer(DeviceNo,0,RxBuffer,&Len,Setup);
	
	/* Write Read Len to Endpoint 0 RxDataLen */
	UsbCoreFunc.CoreUsbSetRxDataLen(DeviceNo,0,Len);

	/* Call Core Routine */
	UsbCoreFunc.CoreUsbRxHandler0(DeviceNo,RxBuffer,(uint32)Len,Setup);
	
	return;	
}

static void
SEUSB20_EP0_TxDone(uint8 DeviceNo)		
{

	/* Call Core Routine */
	UsbCoreFunc.CoreUsbTxHandler(DeviceNo,0);
	
	return;
}

static void
SEUSB20_Tx_Intr(uint8 DeviceNo, uint8 ep)
{

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"Tx Intr for Device %d Ep %d\n",DeviceNo,ep);
	UsbCoreFunc.CoreUsbTxHandler(DeviceNo,ep);		
	return;
}


static void
SEUSB20_Rx_Intr(uint8 DeviceNo, uint8 ep)
{

	uint8* RxData;
	uint16 Len;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"Rx Intr for Device %d Ep %d\n",DeviceNo,ep);
	/* It is an OUT Endpoint , Get Endpoints RxData*/
	RxData = UsbCoreFunc.CoreUsbGetRxData(DeviceNo,ep);
	
	/* Read the Fifo to Endpoint's RxData and Set RxDataLen*/
	SEUSB20_ReadFifoBuffer(DeviceNo,ep,RxData,&Len,0);
	UsbCoreFunc.CoreUsbSetRxDataLen(DeviceNo,ep,Len);
	
	/* Call Core Rx Handler*/
	UsbCoreFunc.CoreUsbRxHandler(DeviceNo,ep);
	
	return;
}	
 
static void SEUSB20_Dma(uint8 DeviceNo,uint8 ep , uint8 *Buffer, uint16 Len)
{
    uint8  *Buffer8;
	uint32 TimeOut=0;
	uint32 RegTemp;
	uint32 Reg32 = 0;
	uint8 Index;
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF)
		return;
	 
	Buffer8  = (uint8*)dmaVirtAddr_write [Index];

	memcpy(&Buffer8[0] , &Buffer[0], Len);

    SEUSB20_WriteReg32(DeviceNo,SEUSB20_DMAADDR, dmaPhyAddr_write[Index]);

	RegTemp = (DMA_START|DMA_DIR_IN|(ep << 16)|Len);
	SEUSB20_WriteReg32(DeviceNo, SEUSB20_DMACTRL , RegTemp);

	Reg32 = *(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+ 0x68);
#if defined (SOC_PILOT_III)	
	while (!(Reg32 & 0x02000000)) 
#else
	while (!(Reg32 & 0x00000200)) 
#endif		
	{
  		TimeOut++;
	    if (TimeOut >=  0x0001000) 
		{    
			TCRIT("Error: Timeout , Didn't receive DMA complete \n"); 
			SEUSB20_FIFOReset(DeviceNo ,SEUSB20_DIR_IN ,ep);
			SEUSB20_Toggle(DeviceNo , SEUSB20_DIR_IN, ep);
			SEUSB20_WriteReg32(DeviceNo, SEUSB20_DMACTRL , 0x0);

			break;
		}

	Reg32 = *(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+ 0x68);
	}
	ClearIntrStatus(DeviceNo,SEUSB20_USBINTS_ST,SEUSB20_DMAINT);
	return;
}

static int 
init_SEUSB20_module (void)
{
	int i;
	int ret_val = 0;

	printk ("Loading usb-20 module...\n");

	if (0 != get_usb_core_funcs (&UsbCoreFunc))
	{
		TCRIT ("init_SEUSB20_module(): Unable to get Core Module Functions...\n");
		return -1;
	}

	for (i = 0; i < NumInst; i++)
	{
		if ((ModuleName != NULL) && (sizeof(seusb20_driver[i].name) > strlen(ModuleName)))
		{
			strcpy (seusb20_driver[i].name, ModuleName);
		}
		else
		{
			TCRIT ("init_SEUSB20_module(): Invalid Module Name (too big or NULL)\n");
			return -1;
		}

		ret_val = register_usb_chip_driver_module (&seusb20_driver[i]);
		switch (ret_val)
		{
		case -1:
			unregister_usb_chip_driver_module (&seusb20_driver[i]);
			break;
		}
	}
	if (!ret_val)
	{
		my_sys = AddSysctlTable("usbhw20",&UsbHw1ctlTable[0]);
	}
	else
	{
		TCRIT ("Error: SEUSB20 modules could not be registered with USB core\n");
	}
	
	return ret_val;

}

static void
exit_SEUSB20_module (void)
{
	int i;
	printk ("exiting usb-20 module\n");	
	for (i = 0; i < NumInst; i++)
		unregister_usb_chip_driver_module (&seusb20_driver[i]);

	if (my_sys) RemoveSysctlTable(my_sys);
	return;
}

static void
reboot_action(uint8 DeviceNo)
{
	uint8 Index;
	uint8 Speed;

	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	Speed = SEUSB20_Conf[Index].DevSpeed;
	SEUSB20_Disable (DeviceNo, Speed);
}

static int
notify_reboot_instance0(struct notifier_block *this, unsigned long code, void *unused)
{

	if (code == SYS_DOWN || code == SYS_HALT)
	{
		if (SEUSB20_Conf[0].DeviceNo != 0xFF)
			reboot_action (SEUSB20_Conf[0].DeviceNo);
	}
	return NOTIFY_DONE;
}

static void
ClearIntrStatus(uint8 DeviceNo,uint16 RegNo, uint8 BitNo)
{
	uint8 Value;
	Value = SEUSB20_ReadReg8(DeviceNo,RegNo);
    Value &= (BitNo);
	SEUSB20_WriteReg8(DeviceNo,RegNo,Value);
	return;
}

static void
SEUSB20_Enable(uint8 DeviceNo, uint8 Speed)
{
	uint8 Index;
	uint8 Status;
	
	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_Enable() is called...\n");
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	SEUSB20_Conf[Index].DevSpeed = Speed;

	/* Clear any pending Interrupts */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBINTS_ST ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBIRQ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IRQ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07PNGIRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07FULLIRQ ,SEUSB20_INT_CLEAR_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07EMPTIRQ ,SEUSB20_INT_CLEAR_ALL);

	if (!InitDone[Index])
	{
		/* Register the ISR */
		TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_Enable(): Before register ISR...\n");
		SEUSB20_RegisterIsr(DeviceNo);
		/* Set Intr Initialization Done */
		InitDone[Index] = 1;
	}

	/* Enable EP0 Interrupts */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IEN,SEUSB20_IN0IEN);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IEN ,SEUSB20_OUT0IEN);
	/* Enable reset,Suspend, Setup data valid and hispeed interrupts
		Don't enable SOF and Setup Token interrupts. */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBIEN,
					  SEUSB20_SUDAVIE |SEUSB20_SUSPIE|SEUSB20_URESIE| SEUSB20_HSPIE);  
	/* Enable USB interrupt and wakeup Interrupt */
	Status = SEUSB20_ReadReg8(DeviceNo,SEUSB20_USBINTS_EN  );
	Status |= (SEUSB20_WUINT|SEUSB20_USBINT);
	SEUSB20_WriteReg8(DeviceNo, SEUSB20_USBINTS_EN ,Status);

	return;
}

static void
SEUSB20_Disable(uint8 DeviceNo, uint8 Speed)
{
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_Disable() is called for Device %d\n", DeviceNo);
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;
	
	/* Disable all Interrupts */
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBINTS_EN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_USBIEN,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IEN,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07PNGIEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07FULLIEN ,SEUSB20_INT_DISABLE_ALL);
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07EMPTIEN ,SEUSB20_INT_DISABLE_ALL);

	/* Disconnect Device from Host */
	SEUSB20_SoftDisconnect(DeviceNo);
	
	if (InitDone[Index])
	{
		TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_Disable(): Freeing Interrupt for Device %d\n", DeviceNo);
		SEUSB20_UnregisterIsr(DeviceNo);
		/* Set Intr Initialization Done */
		InitDone[Index] = 0;
	}
	return;
}


static int
SEUSB20_IntrHandler(uint8 DeviceNo,uint8 InEp)
{
	uint8 IvecReg;
	uint8 Index,ep;
	uint8 USBIntrReg,Out07irq,IN07irq;
	//uint8 Out4cs,Out4con,Out4bcl,Out4bch;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"SEUSB20_IntrHandler(): Intr Entry -----------> for Device %d\n",DeviceNo);	

	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return IRQ_HANDLED;

	kthread_data_usb[Index].device_num = DeviceNo;

	USBIntrReg = SEUSB20_ReadReg8(DeviceNo,SEUSB20_USBINTS_ST );
	IvecReg = SEUSB20_ReadReg8(DeviceNo,SEUSB20_IVECT);
	SEUSB20_ReadReg8(DeviceNo,SEUSB20_USBIRQ);
	Out07irq  = SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT07IRQ);
	IN07irq =  SEUSB20_ReadReg8(DeviceNo,SEUSB20_IN07IRQ);
	SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT4CS);
	SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT4CON);
	SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT4BCL);
	SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT4BCH);


	TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"Int %x %x ",USBIntrReg ,IvecReg );
	TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"OUT07irq = %x ", SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT07IRQ));
	TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"IN07irq = %x ", SEUSB20_ReadReg8(DeviceNo,SEUSB20_IN07IRQ));
	//TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"USBIRQ = %x\n", USBirqReg);


	if (!(USBIntrReg & (SEUSB20_WUINT | SEUSB20_USBINT) ))
	{
#if 0
		TWARN("SEUSB20_IntrHandler(%d): Spurious Intr\n",DeviceNo);
#endif
		return IRQ_HANDLED;
	}

	if (USBIntrReg  & SEUSB20_WUINT )
	{
		TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"Wakeup \n");
		ClearIntrStatus(DeviceNo,SEUSB20_USBINTS_ST ,SEUSB20_WUINT);
	}

	if (USBIntrReg  & SEUSB20_USBINT )
	{
    
		IvecReg = (IvecReg >> 2);
		ClearIntrStatus(DeviceNo,SEUSB20_USBINTS_ST,SEUSB20_USBINT);

        /* Check for Spurious Interrupt */
		if (IvecReg > 0x14)
		{
			TWARN("SEUSB20_IntrHandler(%d): Spurious Intr\n",DeviceNo);
			return IRQ_HANDLED;
		}

		/************************ Check for Bus Conditions  ************************/
		if (IvecReg == SEUSB20_SUSPEND)
		{	
			ClearIntrStatus(DeviceNo,SEUSB20_USBIRQ,SEUSB20_SUSPIR);
		}
		
		if (IvecReg == SEUSB20_USBRESET	)
		{
			kthread_data_usb[Index].action = 0;
			if (test_and_set_bit (ACTION_USB_BUS_RESET, &kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB20_IntrHandler(%d): Bus Reset Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			ClearIntrStatus(DeviceNo,SEUSB20_USBIRQ,SEUSB20_URESIR);
		}

		if (IvecReg == SEUSB20_HSPEED)
		{
			if (test_and_set_bit (ACTION_USB_CONFIG_HISPEED, &kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB20_IntrHandler(%d): Hispeed Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			ClearIntrStatus(DeviceNo,SEUSB20_USBIRQ,SEUSB20_HSPEEDIR);
		}

		if (IvecReg == SEUSB20_SUDAV)
		{
			ClearIntrStatus(DeviceNo,SEUSB20_USBIRQ,(SEUSB20_SUDAVIR | SEUSB20_SUTOKIR));
			if (test_and_set_bit (ACTION_USB_SETUP_PACKET, &kthread_data_usb [Index].action))
			{
				TWARN ("SEUSB20_IntrHandler(%d): Setup Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
		}
		
		if ( IvecReg == SEUSB20_EP0IN)
		{
			ClearIntrStatus(DeviceNo,SEUSB20_IN07IRQ,SEUSB20_IN0IR);
			if (test_and_set_bit (ACTION_USB_EP0_TX_DONE, &kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB20_IntrHandler(%d): EPO IN Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			
		}

		if ( IvecReg == SEUSB20_EP0OUT	)
		{

			ClearIntrStatus(DeviceNo,SEUSB20_OUT07IRQ,SEUSB20_OUT0IR);
			if (test_and_set_bit (ACTION_USB_EP0_RX_DONE, &kthread_data_usb[Index].action))
			{
				TWARN ("SEUSB20_IntrHandler(%d): EPO OUT Intr arrived before previous was executed by kernel thread\n",DeviceNo);
			}
			Usb_OsWakeupDPC (DeviceNo);
			TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"EP0OUT \n ");
			TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"Outxcs %x \n",SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT1CS));
			TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"Out07ien %x \n", SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT07IEN  ));
			TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"SEUSB20_USBINTS_EN  %x \n", SEUSB20_ReadReg8(DeviceNo,SEUSB20_USBINTS_EN  ));
			TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"SEUSB20_OUT07IRQ %x \n", SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT07IRQ));

		}

		for (ep = 1; ep <= SEUSB20_MAX_EP; ep++)
		{
			if (IN07irq & (1 << ep))
			{
				ClearIntrStatus(DeviceNo,SEUSB20_IN07IRQ, (1 << ep));
				SEUSB20_Tx_Intr(DeviceNo, ep);
			}
			if (Out07irq & (1 << ep))
			{
				ClearIntrStatus(DeviceNo,SEUSB20_OUT07IRQ, (1 << ep));
				SEUSB20_Rx_Intr(DeviceNo, ep);
			}
		}
	}


	TDBG_FLAGGED(usbhw20, DEBUG_HW1_INTR,"<-------------------------- SEUSB20 (%d) Intr Exit\n",DeviceNo);	

	return IRQ_HANDLED;
}

static void
SEUSB20_SetAddress(uint8 DeviceNo,uint8 Addr, uint8 Enable)
{
	uint8 Index;

	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	SetAddrEnable [Index] = Enable;
	return;
}

static void
SEUSB20_GetAddress(uint8 DeviceNo,uint8 *Addr, uint8 *Enable)
{
	uint8 Index;

	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	*Addr = SEUSB20_ReadReg8(DeviceNo, SEUSB20_FNADDR);
	*Enable = SetAddrEnable[Index];
	return;
}

static int
SEUSB20_WriteFifoBuffer(uint8 DeviceNo,uint8 ep, uint8 *Buffer, uint16 Len)
{

	uint8 Value;
	uint8 Index;
	int16 i = 0;
	uint16 Reg;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_WriteFifoBuffer() is called for device %d ep %d Len %d\n",
										DeviceNo,ep, Len);

	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint number */
	if (ep > SEUSB20_MAX_EP)
		return 1;

 	if (ep == 0)
	{
		//Clear it here so no int.
		Value = SEUSB20_ReadReg8(DeviceNo,SEUSB20_IN07IRQ);
		Value |= (SEUSB20_IN0IR);
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IRQ,Value);
	}
	if (Len == 0)
	{
		TWARN("SEUSB20_WriteFifoBuffer(): received zero ??? lenght buffer\n");
		return 0;
	}
	
	if (ep == 0)
	{
		Reg = SEUSB20_EP0INDAT ;			
		for (i=0;i<Len;i++)
		{
			SEUSB20_WriteReg8(DeviceNo,(Reg + (i)),Buffer[i]);
		}

		/* Load BC register to send the data to host */
		SEUSB20_WriteReg8(DeviceNo,(SEUSB20_IN0BC + (ep*8)),Len);
	}
	else if ((ep == 1) || (ep == 2))
	{
		Reg = SEUSB20_FIFO1DAT + ((ep-1)*4);
		for (i = 0; i < Len; i++)
		{
			SEUSB20_WriteReg8 (DeviceNo, Reg, Buffer[i]);
		}
		SEUSB20_WriteReg8 (DeviceNo, (SEUSB20_IN1CS + ((ep-1)*8)), 0);
	}
	else
	{
		SEUSB20_Dma(DeviceNo, ep , Buffer, Len);
	}
	
	return 0;
}


static int
SEUSB20_ReadFifoBuffer(uint8 DeviceNo,uint8 ep, uint8 *Buffer, uint16 *Len,uint8 Setup)
{
	uint16 i;
	uint16 ReadLen ; 
	uint16 OutBuff_Reg; 
	uint16 Reg16;
	uint8 Index;

	DEVICE_REQUEST *DevReq = (DEVICE_REQUEST*) Buffer;
	
	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_ReadFifoBuffer() is called for device %d and Setup=%d\n",
										DeviceNo,Setup);

	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint Value */
	if (ep > SEUSB20_MAX_EP)
		return 1;

	if ((ep == 0) && (Setup))
	{
		for (i=0;i<8;i++)
		{	
			Buffer[i]= SEUSB20_ReadReg8(DeviceNo,SEUSB20_SETUPDAT  + (i));
		}
		*Len = 8;
		if ((DevReq->bmRequestType == 0x01) && (DevReq->bRequest == SET_INTERFACE))
		{
			SEUSB20_Toggle(DeviceNo , SEUSB20_DIR_IN, UsbCoreFunc.CoreUsbGetDataInEp (DeviceNo, DevReq->wIndex));			
			SEUSB20_Toggle(DeviceNo , SEUSB20_DIR_OUT, UsbCoreFunc.CoreUsbGetDataOutEp (DeviceNo, DevReq->wIndex));			
		}
		if ((0 == (DevReq->bmRequestType & 0x80)) && (DevReq->wLength))
		{
			SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT0BC,8);
		}
		
		return 0;
	}

	*Len = 0;
	/* Read the Length  */
	ReadLen = 	SEUSB20_ReadReg8(DeviceNo,(SEUSB20_OUT0BC + (ep*8))  );
	if (ep > 0) {
	 Reg16 = SEUSB20_ReadReg8(DeviceNo,(SEUSB20_OUT0BC +(ep*8)+1));
	 ReadLen |= (Reg16 << 8);
	}
	/* Increment total read count */
	(*Len) = ReadLen;
    /* Read the buffer */
	if (ep == 0)
	{
		OutBuff_Reg = SEUSB20_EP0OUTDAT;
		for (i=0;i<ReadLen;i++)
		{		
			Buffer[i]= SEUSB20_ReadReg8(DeviceNo,(OutBuff_Reg +i));
		}
	}
	else 
	{	
		OutBuff_Reg = (SEUSB20_FIFO1DAT + ((ep-1)*4));
		for (i=0;i<ReadLen;i++)
		{		
			Buffer[i] = READ_SEUSB20_BYTE(SEUSB20_Conf[Index].DataReg+OutBuff_Reg);
		}

	}

	/* Re-arm Byte Count register to receive next Out packet 
	   Write any value to BC register */
	switch(ep)
		{
		case 0: 
			SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT0BC,ReadLen);
			SEUSB20_WriteReg8(DeviceNo,SEUSB20_EP0CS,(1<<2));
			break;
		
		default:
			SEUSB20_WriteReg8(DeviceNo,(SEUSB20_OUT1CS + ((ep-1) * 8)), 0x0);
			break;
	
		}

	return 0;
}

static int	
SEUSB20_AllocBuffer(uint8 DeviceNo,uint8 Ep, uint16 Fifo, uint16 MaxPkt, uint8 Dir, uint8 Type)
{
	/* Argument Fifo Size is not used at all for SEUSB20. It is always set to 512 */
	uint8 FifoData;
	uint16 InOutConAddr;
	uint8 InOutConData;
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_AllocBuffer() is called for device %d endpoint %d MaxPkt %d Dir %d Type %d\n",
										DeviceNo,Ep, MaxPkt, Dir, Type);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint Value */
	if (Ep > SEUSB20_MAX_EP)
		return 1;		

	/* Ep 0 has Fixed internal Fifo. We cannot change it */
	if (Ep == 0)
		return 0;

	FifoData = 0;
	InOutConData = 0;

	FifoData |= Ep;
	FifoData |= SEUSB20_FIFOAUTO;
    
	/* If Direction is IN */
	if (Dir)
	{
		FifoData |= SEUSB20_FIFODIR;					
		InOutConAddr = SEUSB20_IN1CON + (8*(Ep-1));    				
	}
	else /* Dir is OUT */
		InOutConAddr = SEUSB20_OUT1CON + (8*(Ep-1));    				

	/* EP Type */
	if (Type == BULK)
		InOutConData |= SEUSB20_EPTYPE_BULK;
	else if (Type == INTERRUPT)
		InOutConData |= SEUSB20_EPTYPE_INT;
	
	/* Buffer type , Single buffer 00b*/
	InOutConData &= ~SEUSB20_SINGLEBUF;						

	/* Write INXCON / OUTXCON */
	SEUSB20_WriteReg8(DeviceNo,InOutConAddr , InOutConData );
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_FIFOCTRL ,FifoData );

	return 0;
} 		

static int
SEUSB20_EnableEp(uint8 DeviceNo, uint8 Ep, uint8 EpDir, uint8 EpType)
{
	 
	uint8 Mask, IntrMask ;
	uint8 Reg8;
	uint16 InOutConAddr;
	uint8 EndpRstData;
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_EnableEp() is called for device %d endpoint %d epdir %d eptype %d\n",
										DeviceNo,Ep,EpDir,EpType);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint Value */
	if (Ep > SEUSB20_MAX_EP)
		return 1;		

	/* For SEUSB20 we should not enable EP 0*/
	if (Ep == 0)
		return 0;
	
	if (EpDir == DIR_IN)
	{
		
		InOutConAddr = SEUSB20_IN1CON + (8*(Ep-1));    				

		/* Set valid bit */
		Reg8 = SEUSB20_ReadReg8(DeviceNo,InOutConAddr) ;
    	Reg8 |= SEUSB20_VAL;
		SEUSB20_WriteReg8(DeviceNo,InOutConAddr , Reg8 );

		//Reset Endpoint
		EndpRstData = 0;
		EndpRstData |= Ep;
		EndpRstData |= BIT4; /* DIR IN */
		/* Write 011deeee*/
		EndpRstData |= 0x60;
		//printk(" Reset Ep %d %x \n ",Ep,EndpRstData ); 
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, EndpRstData); 

		/* Enable ep Intr */
		Mask  	 = 0x1 << Ep;
		IntrMask = SEUSB20_ReadReg8(DeviceNo,SEUSB20_IN07IEN);
		IntrMask |= Mask;
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IEN,IntrMask);

	}
	else if (EpDir == DIR_OUT)
	{
		
		InOutConAddr = SEUSB20_OUT1CON + (8*(Ep-1));    				

		/* Set valid bit */
		Reg8 = SEUSB20_ReadReg8(DeviceNo,InOutConAddr) ;
    	Reg8 |= SEUSB20_VAL;
		SEUSB20_WriteReg8(DeviceNo,InOutConAddr , Reg8 );

		//Reset Endpoint
		EndpRstData = 0;
		EndpRstData |= Ep;
		/* Write 011deeee*/
		EndpRstData |= 0x60;
		//printk(" Reset Ep %d %x \n ",Ep,EndpRstData ); 
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, EndpRstData); 

		/* Enable ep Intr */
		Mask  	 = 0x1 << Ep;
		IntrMask = SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT07IEN);
		IntrMask |= Mask;
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IEN,IntrMask);
	}

    
	return 0;
}

static int
SEUSB20_DisableEp(uint8 DeviceNo, uint8 Ep, uint8 EpDir, uint8 EpType)
{

	uint8 Reg8 , IntrMask  ,Mask ;
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_DisableEp() is called for device %d endpoint %d epdir %d eptype %d\n",
										DeviceNo,Ep,EpDir,EpType);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint Value */
	if (Ep > SEUSB20_MAX_EP)
		return 1;		

	/* For SEUSB20 we should not disable EP 0*/
	if (Ep == 0)
		return 0;

	if (EpDir == DIR_IN)
	{
		/* Set valid bit */
		Reg8 = SEUSB20_ReadReg8(DeviceNo,SEUSB20_IN1CON + (8*(Ep-1)));
		Reg8 &= ~SEUSB20_VAL;
		SEUSB20_WriteReg8(DeviceNo,(SEUSB20_IN1CON + (8*(Ep-1))),Reg8);

		/* Enable ep Intr */
		Mask  	 = 0x1 << Ep;
		IntrMask = SEUSB20_ReadReg8(DeviceNo,SEUSB20_IN07IEN);
		IntrMask &= ~Mask;
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_IN07IEN,IntrMask);

	}
	else if (EpDir == DIR_OUT)
	{
		Reg8 = SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT1CON + (8*(Ep-1)));
		Reg8 &= ~SEUSB20_VAL;
		SEUSB20_WriteReg8(DeviceNo,(SEUSB20_OUT1CON + (8*(Ep-1))),Reg8);


		/* Enable ep Intr */
		Mask  	 = 0x1 << Ep;
		IntrMask = SEUSB20_ReadReg8(DeviceNo,SEUSB20_OUT07IEN);
		IntrMask &= ~Mask;
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_OUT07IEN,IntrMask);

	}
	
	return 0;
}

static int
SEUSB20_GetEpStatus(uint8 DeviceNo,uint8 Ep, uint8 EpDir, uint8 *Enable, uint8 *Stall)
{
	uint8 Value8;
	uint8 Reg8;
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_GetEpStatus() is called for device %d endpoint %d\n",
										DeviceNo,Ep);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	*Enable = 0;
	*Stall = 0;

	/* Validate Endpoint Value */
	if (Ep > SEUSB20_MAX_EP)
		return 1;

	/* For Endpoint 0 */
	if (Ep == 0)
	{
		*Enable = 1;  /* Always Enabled */
		Value8 = SEUSB20_ReadReg8(DeviceNo,SEUSB20_EP0CS);
		*Stall 	= (Value8  & SEUSB20_EP0STALL)?1:0;
		return 0;
	}

	if (EpDir == DIR_IN)
	{
		Reg8 = (SEUSB20_IN1CON + ((Ep-1)*8));
		Value8 = SEUSB20_ReadReg8(DeviceNo, Reg8);
		if (Value8 &SEUSB20_STALL)
			*Stall 	= 1;
	}
	else if (EpDir == DIR_OUT)
	{
		Reg8 = (SEUSB20_OUT1CON + ((Ep-1)*8));
		Value8 = SEUSB20_ReadReg8(DeviceNo,Reg8);
		if (Value8 &SEUSB20_STALL)
			*Stall 	= 1;
	}

	/* Non Zero EP */
	return 0;
}


static int
SEUSB20_StallEp(uint8 DeviceNo,uint8 Ep, uint8 EpDir)
{
	uint8 Value8;
	uint8 Reg8;
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_StallEp() is called for device %d endpoint %d\n",
										DeviceNo,Ep);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	/* Validate Endpoint Value */
	if (Ep > SEUSB20_MAX_EP)
		return 1;		

	/* Call Core Handler */
		UsbCoreFunc.CoreUsbHaltHandler(DeviceNo,Ep,EpDir);	

	if (Ep == 0)
	{
		Value8 = SEUSB20_ReadReg8(DeviceNo,SEUSB20_EP0CS);
		Value8 |= (SEUSB20_HSNAK | SEUSB20_EP0STALL);
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_EP0CS, Value8 );
		
	}
	else
	{
		if (EpDir == DIR_IN)
		{
			Reg8 = (SEUSB20_IN1CON + ((Ep-1)*8));

			Value8 = SEUSB20_ReadReg8(DeviceNo, Reg8);
			Value8 |= SEUSB20_STALL;
			SEUSB20_WriteReg8(DeviceNo,Reg8,Value8 );

		}
		else if (EpDir == DIR_OUT)
		{
			Reg8 = (SEUSB20_OUT1CON + ((Ep-1)*8));
			Value8 = SEUSB20_ReadReg8(DeviceNo,Reg8  );
			Value8 |= SEUSB20_STALL;
			SEUSB20_WriteReg8(DeviceNo,Reg8, Value8 );
		}

	}


	return 0;
	
}

static int		
SEUSB20_UnstallEp(uint8 DeviceNo,uint8 Ep, uint8 EpDir)
{

	uint8  togctl_value;
	uint8 Value8 , Reg8;
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_UnstallEp() is called for device %d endpoint %d\n",
										DeviceNo,Ep);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return 1;

	togctl_value = 0;
	togctl_value |= Ep;
	if(EpDir == DIR_IN)
	   togctl_value |= BIT4;

	SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, togctl_value  ); 
	/* Clear toggle bit  001d eeee*/
	togctl_value |= 0x20;

   	SEUSB20_WriteReg8(DeviceNo,SEUSB20_ENDPRST, togctl_value  ); 


	if (Ep == 0)
	{
		
		Value8 = SEUSB20_ReadReg8(DeviceNo,SEUSB20_EP0CS);
		Value8 |= SEUSB20_HSNAK ;
		Value8 &=  ~SEUSB20_EP0STALL;
		SEUSB20_WriteReg8(DeviceNo,SEUSB20_EP0CS, Value8 );
		
	}

	else
	{
		if (EpDir == DIR_IN)
		{
			Reg8 = (SEUSB20_IN1CON + ((Ep-1)*8));

			Value8 = SEUSB20_ReadReg8(DeviceNo, Reg8);
			Value8 &= ~SEUSB20_STALL;
			SEUSB20_WriteReg8(DeviceNo,Reg8,Value8 );

		}
		else if (EpDir == DIR_OUT)
		{
			
			Reg8 = (SEUSB20_OUT1CON + ((Ep-1)*8));
			
			Value8 = SEUSB20_ReadReg8(DeviceNo,Reg8  );
			Value8 &= ~SEUSB20_STALL;
			SEUSB20_WriteReg8(DeviceNo,Reg8, Value8 );

		}

	}

	/* Call Core Handler */
	UsbCoreFunc.CoreUsbUnHaltHandler(DeviceNo,Ep, EpDir);	
	return 0;

}

static  void
SEUSB20_CompleteRequest(uint8 DeviceNo,uint8 status,DEVICE_REQUEST *Req)
{

	uint8 Ctrl;
	uint8 Index;

	Index = SEUSB20_ConvertDeviceToIndex (DeviceNo);
	if (Index == 0xFF) return;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_CompleteRequest() is called for Device %d Setup Complete with status %d\n",
							DeviceNo,status);
	
	if (status == 1 )	/* Error */
	{
		TCRIT("SEUSB20_CompleteRequest(): Sending Error Status\n");
		SEUSB20_StallEp(DeviceNo,0, DIR_IN);
		return;
	}
	/* Clear HSNAK */
	Ctrl = SEUSB20_ReadReg8(DeviceNo,SEUSB20_EP0CS);
	Ctrl |= SEUSB20_HSNAK;
	SEUSB20_WriteReg8(DeviceNo,SEUSB20_EP0CS ,Ctrl);

	return;
}

/*
   Why we are using SoftConnect:
   When the USB cable is already connected to host and when we initialize
   SEUSB20, the host has no way to detect the change in the configuration
   and will not find the devices exposed by SEUSB20. Disabling and Enabling
   Softconnect simulates removal and insertion of cable, which the host USB
   software uses to reconfigure the devices.
 */


static void		
SEUSB20_DeviceDisconnect(uint8 DeviceNo)
{
	uint8 Index;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_DeviceDisconnect() is called for device %d\n", DeviceNo);
	Index = SEUSB20_ConvertDeviceToIndex (DeviceNo);
	if (Index == 0xFF) return;

	/* Disconnect Device from Host */
	SEUSB20_SoftDisconnect(DeviceNo);
	return;
}

static void
SEUSB20_DeviceReconnect (uint8 DeviceNo)
{
	uint8 Index;
#ifdef CONFIG_SPX_FEATURE_POWER_CONSUMPTION_SUPPORT
	if(IsUsb_Upstream_host  == 0)
		return;
#endif
	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_DeviceReconnect() is called for device %d\n", DeviceNo);
	Index = SEUSB20_ConvertDeviceToIndex (DeviceNo);
	if (Index == 0xFF) return;

	/* Connect Device to Bus */
	SEUSB20_SoftConnect(DeviceNo);
	UsbCoreFunc.CoreUsb_OsDelay(1);
	return;
}

static int
SEUSB20_GetEps (uint8 DeviceNo, uint8* pdata, uint8* num_eps)
{
	uint8 Index;
	uint8 *EpData;
	uint8 Eps;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"SEUSB20_GetEps() is called for device %d\n", DeviceNo);

	Index = SEUSB20_ConvertDeviceToIndex (DeviceNo);
	if (Index == 0xFF)
	{
		TCRIT ("SEUSB20_GetEps(): Error Unable the read Endpoint Information for Device %d\n", DeviceNo);
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
	TDBG_FLAGGED(usbhw20, DEBUG_HW1,"Number of EndPoints = %d\n", *num_eps);
	return 0;


}

static int
UsbDeferProcess0(void *arg)
{

	TDBG_FLAGGED(usbhw20, DEBUG_HW1_DPC,"UsbDeferProcess0() is called for device %d with action %ld\n",
											kthread_data_usb[0].device_num, kthread_data_usb[0].action);

	while (kthread_data_usb[0].action)
	{
		if (test_and_clear_bit (ACTION_USB_BUS_SUSPEND, &kthread_data_usb[0].action))
		{
			SEUSB20_BusSuspend(kthread_data_usb[0].device_num);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_BUS_RESET, &kthread_data_usb[0].action))
		{
			SEUSB20_BusReset(kthread_data_usb[0].device_num);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_SETUP_PACKET, &kthread_data_usb[0].action))
		{
			SEUSB20_EP0_RxDone(kthread_data_usb[0].device_num,SETUP_PKT);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_EP0_TX_DONE, &kthread_data_usb[0].action))
		{
			SEUSB20_EP0_TxDone(kthread_data_usb[0].device_num);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_EP0_RX_DONE, &kthread_data_usb[0].action))
		{
			SEUSB20_EP0_RxDone(kthread_data_usb[0].device_num,NOT_SETUP_PKT);
			continue;
		}
		if (test_and_clear_bit (ACTION_DMA_TRANSFER, &kthread_data_usb[0].action))
		{
			down (&DMAWait);
			SEUSB20_Dma(dma_data[0].devicenum, dma_data[0].ep ,dma_data[0].buffer, dma_data[0].len);
			up (&DMAWait);
			continue;
		}
		if (test_and_clear_bit (ACTION_USB_CONFIG_HISPEED, &kthread_data_usb[0].action))
		{
			UsbCoreFunc.CoreUsbConfigureHS(kthread_data_usb[0].device_num);
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
	DPC_T	*pDPC = (DPC_T*) proc;
#if(LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
	char ProcessName [100];
#endif

	TDBG_FLAGGED(usbhw20, DEBUG_HW1_DPC,"DPCDispatchThread() is called\n");
    Inst = pDPC->Inst;
	DPCprocess = (DPC)pDPC->DPC;

#if(LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
	/* Put in background and release our user space connections */
	sprintf (ProcessName, "%s%d", HW_MODULE_DPC_NAME, Inst);
	daemonize(ProcessName);
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
  kthread_run(DPCDispatchThread,proc,"USB1DPC");
#endif
}

static void
Usb_OsInitDPC(DPC_T *pDPC)
{
	int Inst = pDPC->Inst;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1_DPC,"Usb_OsInitDPC() is called\n");
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

	TDBG_FLAGGED(usbhw20, DEBUG_HW1_DPC,"Usb_OsWakeupDPC() is called for device %d\n",DeviceNo);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	complete(&DPC_Semaphore [Index]);
	return;
}

static void
Usb_OsKillDPC(uint8 DeviceNo)
{
	uint8 Index;

	TDBG_FLAGGED(usbhw20, DEBUG_HW1_DPC,"Usb_OsKillDPC() is called for device %d\n",DeviceNo);
	/* Get Device Index */
	Index = SEUSB20_ConvertDeviceToIndex(DeviceNo);
	if (Index == 0xFF) return;

	DPCThreadRunning [Index] = 0;

	complete(&DPC_Semaphore [Index]);

	/* Wait for DPCDispatchThread to exit */
	wait_for_completion_interruptible(&DPC_Exited [Index]);
	return;
}

module_init(init_SEUSB20_module);
module_exit(exit_SEUSB20_module);

MODULE_AUTHOR("Rama Rao Bisa <RamaB@ami.com>");
MODULE_DESCRIPTION("PILOT-II hardware driver module for USB2.0 Controller");
MODULE_LICENSE("GPL");

