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
/*************************************************************
Module		: PS2 driver module
Modified by	: Vincent Wang  on 07/10/2008
*************************************************************/
/* Version Number of this module */
#define SAMPLE_MAJOR	1
#define SAMPLE_MINOR	0

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/cacheflush.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/version.h>
#include <linux/cdev.h>

#include "helper.h"
#include "driver_hal.h"
#include "reset.h"
#include "ps2main.h"
#include "ps2.h"

#ifdef HAVE_UNLOCKED_IOCTL
  #if HAVE_UNLOCKED_IOCTL
  #define USE_UNLOCKED_IOCTL
  #endif
#endif

static int ps2_irq_requested = 0;
//static int mouse_irq_flag = 0;
//static int keyboard_irq_flag = 0;

/*** Prototype Declaration ***/
static int PS2_MAJOR = 55;
static const char PS2_DEVNAME[] = "PS2";
//static void SendPS2Response (u8 ChannelNum, IPMICmdMsg_T *pKCSCmdMsg);

static int 	init_ps2_module(void);
static void cleanup_ps2_module(void);

static ssize_t keyboard_read(struct file * filp, char * buf,size_t count,loff_t *ppos);
static ssize_t mouse_read(struct file * filp, char * buf,size_t count,loff_t *ppos);

static ssize_t keyboard_write (struct file *file, const char *buf, size_t count,loff_t *offset);
static ssize_t mouse_write (struct file *file, const char *buf, size_t count,loff_t *offset);

static int keyboard_release(struct inode * inode, struct file * filp);
static int mouse_release(struct inode * inode, struct file * filp);

#ifdef USE_UNLOCKED_IOCTL
static long ps2_ioctl(struct file *file,
		     unsigned int cmd, unsigned long arg);
#else
static int ps2_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg);
#endif

static int ps2_open(struct inode * inode, struct file * filp);

static int ReceivePS2Data (char * buf , u8 ChannelNum);

static size_t WritePS2CMD (const char *buf, size_t count, u8 ChannelNum );

void PS2_OsInitSleepStruct(PS2_OsSleepStruct *Sleep);
void PS2_OsWakeupOnTimeout(PS2_OsSleepStruct *Sleep);
long PS2_OsSleepOnTimeout(PS2_OsSleepStruct *Sleep,u8 *Var,long msecs);
static void SleepTimeOut(unsigned long SleepPtr);
typedef int (*HostResetHandler) (void);
static int PS2HostResetHandler(void);
static wait_queue_head_t PS2sleep;
static int Cond = 0;
static HostResetHandler PS2_HostResetHandler = PS2HostResetHandler;

static struct file_operations ps2_fops = {
	owner:		THIS_MODULE,
	read:		NULL,
	write:		NULL,
#ifdef USE_UNLOCKED_IOCTL
	unlocked_ioctl:		ps2_ioctl,
#else
  ioctl: ps2_ioctl,
#endif
	open:		ps2_open,
	release:	NULL,
};

static struct file_operations keyboard_fops = {
	owner:		THIS_MODULE,
	read:		keyboard_read,
	write:		keyboard_write,
#ifdef USE_UNLOCKED_IOCTL
  unlocked_ioctl:   ps2_ioctl,
#else
  ioctl: ps2_ioctl,
#endif

	open:		NULL,
	release:	keyboard_release,
};

static struct file_operations mouse_fops = {
	owner:		THIS_MODULE,
	read:		mouse_read,
	write:		mouse_write,
#ifdef USE_UNLOCKED_IOCTL
  unlocked_ioctl:   ps2_ioctl,
#else
  ioctl: ps2_ioctl,
#endif

	open:		NULL,
	release:	mouse_release,
};

static PS2Buf_T		m_PS2Buf [PS2_MAX_PORTS];

volatile u8 * ps2_vadd;

/*--------------------- If Module define the entry and exit  ------------------*/
#ifdef MODULE

