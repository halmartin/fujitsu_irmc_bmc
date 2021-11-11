/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include "dbgout.h"
#include "function1_mod.h"
#include "reset.h"

#ifdef CONFIG_SPX_FEATURE_GLOBAL_PCIE_FUNCTION1_SUPPORT
#if (CONFIG_SPX_FEATURE_GLOBAL_PCIE_FUNCTION1_SHARED_MEM_SIZE % 16)
#error "PCIe Function1 Shared Memory size should be multiple of 16"
#endif
#endif

#define FUNCTION1_MAJOR 57
#define FUNCTION1_DEVNAME "function10"


#ifdef HAVE_UNLOCKED_IOCTL  
  #if HAVE_UNLOCKED_IOCTL  
  #define USE_UNLOCKED_IOCTL  
  #endif
#endif

#ifdef SOC_PILOT_IV
#define bmc2host_q_status       0x4044A040
#endif

typedef int (*HostResetHandler) (void);
static int PCIeFunction1HostResetHandler(void);
static HostResetHandler PCIeHostResetHandler = PCIeFunction1HostResetHandler;
					
#ifdef USE_UNLOCKED_IOCTL						
static long	Function1DriverIoctl (struct file*, unsigned int, unsigned long); 
#else
static int	Function1DriverIoctl (struct inode*, struct file*, unsigned int, unsigned long); 
#endif
static int	Function1DriverOpen (struct inode *inode,  struct file *file); 
static int	Function1DriverRelease (struct inode *inode,  struct file *file); 
static void ClearQueues(void);

static void* Function1RemapAddr  = NULL;
static void* SharedMemRemapAddr  = NULL;

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

static volatile uint8_t	h2bMsgQDataIrqFlag = 0;
static volatile uint8_t	wait4h2bMsgQDataIrq = 0;
static volatile uint8_t	b2hMsgQFullFlag[MAX_Q] = {0, };
static volatile uint8_t	wait4b2hMsgQNotFullIrq[MAX_Q] = {0, };
static volatile Function1MsgQBuf_t h2bMsgQBuf[MAX_Q];
static volatile bool isFunction1IntEnable = FALSE;

static wait_queue_head_t Function1RecSleepHead;
static wait_queue_head_t Function1SendSleepHead[MAX_Q];
static int EnableSharedMemoryAccess = 0;

struct file_operations function1_fops =  
{ 
	.owner     	=  THIS_MODULE, 
#ifdef USE_UNLOCKED_IOCTL
	.unlocked_ioctl      	=  Function1DriverIoctl,
#else
	.ioctl      	=  Function1DriverIoctl,
#endif
	.open      	=  Function1DriverOpen, 
	.release 		=  Function1DriverRelease, 
}; 

inline static uint32_t
Function1ReadReg32 (uint16_t reg) 
{ 
	volatile uint32_t Addr;

	Addr = (uint32_t)Function1RemapAddr;
	Addr += reg;
	
	return ioread32 ((void * __iomem)Addr);
} 
  
inline static void
Function1WriteReg32 (uint32_t data, uint16_t reg) 
{ 
	volatile uint32_t Addr;

	Addr = (uint32_t)Function1RemapAddr;
	Addr += reg;
	
	iowrite32 (data, (void * __iomem)Addr); 
}

// During reset, system bios has to allocate memory resource for MEM_BAR.
// For this action, the MEM_BAR[3:0) has to be programmed as 0x0 from the BMC side.
static int 
PCIeFunction1HostResetHandler(void)
{
	volatile uint32_t Temp;
	
//	printk ("PCIeFunction1HostResetHandler\n");

	Temp = Function1ReadReg32 (CONFIG_SPACE_OFFSET + 0x10);
	Temp &= ~0x0E;
	Function1WriteReg32 (Temp, CONFIG_SPACE_OFFSET + 0x10);
	EnableSharedMemoryAccess = 0;	

	ClearQueues();
	ClearQueues();  // do it twice, because the queues are not empty after one call!

	return 0;	
}

static int 
Function1DriverOpen (struct inode *inode, struct file *file) 
{ 
	return 0; 
} 
  
static int 
Function1DriverRelease (struct inode *inode, struct file *file) 
{ 
	return 0; 
} 

typedef struct
{
	int		IrqCount;	// number of interrupts
	int		MsgCount;	// number of messages
	int		StatusReg;	// H2B status Register
} func1debug_t;


