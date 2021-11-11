/*****************************************************************
 **                                                             **
 **     (C) Copyright 2009-2015, American Megatrends Inc.       **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                             **
 **         Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                             **
 ****************************************************************/

#include <linux/version.h>
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kernel.h>	/* printk()*/
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include "kcs_hw.h"
#include "kcs.h"
#include "reset.h"
#include "driver_hal.h"
#include "coreTypes.h"

#define FUJITSU_CHG_ESPI_SUPPORT

#define MAX_KCS_HW_CHANNELS    3
#define KCS_HW_DEV_NAME        "kcs_hw"

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
static int kcs_hw_init (void);
static void kcs_hw_exit (void);
#else
static int __init kcs_hw_init (void);
static void __exit kcs_hw_exit (void);
#endif
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
static irqreturn_t kcs_hw_irq_handler ( int irq, void *dev_id );
#else
static irqreturn_t kcs_hw_irq_handler ( int irq, void *dev_id, struct pt_regs *regs );
#endif

unsigned char num_kcs_ch(void);
void enable_kcs_interrupt(u8 ch_num);
void disable_kcs_interrupt(u8 ch_num);
void read_kcs_status(u8 ch_num, u8* status_value);
void write_kcs_status(u8 ch_num, u8 status_value);
void read_kcs_command(u8 ch_num, u8* command);
void write_kcs_data_out(u8 ch_num, u8 data_value);
void read_kcs_data_in(u8 ch_num, u8* data_value);
void kcs_interrupt_enable_user(u8 ch_num);
void kcs_interrupt_disable_user(u8 ch_num);
void kcs_set_obf_status(u8 ch_num);

bool kcs_enabled_by_user[MAX_KCS_HW_CHANNELS] = {FALSE,FALSE,FALSE};
static int kcs_reset(void);

static int btflag = 0;
static int kcsflag = 0;
static int mailboxflag = 0;
static int m_kcs_irq_requested = 0;
static int m_dev_id = 0;

static kcs_hal_operations_t kcs_hw_ops = {
	num_kcs_ch,	
	enable_kcs_interrupt,
	disable_kcs_interrupt,
	read_kcs_status,
	write_kcs_status,
	read_kcs_command,
	read_kcs_data_in,
	write_kcs_data_out,
	kcs_interrupt_enable_user,
	kcs_interrupt_disable_user,
	kcs_set_obf_status
};

static kcs_core_funcs_t *pkcs_core_funcs;

