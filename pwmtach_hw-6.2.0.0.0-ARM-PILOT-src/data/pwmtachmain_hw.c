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
 * PwmTach Hardware Driver
 *
 * Copyright (C) 2010 American Megatrends Inc.
 *
 * This driver provides hardware layer for the PWMTACH driver.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "pwmtach_hw.h"
#include "pwmtach_hw_prop.h"
#include "pwmtach.h"
#include "driver_hal.h"

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
#define MAX_PWMTACH_HW_TACHS	16
#else
#define MAX_PWMTACH_HW_TACHS	8
#endif
#define PWMTACH_HW_DEV_NAME	"onboard_pwmtach"
#define VPWMTACH_REMAP_ADDR ((unsigned long) pwmtach_remap_addr)

static void* pwmtach_remap_addr = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int  __init pwmtach_hw_init (void);
static void __exit pwmtach_hw_exit (void);
#else
static int  __init pwmtach_hw_init (void);
static void __exit pwmtach_hw_exit (void);
#endif
static int init_pwmtach_hw (void);

int enable_pwm_control (unsigned char pwm_num);
unsigned char disable_pwm_control (unsigned char pwm_num);
unsigned char get_pwm_control_status (unsigned char pwm_num);
void enable_all_pwm_control (void);
void disable_all_pwm_control (void);
int enable_fantach_control (unsigned char ft_num);
unsigned char disable_fantach_control (unsigned char ft_num);
unsigned char get_fantach_control_status (unsigned char pwm_num);
void enable_all_fantach_control (void);
void disable_all_fantach_control (void);
void enable_counterresolution (unsigned char pwm_num);
unsigned char disable_counterresolution (unsigned char pwm_num);
unsigned int  get_current_speed (unsigned char tachnum);
void set_prescale (unsigned char pwm_num, unsigned char prescale_value);
unsigned int  get_prescale (unsigned char pwm_num);
void set_dutycycle (unsigned char pwm_num, int dutycycle_value);
unsigned int get_dutycycle (unsigned char pwm_num);
void set_counterresolution (unsigned char pwm_num, unsigned int counterres_value);
unsigned int  get_counterresolution (unsigned char pwm_num);
int trigger_read_fanspeed (unsigned char tachnum);
int can_read_fanspeed (unsigned char tachnum);
unsigned int  get_num_of_pwms (void);
unsigned int  get_num_of_tachs (void);
unsigned int  get_pwm_clk (void);
int set_tach_property (unsigned char property, unsigned char tach_num, unsigned int value);
unsigned int get_tach_property (unsigned char property, unsigned char tach_num);
int set_pwm_property (unsigned char property, unsigned char pwm_num, unsigned int value);
unsigned int get_pwm_property (unsigned char property, unsigned char pwm_num);
int clear_tach_error(unsigned char tach_num);
int clear_pwm_errors (void);

static int m_dev_id = 0;

static pwmtach_hal_operations_t pwmtach_hw_ops = {
	enable_pwm_control,
	disable_pwm_control,
	enable_all_pwm_control,
	disable_all_pwm_control,
	enable_fantach_control,
	disable_fantach_control,
	enable_all_fantach_control,
	disable_all_fantach_control,
	enable_counterresolution,
	disable_counterresolution,
	get_current_speed,
	set_prescale,
	get_prescale,
	set_dutycycle,
	set_counterresolution,
	get_counterresolution,
	trigger_read_fanspeed,
	can_read_fanspeed,
	get_num_of_pwms,
	get_num_of_tachs,
	get_pwm_clk,
	get_dutycycle,
	get_pwm_control_status,
	get_fantach_control_status,
	set_tach_property,
	get_tach_property,
	set_pwm_property,
	get_pwm_property,
	clear_tach_error,
	clear_pwm_errors,
};

static pwmtach_core_funcs_t pwmtach_core_funcs;
static pwmtach_core_funcs_t * ppwmtach_core_funcs = &pwmtach_core_funcs;

static hw_hal_t hw_hal = {
        .dev_type = EDEV_TYPE_PWMTACH,
        .owner = THIS_MODULE,
        .devname = PWMTACH_HW_DEV_NAME,
        .num_instances = MAX_PWMTACH_HW_TACHS,
        .phal_ops = (void *)&pwmtach_hw_ops
};


/****************************************************************************************/

/*  reads a byte from the offset  */
static u8 pwmtach_read_reg(uint32_t offset)
{
	return (ioread8((void __iomem*)offset));
}