static irqreturn_t
PCIeIrqHandler (int irq, void *dev_id)
{
	volatile uint32_t 	Temp;
	int 				noFreeEntries;
	
	unsigned char		RdIdx;
	unsigned char 	    WrIdx;			  
	
	uint8_t				QNum;
	uint8_t				i;
	int                 DataReceived = 0;
	
//	const int Mask = (H2B_QUEUE1_EMPTY | H2B_QUEUE2_EMPTY);

	if ( isFunction1IntEnable )
	{
		Temp = Function1ReadReg32 (HOST_TO_BMC_Q_STATUS);
		Temp &= ~H2B_RING_DOORBELL;
		Function1WriteReg32 (Temp, HOST_TO_BMC_Q_STATUS);
	}

#if 0		// Debug start
	func1debug_t	  *debugIrq;
	
//	printk ("*** Enter PCIe function1 interrupt handler\n");		

	debugIrq = (func1debug_t *)((uint8_t *)SharedMemRemapAddr+0x100);
	debugIrq->IrqCount++;
	Temp = Function1ReadReg32 (HOST_TO_BMC_Q_STATUS);
	debugIrq->StatusReg = Temp;
#endif		// Debug end

	for ( QNum = 0;  QNum < MAX_Q;  QNum++ )
	{
		int Mask, QueReg;
		
		    RdIdx = h2bMsgQBuf[QNum].RdIdx;
		    WrIdx = h2bMsgQBuf[QNum].WrIdx;
		    
	    if(QNum == 0)
	    {
	    	Mask   = H2B_QUEUE1_EMPTY;
	    	QueReg = HOST_TO_BMC_Q1;
	    }
	    else
	    {
	    	Mask   = H2B_QUEUE2_EMPTY;
	    	QueReg = HOST_TO_BMC_Q2;
	    }
		
		// Read until queue is empty or buffer is full
		for ( i = 0;  i < MSG_Q_BUFF_SIZE-1;  i++ )
		{
			// the entry, where the WrIdx points to, needs always to be free
			if ( RdIdx > WrIdx )
				noFreeEntries = RdIdx - WrIdx - 1;
			else // RdIdx <= *pWrIdx
				noFreeEntries = RdIdx + MSG_Q_BUFF_SIZE - WrIdx - 1;
			
			if ( noFreeEntries <= 0 )  // no free entries
			{
//				printk("*** PCIe function1 interrupt disable\n");
				isFunction1IntEnable = FALSE;
				disable_irq_nosync(IRQ_PCIE_TEST);

				Temp = Function1ReadReg32 (HOST_TO_BMC_Q_STATUS);
				Temp |= H2B_RING_DOORBELL;
				Function1WriteReg32 (Temp, HOST_TO_BMC_Q_STATUS);

				break;
			}

			Temp = Function1ReadReg32 (HOST_TO_BMC_Q_STATUS);
			if (Temp == 0)
				// PCIe function 1 interface not initialized
				break;

			if ((Temp & Mask) == 0) 		// The queue is not empty?
			{
				DataReceived = 1;

//				printk ("*** HOST_TO_BMC_Q_STATUS: 0x%08X ***\n", Temp);
				
			    h2bMsgQBuf[QNum].QData[WrIdx] = Function1ReadReg32 (QueReg);
				
				if(WrIdx < MSG_Q_BUFF_SIZE - 1 )
					 WrIdx++;
				else WrIdx = 0;
			}
			else
			{
				// queue is empty
				break;
			}
		}
		
	    h2bMsgQBuf[QNum].WrIdx = WrIdx;
	}
	
	if(DataReceived == 1 )
	{
#if 0		// Debug start
		debugIrq->MsgCount++;
#endif		// Debug end
	    h2bMsgQDataIrqFlag = DataReceived;
		 
		if ( wait4h2bMsgQDataIrq )
		{
			wake_up(&Function1RecSleepHead);
		}
	}

	Temp = Function1ReadReg32(BMC_TO_HOST_Q_STATUS);
	if ( (Temp & B2H_QUEUE1_FULL) == 0 )
	{
		b2hMsgQFullFlag[0] = 0;
		if ( wait4b2hMsgQNotFullIrq[0] == 1 )
			wake_up(&Function1SendSleepHead[0]);
	}
	if ( (Temp & B2H_QUEUE2_FULL) == 0 )
	{
		b2hMsgQFullFlag[1] = 0;
		if ( wait4b2hMsgQNotFullIrq[1] == 1 )
			wake_up(&Function1SendSleepHead[1]);
	}
/*	
	if ( isFunction1IntEnable )
	{
		Temp = Function1ReadReg32 (HOST_TO_BMC_Q_STATUS);
		if ( ((Temp & H2B_QUEUE1_EMPTY) != 0)  && ((Temp & H2B_QUEUE2_EMPTY) != 0) )
		{
			// reset doorbell
			Temp &= ~H2B_RING_DOORBELL;
			Function1WriteReg32 (Temp, HOST_TO_BMC_Q_STATUS);
		}
	}
*/	
	return IRQ_HANDLED;	
}

