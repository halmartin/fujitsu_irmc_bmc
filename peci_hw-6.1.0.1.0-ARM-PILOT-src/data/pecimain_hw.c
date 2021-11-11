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

/*
 * PECI Hardware Specific Driver
 *
 * Copyright (C) 2009 American Megatrends Inc.
 *
 * This driver provides hardware specific layer for the PECI driver.
 */

#include <linux/version.h>
#if (LINUX_VERSION_CODE <  KERNEL_VERSION(3,0,0))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif
#include <linux/kernel.h>	
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include "peci_hw.h"
#include "driver_hal.h"
#include "helper.h"
#include "reset.h"

#define PECI_HW_DEV_NAME	"peci_hw"
#define MAX_PECI_HW_CHANNELS	PECI_HW_MAX_INST

int install_reset_handler(int (*pt2Func)(void));
int uninstall_reset_handler(int (*pt2Func)(void));

static int __init peci_hw_init (void);
static void __exit peci_hw_exit (void);

static int send_peci_hw_cmd(peci_cmd_t *pecicmd);

static peci_hal_operations_t peci_hw_ops = {
	send_peci_cmd	:	send_peci_hw_cmd,
};	

static hw_hal_t hw_hal = {
	.dev_type = EDEV_TYPE_PECI,
	.owner = THIS_MODULE,
	.devname = PECI_HW_DEV_NAME,
	.num_instances = MAX_PECI_HW_CHANNELS,
	.phal_ops = (void *)&peci_hw_ops
};
static peci_core_funcs_t *ppeci_core_funcs;

static int m_dev_id = 0;
static struct completion peci_done;

static void
peci_hw_enable(void)
{
	dbgprint("\n Enabling PECI Hardware Module......\n");

	//enable all peci hardware
	*((volatile unsigned long *) (SE_PECI_VA_BASE + PECICTL_REG)) = PECI_ENABLE;
	/* Set Optimimum Rate ? */
	*((unsigned long *)(SE_PECI_VA_BASE+PECIOPTRATE_REG)) = 0x500;
	
	return;
}

static void
peci_hw_disable(void)
{
	dbgprint("\nDisabling the PECI Hardware module.....\n");
	
	//disable all peci hardware interrupts
	*((volatile unsigned long *)( SE_PECI_VA_BASE+PECICTL_REG)) = 0;
	
	return;
}

static void
peci_hw_enable_intr(void)
{
	dbgprint("\n Enabling PECI Hardware Interrupt......\n");

	/* Enable PECI hardware Intr */
	*((unsigned long *)(SE_PECI_VA_BASE+PECICTL_REG)) |= PECI_INTR_ENABLE;
	
	return;
}

static void
peci_hw_disable_intr(void)
{
	dbgprint("\nDisabling PECI Hardware Interrupt......\n");
	
	/* Disable PECI hardware Intr */
	*((unsigned long *)(SE_PECI_VA_BASE+PECICTL_REG)) &= ~PECI_INTR_ENABLE;

	return;
}

static void 
peci_hw_initiate_data_transfer(void)
{
	dbgprint("\nInitiating PECI data transfer......\n");
	/* clear the interrupt status before setting trigger*/
	*((volatile unsigned long *) (SE_PECI_VA_BASE + PECISTS_REG)) |= PECI_INTR_STATUS;

	/* Trigger Command*/
	*((volatile unsigned long *) (SE_PECI_VA_BASE + PECICTL_REG)) |= (PECI_TRIGGER | PECI_INTR_ENABLE);

	return;
}

static int
peci_hw_is_busy(void)
{
	unsigned long timeout;
	
	/* Wait for Busy to clear */
	timeout = 3;
	while ((*((volatile unsigned long *)(SE_PECI_VA_BASE+PECISTS_REG)) & PECI_BUSY) \
					 && timeout)
	{
		 udelay(1000);
		 timeout--;
	}
	if (*((volatile unsigned long *)(SE_PECI_VA_BASE+PECISTS_REG)) & PECI_BUSY)
	{
		/*Disable PECI and Intr */
		*((volatile unsigned long *)( SE_PECI_VA_BASE+PECICTL_REG)) = 0;	
		return 1;
	}
	return 0;
}

static void
peci_hw_dumpregisters(void)
{
#if defined(DEBUG)
	volatile unsigned long *reg;
	int i;

	reg = (volatile unsigned long *)(SE_PECI_VA_BASE);

	for (i = 0; i < (0x40 / 4); i ++)
		dbgprint("Reg[%02x] = 0x%08lx\n", i * 4, reg[i]);
#endif
	return;
}