/*  writes a byte to the offset */
static void pwmtach_write_reg(uint8_t value, uint32_t offset)
{
	iowrite8(value,(void __iomem*)offset);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int pwmtach_hw_init (void)
#else
static int __init pwmtach_hw_init (void)
#endif
{
	int ret = 0 ;

	if (( pwmtach_remap_addr = ioremap_nocache(PWMBASE, PWMTACH_SIZE)) == NULL)
        {
                printk("failed to map GPIO IO space %08x-%08x to memory", PWMBASE, PWMBASE + PWMTACH_SIZE - 1);
                return -EIO;
        }

	if ((ret = register_hw_hal_module (&hw_hal, (void **) &ppwmtach_core_funcs)) < 0)
	{
		printk (KERN_ERR "%s: failed to register pwmtach hal module\n", PWMTACH_HW_DEV_NAME);
		iounmap(pwmtach_remap_addr);
		return ret;
	}
	m_dev_id = ret;

	if (init_pwmtach_hw() != 0)
	{
                printk("failed to initialize the PWMTACH HW module\n");
                ret = -2;
                goto fail;
        }

	printk("PWMTACH HW Driver, (c) 2010 American Megatrends Inc.\n");
	return 0 ;
fail:
	pwmtach_hw_exit ();
	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static void pwmtach_hw_exit (void)
#else
static void __exit pwmtach_hw_exit (void)
#endif
{
	if (0 != unregister_hw_hal_module (EDEV_TYPE_PWMTACH, m_dev_id))
                printk("failed to unregister the PWMTACH HW module\n");

	if (pwmtach_remap_addr)
	{
		iounmap(pwmtach_remap_addr);
		pwmtach_remap_addr = NULL;
	}

	printk ( "PWMTACH HW Driver is unloaded sucessfully.\n");
	return;
}


int enable_pwm_control(unsigned char pwm_num)
{
	volatile uint8_t CfgVal = 0;
	uint8_t CfgMask = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWCFGOFF;

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		pwm_num -= NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	switch (pwm_num)
	{
		case 0:
			CfgMask = PWM0_CTRL_ENABLE;	break;
		case 1:
			CfgMask = PWM1_CTRL_ENABLE;	break;
		case 2:
			CfgMask = PWM2_CTRL_ENABLE;	break;
		case 3:
			CfgMask = PWM3_CTRL_ENABLE;	break;
		default:
			printk("enable_pwm_control: Invalid pwm_num\n");
			return -1;
	}

	CfgVal = pwmtach_read_reg (RegAddr);
	CfgVal |= CfgMask;
	pwmtach_write_reg ( CfgVal, RegAddr);

	return 0;
}

unsigned char disable_pwm_control(unsigned char pwm_num)
{
	volatile uint8_t CfgVal = 0;
	uint8_t CfgMask = 0;
	uint8_t PrevVal = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWCFGOFF;

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		pwm_num -= NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	switch (pwm_num)
	{
		case 0:
			CfgMask = PWM0_CTRL_ENABLE;     break;
		case 1:
			CfgMask = PWM1_CTRL_ENABLE;     break;
		case 2:
			CfgMask = PWM2_CTRL_ENABLE;     break;
		case 3:
			CfgMask = PWM3_CTRL_ENABLE;     break;
		default:
			printk("disable_pwm_control: Invalid pwm_num\n");
		return -1;
	}

	CfgVal = pwmtach_read_reg (RegAddr);
	PrevVal = (CfgVal & CfgMask) >> pwm_num;
	CfgVal &= ~CfgMask;
	pwmtach_write_reg (CfgVal, RegAddr);

	return PrevVal;
}

unsigned char get_pwm_control_status(unsigned char pwm_num)
{
	volatile uint8_t CfgVal = 0;
	unsigned char bitOffset = 0;
	uint8_t CfgMask = 0;
	uint8_t Status = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWCFGOFF;

	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif
	
	switch (bitOffset)
	{
		case 0:
			CfgMask = PWM0_CTRL_ENABLE;     break;
		case 1:
			CfgMask = PWM1_CTRL_ENABLE;     break;
		case 2:
			CfgMask = PWM2_CTRL_ENABLE;     break;
		case 3:
			CfgMask = PWM3_CTRL_ENABLE;     break;
		default:
			printk("get_pwm_control_status: Invalid pwm_num\n");
		return -1;
	}

	CfgVal = pwmtach_read_reg (RegAddr);
	Status = (CfgVal & CfgMask) >> bitOffset;
	Status = (Status != 0) ? 1 : 0;
	
	return Status;
}


void enable_all_pwm_control (void)
{
	volatile uint8_t CfgVal = 0;
	uint8_t CfgMask = 0;

	CfgVal = pwmtach_read_reg (VPWMTACH_REMAP_ADDR + PWCFGOFF);
	CfgMask = PWM0_CTRL_ENABLE | PWM1_CTRL_ENABLE | PWM2_CTRL_ENABLE | PWM3_CTRL_ENABLE;

	CfgVal |= CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + PWCFGOFF);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	CfgVal = pwmtach_read_reg (VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWCFGOFF);
	CfgMask = PWM0_CTRL_ENABLE | PWM1_CTRL_ENABLE | PWM2_CTRL_ENABLE | PWM3_CTRL_ENABLE;

	CfgVal |= CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWCFGOFF);
#endif

}


void disable_all_pwm_control (void)
{
	volatile uint8_t CfgVal = 0;
	uint8_t CfgMask = 0;

	CfgVal = pwmtach_read_reg (VPWMTACH_REMAP_ADDR + PWCFGOFF);
	CfgMask = PWM0_CTRL_ENABLE | PWM1_CTRL_ENABLE | PWM2_CTRL_ENABLE | PWM3_CTRL_ENABLE;

	CfgVal &= ~CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + PWCFGOFF);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	CfgVal = pwmtach_read_reg (VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWCFGOFF);
	CfgMask = PWM0_CTRL_ENABLE | PWM1_CTRL_ENABLE | PWM2_CTRL_ENABLE | PWM3_CTRL_ENABLE;

	CfgVal &= ~CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWCFGOFF);
#endif
}


