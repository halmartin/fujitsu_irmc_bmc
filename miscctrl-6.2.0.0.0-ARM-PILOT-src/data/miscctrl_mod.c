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
#include <linux/version.h>
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>	/* printk() 		*/
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include "miscctrl.h"
#include <linux/poll.h>


#define MISCCTRL_DRIVER_MAJOR	102
#define MISCCTRL_DRIVER_MINOR	0

#define MAX_PASS_THRU_INTR      0x7
#define MAX_PWBTIN_INTR         0x3
#define BIT0                    0x1
#define BIT1                    0x2
#define BIT2                    0x4


static int miscctrl_irq_RTC_requested  = 0;
static int miscctrl_irq_SWM_requested  = 0;

static int miscctrl_drv_registered = 0;
static int m_chassis_intr_num = 0;

static unsigned char chassis_intrusion_on_init = 0;

static int  init_miscctrl_module(void);
static void exit_miscctrl_module(void);

static int miscctrl_open(struct inode * inode, struct file * filp);
static int miscctrl_close(struct inode * inode, struct file * filp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
static int miscctrl_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg);
#else
static long miscctrl_ioctl(struct file *file,unsigned int cmd, unsigned long arg);
#endif

static unsigned int miscctrl_poll(struct file* file, poll_table *wait);

static irqreturn_t miscctrl_RTChandler (int irq, void *dev_id);
static irqreturn_t miscctrl_SWMhandler (int irq, void *dev_id);

static struct file_operations miscctrl_fops = 
{
	owner:		THIS_MODULE,
	read:		NULL,
	write:		NULL,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
	ioctl:		miscctrl_ioctl,
#else
  unlocked_ioctl: miscctrl_ioctl,
#endif
	open:		miscctrl_open,
	release:	miscctrl_close,
	poll:		miscctrl_poll
};

DECLARE_WAIT_QUEUE_HEAD(g_rtc_chassis_intrusion_wq);
DECLARE_WAIT_QUEUE_HEAD(g_swm_powergood_wq);
DECLARE_WAIT_QUEUE_HEAD(g_pass_thru_wq);
DECLARE_WAIT_QUEUE_HEAD(g_pwbtin_wq);
DECLARE_WAIT_QUEUE_HEAD(g_slp_state_wq);
DECLARE_WAIT_QUEUE_HEAD(g_s3s5_asserted_state_wq);
static u8 g_rtc_chassis_intrusion_pending = 0;

static u8 g_swm_powergood_pending =0;
static u8 g_pass_thru_status = 0;
static u8 g_pwbtin_status = 0;
static u8 g_slp_status = 0;
static u8 g_s3s5_asserted_status=0;

uint32_t g_total_swc_interrupt;
swc_interrupt_t* g_swc_intr;
pending_swc_interrupt_t *g_front_swc_ints = NULL;
pending_swc_interrupt_t *g_rear_swc_ints = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
DEFINE_SPINLOCK(g_swc_lock);
#else
spinlock_t g_swc_lock = SPIN_LOCK_UNLOCKED;
#endif

struct timer_list power_timer;

static void power_timer_thread(unsigned long data)
{
    unsigned char icr = 0;
    icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG));
    icr &= (~ENABLE_PWRBTOUT);
    *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;
    del_timer (&power_timer);
}

/* Add the interrupt item at the rear end of the Q */
void add_swc_interrupt(int int_num, unsigned char data)
{
    pending_swc_interrupt_t *newnode;      /* New node to be inserted */
    newnode=(pending_swc_interrupt_t*)kmalloc(sizeof(pending_swc_interrupt_t),GFP_ATOMIC);
    if (NULL == newnode)
    {
        return;
    }
    newnode->int_num = int_num;
    newnode->output_data = data;
    newnode->next = NULL;
    if(g_front_swc_ints == NULL && g_rear_swc_ints == NULL)
    {
        g_front_swc_ints=newnode;
        g_rear_swc_ints=newnode;
    }
    else
    {
        g_rear_swc_ints->next=newnode;
        g_rear_swc_ints=newnode;
    }
}

/* Removes the interrupt item in the front of the Q */
void remove_swc_interrupt(void)
{
    pending_swc_interrupt_t *delnode;      /* Node to be deleted */
    if(g_front_swc_ints == NULL && g_rear_swc_ints == NULL)
        printk("Miscctrl: Queue is empty to delete any element\n");
    else
    {
        delnode = g_front_swc_ints;
        g_front_swc_ints = g_front_swc_ints->next;
        kfree(delnode);
		if(g_front_swc_ints == NULL)
			g_rear_swc_ints = NULL;
    }
}

int
process_swc_intr (int intr_type, unsigned char data)
{
	int j;
	unsigned long flags;

	/* 
	 * Check if it's a registered swc interrupt 
	 * Check is based on intr_type, so the interrupt type 
	 * should be unique for each interrupts registered. 
	 */
	for(j = 0; j < g_total_swc_interrupt; j++)
	{
		if((intr_type == g_swc_intr[j].int_type) && (data & g_swc_intr[j].int_input_data))
		{
			break;
		}
	}
	
	if (j == g_total_swc_interrupt)
	{
//		printk ("Miscctrl: Spurious SWC interrupts\n");
		return 0;
	}
	/* If it's a registered swc interrupt */
	/* Add it to the pending swc interrupts structure */
	spin_lock_irqsave(&g_swc_lock, flags);
	add_swc_interrupt(g_swc_intr[j].int_num, (data & g_swc_intr[j].int_input_data));
	spin_unlock_irqrestore (&g_swc_lock, flags);
	
	return 0;
}