static int 
init_function1_module (void)
{
	int Err;
	int Ret = 0;
	int i;
	volatile uint32_t Temp;
	
	memset( &h2bMsgQBuf, 0, sizeof(h2bMsgQBuf) );
	
	if ((Err = register_chrdev (FUNCTION1_MAJOR, FUNCTION1_DEVNAME, &function1_fops)) < 0) 
	{ 
		TCRIT ("failed to register device %s (err %d)\n", FUNCTION1_DEVNAME, Err); 
		Ret = -ENODEV; 
		goto fail; 
	} 

#ifdef SOC_PILOT_IV
	*(volatile unsigned long *)IO_ADDRESS(0x40100198) = 0x31;
#endif
	 
	Function1RemapAddr = ioremap_nocache (FUNCTION1_BASE_ADDR, FUNCTION1_REGS_SIZE); 
	if (Function1RemapAddr == NULL) 
	{ 
		TCRIT ("failed to map Function1 IO space to memory\n"); 
		Ret = -EIO; 
		goto fail1;
	} 

	printk ("FUNCTION1_SHARED_MEM_START = 0x%08x\n", (unsigned int) FUNCTION1_SHARED_MEM_START);
	printk ("FUNCTION1_SHARED_MEM_LEN = 0x%08x\n", (unsigned int) FUNCTION1_SHARED_MEM_LEN);
	SharedMemRemapAddr = ioremap_nocache (FUNCTION1_SHARED_MEM_START, FUNCTION1_SHARED_MEM_LEN);
	if (SharedMemRemapAddr == NULL) 
	{ 
		TCRIT ("failed to map Function1 shared memory\n"); 
		Ret = -EIO; 
		goto fail2;
	} 

/*	
    SysCtlRegRemapAddr = ioremap_nocache (SYSCTL_REG_MEM_START, SYSCTL_REG_MEM_LEN);
	if (SysCtlRegRemapAddr == NULL) 
	{ 
		TCRIT ("failed to map SysCtl reg IO space to memory\n"); 
		Ret = -EIO; 
		goto fail;
	} 
*/
	
	/* clear h2bq1 and h2bq2 */
	ClearQueues();
	
	// now clear Doorbell
	Temp = Function1ReadReg32 (HOST_TO_BMC_Q_STATUS);
//	printk("HOST_TO_BMC_Q_STATUS: %x\n",Temp);
	Temp &= ~H2B_RING_DOORBELL;
	Function1WriteReg32 (Temp, HOST_TO_BMC_Q_STATUS);

	
	//	printk("init_waitqueue_head Rec\n");
	init_waitqueue_head (&Function1RecSleepHead);
//	printk("init_waitqueue_head Send\n");
	for (i = 0; i < MAX_Q; i++)
	{
		init_waitqueue_head (&Function1SendSleepHead[i]);
	}

	isFunction1IntEnable = TRUE;  // must be set before we call request_irq, so the IRQ-Handler has the chance
								  // to clear the doorbell

#ifdef SOC_PILOT_IV
#ifdef CONFIG_SPX_FEATURE_GLOBAL_PCIE_FUNCTION1_ENABLE_HOST_INTR
	*(volatile unsigned long *) IO_ADDRESS(bmc2host_q_status) =  B2H_ENABLE_INTB;	// Enable only door bell interrupts 
#else
	*(volatile unsigned long *) IO_ADDRESS(bmc2host_q_status) &= ~B2H_ENABLE_INTB;	// Disable interrupt generation		
#endif
#endif


//	printk("request_irq\n");
	if ( (Ret = request_irq(IRQ_PCIE_TEST,  PCIeIrqHandler, 0, "PCIeFunction1", (void*)Function1RemapAddr)) != 0)  // no IRQF_DISABLED for slow interrupt
	{
		printk (KERN_ERR "%s: failed to request irq %d (err %d)\n", FUNCTION1_DEVNAME, IRQ_PCIE_TEST, Ret);
		Ret = -ENODEV;
		goto fail3;
	}
#ifdef SOC_PILOT_IV
	// Now enable the device
	*(volatile unsigned long *)IO_ADDRESS(0x4010091C) |= 0x00000007;
#endif
//	printk("install_reset_handler\n");
	install_reset_handler(PCIeHostResetHandler);
	
	printk ("Function1 HW driver loaded...\n");
	return 0;	

fail1:
	unregister_chrdev(FUNCTION1_MAJOR, FUNCTION1_DEVNAME);
	goto fail;

fail2:
	unregister_chrdev(FUNCTION1_MAJOR, FUNCTION1_DEVNAME);
	iounmap (Function1RemapAddr);
	Function1RemapAddr = NULL;
	goto fail;

fail3:
	unregister_chrdev(FUNCTION1_MAJOR, FUNCTION1_DEVNAME);
	iounmap (Function1RemapAddr);
	Function1RemapAddr = NULL;
	iounmap (SharedMemRemapAddr);
	SharedMemRemapAddr = NULL;
	goto fail;


fail:
	return Ret;
}