static irqreturn_t ps2_handler ( int irq, void *dev_id)
{	
	u8	Value;
	PS2Buf_T*		pPS2Buf;


	READ_INT_STATUS_REG(KEYBOARD,Value);
	if(Value & KMISR_RXINT)
	{
		ENABLE_FORCE_CLOCK_LOW(KEYBOARD);
		pPS2Buf = &m_PS2Buf [KEYBOARD];
		READ_DATA(KEYBOARD, Value);
		pPS2Buf->PS2RcvData [0] = Value;
		pPS2Buf->DataFlag = 1;
		if (pPS2Buf->PS2IFActive)
		{
			pPS2Buf->PS2Wakeup = 1;
			PS2_OsWakeupOnTimeout(&(pPS2Buf->PS2ReceiveWait));
		}
	}

	READ_INT_STATUS_REG(MOUSE,Value);
	if(Value & KMISR_RXINT)
	{
		ENABLE_FORCE_CLOCK_LOW(MOUSE);
		pPS2Buf = &m_PS2Buf [MOUSE];
		READ_DATA(MOUSE, Value);
//		printk("{%02x}", Value);
		pPS2Buf->PS2RcvData [0] = Value;
		pPS2Buf->DataFlag = 1;
		if (pPS2Buf->PS2IFActive)
		{
			pPS2Buf->PS2Wakeup = 1;
			PS2_OsWakeupOnTimeout(&(pPS2Buf->PS2ReceiveWait));
		}
	}
	return (IRQ_HANDLED);
}

static int 
PS2HostResetHandler(void)
{
	Cond = 1;
	wake_up (&PS2sleep);
	return 0;
}


static int
init_ps2_module(void)
{
    int err, ret = 0, Value;
    u8  ps2port;

	PS2Buf_T*		pPS2Buf1;
	PS2Buf_T*		pPS2Buf2;

	pPS2Buf1 = &m_PS2Buf [KEYBOARD];
	pPS2Buf2 = &m_PS2Buf [MOUSE];

	printk("PS2 Driver Version %d.%d\n",SAMPLE_MAJOR,SAMPLE_MINOR);
	printk("Copyright (c) 2009-2015 American Megatrends Inc.\n");

   /* ----  Register the character device ------------------- */
    if ((err = register_chrdev(PS2_MAJOR, PS2_DEVNAME, &ps2_fops)) < 0)
    {
		printk ("failed to register device (err %d)\n", err);
		ret = -ENODEV;
		goto fail;
    }

	ps2_vadd = (u8*)SE_PS2_VA_BASE;

	pPS2Buf1->PS2Wakeup = 0;
	pPS2Buf2->PS2Wakeup = 0;

	pPS2Buf1->PS2IFActive = 0;
	pPS2Buf2->PS2IFActive = 0;

	pPS2Buf1->FirstTime = 1;
	pPS2Buf2->FirstTime = 1;
	
	pPS2Buf1->DataFlag = 0;
	pPS2Buf2->DataFlag = 0;

	if ((err = request_irq(IRQ_PS2, ps2_handler, IRQF_DISABLED, "ps2", 0)) < 0)
	{
		printk("failed to request irq %d (err %d)", IRQ_PS2, err);
 	    ret = -EBUSY;
		goto fail;
	}
	ps2_irq_requested = 1;

    /* Enable the PS2 interface and let CLK and DATA lines going low, indicating we are busy */
    for (ps2port=0; ps2port<PS2_MAX_PORTS; ps2port++)
    {
		WRITE_PS2_CONTROL_REG(ps2port,0);
		ENABLE_PS2_INTERFACE(ps2port);
		/* Set PS2 Divisor*/
		//SET_PS2_DIVISOR_REG(ps2port,0x0C);

		/* Dummy read for make sure no any stale interrupts */
		READ_DATA(ps2port, Value);

		/* Enable PS2 Receive Interrupt */
		Value = KMCTL_RXIEN;
		ENABLE_PS2_INTERRUPTS(ps2port, Value);
	}
	init_waitqueue_head(&PS2sleep);
	install_reset_handler(PS2_HostResetHandler);
	return ret;	/* Return 1 to not load the module */

fail:
	cleanup_ps2_module ();
	return ret;
}

static void
cleanup_ps2_module(void)
{

	printk("Unloading PS2 Module ..............\n");

	/* free irq */
	if (ps2_irq_requested)
	{
		free_irq(IRQ_PS2, 0);
		ps2_irq_requested = 0;
	}

	/* unregister the device */
	unregister_chrdev(PS2_MAJOR, PS2_DEVNAME);
	uninstall_reset_handler(PS2_HostResetHandler);
	printk("PS2 module unloaded sucessfully.\n");

	return;
}