static void 
process_pending_swc_interrupt(void* intr_data)
{
	int int_num;
	int i = 0;
	
	if (g_front_swc_ints == NULL)
		return;

	/* Read it from the pending interrupts */
	int_num = g_front_swc_ints->int_num;

	for(i = 0; i < g_total_swc_interrupt; i++)
	{
		if(int_num == g_swc_intr[i].int_num)
		{
			/* We need to return info for this interrupt swc */
			memcpy(intr_data, (void *) &g_swc_intr[i],sizeof(swc_interrupt_t));
			((swc_interrupt_t *)intr_data)->int_output_data = g_front_swc_ints->output_data;
			break;
		}
	}
	/* Remove the read interrupt entry from the pending interrupts */
	remove_swc_interrupt();
	return;
}

static int 
wait_for_swc_interrupt (void* int_data)
{
	unsigned long flags;
	if(g_front_swc_ints != NULL)
	{
		/* Interrupt already pending */
		spin_lock_irqsave(&g_swc_lock, flags);
		process_pending_swc_interrupt(int_data);
		spin_unlock_irqrestore (&g_swc_lock, flags);
	}
	else
	{
		if(wait_event_interruptible(g_pass_thru_wq,(g_front_swc_ints != NULL)))
			return -ERESTARTSYS;

		spin_lock_irqsave(&g_swc_lock, flags);
		process_pending_swc_interrupt(int_data);
		spin_unlock_irqrestore (&g_swc_lock, flags);
	}
	return 0; 
}

static int 
enable_disable_pwrgd_intr (unsigned char data)
{
	unsigned char icr = 0;
	if(0 ==  data)
	{
	/* Disable  Power Good signal Interrupt  by default */
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+WKP_CFG0_REG));
		icr &= DISABLE_PWRGD_INTR;
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+WKP_CFG0_REG)) = icr;
	}
	else if (1 == data)
	{
		/* Enable Power Good signal Interrupt  by default */
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+WKP_CFG0_REG));
		icr |= ENABLE_PWRGD_INTR;
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+WKP_CFG0_REG)) = icr;
	}
	else
	{
		return -EFAULT;
	}
	return 0;
}

static int 
enable_disable_slp_state_intr (unsigned char data)
{
    unsigned char icr = 0;
    if(0 == data)
    {
        /* Disable SLP State Interrupt  by default */
        icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SW_CTL_REG));
        icr &= (~ENABLE_SLP_STATE_INTR);
        *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SW_CTL_REG)) = icr;
    }
    else if (1 == data)
    {
        /* Enable SLP State Interrupt  by default */
        icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SW_CTL_REG));
        icr |= ENABLE_SLP_STATE_INTR;
        *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SW_CTL_REG)) = icr;
    }
    else
    {
        return -EFAULT;
    }
    return 0;
}

static int 
enable_disable_pwbtin_intr (unsigned char data)
{
	unsigned char stat;
	if (MAX_PWBTIN_INTR < data)
	{
		return -EFAULT;
	}
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PWBTIN_STS_REG));
	stat &= 0xF3;
	stat |=  data << 2;
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PWBTIN_STS_REG)) = stat;
	return 0;
}

static int 
enable_disable_passthru_intr (unsigned char data)
{
	unsigned char stat;
	if (MAX_PASS_THRU_INTR < data)
	{
		return -EFAULT;
	}
	
	/* Enable/Disable interrupt for PASS -THRU */
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS1_CFG));
	if (data & BIT0)
	{
		stat |= ENABLE_PASS_THRU_IRQ;
	}
	else
	{
		stat &= ~ENABLE_PASS_THRU_IRQ;
	}
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS1_CFG)) = stat;
		
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS2_CFG));
	if (data & BIT1)
	{
		stat |= ENABLE_PASS_THRU_IRQ;
	}
	else
	{
		stat &= ~ENABLE_PASS_THRU_IRQ;
	}
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS2_CFG)) = stat;
			
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS3_CFG));
	if (data & BIT2)
	{
		stat |= ENABLE_PASS_THRU_IRQ;
	}
	else
	{
		stat &= ~ENABLE_PASS_THRU_IRQ;
	}
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS3_CFG)) = stat;
	return 0;
}

#define SWC_MAX_CALLBACKS 16

typedef struct swc_callback_t {	
	        int  (*CallBackHandler)(void);
		unsigned char interrupt_type;
} swc_callback;
swc_callback     g_swc_callback[SWC_MAX_CALLBACKS];

void 
SWCCallBack(unsigned char intr_type)
{
	int i;
	for(i = 0; i < SWC_MAX_CALLBACKS; i++)
	{
		if ((NULL != g_swc_callback[i].CallBackHandler) && (intr_type == g_swc_callback[i].interrupt_type))
			(*(g_swc_callback[i].CallBackHandler)) ();
	}
	return ;
}
int
install_swc_handler(int (*pt2Func)(void), unsigned char intr_type)
{
	int i ;
	for (i = 0; i < SWC_MAX_CALLBACKS; i++)
	{
		if (g_swc_callback[i].CallBackHandler == NULL)
		{
			g_swc_callback[i].CallBackHandler = pt2Func;
			g_swc_callback[i].interrupt_type = intr_type; 
			return 0;
		}
	}
	printk("ERROR: No Available Slot to Install SWC Handler\n");
	return 1;
}
int
uninstall_swc_handler(int (*pt2Func)(void), unsigned char intr_type)
{
	int i, retval = 1;
	for (i = 0; i < SWC_MAX_CALLBACKS; i++)
	{
		if((pt2Func == g_swc_callback[i].CallBackHandler) && (intr_type ==  g_swc_callback[i].interrupt_type))
		{
			g_swc_callback[i].CallBackHandler = NULL;
			retval = 0;
		}
	}
	if(retval == 1)
		printk("ERROR: Trying to uninstall a invalid SWC Handler\n");
	return retval;
}
EXPORT_SYMBOL(install_swc_handler);
EXPORT_SYMBOL(uninstall_swc_handler);