int enable_fantach_control (unsigned char ft_num)
{
	volatile uint8_t CfgVal = 0;
	unsigned char bitOffset = 0;
	uint8_t CfgMask = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + FMCFGOFF;

	bitOffset = ft_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(ft_num > MAXFTMCHN_PILOT)
	{
		bitOffset = ft_num - NOFEXTFTMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	switch(bitOffset)
	{
		case 0:
			CfgMask = FT0_MONITOR_ENABLE; 	break;
		case 1:
			CfgMask = FT1_MONITOR_ENABLE;	break;
		case 2:
			CfgMask = FT2_MONITOR_ENABLE;	break;
		case 3:
			CfgMask = FT3_MONITOR_ENABLE;	break;
		case 4:
			CfgMask = FT4_MONITOR_ENABLE; 	break;
		case 5:
			CfgMask = FT5_MONITOR_ENABLE;	break;
		case 6:
			CfgMask = FT6_MONITOR_ENABLE;	break;
		case 7:
			CfgMask = FT7_MONITOR_ENABLE;	break;
		default:
			printk("enable_fantach_control: Invalid ft_num\n");
			return -1;
	}

	CfgVal = pwmtach_read_reg(RegAddr);
	CfgVal |= CfgMask;
	pwmtach_write_reg(CfgVal, RegAddr);

	return 0;
}

unsigned char disable_fantach_control (unsigned char ft_num)
{
	volatile uint8_t CfgVal = 0;
	unsigned char bitOffset = 0;
	uint8_t CfgMask = 0;
	uint8_t PrevVal = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + FMCFGOFF;

	bitOffset = ft_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(ft_num > MAXFTMCHN_PILOT)
	{
		bitOffset = ft_num - NOFEXTFTMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	switch(bitOffset)
	{
		case 0:
			CfgMask = FT0_MONITOR_ENABLE; 	break;
		case 1:
			CfgMask = FT1_MONITOR_ENABLE;	break;
		case 2:
			CfgMask = FT2_MONITOR_ENABLE;	break;
		case 3:
			CfgMask = FT3_MONITOR_ENABLE;	break;
		case 4:
			CfgMask = FT4_MONITOR_ENABLE; 	break;
		case 5:
			CfgMask = FT5_MONITOR_ENABLE;	break;
		case 6:
			CfgMask = FT6_MONITOR_ENABLE;	break;
		case 7:
			CfgMask = FT7_MONITOR_ENABLE;	break;
		default:
			printk("disable_fantach_control: Invalid ft_num\n");
			return -1;
	}

	CfgVal = pwmtach_read_reg(RegAddr);
	PrevVal = (CfgVal & CfgMask) >> bitOffset;
	CfgVal &= ~CfgMask;
	pwmtach_write_reg (CfgVal, RegAddr);

	return PrevVal;
}

unsigned char get_fantach_control_status (unsigned char ft_num)
{
	volatile uint8_t CfgVal = 0;
	unsigned char bitOffset = 0;
	uint8_t CfgMask = 0;
	uint8_t Status = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + FMCFGOFF;

	bitOffset = ft_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(ft_num > MAXFTMCHN_PILOT)
	{
		bitOffset = ft_num - NOFEXTFTMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	switch(bitOffset)
	{
		case 0:
			CfgMask = FT0_MONITOR_ENABLE; 	break;
		case 1:
			CfgMask = FT1_MONITOR_ENABLE;	break;
		case 2:
			CfgMask = FT2_MONITOR_ENABLE;	break;
		case 3:
			CfgMask = FT3_MONITOR_ENABLE;	break;
		case 4:
			CfgMask = FT4_MONITOR_ENABLE; 	break;
		case 5:
			CfgMask = FT5_MONITOR_ENABLE;	break;
		case 6:
			CfgMask = FT6_MONITOR_ENABLE;	break;
		case 7:
			CfgMask = FT7_MONITOR_ENABLE;	break;
		default:
			printk("get_fantach_control_status: Invalid ft_num\n");
			return -1;
	}

	CfgVal = pwmtach_read_reg(RegAddr);
	Status = (CfgVal & CfgMask) >> bitOffset;
	Status = (Status != 0) ? 1 : 0;

	return Status;
}

void enable_all_fantach_control (void)
{
	volatile uint8_t CfgVal = 0;
	uint8_t CfgMask = 0;

	CfgVal = pwmtach_read_reg(VPWMTACH_REMAP_ADDR + FMCFGOFF);
	CfgMask = FT0_MONITOR_ENABLE | FT1_MONITOR_ENABLE | FT2_MONITOR_ENABLE | FT3_MONITOR_ENABLE | FT4_MONITOR_ENABLE
		| FT5_MONITOR_ENABLE | FT6_MONITOR_ENABLE | FT7_MONITOR_ENABLE;

	CfgVal |= CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + FMCFGOFF);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	CfgVal = pwmtach_read_reg(VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + FMCFGOFF);
	CfgMask = FT0_MONITOR_ENABLE | FT1_MONITOR_ENABLE | FT2_MONITOR_ENABLE | FT3_MONITOR_ENABLE | FT4_MONITOR_ENABLE
		| FT5_MONITOR_ENABLE | FT6_MONITOR_ENABLE | FT7_MONITOR_ENABLE;

	CfgVal |= CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + FMCFGOFF);
#endif
}

void disable_all_fantach_control (void)
{
	volatile uint8_t CfgVal = 0;
	uint8_t CfgMask = 0;

	CfgVal = pwmtach_read_reg(VPWMTACH_REMAP_ADDR + FMCFGOFF);
	CfgMask = FT0_MONITOR_ENABLE | FT1_MONITOR_ENABLE | FT2_MONITOR_ENABLE | FT3_MONITOR_ENABLE | FT4_MONITOR_ENABLE
		| FT5_MONITOR_ENABLE | FT6_MONITOR_ENABLE | FT7_MONITOR_ENABLE;

	CfgVal &= ~CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + FMCFGOFF);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	CfgVal = pwmtach_read_reg(VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + FMCFGOFF);
	CfgMask = FT0_MONITOR_ENABLE | FT1_MONITOR_ENABLE | FT2_MONITOR_ENABLE | FT3_MONITOR_ENABLE | FT4_MONITOR_ENABLE
		| FT5_MONITOR_ENABLE | FT6_MONITOR_ENABLE | FT7_MONITOR_ENABLE;

	CfgVal &= ~CfgMask;
	pwmtach_write_reg (CfgVal, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + FMCFGOFF);
#endif
}