/*********** Open PS2 ***********/
static int ps2_open(struct inode * inode, struct file * filp)
{
	//PS2Buf_T*		pPS2Buf;

	switch (MINOR(inode->i_rdev)) {
	case 0:
		ENABLE_PS2_INTERFACE(KEYBOARD);
		
		//pPS2Buf = &m_PS2Buf [KEYBOARD];
		filp->f_op = &keyboard_fops;
		break;
	
	case 1:
		//pPS2Buf = &m_PS2Buf [MOUSE];
		filp->f_op = &mouse_fops;
		break;
	
	default:
		printk("Default\n");
		return -ENXIO;
        }
	return 0;
}

/*********** Receive Keyboard/Mouse Data ***********/
static ssize_t keyboard_read(struct file *filp, char * buf,size_t count,loff_t *ppos)
{
	return ( ReceivePS2Data( (void *)buf , KEYBOARD));
}

static ssize_t mouse_read(struct file *filp, char * buf,size_t count,loff_t *ppos)
{
	return ( ReceivePS2Data( (void *)buf , MOUSE));
}

static int ReceivePS2Data(char * buf , u8 ChannelNum)
{
	PS2Buf_T*	pPS2Buf;
	pPS2Buf = &m_PS2Buf [ChannelNum];

	/* Initialize Sleep Structure for the First Time */
	if (pPS2Buf->FirstTime)
	{
		PS2_OsInitSleepStruct(&(pPS2Buf->PS2ReceiveWait));
		pPS2Buf->FirstTime = 0;
	}

	pPS2Buf->PS2IFActive = 1;
	if (pPS2Buf->DataFlag == 0)
		PS2_OsSleepOnTimeout(&(pPS2Buf->PS2ReceiveWait),&(pPS2Buf->PS2Wakeup),0);
	pPS2Buf->PS2Wakeup = 0;
	pPS2Buf->PS2IFActive = 0;

	/*Copy from kernel data area to user data area*/
	if (__copy_to_user( (void *)buf, (void *)&pPS2Buf->PS2RcvData, 1))
	{
		return -1;
	}
	pPS2Buf->DataFlag = 0;

	DISABLE_FORCE_CLOCK_LOW(ChannelNum);
	return 0;
}

/*********** Write Keyboard/Mouse Command ***********/
static ssize_t keyboard_write (struct file *file, const char *buf, size_t count, loff_t *offset)
{
	return ( WritePS2CMD( (void *)buf , count ,KEYBOARD));
}

static ssize_t mouse_write (struct file *file, const char *buf, size_t count, loff_t *offset)
{
	return ( WritePS2CMD( (void *)buf , count ,MOUSE));
}

static size_t WritePS2CMD (const char *buf, size_t count, u8 ChannelNum)
{
	u8			i;
	PS2Buf_T*	pPS2Buf;
	pPS2Buf = 	&m_PS2Buf [ChannelNum];

	/* Copy from user data area to kernel data area*/
	if (__copy_from_user((void *)&pPS2Buf->PS2WRCmd,(void *)buf,count))
	{
		return -1;
	}
	for(i=0; i<count; i++)
		WRITE_CMD(ChannelNum,pPS2Buf->PS2WRCmd[i]);

	return 0;
}

/*********** OS Sleep and Wake Up ***********/
void 
PS2_OsInitSleepStruct(PS2_OsSleepStruct *Sleep)
{
	init_waitqueue_head(&(Sleep->queue));
	init_timer(&(Sleep->tim));
	Sleep->Timeout = 0;
	return;
}

void 
PS2_OsWakeupOnTimeout(PS2_OsSleepStruct *Sleep)
{
	//int dtimer;

	/* Wakeup Process and Kill timeout handler */
	del_timer(&(Sleep->tim));
	wake_up(&(Sleep->queue));

	if (Sleep->Timeout)
		printk("WARNING:Wakeup for Sleep Struct Called after timeout\n");
	return;
}