static int
enable_swc_interrupts (void)
{
	int i;

	for (i = 0; i < g_total_swc_interrupt; i++)
	{
		switch (g_swc_intr[i].int_type)
		{
		case POWER_GOOD_INTR:
			
			if (0 != enable_disable_pwrgd_intr(g_swc_intr[i].int_input_data))
			{
				printk ("Miscctrl: invalid input data for swc interrupt : %d\n", i);
			}
			break;
		case PASS_THRU_INTR:
			if (0 != enable_disable_passthru_intr(g_swc_intr[i].int_input_data))
			{
				printk ("Miscctrl: invalid input data for passthru interrupt : %d\n", i);
			}
			break;
		case PWRBTNIN_INTR:
			if (0 != enable_disable_pwbtin_intr(g_swc_intr[i].int_input_data))
			{
				printk ("Miscctrl: invalid input data for pwbtin interrupt : %d\n", i);
			}
			break;
		case SLP_STATE_INTR:
			if (0 != enable_disable_slp_state_intr(g_swc_intr[i].int_input_data))
			{
				printk ("Miscctrl: invalid input data for slp state interrupt : %d\n", i);
			}
			break;
		default:
			printk ("Miscctrl: Invalid Interrupt type for SWC\n");
		}
	}
	return 0;
}

static void
enable_battery_clock(void)
{
	unsigned char icr = 0;
	icr = *((volatile unsigned char *)(SE_PILOT_SPEC_VA_BASE + PRS_SIOCFG7_REG));
	icr &= ~SIOCFG7_CLK_DSBL;
	*((volatile unsigned char *)(SE_PILOT_SPEC_VA_BASE + PRS_SIOCFG7_REG)) = icr;
	
	do{
		icr = *((volatile unsigned char *)(SE_PILOT_SPEC_VA_BASE + PRS_SIOCFG7_REG));
	}while(icr & SIOCFG7_CLK_DSBL);
}

static void
disable_battery_clock(void)
{
	unsigned char icr = 0;
	icr = *((volatile unsigned char *)(SE_PILOT_SPEC_VA_BASE + PRS_SIOCFG7_REG));
	icr |= SIOCFG7_CLK_DSBL;
	*((volatile unsigned char *)(SE_PILOT_SPEC_VA_BASE + PRS_SIOCFG7_REG)) = icr;
}

static int
battery_backed_read(void * data, unsigned char offset, unsigned char size)
{
	unsigned char buf[MAX_BAT_BACKED_REG];
	
	if((offset + size) >  MAX_BAT_BACKED_REG )
		return -EINVAL;
	
	enable_battery_clock();
	memcpy(buf, (void *)(SE_RTC_VA_BASE + RTC_BBR_BASE + offset), size);
	disable_battery_clock();
	
	if (copy_to_user(data, buf, size))
		return -EFAULT;
	
	return 0;
}

static int
battery_backed_write(void * data, unsigned char offset, unsigned char size)
{
	unsigned char buf[MAX_BAT_BACKED_REG];
	
	if((offset + size) >  MAX_BAT_BACKED_REG )
		return -EINVAL;
	
	if (copy_from_user(buf, data, size))
		return -EFAULT;
	
	enable_battery_clock();
	memcpy((void *)(SE_RTC_VA_BASE + RTC_BBR_BASE + offset), buf, size);
	disable_battery_clock();
	
	return 0;
}

static void
GenerateSWEvent (unsigned char data, unsigned char local_reg)
{
    u8 icr = 0;
    // Generate low pulse; first reset intstatus, then enable the int, 
    // Generate a sw event and reset status again (pulse is ready). Disable local event in the end
    
    icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + WKP_STS0_REG));
    icr |= SET_SW_EVENT_STS;                                        // reset sw event status reg 0x00
    *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + WKP_STS0_REG)) = icr;
    
    icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + GPE1_STS0_REG));
    icr |= SET_SW_EVENT_STS;                                        // reset sw event status reg 0x2F
    *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + GPE1_STS0_REG)) = icr;
    
    if (0 != (data & ENABLE_LOCAL_EVENT))
    {
        icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + local_reg));
        icr |= SET_SW_EVENT_STS;                                        // local event enable
        *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + local_reg)) = icr;
    }
    
    if (0 != (data & GENERATE_EVENT))
    {
        icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + SW_CTL_REG));
        icr |= GEN_SW_EVENT;                                        // generate sw event if GenSWEvent is set
        *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + SW_CTL_REG)) = icr;
    }
    
    icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + WKP_STS0_REG));
    icr |= SET_SW_EVENT_STS;                                        // reset sw event status reg 0x00
    *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + WKP_STS0_REG)) = icr;
    
    icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + GPE1_STS0_REG));
    icr |= SET_SW_EVENT_STS;                                        // reset sw event status reg 0x2F
    *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + GPE1_STS0_REG)) = icr;
    
    icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + local_reg));
    icr &= (~SET_SW_EVENT_STS);                                        // disable local Event.
    *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE + local_reg)) = icr;
    return;
}

static int 
miscctrl_close(struct inode * inode, struct file * filp)
{
	//printk("MISCCTRL: device close \n");
	return 0;
}

static int 
miscctrl_open(struct inode * inode, struct file * filp)
{
	//printk("MISCCTRL: device open \n");
	return 0;
}