static hw_hal_t hw_hal = {
	.dev_type = EDEV_TYPE_KCS,
	.owner = THIS_MODULE,
	.devname = KCS_HW_DEV_NAME,
	.num_instances = MAX_KCS_HW_CHANNELS,
	.phal_ops = (void *)&kcs_hw_ops
};
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
static int kcs_hw_init (void)
#else
static int __init kcs_hw_init (void)
#endif
{
	int ret = 0;
	u8  ch_num = 0, status = 0;
	int ch = 0;

	for(ch = 0;ch < MAX_KCS_HW_CHANNELS;ch ++)
	{
		disable_kcs_interrupt(ch);
	}

	pkcs_core_funcs = (kcs_core_funcs_t*) kmalloc (sizeof(kcs_core_funcs_t), GFP_KERNEL);
	if (!pkcs_core_funcs)
		return -ENOMEM;

	if ((ret = register_hw_hal_module (&hw_hal, (void **)&pkcs_core_funcs)) < 0)
	{
		printk (KERN_ERR "%s: failed to register kcs hal module\n", KCS_HW_DEV_NAME);
		return ret;
	}
	m_dev_id = ret;

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
	if ((ret = request_irq(LPC_IRQ_ADDR, kcs_hw_irq_handler, IRQF_DISABLED | IRQF_SHARED, "kcs", (void*)kcs_hw_irq_handler)) != 0)
#else
	if ((ret = request_irq(LPC_IRQ_ADDR, kcs_hw_irq_handler, SA_INTERRUPT, "kcs", 0)) != 0)
#endif
	{
		printk (KERN_ERR "%s: failed to request irq %d (err %d)\n", KCS_HW_DEV_NAME, LPC_IRQ_ADDR, ret);
		ret = -ENODEV;
		goto fail;
	}
	m_kcs_irq_requested = 1;	

	//printk ("kcs_hw ptr: %p\n", pkcs_core_funcs);	
	for (ch_num = 0; ch_num < MAX_KCS_HW_CHANNELS; ++ch_num)
	{
		read_kcs_status (ch_num, &status);
		status = status | ERROR_STATE;
		write_kcs_status (ch_num, status);
	}		

	//enable_kcs_interrupt();

	install_reset_handler(kcs_reset);
#ifdef FUJITSU_CHG_ESPI_SUPPORT
	InstalleSPIResetHandler(kcs_reset);
#endif

	printk("KCS HW Driver, (c) 2009-2015 American Megatrends Inc.\n");

	return 0;
fail:
	kcs_hw_exit();
	return -1;
}
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,0,0))
static void kcs_hw_exit (void)
#else
static void __exit kcs_hw_exit (void)
#endif
{
  int ch = 0;
  
  for(ch = 0;ch < MAX_KCS_HW_CHANNELS;ch ++)
  {
	    disable_kcs_interrupt(ch);
  }

	if (m_kcs_irq_requested)
	{
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
		free_irq(LPC_IRQ_ADDR, (void*)kcs_hw_irq_handler);
#else
		free_irq(LPC_IRQ_ADDR, 0);
#endif
		m_kcs_irq_requested = 0;
	}

	unregister_hw_hal_module (EDEV_TYPE_KCS, m_dev_id);

	uninstall_reset_handler(kcs_reset);
#ifdef FUJITSU_CHG_ESPI_SUPPORT
	UnInstalleSPIResetHandler(kcs_reset);
#endif

	if (!pkcs_core_funcs)
		kfree (pkcs_core_funcs);

	return;
}


static int kcs_reset(void)
{
  int ch = 0;
 
  if (pkcs_core_funcs->is_hw_test_mode() == 1)
      return 0;

  for(ch = 0;ch < MAX_KCS_HW_CHANNELS; ch ++)
  {
	    if(kcs_enabled_by_user[ch] == TRUE) 
	    {
		    enable_kcs_interrupt(ch); 
	    }
  }
        return 0;
}

static int
IsSpuriousIntr (void)
{
	unsigned char intr_stat;

	kcsflag = 0;
	btflag = 0;
	mailboxflag = 0;

	intr_stat = *(unsigned char *)(SE_KCS5_BT_SMIC_MAIL_VA_BASE + 0x38);
	if (intr_stat & 0x20)
		btflag = 1;
	if (intr_stat & 0x1C)
		kcsflag = 1;
	if (intr_stat & 0x80)
		mailboxflag  = 1;

	if (kcsflag || btflag || mailboxflag)
		return 0;
	else
		return 1;
}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
static irqreturn_t kcs_hw_irq_handler ( int irq, void *dev_id )
#else
static irqreturn_t kcs_hw_irq_handler ( int irq, void *dev_id, struct pt_regs *regs )
#endif
{
	int ret = 0;
	u8  ch_num = 0, status = 0;

	if (IsSpuriousIntr())
	{
		return IRQ_HANDLED;
	}
	if (kcsflag)
	{
		for (ch_num = 0; ch_num < MAX_KCS_HW_CHANNELS; ++ch_num)
		{
			status = *(kcs_vadd[ch_num] + STATUS[ch_num]);

			if (0 != (status & 0x02))
			{
				ret = pkcs_core_funcs->process_kcs_intr (m_dev_id, ch_num);
				if (ret == 0) 
				{
					status = *(kcs_vadd[ch_num] + STATUS[ch_num]);

					//IBF is cleared when there is a write to the status register with bit[1] set from the BMC firmware  after reading the data register
					status = status | 0x02;
					*(kcs_vadd[ch_num] + STATUS[ch_num]) = status;

				}
			}
		}
	}
	if (btflag || mailboxflag)
		return (IRQ_NONE);
	else
		return IRQ_HANDLED;
}