void enable_counterresolution(unsigned char pwm_num)
{
	volatile uint8_t  CfgVal;
	unsigned char bitOffset = 0;
	uint8_t CfgMask = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWCRCSROFF;

	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	CfgVal = pwmtach_read_reg(RegAddr);

	if(bitOffset == 0)
		CfgMask = PWM0_ENB_PROG_CNTR_RES;
	else if(bitOffset == 1)
		CfgMask = PWM1_ENB_PROG_CNTR_RES;
	else if(bitOffset == 2)
		CfgMask = PWM2_ENB_PROG_CNTR_RES;
	else if(bitOffset == 3)
		CfgMask = PWM3_ENB_PROG_CNTR_RES;

	CfgVal |= CfgMask;
	pwmtach_write_reg (CfgVal, RegAddr);
}

unsigned char disable_counterresolution(unsigned char pwm_num)
{
	volatile uint8_t  CfgVal;
	unsigned char bitOffset = 0;
	uint8_t CfgMask = 0;
	uint8_t PrevVal = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWCRCSROFF;

	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	CfgVal = pwmtach_read_reg(RegAddr);

	if(bitOffset == 0)
		CfgMask = PWM0_ENB_PROG_CNTR_RES;
	else if(bitOffset == 1)
		CfgMask = PWM1_ENB_PROG_CNTR_RES;
	else if(bitOffset == 2)
		CfgMask = PWM2_ENB_PROG_CNTR_RES;
	else if(bitOffset == 3)
		CfgMask = PWM3_ENB_PROG_CNTR_RES;

	PrevVal = (CfgVal & CfgMask) >> bitOffset;
	CfgVal &= ~CfgMask;
	pwmtach_write_reg (CfgVal, RegAddr);

	return PrevVal;
}
static unsigned int callRPM(unsigned char tachnum, unsigned char rawVal)
{
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    unsigned char bitOffset = 0;
    unsigned int clockFactor = 0;

    if(rawVal == 0) 
    	return 0; //avoid DIV/0
    
    bitOffset = tachnum;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(tachnum > MAXFTMCHN_PILOT)
    {
    	bitOffset = tachnum - NOFEXTFTMCHN;
        RegAddr += PWMTACHEXTOFF;
    }
#endif
    if (bitOffset > 7) 
    	return 0;
  
        
    RegAddr += FMCSR0OFF + (0x0C * bitOffset);
    RegVal = pwmtach_read_reg(RegAddr);
    RegVal >>= 6; // use 7:6 bits 
    
    switch(RegVal & 0x03)
    {
    	case 0x00:
    		clockFactor = CLK_SELECT_00;
      		break;
    	case 0x01:
    		clockFactor = CLK_SELECT_01;
    		break;
    	case 0x02:
    		clockFactor = CLK_SELECT_10;
    		break;
    	case 0x03:
    		clockFactor = CLK_SELECT_11;
    		break;
    }
	return ((60 * clockFactor)/rawVal);
}

unsigned int get_current_speed(unsigned char tachnum)
{
	volatile uint8_t CurrentSpeed = 0;
	unsigned char bitOffset = 0;
	int rawdata = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + FMSPR0OFF;

	rawdata = (tachnum & 0x80) ? 1 : 0;
	tachnum &= 0x7f;

	bitOffset = tachnum;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(tachnum > MAXFTMCHN_PILOT)
	{
		bitOffset = tachnum - NOFEXTFTMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	CurrentSpeed = pwmtach_read_reg(RegAddr + (bitOffset * 0x0C));
	
	if (rawdata)
		return CurrentSpeed;

	return callRPM(tachnum, CurrentSpeed);
}


void set_prescale (unsigned char pwm_num, unsigned char prescale_value)
{
	uint8_t prev_pwm_ctrl_state = 0;
	unsigned char bitOffset = 0;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWPSR0OFF;
	uint8_t RegVal;

	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif
	if (bitOffset > 3)
	{
		return;
	}

	RegVal = pwmtach_read_reg((RegAddr + (bitOffset * 8)));
	RegVal &= 0x80;

	prev_pwm_ctrl_state = disable_pwm_control(pwm_num);

	pwmtach_write_reg ( RegVal | ((prescale_value-1) & 0x7F), (RegAddr + (bitOffset * 8)));
	if (prev_pwm_ctrl_state)
		enable_pwm_control(pwm_num);
}


unsigned int get_prescale (unsigned char pwm_num)
{
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWPSR0OFF;
	unsigned char prescale_reg;
	unsigned char bitOffset = 0;

	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	prescale_reg = pwmtach_read_reg(RegAddr + (bitOffset * 8));
	return (prescale_reg & 0x7F);
}


void set_dutycycle(unsigned char pwm_num, int dutycycle_value)
{
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWDCR0OFF;
	unsigned char bitOffset = 0;
	
	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif	
	pwmtach_write_reg(dutycycle_value, (RegAddr + (bitOffset * 8)));
}
unsigned int get_dutycycle(unsigned char pwm_num)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWDCR0OFF;
    uint8_t RegVal = 0;
    unsigned char bitOffset = 0;
    
    bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(pwm_num > MAXPWMCHN_PILOT)
    {
    	bitOffset = pwm_num - NOFEXTPWMCHN; 
        RegAddr += PWMTACHEXTOFF; 
    }
#endif    
    RegVal = (pwmtach_read_reg((RegAddr + (bitOffset * 8))));
    return RegVal;
}