static int
init_miscctrl_module(void)
{
	int err, ret = 0;
	u8 stat = 0;
	u8 icr = 0;

	printk("PILOT MISCCTRL Driver Version %d.%d\n",MISCCTRL_DRIVER_MAJOR,MISCCTRL_DRIVER_MINOR);

	printk("Copyright (c) 2009-2016 American Megatrends Inc.\n");

    /* ----  Register the character device ------------------- */
    if ((err = register_chrdev(MISCCTRL_DEVICE_MAJOR, MISCCTRL_DEVNAME, &miscctrl_fops)) < 0)
    {
		printk ("Miscctrl: Failed to register MISCCTRL device (err %d)\n", err);
		ret = -ENODEV;
		goto fail;
    }
	
	miscctrl_drv_registered = 1;
	/* 
	 * Store intrusion IRQ CHASI status. 
	 * RTC_STAT register is R/W1C, so we write back the irq status if it was set.
	 * 
	 * This should happen before we register an interrupt handler for IRQ_RTC; on iRMC reset, 
	 * interrupts are enabled from previous boot and will modify the state before we store it.
	 */
	chassis_intrusion_on_init = *((volatile unsigned char *)(SE_RTC_VA_BASE+RTC_STAT_REG)) & CHASSIS_INTRUSION_STATE_CHANGE;
	*((volatile unsigned char *)(SE_RTC_VA_BASE+RTC_STAT_REG)) = chassis_intrusion_on_init & CHASSIS_INTRUSION_STATE_CHANGE;

	if ((err = request_irq(IRQ_RTC, miscctrl_RTChandler, IRQF_DISABLED, "miscctrl", 0)) < 0)
	{
		printk("Miscctrl: Failed to request RTC irq %d (err %d)", IRQ_RTC, err);
 	    ret = -EBUSY;
		goto fail;
	}
#if 0 
	/* Commented : In case of a FW restart after an intrusion has 
	   been done while AC-off, this bit has to be read before it is cleared. */
	/* Clear Interrupt Status */
	stat = *((unsigned char *)(SE_RTC_VA_BASE+RTC_STAT_REG));
	stat |= CHASSIS_INTRUSION_STATE_CHANGE;
	*((unsigned char *)(SE_RTC_VA_BASE+RTC_STAT_REG)) = stat;
#endif

	/* Enable Chassis Intrusion by default */
	icr = *((unsigned char *)(SE_RTC_VA_BASE+RTC_ICR_REG));
	icr |= ENABLE_CHASSIS_INTRUSION_INTR;
	*((unsigned char *)(SE_RTC_VA_BASE+RTC_ICR_REG)) = icr;

	miscctrl_irq_RTC_requested = 1;

	if ((err = request_irq(IRQ_SIO_SWC, miscctrl_SWMhandler, IRQF_DISABLED | IRQF_SHARED, "miscctrl", (void *)("miscctrl"))) < 0)
	{
		printk("Miscctrl: Failed to request SWM irq %d (err %d)", IRQ_SYS_WAKEUP, err);
 	    ret = -EBUSY;
		goto fail;
	}

	/* Clear Interrupt Status */
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT0_REG));
	stat |= PWRGDINTSTATUS;
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT0_REG)) = stat;
	
#if 0
	/* Enable interrupt for PASS -THRU */
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS1_CFG));
	stat |= ENABLE_PASS_THRU_INTR;
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS1_CFG)) = stat;
	
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS2_CFG));
	stat |= ENABLE_PASS_THRU_INTR;
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS2_CFG)) = stat;
		
	stat= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS3_CFG));
	stat |= ENABLE_PASS_THRU_INTR;
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS3_CFG)) = stat;
#endif

#if 1
	/* Enable Power Good signal Interrupt  by default */
	icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+WKP_CFG0_REG));
	icr |= ENABLE_PWRGD_INTR;
	*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+WKP_CFG0_REG)) = icr;

#endif

	miscctrl_irq_SWM_requested = 1;	
	return ret;	

fail:
	exit_miscctrl_module ();
	return ret;
}