static void
exit_function1_module (void)
{
	free_irq(IRQ_PCIE_TEST, (void*)Function1RemapAddr);

//	printk ("uninstall_reset_handler  %s:%i\n",__FUNCTION__,__LINE__);  // for debug output
	uninstall_reset_handler(PCIeHostResetHandler);

//	printk ("%s:%i\n",__FUNCTION__,__LINE__);  // for debug output
	if (Function1RemapAddr)
	{
		printk ("%s:%i\n",__FUNCTION__,__LINE__);
		iounmap (Function1RemapAddr);
		Function1RemapAddr = NULL;
	}
	if (SharedMemRemapAddr)
	{
		printk ("%s:%i\n",__FUNCTION__,__LINE__);
		iounmap (SharedMemRemapAddr);
		SharedMemRemapAddr = NULL;
	}

/*	
    if (SysCtlRegRemapAddr)
	{
		printk ("%s:%i\n",__FUNCTION__,__LINE__);
		iounmap (SysCtlRegRemapAddr);
		SysCtlRegRemapAddr = NULL;
	}
*/
	unregister_chrdev(FUNCTION1_MAJOR, FUNCTION1_DEVNAME);
	printk ("function1hw module unloaded\n");
	return;
}


static uint8_t 
Function1ReadMsgQData (Function1MsgQ_t *pMsgQ) 
{
	int 				i;
	uint16_t 			QNum;
	uint8_t				isDataRead = 0;
	
	unsigned char RdIdx;	
	unsigned char WrIdx;
	
    int                 FreeEntries;
    
	for ( QNum = 0;  QNum < MAX_Q;  QNum++ )
	{
		pMsgQ->ValidQLength[QNum] = 0;

		WrIdx = h2bMsgQBuf[QNum].WrIdx;
		RdIdx = h2bMsgQBuf[QNum].RdIdx;

		// Read until queue is empty or buffer is full
		for ( i = 0;  i < MAX_DWORDS_PER_MSGQ;  i++ )
		{
			if(RdIdx != WrIdx)
			{
				isDataRead = 1;
				pMsgQ->QData[QNum][i] = h2bMsgQBuf[QNum].QData[RdIdx];
                						h2bMsgQBuf[QNum].QData[RdIdx] = 0;			// Invalidate the buffer for delayed error detection
				pMsgQ->ValidQLength[QNum]++;
				
				if(RdIdx < MSG_Q_BUFF_SIZE - 1 )
				     RdIdx++;
				else RdIdx = 0;
				
//				printk ("*** Read data from host2bmc queue %d: 0x%08X ***\n", QNum + 1, pMsgQ->QData[QNum][i]);
			}
			else
			{
				// queue is empty
				break;
			}
		}
		
		h2bMsgQBuf[QNum].RdIdx = RdIdx;	
	}	
	
	// Be aware that this function and the interrupt service handler are running in parallel. It is therefore possible, that the ISR has disabled the interrupt although this function
	// has freed some space in the queue buffer(s). Therefore an additional check of this special case is necessary here.
	
	if(isFunction1IntEnable == FALSE)								// At least one queue buffer could be full?
	{
	 int QueuesNotFull = 0;
	 
 	 for ( QNum = 0;  QNum < MAX_Q;  QNum++ )						// Check every queue for free space
	 {
 		RdIdx = h2bMsgQBuf[QNum].RdIdx; 
 		WrIdx = h2bMsgQBuf[QNum].WrIdx;
	 
 	 	if ( RdIdx > WrIdx )
 	 		FreeEntries = RdIdx - WrIdx - 1;
 	 	else // RdIdx <= *pWrIdx
 	 		FreeEntries = RdIdx + MSG_Q_BUFF_SIZE - WrIdx - 1;

 	    if(FreeEntries > 0)
	      QueuesNotFull |= (1 << QNum);
	 }
 	 
  	 if(QueuesNotFull == 3)											// There is some space free in both queues?			
 	 {
 		isFunction1IntEnable = TRUE;								// Correct, ...
 		enable_irq(IRQ_PCIE_TEST);									//  ... enable interrupts. Function will not return until a maybe running ISR is finished
  	 }
	}

	return isDataRead;
}