void set_counterresolution(unsigned char pwm_num, unsigned int counterres_value)
{
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWCRVR0OFF;
	unsigned char bitOffset = 0;

	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	pwmtach_write_reg(counterres_value, (RegAddr + (bitOffset * 4)) );
}


unsigned int get_counterresolution(unsigned char pwm_num)
{
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + PWCRVR0OFF;
	unsigned char bitOffset = 0;

	bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(pwm_num > MAXPWMCHN_PILOT)
	{
		bitOffset = pwm_num - NOFEXTPWMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	return pwmtach_read_reg(RegAddr + (bitOffset * 4));
}

int trigger_read_fanspeed (unsigned char tachnum)
{
	//unsigned char bitOffset = 0;
	//tachnum &= 0x7F; //ignore RAW-bit
	//
	//bitOffset = tachnum;
	return 0;
}

int  can_read_fanspeed(unsigned char tachnum)
{
	volatile uint8_t ctrl_stat_val;
	uint32_t RegAddr = VPWMTACH_REMAP_ADDR + FMCSR0OFF;
	unsigned char bitOffset = 0;

	tachnum &= 0x7F; //ignore RAW-bit
	
	bitOffset = tachnum;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if(tachnum > MAXFTMCHN_PILOT)
	{
		bitOffset = tachnum - NOFEXTFTMCHN;
		RegAddr += PWMTACHEXTOFF;
	}
#endif

	ctrl_stat_val = pwmtach_read_reg(RegAddr + (bitOffset * 0x0C));

        /* Clearing the bits - One Sec Error, Overflow and Over Threshold - if set, 
           because only when cleared we can read the fan speed */
	if ((ctrl_stat_val & 0x0E) != 0) 
	{
		pwmtach_write_reg (ctrl_stat_val & 0xFE, RegAddr + (bitOffset * 0x0C));
	}

	if (ctrl_stat_val & SPEED_READY)
		return 1;
	return 0;
}


unsigned int get_num_of_pwms (void)
{
        return NUMPWM;
}


unsigned int get_num_of_tachs (void)
{
	return NUMFT;
}


unsigned int get_pwm_clk (void)
{
	return PWM_CLK_FREQ;
}

int set_tach_filterselect (unsigned char tach_num, unsigned char value)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    unsigned char bitOffset = 0; 
    uint8_t TmpVal = 0;
    uint8_t prev_ft_ctrl_state = 0;
   
    /* This takes only 2 bits 
     * bit 0 - Fan Tach Monitor <tach_num> Filter Select[0]
     * bit 1 - Fan Tach Monitor <tach_num> Filter Select[1]
     * 
     * 00: Filter value is ~780us(Default)
     * 01: Filter value is ~530us
     * 10: Filter value is ~280us
     * 11: Filter value is ~155us
     */  
    bitOffset = tach_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(tach_num > MAXFTMCHN_PILOT)
    {
    	bitOffset = tach_num - NOFEXTFTMCHN;
        RegAddr += PWMTACHEXTOFF;
    }
#endif
    
    if (bitOffset <= 3)
    {
        RegAddr += FMPFSR0OFF;
    }
    else if (bitOffset <= 7)
    {
        RegAddr += FMPFSR1OFF;
        bitOffset -= 4;
    }
    else
    {
        return -1;
    }
    
    RegVal = pwmtach_read_reg(RegAddr);        
    if ( ( (RegVal) & ((0x03) << (bitOffset * 2)) ) != (value << (bitOffset * 2)) )
    {
    	prev_ft_ctrl_state = disable_fantach_control (tach_num);
    	
    	TmpVal = ~ ((0x03) << (bitOffset * 2)); // Used to clear the 2 bits.
    	RegVal = ( RegVal & TmpVal ) | (value << (bitOffset * 2));
    	pwmtach_write_reg (RegVal, RegAddr);
		
    	//Make sure the filter is anabled
    	RegAddr = VPWMTACH_REMAP_ADDR;
    	bitOffset = tach_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
   		if(tach_num > MAXFTMCHN_PILOT)
    	{
   			bitOffset = tach_num - NOFEXTFTMCHN;
        	RegAddr += PWMTACHEXTOFF;
    	}
#endif
   		
   		if (bitOffset > 7)
    	{
        	return -1;
    	}
      
    	RegAddr += FMCSR0OFF + (0x0C * bitOffset);
    	RegVal = pwmtach_read_reg(RegAddr);
    	RegVal = ( RegVal & 0xDF );
    	pwmtach_write_reg (RegVal, RegAddr);
    	
    	if(prev_ft_ctrl_state){
    		enable_fantach_control (tach_num);
    	}
    }    
    
    return 0;
}