static unsigned int
miscctrl_poll(struct file* file, poll_table *wait)
{
	unsigned int mask = 0, i =0;
	unsigned char interrupt = 0;
	//we have to actually wait on a wait queue here
	//the qs to wait on are the swc int q and the chassis int q

	//with poll wait we can add both to poll_wait and then on any of these wait qs a event will cause poll to return to the applications
	//note that poll_Wait does not go to sleep immediately it is handled by the kernel

	poll_wait(file, &g_pass_thru_wq, wait);
	poll_wait(file, &g_swm_powergood_wq, wait);
	poll_wait(file, &g_pwbtin_wq, wait);
	poll_wait(file, &g_slp_state_wq, wait);
	poll_wait(file, &g_rtc_chassis_intrusion_wq, wait);
	poll_wait(file, &g_s3s5_asserted_state_wq, wait);

	//we also have to put the conditions here that help us determine if there is a interrupt recd
	if(g_front_swc_ints != NULL)
	{
		for (i = 0; i < g_total_swc_interrupt; i++)
		{
			switch (g_swc_intr[i].int_type)
			{
				case POWER_GOOD_INTR:
					if(g_swm_powergood_pending != 0)
						interrupt = 1;
					break;
				case PASS_THRU_INTR:
					if(g_pass_thru_status != 0)
						interrupt = 1;
					break;
				case PWRBTNIN_INTR:
					if(g_pwbtin_status != 0)
						interrupt = 1;
					break;
				case SLP_STATE_INTR:
					if(g_slp_status != 0)
						interrupt = 1;
					break;
				default:
					break;
			}
		}

		if(interrupt)
		{
			mask |= POLLPRI;
		}

	}
	else if (g_rtc_chassis_intrusion_pending != 0)
	{
		mask |= POLLIN;
	}
	
	return mask;

}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
static int miscctrl_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
#else
static long miscctrl_ioctl(struct file *file,unsigned int cmd, unsigned long arg)
#endif
{
	MISCCTRLCMD miscctrl_field;
	unsigned char icr = 0;
	unsigned int data = 0;
	int ret = 1;
	swc_interrupt_data_t  int_data;
	swc_interrupt_t ret_swc_data;
	unsigned long stampn = 0;

	if (__copy_from_user((void *)&miscctrl_field ,(void *)arg,sizeof(MISCCTRLCMD)))
	{ 
		printk("Miscctrl: Error copying data from user \n"); 
		return -EFAULT; 
	}
	
	switch (cmd)
	{
	case ENABLE_CHASSIS_INTRUSION :
		icr = *((unsigned char *)(SE_RTC_VA_BASE+RTC_ICR_REG));
		icr |= ENABLE_CHASSIS_INTRUSION_INTR;
		*((unsigned char *)(SE_RTC_VA_BASE+RTC_ICR_REG)) = icr;
		break;

	case REGISTER_CHASSIS_INTRUSION:
		if (__copy_from_user((void *)&ret , (void *)arg, sizeof(int)))
		{
			printk("Miscctrl: Error copying data from user \n"); 
			return -EFAULT; 
		}
		m_chassis_intr_num = ret;
		break;
		
	case UNREGISTER_CHASSIS_INTRUSION:
		m_chassis_intr_num = 0;
		break;
		
	case WAIT_FOR_CHASSIS_INTRUSION:
		if(!g_rtc_chassis_intrusion_pending)
		{
			if(wait_event_interruptible(g_rtc_chassis_intrusion_wq,(g_rtc_chassis_intrusion_pending != 0)))
				return -ERESTARTSYS;
		}
		g_rtc_chassis_intrusion_pending = 0;
		
		if(copy_to_user((void __user *)arg, (void*) &m_chassis_intr_num, sizeof(int)))
			return -EFAULT;
		break;

	case DISABLE_CHASSIS_INTRUSION:
		icr = *((unsigned char *)(SE_RTC_VA_BASE+RTC_ICR_REG));
		icr &= ~ENABLE_CHASSIS_INTRUSION_INTR;
		*((unsigned char *)(SE_RTC_VA_BASE+RTC_ICR_REG)) = icr;
		break;
		
	case GET_INTRUSION_RAW_STATUS:
		icr = *((unsigned char *)(SE_RTC_VA_BASE+RTC_STAT_REG));
		if( (icr & CHASSIS_INTRUSION_RAW_STATUS))
		{
			miscctrl_field.data=1;
		}else
		{
			miscctrl_field.data=0;
		}
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;    
		break;
			
	case SET_PWRBTOUT:
		
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG));
		if (0 == miscctrl_field.data)
		{
			icr &= ~ENABLE_PWRBTOUT ;
		}
		else
		{
			icr |= ENABLE_PWRBTOUT ;
		}
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;
		break;
		
	case SET_POWER_BT_LOCK:
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG));
		if (0 == miscctrl_field.data)
		{
			icr &= ~ENABLE_PWR_BT_LOCK;
		}
		else
		{
			icr |= ENABLE_PWR_BT_LOCK;
		}
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;
		break;
			
	case GET_ONCTLnSTATUS :
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT0_REG));
		if( (icr & PWRONCTLSTATUS))
			
		{
			miscctrl_field.data=1;
		}else
		{
			miscctrl_field.data=0;
		}
		
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;	
		break;
	case GET_PWRGDSTATUS:
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT0_REG));
		if ((icr & PWRGDSTATUS) )
			{
				miscctrl_field.data=1;
			}else
			{
				miscctrl_field.data=0;
			}
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;

		break;

	case ENABLE_DISABLE_PWRGD_INTR:
		if (0 != enable_disable_pwrgd_intr (miscctrl_field.data))
		{
			return -EFAULT;
		}
		break;
		
	case ENABLE_DISABLE_PWBTIN_INTR:
		if (0 != enable_disable_pwbtin_intr (miscctrl_field.data))
		{
			return -EFAULT;
		}
		break;
		
	case ENABLE_DISABLE_PASS_THRU_INTR:
		if (0 != enable_disable_passthru_intr (miscctrl_field.data))
		{
			return -EFAULT;
		}
		break;

	case POWER_ON_OFF:

        //Start control power btn
        icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG));
        if(miscctrl_field.data == POWER_CHANGE_NO_WAIT) 
        {
            if (timer_pending(&power_timer))
            {
                printk("Miscctrl: Timer is still pending to run\n");
                return -EBUSY;
            }

            /* Add timer to delay to perform further operations */
            init_timer(&power_timer);
            power_timer.data = 0;
            power_timer.function = power_timer_thread;
            power_timer.expires = jiffies + (miscctrl_field.timeout * HZ/1000);
            icr |= ENABLE_PWRBTOUT;
            *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;
            
            add_timer(&power_timer);
        }
        else if(miscctrl_field.data == POWER_CHANGE_WAIT_FOR_PWRGOOD) 
        {
            g_swm_powergood_pending = 0;
            icr |= ENABLE_PWRBTOUT;
            *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;

            /***********wait for power status change************/
            stampn = (miscctrl_field.timeout * HZ) / 1000; /* timeout in milliseconds */
            ret = wait_event_interruptible_timeout(g_swm_powergood_wq,
                        (g_swm_powergood_pending != 0), stampn);
            g_swm_powergood_pending = 0;
            /**********************************/

            icr &= (~ENABLE_PWRBTOUT);
            *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;
            if(ret < 0)
            {
                return -EFAULT; // return error on failure and timeout case
            }
            else if (ret == 0)
            {
                return -ETIMEDOUT;
            }
        }
        else
        {
            printk("Miscctrl: Power On/Off - INVALID option is selected.\n");
            return -EINVAL;
        }
        break;
        

	case ACPI_PWR_OFF_STATE_SYNC:
		
		icr= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT1_REG));
		/* Return immediately if the system is already powered off */
		if((icr & (SLPS3_STATUS|SLPS5_STATUS))==0x00)
		{
			miscctrl_field.timeout=(miscctrl_field.timeout * HZ) / 1000;
			if(miscctrl_field.timeout>INT_MAX)
			{
				miscctrl_field.timeout=INT_MAX;
			}
			return (int)miscctrl_field.timeout;
		}
		
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG));
		icr |= ENABLE_PWRBTOUT;
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;
		/***********wait for sleep status change************/
		stampn = (miscctrl_field.timeout * HZ) / 1000; /* timeout in milliseconds */
		g_s3s5_asserted_status=0;
		ret = wait_event_interruptible_timeout(g_s3s5_asserted_state_wq,
			(g_s3s5_asserted_status!=0), stampn);        
		g_s3s5_asserted_status=0;
		/**********************************/

		icr &= (~ENABLE_PWRBTOUT);
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SLP_ST_REG)) = icr;
		if(ret < 0)
		{
			return -EFAULT; // return error on failure and timeout case
		}
		else if (ret == 0)
		{
			return -ETIMEDOUT;
		}
		break;
		
	case WAIT_FOR_PWRGD_CHANGE:

		if(!g_swm_powergood_pending)
		{
			if(wait_event_interruptible(g_swm_powergood_wq,(g_swm_powergood_pending != 0)))
			    return -EFAULT;
		}

		if(copy_to_user((void __user *)arg, (void*) &g_swm_powergood_pending, sizeof(u8)))
			return -EFAULT;
		
		g_swm_powergood_pending = 0;
		break;
		
	case WAIT_FOR_PASS_THRU_INTR:
		if(!g_pass_thru_status)
		{
			if(wait_event_interruptible(g_pass_thru_wq,(g_pass_thru_status != 0)))
				return -EFAULT;
		}
		if(copy_to_user((void __user *)arg, (void*) &g_pass_thru_status, sizeof(u8)))
			return -EFAULT;
		g_pass_thru_status = 0;
		break;
		
	case WAIT_FOR_PWBTIN_INTR:
		if (!g_pwbtin_status)
		{
			if(wait_event_interruptible(g_pwbtin_wq, (g_pwbtin_status != 0)))
				return -EFAULT;
		}
		if(copy_to_user((void __user *)arg, (void*) &g_pwbtin_status, sizeof(u8)))
			return -EFAULT;
		g_pwbtin_status = 0;
		break;

	case ENABLE_DISABLE_SLP_STATE_INTR:
		if (0 != enable_disable_slp_state_intr (miscctrl_field.data))
		{
			return -EFAULT;
		}
		break;

	case WAIT_FOR_SLP_STATE_INTR:
		if (!g_slp_status)
		{
			if(wait_event_interruptible(g_slp_state_wq, (g_slp_status != 0)))
				return -EFAULT;
		}
		if(copy_to_user((void __user *)arg, (void*) &g_slp_status, sizeof(u8)))
			return -EFAULT;
		g_slp_status = 0;
		break;

	case GET_SLP3STATUS:
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT1_REG));
		if( (icr & SLPS3_STATUS))
		{
			miscctrl_field.data=1;
		}else
		{
			miscctrl_field.data=0;
		}
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;
		break;

	case GET_SLP5STATUS:
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT1_REG));
		if( (icr & SLPS5_STATUS))
			{
				miscctrl_field.data=1;
			}else
			{
				miscctrl_field.data=0;
			}
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;
		break;

	case ENABLE_PASSTHROUGH :
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_CTL_REG));
		if(1 == miscctrl_field.Index )
		{
			icr &=ENABLE_PASSTHRUIN1;
		
		}else if(2 == miscctrl_field.Index )
		{
			icr &=ENABLE_PASSTHRUIN2;
		}else if(3 == miscctrl_field.Index )
		{
			icr &=ENABLE_PASSTHRUIN3;
		}else{
			printk("Miscctrl(ENABLE_PASSTHROUGH): Error in Index\n");
			return -EFAULT;
		}
			
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_CTL_REG)) = icr;
		break;
		
	case DISABLE_PASSTHROUGH:
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_CTL_REG));

		if(1 == miscctrl_field.Index )
		{
			icr |= ~ENABLE_PASSTHRUIN1;
			
		}else if(2 == miscctrl_field.Index )
		{
			icr |= ~ENABLE_PASSTHRUIN2;
		}else if(3 == miscctrl_field.Index )
		{
			icr |= ~ENABLE_PASSTHRUIN3;
		}else
		{
			printk("Miscctrl(DISABLE_PASSTHROUGH): Error in Index\n");
			return -EFAULT;
		}

		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_CTL_REG)) = icr;
		
		break;
	case SET_PASSTHRU_DATA :
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_CTL_REG));

		if(1 == miscctrl_field.Index )
		{
			if(miscctrl_field.data)
			{
				icr |= (miscctrl_field.data << 1);
			}
			else
			{
				icr &= ~(1 << 1);
			}
		}else if( 2 == miscctrl_field.Index )
		{
			if(miscctrl_field.data)
			{
				icr |= (miscctrl_field.data << 3);
			}
			else
			{
				icr &= ~(1 << 3);
			}
		}
		else if( 3 == miscctrl_field.Index )
		{
			if(miscctrl_field.data)
			{
				icr |= (miscctrl_field.data << 5);
			}
			else
			{
				icr &= ~(1 << 5);
			}
		}
		else
		{
			printk("Miscctrl(SET_PASSTHRU_DATA): Error in Index\n");
			return -EFAULT;
		}
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_CTL_REG)) = icr;
		
		break;
	case GET_SWC_REG_VAL :
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+miscctrl_field.Index));
		miscctrl_field.data = icr;
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;
		
		break;
	case SET_SWC_REG_VAL :
		icr = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+miscctrl_field.Index));
		icr = miscctrl_field.data;
		*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+miscctrl_field.Index)) = icr;
		
		break;
	case GET_SYS_CLK_CTRL_REG_VAL :
		data = *((unsigned int *)(SE_SYS_CLK_VA_BASE+miscctrl_field.Index));
		miscctrl_field.data = data;
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;
		
		break;
	case SET_SYS_CLK_CTRL_REG_VAL :
		data = *((unsigned int *)(SE_SYS_CLK_VA_BASE+miscctrl_field.Index));
		data = miscctrl_field.data;
		*((unsigned int *)(SE_SYS_CLK_VA_BASE+miscctrl_field.Index)) = data;
		
		break;
	case GET_RTC_REG_VAL :
		data = *((unsigned int *)(SE_RTC_VA_BASE+miscctrl_field.Index));
		miscctrl_field.data = data;
		if (copy_to_user((void*)arg, &miscctrl_field , sizeof(MISCCTRLCMD))) return -EFAULT;
		
		break;
	case SET_RTC_REG_VAL :
		data = *((unsigned int *)(SE_RTC_VA_BASE+miscctrl_field.Index));
		data = miscctrl_field.data;
		*((unsigned int *)(SE_SYS_CLK_VA_BASE+miscctrl_field.Index)) = data;
		
		break;
	case REGISTER_SWC_INTS:

		if (__copy_from_user((void *)&int_data ,(void *)arg,sizeof(swc_interrupt_data_t)))
		{ 
			printk("Miscctrl: Error copying data from user \n"); 
			return -EFAULT; 
		}

		g_total_swc_interrupt = int_data.total_interrupts;
		g_swc_intr = (swc_interrupt_t *) kmalloc(sizeof(swc_interrupt_t) * g_total_swc_interrupt, GFP_KERNEL);
		
		if (NULL == g_swc_intr)
		{
			printk ("Miscctrl: Memory allocation error\n");
			return -EFAULT;
		}
		if (__copy_from_user((void *)g_swc_intr ,(void __user *)int_data.int_swc_info, sizeof(swc_interrupt_t) * g_total_swc_interrupt))
		{ 
			printk("Miscctrl: Error copying data from user \n"); 
			return -EFAULT;
		}
		enable_swc_interrupts ();

		break;

	case WAIT_FOR_SWC_INT:
		if (0 != wait_for_swc_interrupt ((void*)&ret_swc_data ))
			return -1;
			
		if(copy_to_user((void __user *)arg, (void*) &ret_swc_data, sizeof(swc_interrupt_t)))
			return -EFAULT;

		break;
	case UNREGISTER_SWC_INTS:
		/* Remove all the pending interrupts and free the memory */
		while(g_front_swc_ints != NULL)
		{
			remove_swc_interrupt();
		}
		kfree(g_swc_intr);
		g_total_swc_interrupt = 0;
		g_pwbtin_status = 0;
		g_swm_powergood_pending = 0;
		g_pass_thru_status = 0;
		g_slp_status = 0;
		
		break;
		
	case GEN_SMI_SW_EVENT:
        if (miscctrl_field.data > (ENABLE_GLOBAL_EVENT | ENABLE_LOCAL_EVENT | GENERATE_EVENT))
        {
            printk ("Miscctrl: Error in input data\n");
            return -EINVAL;
        }
        /* generate SMI event with PilotII */
        if (0 != (miscctrl_field.data & ENABLE_GLOBAL_EVENT))
        {
            icr = *((unsigned char *)(SE_PILOT_SPEC_VA_BASE + LPC_CTRL0_REG));
            icr |= ENABLE_GLOBAL_SMI_EVENT;                             // global SMI enable 5th bit for SMI
            *((unsigned char *)(SE_PILOT_SPEC_VA_BASE + LPC_CTRL0_REG)) = icr;
        }
        GenerateSWEvent (miscctrl_field.data, WKP_SMIEN0_REG);
        break;
        
    case GEN_SCI_SW_EVENT:
        if (miscctrl_field.data > (ENABLE_GLOBAL_EVENT | ENABLE_LOCAL_EVENT | GENERATE_EVENT))
        {
            printk ("Miscctrl: Error in input data\n");
            return -EINVAL;
        }
        /* generate SCI event with PilotII */
        if (0 != (miscctrl_field.data & ENABLE_GLOBAL_EVENT))
        {
            icr = *((unsigned char *)(SE_PILOT_SPEC_VA_BASE + LPC_CTRL0_REG));
            icr |= ENABLE_GLOBAL_SCI_EVENT;                             // global SMI enable 5th bit for SMI
            *((unsigned char *)(SE_PILOT_SPEC_VA_BASE + LPC_CTRL0_REG)) = icr;
        }
        GenerateSWEvent (miscctrl_field.data, GPE1_EN0_REG);
        break;
        
    case BATTERY_BACKED_READ:
    {
    	int ret;
    	ret = battery_backed_read((void *)miscctrl_field.data, miscctrl_field.Index, miscctrl_field.timeout);
    	if (ret)
    		return ret;
    	break;
    }
    case BATTERY_BACKED_WRITE:
    {
    	int ret;
    	ret = battery_backed_write((void *)miscctrl_field.data, miscctrl_field.Index, miscctrl_field.timeout);
    	if (ret)
    		return ret;
    	break;
    }
    case INTRUSION_STATE_ON_INIT:
    {
    	chassis_intrusion_on_init |= *((volatile unsigned char *)SE_RTC_VA_BASE+RTC_STAT_REG) & CHASSIS_INTRUSION_RAW_STATUS;
    	if (copy_to_user((void *)miscctrl_field.data, &chassis_intrusion_on_init, sizeof(chassis_intrusion_on_init)))
    		return -EFAULT;
    		chassis_intrusion_on_init=0;
    	break;
    }
	default :
		printk ( "Miscctrl: Invalid MISCCTRL IOCTL Command\n");
		return -EINVAL;
	}
	return 0;
}