long 
PS2_OsSleepOnTimeout(PS2_OsSleepStruct *Sleep,u8 *Var,long msecs)
{
	long timeout;	/* In jiffies */
	//int  dtimer;
	u8 *volatile Condition = Var;

	/* Convert milliseconds into jiffies*/
	timeout = (HZ*msecs)/1000;

	/* Start the timeout */
	Sleep->tim.function = SleepTimeOut;
	Sleep->tim.data     = (unsigned long)Sleep;
	Sleep->tim.expires  = jiffies+timeout;
	Sleep->Timeout      = 0;
	if (msecs)
		add_timer(&(Sleep->tim));

	/* Sleep on the Condition for a wakeup */
	wait_event_interruptible(Sleep->queue,((*Condition)||(Sleep->Timeout)));

	/* Normally del_timer will be called by Wakeup or timeout routine */
	/* But when wakeup is called before add_timer() and wait_event(),
      it will not be killed by wakeup routine and will be killed only
	   after a timeout happens. So we kill it here as soon as possible */
	if (msecs)
		del_timer(&(Sleep->tim));

	if (Sleep->Timeout)
		return 0;
	return timeout;
}

static
void
SleepTimeOut(unsigned long SleepPtr)
{	
	PS2_OsSleepStruct *Sleep;

	/* Set Timeout and Wakeup Sleep Process */
	Sleep = (PS2_OsSleepStruct *)SleepPtr;
	Sleep->Timeout = 1;
	wake_up(&(Sleep->queue));

	return;
}








/*********** PS2 IO Control ***********/
#ifdef USE_UNLOCKED_IOCTL
static long ps2_ioctl(struct file *file,
		     unsigned int cmd, unsigned long arg)
#else
static int ps2_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
#endif
{
	u8			Status;
	PS2Buf_T*	pPS2Buf;


	switch(cmd)
	{
	case INFORM_LPC_RESET:
		wait_event_interruptible(PS2sleep, (Cond == 1));
		Cond = 0;
		break;
	case ENABLE_KEYBOARD:
		ENABLE_PS2_INTERFACE(KEYBOARD);
		break;
	case DISABLE_KEYBOARD:
		DISABLE_PS2_INTERFACE(KEYBOARD);
		break;
	case ENABLE_MOUSE:
		ENABLE_PS2_INTERFACE(MOUSE);
		break;
	case DISABLE_MOUSE:
		DISABLE_PS2_INTERFACE(MOUSE);
		break;
	case RESET_PS2:
		DISABLE_PS2_INTERFACE(MOUSE);
		ENABLE_PS2_INTERFACE(MOUSE);
		break;
	case READ_KB_STATUS:

		if ( copy_from_user(&pPS2Buf, (void*)arg, 1))
			return -EFAULT;

		pPS2Buf = &m_PS2Buf [KEYBOARD];
		READ_PS2_STATUS_REG(KEYBOARD, Status);
		pPS2Buf->PS2RcvData [0] = Status;
		if (__copy_to_user( (void*)arg, (void *)&pPS2Buf->PS2RcvData, 1))
		{
			return -1;
		}
		break;

	case READ_MS_STATUS:
		if ( copy_from_user(&pPS2Buf, (void*)arg, 1))
			return -EFAULT;

		pPS2Buf = &m_PS2Buf [MOUSE];
		READ_PS2_STATUS_REG(MOUSE, Status);
		pPS2Buf->PS2RcvData [0] = Status;
		if (__copy_to_user( (void*)arg, (void *)&pPS2Buf->PS2RcvData, 1))
		{
			return -1;
		}
		break;
    default:
		printk("Invalid IOCTL command\n");
		break;
	}
	return 0;
}

static int keyboard_release(struct inode * inode, struct file * filp)
{
	DISABLE_PS2_INTERFACE(KEYBOARD);
	
	return 0;
}

static int mouse_release(struct inode * inode, struct file * filp)
{
	return 0;
}

/*--------------------------- Module Information Follows --------------------------*/
module_init (init_ps2_module);
module_exit (cleanup_ps2_module);

MODULE_AUTHOR(" Vincent Wang - American Megatrends Inc");
MODULE_DESCRIPTION("This is PS2 driver module.");
MODULE_LICENSE ("GPL");
#endif	/* MODULE */
