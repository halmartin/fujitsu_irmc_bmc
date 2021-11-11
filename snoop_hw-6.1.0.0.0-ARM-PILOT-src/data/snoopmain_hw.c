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
#include <linux/kfifo.h>
#include "snoop.h"
#include "driver_hal.h"


#define SNOOP_IRQ_ADDR		IRQ_SIO_PSR
#define SNOOP_HW_DEV_NAME	"snoop"
#define MAX_SNOOP_DEVICES	255
#define SNOOP_BUFSIZE		512

#define SNOOP_INTERRUPT_BIT	0x01

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int snoop_hw_init(void);
static void snoop_hw_exit(void);
#else
static int __init snoop_hw_init(void);
static void __exit snoop_hw_exit(void);
#endif
irqreturn_t snoop_hw_irq_handler ( int irq, void *dev_id );

static int sio_irq_requested = 0;
static int m_dev_id = 0;

void enable_snoop_interrupt (int ch_num);
void disable_snoop_interrupt (int ch_num);
void reset_snoop (int ch_num);

static snoop_hal_operations_t snoop_hw_ops = {
	enable_snoop_interrupt,
	disable_snoop_interrupt,
	reset_snoop,
};

static snoop_core_funcs_t *psnoop_core_funcs;

static hw_hal_t hw_hal = {
	.dev_type = EDEV_TYPE_SNOOP,
	.owner = THIS_MODULE,
	.devname = SNOOP_HW_DEV_NAME,
	.num_instances = MAX_SNOOP_DEVICES,
	.phal_ops = (void *)&snoop_hw_ops	
};


struct kfifo * g_current_codes;
struct kfifo * g_previous_codes;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int snoop_hw_init(void)
#else
static int __init
snoop_hw_init(void)
#endif
{
	int ret = 0;

	if ((ret = register_hw_hal_module (&hw_hal, (void **)&psnoop_core_funcs)) < 0)
	{
		printk(KERN_ERR "%s: failed to register snoop hal module\n", SNOOP_HW_DEV_NAME);
		return -1;
	}
	m_dev_id = ret;

	psnoop_core_funcs->get_snoop_core_data( &g_current_codes, &g_previous_codes, m_dev_id, 0 );

	if ((ret = request_irq(SNOOP_IRQ_ADDR, snoop_hw_irq_handler, IRQF_SHARED, "snoop", (void*)("snoop_irq"))) != 0)
	{
		printk (KERN_ERR "%s: failed to request irq %d (err %d)\n", SNOOP_HW_DEV_NAME, SNOOP_IRQ_ADDR, ret);
		ret = -ENODEV;
		goto fail;
	}
        sio_irq_requested = 1;

	enable_snoop_interrupt(0);
	
	printk("Snoop HW Driver, (c) 2009-2015 American Megatrends Inc.\n");
	return ret;

fail:
	snoop_hw_exit();
	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static void snoop_hw_exit(void)
#else
static void __exit
snoop_hw_exit(void)
#endif
{
	disable_snoop_interrupt(0);

	if (sio_irq_requested)
        {
                free_irq (SNOOP_IRQ_ADDR, (void*)("snoop_irq"));
                sio_irq_requested = 0;
        }
	
	unregister_hw_hal_module (EDEV_TYPE_SNOOP, m_dev_id);

	printk("Snoop HW Driver unloaded.\n");
}


irqreturn_t snoop_hw_irq_handler ( int irq, void *dev_id )
{
        unsigned char port_intr = 0;
        int got = 0;
        unsigned char data = 0;

        while (1)
        {
                /* Check if data present */
                port_intr = *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x42);
                if ((port_intr & SNOOP_INTERRUPT_BIT) == 0)
                        break;

                /* Read Data */
                data = *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x40);

                /* Display in LEDs (Output to Port4 Data Out register) */
                *(unsigned char *)(SE_GPIO_VA_BASE + 0x48) = data;

                /* Store it in buffer */
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,4,11))
    memcpy (g_current_codes->kfifo.data+g_current_codes->kfifo.in,(void *)&data,1);

     g_current_codes->kfifo.in = (g_current_codes->kfifo.in + 1) % 512;
		if ( g_current_codes->kfifo.mask == (SNOOP_BUFSIZE-1))
		{
			g_current_codes->kfifo.mask++;
			g_current_codes->kfifo.out = (g_current_codes->kfifo.out + 1) % SNOOP_BUFSIZE;
		}
		else if ( g_current_codes->kfifo.mask == SNOOP_BUFSIZE )
			g_current_codes->kfifo.out = (g_current_codes->kfifo.out + 1) % SNOOP_BUFSIZE;
		else	
			g_current_codes->kfifo.mask++;
#else
		g_current_codes->buffer[ g_current_codes->in ] = data;
		g_current_codes->in = (g_current_codes->in + 1) % SNOOP_BUFSIZE;
		if ( g_current_codes->size == (SNOOP_BUFSIZE-1))
		{
			g_current_codes->size++;
			g_current_codes->out = (g_current_codes->out + 1) % SNOOP_BUFSIZE;
		}
		else if ( g_current_codes->size == SNOOP_BUFSIZE )
			g_current_codes->out = (g_current_codes->out + 1) % SNOOP_BUFSIZE;
		else	
			g_current_codes->size ++;
#endif
                got++;
	}

	if (!got)
		return IRQ_NONE;

	return IRQ_HANDLED;
}


void
enable_snoop_interrupt(int ch_num)
{
        volatile unsigned char port_capture_control = 0;
        /* Enable Port Capture FIFO Interrupt */
        port_capture_control = *(volatile unsigned char*) (SE_PILOT_SPEC_VA_BASE + 0x45);
	port_capture_control |= 0x04;
        *(volatile unsigned char*) (SE_PILOT_SPEC_VA_BASE + 0x45) = port_capture_control;	
}


void
disable_snoop_interrupt(int ch_num)
{
        volatile unsigned char port_capture_control = 0;
        /* Disable Port Capture FIFO Interrupt */
        port_capture_control = *(volatile unsigned char*) (SE_PILOT_SPEC_VA_BASE + 0x45);
	port_capture_control &= (~0x04);
        *(volatile unsigned char*) (SE_PILOT_SPEC_VA_BASE + 0x45) = port_capture_control;	
}


void
reset_snoop(int ch_num)
{
	enable_snoop_interrupt(0);
}


/*--------------------------- Module Information Follows --------------------------*/
module_init (snoop_hw_init);
module_exit (snoop_hw_exit);

MODULE_AUTHOR("American Megatrends Inc");
MODULE_DESCRIPTION("This is SNOOP HW driver module.");
MODULE_LICENSE("GPL");