static irqreturn_t miscctrl_RTChandler (int irq, void *dev_id)
{	
	volatile unsigned char status = 0;

	if(irq == IRQ_RTC)
	{
		/* RTC interrupt */
		printk("Miscctrl: RTC interrupt\n");
		/* Check the 2nd bit in RTC_STAT for 1 */
		status = *((unsigned char *)(SE_RTC_VA_BASE+RTC_STAT_REG));
		if(status & CHASSIS_INTRUSION_STATE_CHANGE)
		{
			/* Chassis Intrusion happened */
			g_rtc_chassis_intrusion_pending = 1;

			/* Clear Interrupt Status */
			/* 
			 * Register RTC_STAT  has 7 1wc bits, if we write back what we read, we clean them all. 
			 * What we really wanted to do is to clear just the CHASSIS_INTRUSION_STATE_CHANGE bit.
			 */
			*((unsigned char *)(SE_RTC_VA_BASE+RTC_STAT_REG)) = CHASSIS_INTRUSION_STATE_CHANGE;
			wake_up_interruptible(&g_rtc_chassis_intrusion_wq);
		}
	}
	return (IRQ_HANDLED);
}


static irqreturn_t miscctrl_SWMhandler (int irq, void *dev_id)
{	
	volatile unsigned char status = 0;
	
	if(irq == IRQ_SIO_SWC)
	{
		//printk("SWMRecieved \n");
		/*Check the 2nd Bit for Power Good Interrupt Change */
		status= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT0_REG));
		if (POWER_GOOD_BIT & status)
		{
			/* Wait_for_PowerGood   happned */
			g_swm_powergood_pending = 1;
//			printk ("Miscctrl: SWC_STAT0_REG = %x\n", status);
			/* Clear Interrupt Status */
			*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT0_REG)) = status;
			process_swc_intr(POWER_GOOD_INTR, g_swm_powergood_pending);
			wake_up_interruptible(&g_swm_powergood_wq);
