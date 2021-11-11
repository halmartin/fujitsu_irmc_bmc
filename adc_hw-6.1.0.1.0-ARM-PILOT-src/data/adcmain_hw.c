/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * adc_hw.c
 * ADC driver Hardware functions for Pilot-II/Pilot-III
 *
 *****************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <mach/platform.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/delay.h>
#include "helper.h"
#include "driver_hal.h"

#include "adc_hw.h"
#include "adc.h"

#define ADC_HW_MAX_INST	1
static void *adc_remap_addr = NULL;

#define VESTS ((unsigned long *)(((unsigned long)adc_remap_addr)+0x00))
#define VCNT  ((unsigned long *)(((unsigned long)adc_remap_addr)+0x08))
#define VCHNE ((unsigned long *)(((unsigned long)adc_remap_addr)+0x0C))
#ifdef SOC_PILOT_IV
#define VCHR(x)  ((unsigned long *)(((unsigned long)adc_remap_addr)+0x60+((x)*4)))
#else
#define VCHR(x)  ((unsigned long *)(((unsigned long)adc_remap_addr)+0x50+((x)*4)))
#endif

static int m_dev_id = 0;

static int adc_hw_module_init(void);
static void adc_hw_module_exit(void);

static	int adc_read_channel (uint16_t *adc_value, int channel);
static	int adc_get_resolution (uint16_t *adc_resolution);
static	int adc_get_reference_voltage (uint16_t *adc_ref_volatge);
static	int adc_reboot_notifier (void);

extern int register_adc_hw_module (ADC_HW* AdcHw);

//static adc_dev_t adc_dev;

static adc_core_funcs_t *padc_core_funcs;

static adc_hal_operations_t adc_hw_ops = {
	adc_read_channel,
	adc_get_resolution,
	adc_get_reference_voltage,
	adc_reboot_notifier,
};

static hw_hal_t hw_hal = {
	.dev_type = EDEV_TYPE_ADC,
	.owner = THIS_MODULE,
	.devname = ADC_HW_DEV_NAME,
	.num_instances = ADC_HW_MAX_INST,
	.phal_ops = (void *)&adc_hw_ops
};


/*
 * adc_read_channel
 */
int
adc_read_channel(uint16_t *adc_value, int channel)
{

	unsigned long value;
	int rawdata = ((channel & 0x80) ? 1 : 0);
#if defined (SOC_PILOT_IV) || defined (SOC_PILOT_III)
	unsigned int  count = 100;
#endif

	channel &= 0x7f;
#ifdef SOC_PILOT_IV
	if ((channel < ADC_CHANNEL0_NUM) || (channel > ADC_CHANNEL8_NUM))
#else
	if ((channel < ADC_CHANNEL0_NUM) || (channel > ADC_CHANNEL15_NUM))
#endif
	{
		printk (KERN_ERR "Invalid ADC Channel Number = 0x%x\n", channel);
		return -1;
	}

#if defined (SOC_PILOT_IV) || defined (SOC_PILOT_III)
#if defined (SOC_PILOT_IV)
	if(channel == ADC_CHANNEL7_NUM)
#else
	if(channel == ADC_CHANNEL15_NUM)
#endif
	 {
		 *VCHNE |= 0x8000; //Channel Enable(Start sampling channel)
		 do //maximum delay 1ms
		 {
			 udelay(10);
			 --count;
		 }while((((*VESTS) & 0x01) != 0x01) && count );
	 }
#endif

	*adc_value = 0;

	/* Two channels value in one register */
	value = *VCHR(channel/2);

	//printk("Value read for %d = 0x%08lx\n",channel,value);

	/* odd value channel in  16:25 */
	if (channel % 2)
		value >>= 16;

	value &= 0x3FF;

//	printk("Value read for %d = 0x%08lx\n",channel,value);
    if (0 != rawdata)
    {
		*adc_value = value;
    }
    else
    {
#if defined (SOC_PILOT_IV)
		if (ADC_TEMP_SENSOR_CHANNEL == channel)
		{
			// 1024*0.00488 = 4.99712 ~= 5 and Ref V = 2.5v
			// 2.5/5 = 0.5 (so value >> 1)
			*adc_value = ((value >> 1) - 273);
		}
		else if (ADC_CHANNEL7_NUM == channel)
		{
			// According to Pilot-4 spec v0.26
			// Following equation is used to determine voltage level at AVIN_BAT_7
			// AVIN_BAT_7 = (( Channel Raw Register * VREF)/1024)*(4/3)
			// Consider ( Channel Raw Register * VREFX) as X
			// So, X divided by 1024 and then multiplying with 4/3 is equal to X divided by 768, (X >> 8)/3
			*adc_value=((value * ADC_REF_VOLTAGE_IN_MVOLTS) >> 8)/3;
		}
		else
#elif defined (SOC_PILOT_III)
		if(channel == ADC_CHANNEL15_NUM)
			*adc_value=((value * ADC_REF_VOLTAGE_IN_MVOLTS) >> 8)/3;//Dividing by 0.75(RINT) for channel 15 for Pilot-III
		else
#elif defined (SOC_PILOT_II)
		if ((channel >=12) && (channel <=14))
			*adc_value = (value-512)/4; /* Temperature */
		else
#endif
			*adc_value = (value * ADC_REF_VOLTAGE_IN_MVOLTS) >> 10;

	}
	return 0;
}