unsigned char
num_kcs_ch (void)
{
	return MAX_KCS_HW_CHANNELS;
}
	
void
enable_kcs_interrupt (u8 ch_num)
{
    volatile unsigned char reg;

    reg = *(unsigned char *)(SE_KCS5_BT_SMIC_MAIL_VA_BASE + 0x39);

    if(ch_num == 0)
    {
      reg |= 0x04;
    }
    else if(ch_num == 1)
    {
      reg |= 0x08;
    }
    else if(ch_num == 2)
    {
      reg |= 0x10; 
    }

    *(unsigned char *)(SE_KCS5_BT_SMIC_MAIL_VA_BASE + 0x39) = reg;
}

void
disable_kcs_interrupt (u8 ch_num)
{
    volatile unsigned char reg;

    reg = *(unsigned char *)(SE_KCS5_BT_SMIC_MAIL_VA_BASE + 0x39);

    if(ch_num == 0)
    {
        reg &= ~0x04;
    }
    else if(ch_num == 1)
    {
        reg &= ~0x08;
    }
    else if(ch_num == 2)
    {
        reg &= ~0x10; 
    }

    *(unsigned char *)(SE_KCS5_BT_SMIC_MAIL_VA_BASE + 0x39) = reg;
}

void 
kcs_interrupt_enable_user (u8 ch_num)
{
    if (ch_num < MAX_KCS_HW_CHANNELS)
    {
        kcs_enabled_by_user[ch_num] = TRUE;
        enable_kcs_interrupt(ch_num);
    }
}

void
kcs_interrupt_disable_user (u8 ch_num)
{
    if (ch_num < MAX_KCS_HW_CHANNELS)
    {
        kcs_enabled_by_user[ch_num] = FALSE;
        disable_kcs_interrupt(ch_num);
    }
}

void 
read_kcs_status(u8 ch_num, u8* status_value)
{
	if (ch_num < MAX_KCS_HW_CHANNELS)
		*status_value = *(kcs_vadd[ch_num] + STATUS[ch_num]);
	else
		*status_value = 0;
}

void 
write_kcs_status(u8 ch_num, u8 status_value)
{
	if (ch_num < MAX_KCS_HW_CHANNELS)
		*(kcs_vadd[ch_num] + STATUS[ch_num]) = (status_value & 0xFD); //don't set IBF
}

void 
read_kcs_command(u8 ch_num, u8* command)
{
	if (ch_num < MAX_KCS_HW_CHANNELS)
		*command = *(kcs_vadd[ch_num] + CMD_IN[ch_num]);
	else
		*command = 0;
}

void
kcs_set_obf_status(u8 ch_num)
{
	
#if 0
    u8 status_value=0;

    if (ch_num < MAX_KCS_HW_CHANNELS)
        status_value = *(kcs_vadd[ch_num] + STATUS[ch_num]);
    else
        status_value = 0;
    status_value=status_value | 0x01;
    if (ch_num < MAX_KCS_HW_CHANNELS)
        *(kcs_vadd[ch_num] + STATUS[ch_num]) = status_value;
#endif
}

void 
read_kcs_data_in(u8 ch_num, u8* data_value)
{
	if (ch_num < MAX_KCS_HW_CHANNELS)
		*data_value = *(kcs_vadd[ch_num] + DATA_IN[ch_num]);
	else
		*data_value = 0;
}

void 
write_kcs_data_out(u8 ch_num, u8 data_value)
{
	if (ch_num < MAX_KCS_HW_CHANNELS)
		*(kcs_vadd[ch_num] + DATA_OUT[ch_num]) = data_value;
}

/*--------------------------- Module Information Follows --------------------------*/
module_init (kcs_hw_init);
module_exit (kcs_hw_exit);

MODULE_AUTHOR("American Megatrends Inc");
MODULE_DESCRIPTION("This is KCS HW driver module.");
MODULE_LICENSE("GPL");