int set_tach_clockselect (unsigned char tach_num, unsigned char value)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    unsigned char bitOffset = 0; 
    uint8_t TmpVal = 0;
    uint8_t prev_ft_ctrl_state = 0;
    /* This takes only 2 bits 
     * bit 0 - Clock Select[0]
     * bit 1 - Clock Select[1]
     * 
     * 00: 16 KHz
     * 01: 8 KHz (default) 
     * 10: 4 KHz
     * 11: 2 KHz
     */
    value = value & 0x03;
    bitOffset = tach_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(tach_num > MAXFTMCHN_PILOT)
    {
    	bitOffset = tach_num - NOFEXTFTMCHN;
        RegAddr += PWMTACHEXTOFF;
    }
#endif
    if (bitOffset > 7)
    {
        return -1;
    }
        
    RegAddr += FMCSR0OFF + (0x0C * bitOffset);
    RegVal = pwmtach_read_reg(RegAddr);
    
    if( (RegVal & (0xC0)) != (value << 6) )
    {
    	prev_ft_ctrl_state = disable_fantach_control (tach_num);
    	
    	TmpVal = (~ (0xC0)) & 0xFF; // Used to clear the 2 bits.
    	RegVal = ( RegVal & TmpVal ) | (value << 6);    
    	pwmtach_write_reg (RegVal, RegAddr);
    	
    	if(prev_ft_ctrl_state){
    		enable_fantach_control (tach_num);
    	}
    }        
    return 0;
}

int clear_tach_error (unsigned char tach_num)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    unsigned char bitOffset = 0;
    
    bitOffset = tach_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(tach_num > MAXFTMCHN_PILOT)
    {
    	bitOffset = tach_num - NOFEXTFTMCHN;
        RegAddr += PWMTACHEXTOFF;
    }
#endif
    if (bitOffset > 7)
    {
        return -1;
    }
        
    RegAddr += FMCSR0OFF + (0x0C * bitOffset);
    RegVal = pwmtach_read_reg(RegAddr);    
    RegVal = ( RegVal & 0xF7 );	
    pwmtach_write_reg (RegVal, RegAddr);
    
    return 0;
}

int set_pwm_division (unsigned char pwm_num, unsigned char value)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    uint8_t TmpVal = 0;
    unsigned char bitOffset = 0;
    
    /* This takes only one bit
     * value differ according to PWDCR[7].
     */
    value &= 0x01;
    bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(pwm_num > MAXPWMCHN_PILOT)
    {
    	bitOffset = pwm_num - NOFEXTPWMCHN;
        RegAddr += PWMTACHEXTOFF;
    }
#endif
    if (bitOffset > 3)
    {
        return -1;
    }
    RegAddr += PWOCFOFF;
    RegVal = pwmtach_read_reg(RegAddr);
    TmpVal = ~ ((0x01) << (4 + bitOffset)); // Used to clear the 1 bit from 4 - 7.
    RegVal = ( RegVal & TmpVal ) | (value << (4 + bitOffset));
    pwmtach_write_reg (RegVal, RegAddr);
    return 0;
}

int set_pwm_clockselect (unsigned char pwm_num, unsigned char value)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    uint8_t TmpVal = 0;
    uint8_t prev_pwm_ctrl_state = 0;
    unsigned char bitOffset = 0;
    /* This takes only one bit
     * 0: 8.33 MHz
     * 1: 200 KHz
     */
    value &= 0x01;
    bitOffset = pwm_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(pwm_num > MAXPWMCHN_PILOT)
    {
    	bitOffset = pwm_num - NOFEXTPWMCHN;
        RegAddr += PWMTACHEXTOFF;
    }
#endif
    if (bitOffset > 3)
    {
        return -1;
    }
       
    RegAddr += PWPSR0OFF + (8 * bitOffset);
    RegVal = pwmtach_read_reg(RegAddr);
    
    if ( (RegVal & 0x80) != (value << 7 ) )
    {
    	prev_pwm_ctrl_state = disable_pwm_control(pwm_num);
		
    	TmpVal = (~ (0x80)) & 0xFF; // Used to clear the 1 bit.
    	RegVal = ( RegVal & TmpVal ) | (value << 7);	
    	pwmtach_write_reg (RegVal, RegAddr);
		
    	if (prev_pwm_ctrl_state){		
    		enable_pwm_control(pwm_num);
    	}
    }    
    
    return 0;
}

int clear_pwm_errors (void)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    
    RegAddr += PWCRCSROFF;
    RegVal = pwmtach_read_reg(RegAddr);    
    RegVal = ( RegVal & ~PWM0_PROG_CNTR_RES_ERR & ~PWM1_PROG_CNTR_RES_ERR & ~PWM2_PROG_CNTR_RES_ERR & ~PWM3_PROG_CNTR_RES_ERR );
	
    pwmtach_write_reg (RegVal, RegAddr);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    pwmtach_write_reg (RegVal, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWCRCSROFF);
#endif 
    return 0;
}