static void Function1b2hRingDoorbell(void)
{
	uint32_t 	Status;

	Status = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
	Status |= B2H_INT_STATUS_DOORBELL;
	iowrite32 (Status, Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
}



static Function1SharedMem_t SharedMem;

#ifdef USE_UNLOCKED_IOCTL						
static long	Function1DriverIoctl (struct file* file, unsigned int cmd, unsigned long arg)
#else
static int	Function1DriverIoctl (struct inode*, struct file* file, unsigned int cmd, unsigned long arg)
#endif
{ 
	Function1MsgQ_t MsgQ;
	uint32_t Temp;
	int i;
	
	switch (cmd)
	{
	case ENABLE_SHARED_MEM:
		{
			volatile uint32_t Temp;

			Temp = Function1ReadReg32 (CONFIG_SPACE_OFFSET + 0x10);
			Temp |= 0x0E;
			Function1WriteReg32 (Temp, CONFIG_SPACE_OFFSET + 0x10);
			EnableSharedMemoryAccess = 1;
		}
	break;	
	case DISABLE_SHARED_MEM:
		{
			volatile uint32_t Temp;

			Temp = Function1ReadReg32 (CONFIG_SPACE_OFFSET + 0x10);
			Temp &= ~0x0E;
			Function1WriteReg32 (Temp, CONFIG_SPACE_OFFSET + 0x10);
			EnableSharedMemoryAccess = 0;
		}
	break;
	
	case CLEAR_H2B_MSG_BUFFERS:
		{
		  volatile uint32_t Temp;
		  if (ioread32 (Function1RemapAddr + HOST_TO_BMC_Q_STATUS) != 0)  // is interface initialized?
		  {
			disable_irq(IRQ_PCIE_TEST);
			
			// clear HOST_TO_BMC_Q1
			Temp = ioread32 (Function1RemapAddr + HOST_TO_BMC_Q_STATUS);
			if( (Temp & H2B_QUEUE1_EMPTY) == 0 )
			{
				uint32_t data;
				for ( i = 0;  i <0x10;  i++ )		// read 16 times, read more than queue depth
				{
					data = Function1ReadReg32(HOST_TO_BMC_Q1);
//					printk("%d Clear Q1: %x   Flag: %x\n",i,data, Temp);
					if (data == 0xFFFFFFFF) break;
				}
				Temp = ioread32 (Function1RemapAddr + HOST_TO_BMC_Q_STATUS);
				if( (Temp & H2B_QUEUE1_EMPTY) == 0 )
				   TCRIT ("Function1DriverIoctl():in CLEAR_H2B_MSG_BUFFERS ioctl, cannot clear HOST_TO_BMC_Q1: %x\n", Temp);				
			}

			// clear HOST_TO_BMC_Q2
			Temp = ioread32 (Function1RemapAddr + HOST_TO_BMC_Q_STATUS);
			if( (Temp & H2B_QUEUE2_EMPTY) == 0 )
			{
				uint32_t data;
				for ( i = 0;  i <0x10;  i++ )		// read 16 times, read more than queue depth
				{
					data = Function1ReadReg32(HOST_TO_BMC_Q2);
//					printk("%d Clear Q2: %x   Flag: %x\n",i,data, Temp);
					if (data == 0xFFFFFFFF) break;
				}
				Temp = ioread32 (Function1RemapAddr + HOST_TO_BMC_Q_STATUS);
				if( (Temp & H2B_QUEUE2_EMPTY) == 0 )
				   TCRIT ("Function1DriverIoctl():in CLEAR_H2B_MSG_BUFFERS ioctl, cannot clear HOST_TO_BMC_Q2 %x\n",Temp);				
			}

			
			/* old code
			for ( i = 0;  i < MAX_DWORDS_PER_MSGQ;  i++ )
			{
				Temp = Function1ReadReg32(HOST_TO_BMC_Q1);
				printk("%d Clear Q1: %x\n",i,Temp);
				Temp = Function1ReadReg32(HOST_TO_BMC_Q2);
				printk("%d Clear Q2: %x\n",i,Temp);
			}
			*/
			memset( &h2bMsgQBuf, 0, sizeof(h2bMsgQBuf) );
			enable_irq(IRQ_PCIE_TEST);			
		}
	  }
	break;

	case READ_FROM_SHARED_MEM:
		if (EnableSharedMemoryAccess == 0)
		{
			TCRIT ("Function1DriverIoctl():in READ_FROM_SHARED_MEM ioctl received without enabling shared memory access\n");
			return -1;			
		}
	       if (__copy_from_user((void *)(&SharedMem), (void*)arg, sizeof(SharedMem.Offset) + sizeof (SharedMem.Len)))
	       {
	            TCRIT ("Function1DriverIoctl(): __copy_from_user failed in READ_FROM_SHARED_MEM\n");
	            return -1;
	       }
		if (SharedMem.Offset + SharedMem.Len	>= FUNCTION1_SHARED_MEM_LEN)
		{
			TCRIT ("Function1DriverIoctl():in READ_FROM_SHARED_MEM ioctl, Shared memory access denied as offset %d and Len %d are beyond shared memory %d limits\n", SharedMem.Offset,  SharedMem.Len,  FUNCTION1_SHARED_MEM_LEN);
			return -1;			
		}
		if (__copy_to_user (((Function1SharedMem_t*)arg)->Data, SharedMemRemapAddr+SharedMem.Offset, SharedMem.Len))
		{
	           TWARN ("Function1DriverIoctl(): __copy_to_user failed in READ_FROM_SHARED_MEM\n");
	           return -1;
		}
		
	break;

	case WRITE_TO_SHARED_MEM:
		if (EnableSharedMemoryAccess == 0)
		{
			TCRIT ("Function1DriverIoctl():in WRITE_TO_SHARED_MEM ioctl received without enabling shared memory access\n");
			return -1;			
		}
	        if (__copy_from_user((void *)(&SharedMem), (void*)arg, sizeof(Function1SharedMem_t)))
	        {
	            TCRIT ("Function1DriverIoctl(): __copy_from_user failed in WRITE_TO_SHARED_MEM\n");
	            return -1;
	        }
		if (SharedMem.Offset + SharedMem.Len	>= FUNCTION1_SHARED_MEM_LEN)
		{
			TCRIT ("Function1DriverIoctl():in WRITE_TO_SHARED_MEM ioctl, Shared memory access denied as offset %d and Len %d are beyond shared memory %d limits\n", SharedMem.Offset,  SharedMem.Len,  FUNCTION1_SHARED_MEM_LEN);
			return -1;
		}
		memcpy (SharedMemRemapAddr+SharedMem.Offset, SharedMem.Data, SharedMem.Len);
	break;


	case WAIT_FOR_HOST_MSG:
	{
		uint8_t isDataRead;
		
		h2bMsgQDataIrqFlag = 0;
		
		isDataRead = Function1ReadMsgQData( &MsgQ );
		
		if ( isDataRead == 0 )
		{
//			printk ("*** Wait for PCIe function1 interrupt ***\n");
			wait4h2bMsgQDataIrq = 1;
			(void) wait_event_interruptible_timeout(Function1RecSleepHead, (h2bMsgQDataIrqFlag == 1), 1000);		// Wait up to 10 seconds
			wait4h2bMsgQDataIrq = 0;
			isDataRead = Function1ReadMsgQData( &MsgQ );
		}
		else 
		{
//			printk ("*** PCIe function1 data in queue:  Q1: %d, Q2: %d ***\n", MsgQ.ValidQLength[0], MsgQ.ValidQLength[1]);		
		}
		
#if 0		
		if ( (isDataRead != 0) && (isFunction1IntEnable == FALSE) )
		{
//			printk("*** PCIe function1 interrupt enable\n");
			isFunction1IntEnable = TRUE;
			enable_irq(IRQ_PCIE_TEST);
		}
#endif
		
//		printk ("*** PCIe function1 copy to user\n");		
		if (__copy_to_user((void*)arg, &MsgQ,  sizeof (Function1MsgQ_t)))
		{
			TWARN ("Function1DriverIoctl(): __copy_to_user failed in WAIT_FOR_HOST_MSGQ2\n");
			return -1;				
		}			
		
	}
	break;

	//
	// special Cmd to clear the message buffer at the start of the function1 daemon
	//
	case CLEAR_MSG_FROM_HOST:
	{
		uint8_t isDataRead;
		
		h2bMsgQDataIrqFlag = 0;
		
		isDataRead = Function1ReadMsgQData( &MsgQ );

#if 0			
		if ( (isDataRead != 0) && (isFunction1IntEnable == FALSE) )
		{
//			printk("*** PCIe function1 interrupt re-enable\n");
			isFunction1IntEnable = TRUE;
			enable_irq(IRQ_PCIE_TEST);
		}
#endif
		
//			printk ("*** PCIe function1 copy to user\n");		
		if (__copy_to_user((void*)arg, &MsgQ,  sizeof (Function1MsgQ_t)))
		{
			TWARN ("Function1DriverIoctl(): __copy_to_user failed in CLEAR_MSG_FROM_HOST\n");
			return -1;				
		}			
			
	}
	break;

	case SEND_MSG_TO_HOST:
	{
		uint16_t 	QNum;
		bool		isMsgSent = 0;
		uint32_t	QFullMask;
		long		timeout = 10;	// in jiffies (10ms)
		int			Cnt=0, Delay;
		
		if (__copy_from_user((void *)(&MsgQ), (void*)arg,  sizeof (Function1MsgQ_t)))
		{
			TCRIT ("Function1DriverIoctl(): __copy_from_user failed in SEND_MSG_TO_HOST ioctl\n");
			return -1;
		}
		if (MsgQ.QNum > MSG_Q2)
		{
			TCRIT ("Function1DriverIoctl (): Invalid message queue %d received in SEND_MSG_TO_HOST ioctl\n", MsgQ.QNum);
			return -1;
		}
		QNum = MsgQ.QNum;
		if (MsgQ.ValidQLength[QNum] > MAX_DWORDS_PER_MSGQ)
		{
			TCRIT ("Function1DriverIoctl (): Invalid message queue size %d received in SEND_MSG_TO_HOST ioctl\n", MsgQ.ValidQLength[QNum]);
			return -1;
		}

		if (MsgQ.QNum == MSG_Q1)
			QFullMask = B2H_QUEUE1_FULL;
		else
			QFullMask = B2H_QUEUE2_FULL;
		
		for (i = 0; i < MsgQ.ValidQLength[QNum]; i++)
		{
			b2hMsgQFullFlag[MsgQ.QNum] = 1;		// must be set in advance to avoid races
			
			Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
			if( (Temp & QFullMask) == 0 )
				b2hMsgQFullFlag[MsgQ.QNum] = 0;
			else
			{
				long	ret;

				
//JS:				printk ("BMC_TO_HOST_Q_STATUS = 0x%08x\n", Temp);				
//JS:				TWARN ("Function1DriverIoctl(): in SEND_MSG_TO_HOST BMC 2 HOST Q is found to be full\n");
				
				if ( isMsgSent == 1 )
					Function1b2hRingDoorbell();
				
				Delay = 200;
				
				while ( 1 )
				{

					Cnt += timeout;										//JS: Needed for a delayed warning message
					wait4b2hMsgQNotFullIrq[MsgQ.QNum] = 1;
					ret = wait_event_interruptible_timeout(Function1SendSleepHead[MsgQ.QNum], (b2hMsgQFullFlag[MsgQ.QNum] == 0), timeout);
					wait4b2hMsgQNotFullIrq[MsgQ.QNum] = 0;					
					if ( ret > 0 )
					{
					 if(Cnt-ret > 30)	                                //JS: Show warning for waiting times longer than 300ms
					  TWARN ("Function1DriverIoctl(): in SEND_MSG_TO_HOST BMC 2 HOST Q was at least for %dms full\n", (Cnt-(int)ret)*10);
					 break;
					}
					//In case of an signal we should exit the function
					else if ( ret < 0 )
					{
						return -1;
					}
					// Timeout, check without interrupt
					Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
					
					if( (Temp & QFullMask) == 0 )
					{
					 if(Cnt-ret > 30)	                                //JS: Show warning for waiting times longer than 300ms
					  TWARN ("Function1DriverIoctl(): in SEND_MSG_TO_HOST BMC 2 HOST Q was at least for %dms full. Poll wakeup\n", (Cnt-(int)ret)*10);
					 
					 b2hMsgQFullFlag[MsgQ.QNum] = 0;
					 
					 break;
					}
					
					if((Cnt <= 800) && ((Cnt % Delay) == 0))               //JS: Show warnings in the first 8 seconds
					{
					  Delay += Delay;
					  TWARN ("Function1DriverIoctl(): in SEND_MSG_TO_HOST BMC 2 HOST Q still full since %dms\n", (Cnt-(int)ret)*10);
					}
				}				
			}
			isMsgSent = 1;
			iowrite32 (MsgQ.QData[QNum][i], Function1RemapAddr + BMC_TO_HOST_Q1  +  MsgQ.QNum *  0x10);
/*
			{ 
				// NZ: for debug only
				uint32_t *B2HData;
				uint32_t *rdoffsetB2H;
				uint32_t *addr;

				if((MsgQ.QData[QNum][i] & 0xFF) == 0x08)
				{
					B2HData = (uint32_t *)0x404B6DD8;
					rdoffsetB2H = (uint32_t *)0x404B6DD0;
					addr = (uint32_t *)((uint32_t)(*rdoffsetB2H) + (uint32_t)B2HData + 0x14);
					printk("->%x  %x  %x %x<-\n",MsgQ.QData[QNum][i],(*rdoffsetB2H),(*addr),*((uint32_t *)((uint32_t)addr+4)));					
				}
				else
				{
					printk("->%x<-\n",MsgQ.QData[QNum][i]);										
				}
			}
*/
		}

		if ( isMsgSent == 1 )
			Function1b2hRingDoorbell();
	}				
	break;
	
	case CLEAR_QUEUES:
	{
		ClearQueues();
		ClearQueues();
	}
	break;

#if 0
	case SEND_MSG_TO_HOST_OLD:
		if (__copy_from_user((void *)(&MsgQ), (void*)arg,  sizeof (Function1MsgQ_t)))
		{
			TCRIT ("Function1DriverIoctl(): __copy_from_user failed in SEND_MSG_TO_HOST ioctl\n");
			return -1;
		}
		if (MsgQ.QNum > MSG_Q2)
		{
			TCRIT ("Function1DriverIoctl (): Invalid message queue %d received in SEND_MSG_TO_HOST ioctl\n", MsgQ.QNum);
			return -1;
		}
		QNum = MsgQ.QNum;
		if (MsgQ.ValidQLength[QNum] > MAX_DWORDS_PER_MSGQ)
		{
			TCRIT ("Function1DriverIoctl (): Invalid message queue size %d received in SEND_MSG_TO_HOST ioctl\n", MsgQ.ValidQLength[QNum]);
			return -1;
		}

		Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
		if((((0 == (Temp & B2H_QUEUE2_EMPTY)) && (MsgQ.QNum == MSG_Q2)) || ((0 ==(Temp & B2H_QUEUE1_EMPTY)) && (MsgQ.QNum == MSG_Q1))))
		{
			printk ("BMC_TO_HOST_Q_STATUS = 0x%08x\n", Temp);				
			TWARN ("Function1DriverIoctl(): in SEND_MSG_TO_HOST BMC 2 HOST Q is found to be not empty\n");
			return -1;
		}	
		*/

		for (i = 0; i < MsgQ.ValidQLength[QNum]; i++)
		{
			Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
			if( ((Temp & B2H_QUEUE2_FULL) && (MsgQ.QNum == MSG_Q2)) || ((Temp & B2H_QUEUE1_FULL) && (MsgQ.QNum == MSG_Q1)))
			{
				printk ("BMC_TO_HOST_Q_STATUS = 0x%08x\n", Temp);				
				TWARN ("Function1DriverIoctl(): in SEND_MSG_TO_HOST BMC 2 HOST Q %d is found to be full  i: %d\n",QNum, i);
				Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
				printk ("BMC_TO_HOST_Q_STATUS = 0x%08x\n", Temp);
			}	
			iowrite32 (MsgQ.QData[QNum][i], Function1RemapAddr + BMC_TO_HOST_Q1  +  MsgQ.QNum *  0x10);
		}

		Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
		Temp |= B2H_QUEUE2_FULL_UNMASK | B2H_QUEUE1_FULL_UNMASK;
		iowrite32 (Temp, Function1RemapAddr + BMC_TO_HOST_Q_STATUS);

		if (MsgQ.ValidQLength[QNum] < MAX_DWORDS_PER_MSGQ)
		{
			Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
			Temp |= B2H_INT_STATUS_DOORBELL;
			iowrite32 (Temp, Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
		}
#ifdef CONFIG_SPX_FEATURE_GLOBAL_PCIE_FUNCTION1_ENABLE_HOST_INTR
		Temp = ioread32 (Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
		Temp |= B2H_ENABLE_INTB;
		iowrite32 (Temp, Function1RemapAddr + BMC_TO_HOST_Q_STATUS);
#endif
		
	break;
#endif
	
	default:
	TCRIT ("Unsupported IOCTL %d is received for Function1DriverIoctl\n", cmd);
	return -1;
	}
	return 0; 
} 


//
// clear H2B Queues
//
static void ClearQueues()
{
	int i;
	volatile uint32_t Temp;

	/* clear h2bq1 and h2bq2 */
	for (i = 0; i <= MAX_DWORDS_PER_MSGQ; i++)
	{
		Temp = Function1ReadReg32 (HOST_TO_BMC_Q1);
		if (Temp == 0xFFFFFFFF) break;
#if 0
		else printk ("HOST_TO_BMC_Q1 data = 0x%08x\n", Temp);
#endif
	}
	if (i >= MAX_DWORDS_PER_MSGQ && Temp != 0xFFFFFFFF)
	{
		Temp = Function1ReadReg32(HOST_TO_BMC_Q_STATUS);
#if 0
		TWARN ("host2bmc message queue1 could not be cleared; status: 0x%08x\n",Temp);
#endif
	}
	for (i = 0; i <= MAX_DWORDS_PER_MSGQ; i++)
	{
		Temp = Function1ReadReg32 (HOST_TO_BMC_Q2);
		if (Temp == 0xFFFFFFFF) break;
#if 0
		else printk ("HOST_TO_BMC_Q2 data = 0x%08x\n", Temp);
#endif
	}
	if (i >= MAX_DWORDS_PER_MSGQ && Temp != 0xFFFFFFFF)
	{
		Temp = Function1ReadReg32(HOST_TO_BMC_Q_STATUS);
#if 0
		TWARN ("host2bmc message queue2 could not be cleared; status: 0x%08x\n",Temp);
#endif
	}

}


module_init (init_function1_module);
module_exit (exit_function1_module);

MODULE_AUTHOR("Rama Rao Bisa <RamaB@ami.com>");
MODULE_DESCRIPTION("PILOT PCIe Function1 driver module");
MODULE_LICENSE ("GPL");