static int
peci_check_write_transfer(unsigned char write_fcs)
{
	if((*((volatile unsigned char *)(SE_PECI_VA_BASE + PECIHWFCS_REG))) == write_fcs)
		return 0;

	return -1;
}

static int
peci_check_read_transfer(unsigned char read_fcs)
{
	if((*((volatile unsigned short *)(SE_PECI_VA_BASE + PECIHWFCS_REG)) >> 8) == read_fcs)
		return 0;

	return -1;
}

static void
write_tx_queue(unsigned char target, unsigned char tx_len, unsigned char rx_len, unsigned char *pdata)
{
   	unsigned long Tx0;
	unsigned char *tx_queue;
	int i;

	/* Fill the initial params */
	Tx0 = (rx_len << 16) | (tx_len << 8) | target;
	if (tx_len >= 1)
		Tx0 |= (pdata[0] << 24);

	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(0))) = Tx0;
	#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(1))) = 0;
	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(2))) = 0;
	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(3))) = 0;
	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(4))) = 0;
	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(5))) = 0;
	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(6))) = 0;
	*((volatile unsigned long *)(SE_PECI_VA_BASE+PECITXQ_REG(7))) = 0;
	#endif

	if (tx_len > 1)
	{
		/* We already used the Data[0](Command) and TXQ(0) */
		tx_queue = ((unsigned char *)(SE_PECI_VA_BASE+PECITXQ_REG(1)));
		for(i=1;i<tx_len;i++)
			tx_queue[i-1] = pdata[i];
	}
	return;
}

static void 
peci_hw_write_data(unsigned char target, unsigned char tx_len, unsigned char rx_len, unsigned char *pdata, unsigned char rx_offset)
{
	#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	unsigned long writeValue;
	#endif

	/* Write data to Tx Queue */
	write_tx_queue(target, tx_len, rx_len, pdata);

	#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	/* Set RX Offset */
	writeValue = (unsigned long) rx_offset;
	writeValue = writeValue << 8;
	*((volatile unsigned long *) (SE_PECI_VA_BASE + PECIRXOFF_REG)) = writeValue;
	#else
	*((volatile unsigned long *) (SE_PECI_VA_BASE + PECIRXOFF_REG)) = rx_offset;
	#endif

	return;
}

static void 
peci_hw_read_data(volatile unsigned char **p_RecvBuffer)
{
	peci_hw_dumpregisters();

	/* Copy Data from RXQ to ReadData */
	*p_RecvBuffer = ((volatile unsigned char *)(SE_PECI_VA_BASE + PECIRXQ_REG(0)));

	return ;
}

static int 
peci_reset(void)
{
	peci_hw_disable_intr();
	peci_hw_disable();
	peci_hw_enable();
	peci_hw_enable_intr();

	return 0;
}

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
static void
enable_hw_awfcs (void)
{
	*((volatile unsigned long *)( SE_PECI_VA_BASE+PECICTL_REG)) |= PECI_AWFCS_EN;
}

static void
disable_hw_awfcs (void)
{
	*((volatile unsigned long *)( SE_PECI_VA_BASE+PECICTL_REG)) &= ~(PECI_AWFCS_EN);
}
#endif