static unsigned int get_tach_fmcsr_bit (unsigned char tach_num, unsigned char bit_pos)
{
    uint32_t RegAddr = VPWMTACH_REMAP_ADDR;
    volatile uint8_t RegVal = 0;
    unsigned int Value;
    unsigned char bitOffset = 0;
    
    bitOffset = tach_num;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
    if(tach_num > MAXFTMCHN_PILOT)
    {
    	bitOffset = tach_num - NOFEXTFTMCHN;
        RegAddr += PWMTACHEXTOFF;
    }
#endif
    if (bitOffset > 7)
    {
        return 0xFFFFFFFF;
    }
    
    RegAddr += FMCSR0OFF + (0x0C * bitOffset);
    RegVal = pwmtach_read_reg(RegAddr);
    Value = (RegVal & (0x01 << bit_pos)) ? 1 : 0;
    return Value;
}

unsigned int get_tach_error (unsigned char tach_num)
{
    return get_tach_fmcsr_bit (tach_num, 3);
}

unsigned int get_tach_overflow (unsigned char tach_num)
{
    return get_tach_fmcsr_bit (tach_num, 2);
}

unsigned int get_tach_speedready (unsigned char tach_num)
{
    return get_tach_fmcsr_bit (tach_num, 0);
}

/*
 * @fn set_pwm_tach_property
 * @brief Generic function to set a property of either pwm or tach.
 * @param[in] property - Property to set and this can be different for different platform.
 * @param[in] pwm_tach_num - pwm number or tach number according to property.
 * @param[in] value - appropriate value to set
 * @retval 0 on success, otherwise -1.
 */

/*
 * PWM TACH PROPERTY
#define TACH_FILTERSELECT				0x01
#define TACH_CLOCKSELECT				0x02
#define TACH_ERROR						0x03
#define TACH_OVERFLOW					0x04
#define TACH_SPEEDREADY					0x05

#define PWM_DIVISION_128_64				0x06
#define PWM_CLOCKSELECT					0x07
#define PWM_PRESCALEVALUE				0x08
#define PWM_COUNTERRESOLUTIONVALUE		0x09
*/

int set_pwm_tach_property (unsigned char property, unsigned char pwm_tach_num, unsigned int value)
{
    switch (property)
    {
    case TACH_FILTERSELECT:
        /* This takes only 2 bits */
        return set_tach_filterselect (pwm_tach_num, value & 0x03); 
    case TACH_CLOCKSELECT:
        /* This takes only 2 bits */
        return set_tach_clockselect (pwm_tach_num, value & 0x03);
    case TACH_ERROR:
    case TACH_OVERFLOW:
    case TACH_SPEEDREADY:
        // Currently not supported...
        return -1;
        break;
        
    case PWM_DIVISION_128_64:
        return set_pwm_division (pwm_tach_num, value & 0x01);
        break;
    case PWM_CLOCKSELECT:
        return set_pwm_clockselect (pwm_tach_num, value & 0x01);
    case PWM_PRESCALEVALUE:
        /* This takes only 7 bits */
        set_prescale(pwm_tach_num, value & 0x7F);
        break;
    case PWM_COUNTERRESOLUTIONVALUE:
        /* This takes 8 bits */
        set_counterresolution(pwm_tach_num, value & 0xFF);
        break;
    default:
        return -1;
    }
    return 0;
}

/*
 * @fn get_pwm_tach_property
 * @brief Generic function to get a property of either pwm ot tach.
 * @param[in] property - Property to get and this can be different for different platform.
 * @param[in] pwm_tach_num - pwm number or tach number according to property.
 * @retval return appropriate value, 0xFFFFFFFF on failure.
 */
unsigned int get_pwm_tach_property (unsigned char property, unsigned char pwm_tach_num)
{
    switch (property)
    {
    case TACH_FILTERSELECT:    	
    case TACH_CLOCKSELECT:
        // Currently not supported..
        return 0xFFFFFFFF;
        break;
    case TACH_ERROR:
        return get_tach_error (pwm_tach_num);
    case TACH_OVERFLOW:
        return get_tach_overflow (pwm_tach_num);
    case TACH_SPEEDREADY:
        return get_tach_speedready (pwm_tach_num);
    case PWM_DIVISION_128_64:
    case PWM_CLOCKSELECT:
    case PWM_PRESCALEVALUE:
    case PWM_COUNTERRESOLUTIONVALUE:
        // Currently not supported...
    default:
        return 0xFFFFFFFF;
    }
    return 0;	
}

int set_tach_property (unsigned char property, unsigned char tach_num, unsigned int value)
{
    return set_pwm_tach_property(property, tach_num, value);
}

unsigned int get_tach_property (unsigned char property, unsigned char tach_num)
{
    return get_pwm_tach_property(property, tach_num);
}

int set_pwm_property (unsigned char property, unsigned char pwm_num, unsigned int value)
{
    return set_pwm_tach_property(property, pwm_num, value);
}

unsigned int get_pwm_property (unsigned char property, unsigned char pwm_num)
{
    return get_pwm_tach_property(property, pwm_num);
}