//			printk("Miscctrl: Power Good interrupt occurred \n");
			SWCCallBack(POWER_GOOD_INTR);
		}
		
		status= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT1_REG));
		if (SLP_INTR_STATUS & status)
		{
			g_slp_status = 1;
//			printk ("Miscctrl: SWC_STAT1_REG = %x\n", status);
			/* Clear Interrupt Status */
			*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+SWC_STAT1_REG)) = status;
			process_swc_intr(SLP_STATE_INTR, g_slp_status);
			wake_up_interruptible(&g_slp_state_wq);
			if((status & (SLPS3_STATUS|SLPS5_STATUS))==0x00)
			{
				g_s3s5_asserted_status=1;
				wake_up_interruptible(&g_s3s5_asserted_state_wq);
			}

			//printk("Miscctrl: SLP State interrupt occurred \n");
		}

		status= *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_STS_REG));
		if (PASS_THRU_STATUS_BIT & status)
		{
			g_pass_thru_status = status & PASS_THRU_STATUS_BIT;
//			printk ("Miscctrl: PASS_STS_REG = %x\n", status);
			/* Clear Interrupt Status */
			*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PASS_STS_REG)) = status;
			
			g_pass_thru_status >>= 3; //! To make the interrupt bits in 0, 1, 2 location. 
			process_swc_intr(PASS_THRU_INTR, g_pass_thru_status);
			wake_up_interruptible(&g_pass_thru_wq);