/*
 * adc_get_resolution
 */
int
adc_get_resolution(uint16_t *adc_resolution)
{
	*adc_resolution = ADC_NUM_BITS_FOR_RESOLUTION;
	return 0;
}

/*
 * adc_get_resolution
 */
int
adc_get_reference_voltage(uint16_t *adc_ref_volatge)
{
	*adc_ref_volatge = ADC_REF_VOLTAGE_IN_MVOLTS;
	return 0;
}

/*
 * adc_reboot_notifier
 */
int
adc_reboot_notifier(void)
{
	return 0;
}

/*
 * adc_hw_module_exit
 */
static void
adc_hw_module_exit(void)
{
	//adc_dev_t *dev = &adc_dev;

	unregister_hw_hal_module (EDEV_TYPE_ADC, m_dev_id);

	/* Disable  Monitoring, set channel 15 sample count to default*/ 
	*VCNT  &= (~0x00003F01);
	
	/* Disable  All channales */
	*VCHNE = 0x0;
	/* Disable  Global ADC */
	*((unsigned long *)(SE_SYS_CLK_VA_BASE + 0x34)) &= (~0x10);

	if (adc_remap_addr) iounmap(adc_remap_addr);
	adc_remap_addr = NULL;

#if defined (SOC_PILOT_IV)
	printk ("PILOT-IV adc hw module unloaded\n");
#elif defined (SOC_PILOT_III)
	printk ("PILOT-III adc hw module unloaded\n");
#else
	printk ("PILOT-II adc hw module unloaded\n");
#endif
}


/*
 * adc_hw_module_init
 */
static int adc_hw_module_init(void)
{

	int ret;
	//adc_dev_t	*dev = &adc_dev;

	if ((ret = register_hw_hal_module (&hw_hal, (void **)&padc_core_funcs)) < 0)
		{
			printk (KERN_ERR "%s: failed to register adc hal module\n", ADC_HW_DEV_NAME);
			return ret;
		}
	m_dev_id = ret;

	if((adc_remap_addr = ioremap_nocache(SE_ADC_THERMAL_BASE, ADC_REGS_SIZE)) == NULL)
	{
		printk("failed to map adc  IO space to memory\n");
		ret = -EIO;
	}

#ifdef SOC_PILOT_IV
	/* Enable Global ADC */
	*((unsigned long *)(SE_SYS_CLK_VA_BASE + 0x34)) |= 0x10;

	// Clear the bit indicating external Vref. Using the internal Vref.
	*VCNT &= ~0x04;

	/* Enable channel 0-8 */
	*VCHNE = 0x1FF;
	/* Enable Monitoring, set channel 7 sample count to 6 */
	*VCNT  |= 0x601;
#else
	/* Enable channel 0-14, not channel 15 */
	*VCHNE = 0x7FFF;
	/* Enable Monitoring, set channel 15 sample count to 6  */
	*VCNT  |= 0x00000601;
#endif

#if defined (SOC_PILOT_IV)
        printk ("PILOT-IV adc hw module loaded\n");
#elif defined (SOC_PILOT_III)
        printk ("PILOT-III adc hw module loaded\n");
#else
        printk ("PILOT-II adc hw module loaded\n");
#endif
	return 0;

}


module_init(adc_hw_module_init);
module_exit(adc_hw_module_exit);

MODULE_AUTHOR("American Megatrends Inc");
MODULE_DESCRIPTION("ADC HW driver module.");
MODULE_LICENSE("GPL");