static int init_pwmtach_hw (void)
{
	volatile uint8_t  value;
	uint8_t i = 0;
	uint8_t prev_ft_ctrl_state = 0;

	/* Initialize the PWOCF register to have the fixed 256 divisor set for all PWMs */
	value = (uint8_t) ~FAN0_ENB_ON_OTS & ~FAN1_ENB_ON_OTS & ~FAN2_ENB_ON_OTS & ~FAN3_ENB_ON_OTS
		& ~PWM0_ENB_DIV_BY_128_64 & ~PWM1_ENB_DIV_BY_128_64 & ~PWM2_ENB_DIV_BY_128_64 & ~PWM3_ENB_DIV_BY_128_64;
	pwmtach_write_reg (value, VPWMTACH_REMAP_ADDR + PWOCFOFF);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	pwmtach_write_reg (value, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWOCFOFF);
#endif

	/* Set Duty Cycle register to 0x80 to reflect 50% duty cycle for all PWMs */
	for (i = 0; i < NUMPWM; i++)
	{
		pwmtach_write_reg(0x80, VPWMTACH_REMAP_ADDR + (PWDCR0OFF + (i * 8)));
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
		pwmtach_write_reg(0x80, (VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + (PWDCR0OFF + (i * 8))));
#endif
	}

	/* Set all PWCRVR registers to value 0 as we don't want to use counter resolution value */
	for (i = 0; i < NUMPWM; i++)
	{
		pwmtach_write_reg(0x00,  VPWMTACH_REMAP_ADDR + (PWCRVR0OFF + (i * 4)));
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
		pwmtach_write_reg(0x00,  (VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + (PWCRVR0OFF + (i * 4))));
#endif
	}

	/* Set bits for all PWMs in the PWCRCSR to value 0 to not use programmable counter resolution */
	value = (uint8_t) ~PWM0_ENB_PROG_CNTR_RES & ~PWM1_ENB_PROG_CNTR_RES &	~PWM2_ENB_PROG_CNTR_RES & ~PWM3_ENB_PROG_CNTR_RES
		& ~PWM0_PROG_CNTR_RES_ERR & ~PWM1_PROG_CNTR_RES_ERR & ~PWM2_PROG_CNTR_RES_ERR & ~PWM3_PROG_CNTR_RES_ERR;
	pwmtach_write_reg (value, VPWMTACH_REMAP_ADDR + PWCRCSROFF);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	pwmtach_write_reg (value, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWCRCSROFF);
#endif

	/* Set bits for all PWMs in the PWCFG to value 0 to not use PWM invert */
	value = (uint8_t) ~PWM0_CTRL_ENABLE &	~PWM1_CTRL_ENABLE &	~PWM2_CTRL_ENABLE &	~PWM3_CTRL_ENABLE &
		~PWM0_INVERT_ENABLE & ~PWM1_INVERT_ENABLE &	~PWM2_INVERT_ENABLE & ~PWM3_INVERT_ENABLE;
	pwmtach_write_reg (value, VPWMTACH_REMAP_ADDR + PWCFGOFF);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	pwmtach_write_reg (value, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + PWCFGOFF);
#endif

	/* Write PreScale register for all PWMs to set value as 0x07 */
	for(i = 0; i < NUMPWM; i++)
		set_prescale (i,0x07);

	/*********** Initialize FAN TACH registers ************/
	/* set Threshold_value = (60 * TACH_Clk_Frequency)/Mininum_desired_RPM , here Clk Freq = 4KHz, Min RPM = 1000 */
	for(i = 0; i < NUMFT; i++)
	{
        prev_ft_ctrl_state = disable_fantach_control (i);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
		if(i > MAXFTMCHN_PILOT)
			pwmtach_write_reg (0xF0, VPWMTACH_REMAP_ADDR + PWMTACHEXTOFF + FMTHR0OFF + ((i - NOFEXTFTMCHN) * 0x0C));
		else
			pwmtach_write_reg (0xF0, VPWMTACH_REMAP_ADDR + FMTHR0OFF + (i * 0x0C));
#else
		pwmtach_write_reg (0xF0, VPWMTACH_REMAP_ADDR + FMTHR0OFF + (i * 0x0C));
#endif
        if(prev_ft_ctrl_state)
			enable_fantach_control (i);
	}

	/* Set all FMCSR registers to chose 4KHz clock, filter disable, interrupt enable */
	for(i = 0; i < NUMFT; i++)
	{
		prev_ft_ctrl_state = disable_fantach_control (i);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
		if(i > MAXFTMCHN_PILOT)
		{
			pwmtach_write_reg (CLK_SELECT_4KHZ|FILTER_DISABLE|INTERRUPT_ENABLE,
				VPWMTACH_REMAP_ADDR + FMCSR0OFF + ((i - NOFEXTFTMCHN) * 0x0C));
		}
		else
		{
			pwmtach_write_reg (CLK_SELECT_4KHZ|FILTER_DISABLE|INTERRUPT_ENABLE,
				VPWMTACH_REMAP_ADDR + FMCSR0OFF + (i * 0x0C));
		}
#else
		pwmtach_write_reg (CLK_SELECT_4KHZ|FILTER_DISABLE|INTERRUPT_ENABLE,
			VPWMTACH_REMAP_ADDR + FMCSR0OFF + (i * 0x0C));
#endif

		if(prev_ft_ctrl_state)
			enable_fantach_control (i);
	}

	/* Set all filter select registers to value 0 */
	pwmtach_write_reg (0x00, VPWMTACH_REMAP_ADDR + FMPFSR0OFF);
	pwmtach_write_reg (0x00, VPWMTACH_REMAP_ADDR + FMPFSR1OFF);

	return 0;
}

/********************************************************************************/

module_init (pwmtach_hw_init);
module_exit (pwmtach_hw_exit);

MODULE_AUTHOR("American Megatrends Inc");
MODULE_DESCRIPTION("This is PwmTach HW driver module");
MODULE_LICENSE("GPL");