//			printk ("Miscctrl: Pass-Thru Interrupt occured\n");
		}

		
		status = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PWBTIN_STS_REG));
		g_pwbtin_status = (status & PWBTIN_STATUS_BIT) & ((status & (PWBTIN_STATUS_BIT << 2)) >> 2);
		if (g_pwbtin_status)
		{
//			printk ("Miscctrl: PWBTIN_STS_REG = %x\n", status);
			/* Clear Interrupt Status */
			*((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PWBTIN_STS_REG)) = status;
			process_swc_intr(PWRBTNIN_INTR, g_pwbtin_status);
			wake_up_interruptible(&g_pwbtin_wq);
//			printk ("Miscctrl: PWBTIN Interrupt occured\n");
		} 
		else if (status & PWBTIN_STATUS_BIT)
		{
//		    printk ("Miscctrl: PWBTIN_STS_REG updated = %x\n", status);
		    *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PWBTIN_STS_REG)) = status;
		}
	}
	return (IRQ_HANDLED);
}

static void
exit_miscctrl_module(void)
{
	printk("Unloading PILOT MISCCTRL Module ..............\n");

	/* free RTC  irq */
	if (miscctrl_irq_RTC_requested)
	{
		free_irq(IRQ_RTC, 0);
		miscctrl_irq_RTC_requested = 0;
	}

	/* free SWC  irq */
	if (miscctrl_irq_SWM_requested)
	{
		free_irq(IRQ_SIO_SWC, (void *)("miscctrl"));
		miscctrl_irq_SWM_requested = 0;
	}

	/* unregister the device */
	if (miscctrl_drv_registered)
	{
		unregister_chrdev(MISCCTRL_DEVICE_MAJOR, MISCCTRL_DEVNAME);
		miscctrl_drv_registered = 0;
	}

	printk("PILOT MISCCTRL module unloaded sucessfully.\n");

	return;
}

module_init (init_miscctrl_module);
module_exit (exit_miscctrl_module);

MODULE_AUTHOR("Anurag Bhatia- American Megatrends Inc");
MODULE_DESCRIPTION("Pilot SOC MISCCTRL Driver.");
MODULE_LICENSE("GPL");