static int 
send_peci_hw_cmd(peci_cmd_t *pecicmd)
{
	int i, retval =0;
	int status = 0;
	unsigned int total_length, rx_fcs, tx_fcs;
	volatile unsigned char *pread_buffer;
	unsigned long rx_offset, expire = msecs_to_jiffies(1000);

	/* Calculate Some Values */
	total_length =  pecicmd->read_len + 1; 	/* +1 for tx_fcs */

	if (pecicmd->read_len)
		total_length ++;   		/* Add one for rx_fcs */

	rx_fcs = 1 + pecicmd->read_len;		
	tx_fcs = 0;
	rx_offset = 0;

	tx_fcs += (FEEDBACK_LENGTH + pecicmd->write_len);
	rx_fcs += (FEEDBACK_LENGTH + pecicmd->write_len);
	total_length += (FEEDBACK_LENGTH + pecicmd->write_len);

	dbgprint("TotalLen = %d rx_fcs = %d tx_fcs = %d rx_offset = %ld \n",
			total_length,rx_fcs,tx_fcs,rx_offset);
		
	init_completion(&peci_done);

	peci_hw_enable();

	peci_hw_enable_intr ();

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	/* Check if AWFCS is requested and if so, enable it */
	if (pecicmd->AWFCS)
	{
		enable_hw_awfcs();
	}
	else
	{
		disable_hw_awfcs();
	}
#endif

	//Send the command to peci hardware
	peci_hw_write_data(pecicmd->target, pecicmd->write_len, pecicmd->read_len, pecicmd->write_buffer, rx_offset);

	peci_hw_dumpregisters();

	//Initiating the transfer
	peci_hw_initiate_data_transfer ();

	if(wait_for_completion_timeout(&peci_done, expire) == 0)
	{
		dbgprint("ERROR: PECI Intr Timeout happened\n");
		retval = -ETIMEDOUT;
		goto ret_process;
	}

	/* Wait for Busy to clear */
	if(peci_hw_is_busy () < 0)
	{
		dbgprint("ERROR: PECI Busy Timeout happened\n");
		retval = -EBUSY;
		peci_hw_disable_intr ();
		goto ret_process;
	}

	/* Receive Response from appropriate peci hardware */
	peci_hw_read_data (&pread_buffer);

	/* Compare FCS */
	if(peci_check_write_transfer (pread_buffer[tx_fcs]) != 0)
		status = -1;
	
	if (pecicmd->read_len)
	{
		if(peci_check_read_transfer (pread_buffer[rx_fcs]) != 0)
			status = -1;
	}

	pecicmd->status = status;

	//remove the first and last byte(FCS value) and read only the raw value.
	if (!pecicmd->Xmit_Feedback)
	{
		rx_offset = (FEEDBACK_LENGTH + pecicmd->write_len + 1);		/* add one for rx_fcs */
		dbgprint("xmit_feedback is not set\n");
		for(i=0;i<pecicmd->read_len;i++)
			pecicmd->read_buffer[i]=pread_buffer[i+rx_offset];
	}
	else
	{
		dbgprint("xmit_feedback is set\n");
		pecicmd->read_len = total_length;
		for(i=0;i<pecicmd->read_len;i++)
			pecicmd->read_buffer[i]=pread_buffer[i];
	}

#ifdef DEBUG
	for (i = 0;i < pecicmd->read_len; i++)
	{
		dbgprint("\nPECI RxdBuffer[%d] = 0x%x\n",i, pecicmd->read_buffer[i]);
	}
#endif
	
	dbgprint("\nPECI Total Response length = %d\n",pecicmd->read_len);

ret_process:	
	peci_hw_disable_intr();
	peci_hw_disable();

	return retval;
}

static irqreturn_t 
peci_handler (int irq, void *dev_id)
{	
	dbgprint("PECI: device intr \n");

	if(*((volatile unsigned long *)(SE_PECI_VA_BASE+PECISTS_REG)) & PECI_INTR_STATUS)
	{
		*((volatile unsigned long *)(SE_PECI_VA_BASE+PECISTS_REG)) |= PECI_INTR_STATUS;
	dbgprint("PECI: device intr occured \n");

		complete(&peci_done);
	}

	return (IRQ_HANDLED);
}

static int __init
peci_hw_init(void)
{
	int ret = 0;

	printk("PECI HW Driver, (c) 2009-2015 American Megatrends Inc.\n");

	if ((ret = register_hw_hal_module (&hw_hal, (void **)&ppeci_core_funcs)) < 0)
	{
		printk (KERN_ERR "%s: failed to register peci hal module\n", PECI_HW_DEV_NAME);
		return ret;
	}
	m_dev_id = ret;

	dbgprint("\nLoading PILOT-II PECI Hardware Module.......\n");

	//Registering the interrupt service routine
	if ((ret = request_irq(IRQ_PECI, peci_handler, IRQF_DISABLED, PECI_DEV_NAME, 0)) < 0)
	{
		dbgprint("ERROR: Failed to request PECI irq %d name %s (err %d)",IRQ_PECI,PECI_DEV_NAME, ret);
		unregister_hw_hal_module (EDEV_TYPE_PECI, m_dev_id);
		return ret;
	}

	install_reset_handler(peci_reset);

	return 0;
}

static void __exit
peci_hw_exit(void)
{
	dbgprint("Unloading PILOT-II PECI Hardware Module ..............\n");
	
	unregister_hw_hal_module (EDEV_TYPE_PECI, m_dev_id);
	
	/* free irq */
	free_irq(IRQ_PECI,0);

	uninstall_reset_handler(peci_reset);

	return;
}

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("Pilot PECI controller driver");
MODULE_LICENSE("GPL");

module_init (peci_hw_init);
module_exit (peci_hw_exit);
