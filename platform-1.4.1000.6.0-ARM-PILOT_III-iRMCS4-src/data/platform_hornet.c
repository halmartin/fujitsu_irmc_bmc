/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross              **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/

/****************************************************************
 *
 * platform_hornet.c
 * platform-specific initialization module for HORNET
 *
 ****************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/uaccess.h>


#define PIN_CTRL_BASE         SE_TOP_LVL_PIN_BASE
#define PIN_CTRL_SIZE		  256 
//see 3.23 Top Level Pin Control Registers
// Server Management Controller (Pilot3)
// Hardware Specifications
// 			&
// Programmers Reference Guide
// (Preliminary Version 0.996)
#define PILOT_III_TOP_LEVEL_GPIOEN_0			 0x00
#define PILOT_III_TOP_LEVEL_PIN_CTRL_PINMUXCTL   0x04
#define PILOT_III_TOP_LEVEL_GPIOEN_1			 0x08
#define PILOT_III_TOP_LEVEL_GPIOEN_2			 0x0C

#define GPIOEN_0_DEF 	0x000000FF
#define GPIOEN_0_MASK 	0x000FFFFF
static uint32_t GPIOEN_0 = GPIOEN_0_DEF;
module_param (GPIOEN_0, uint, 0000);
MODULE_PARM_DESC(GPIOEN_0, "GPIO Enable Register 0");

#define PINMUXCTL_DEF	0x0003FFFF
#define PINMUXCTL_MASK	0x01FFFFFF
static uint32_t PINMUXCTL = PINMUXCTL_DEF;
module_param (PINMUXCTL, uint, 0000);
MODULE_PARM_DESC(PINMUXCTL, "Pin Mux Control Register");

#define GPIOEN_1_DEF 	0x00000000
#define GPIOEN_1_MASK 	0xFF0FFFFF
static uint32_t GPIOEN_1 = GPIOEN_1_DEF;
module_param (GPIOEN_1, uint, 0000);
MODULE_PARM_DESC(GPIOEN_1, "GPIO Enable Register 1");

#define GPIOEN_2_DEF 	0x00000000
#define GPIOEN_2_MASK 	0x3FFFFFFF
static uint32_t GPIOEN_2 = GPIOEN_2_DEF;
module_param (GPIOEN_2, uint, 0000);
MODULE_PARM_DESC(GPIOEN_2, "GPIO Enable Register 2");


int __init init_module(void)
{
	uint32_t reg;
	void* remap_addr = NULL;
	if (( remap_addr = ioremap_nocache(PIN_CTRL_BASE, PIN_CTRL_SIZE)) == NULL)
	{
		printk("failed to map GPIO IO space %08x-%08x to memory", PIN_CTRL_BASE, PIN_CTRL_BASE + PIN_CTRL_SIZE - 1);
		return -EIO;
	}
	//GPIO Enable Register 0
	printk(KERN_DEBUG "\nGPIO Enable Register 0: 0x%08X\n",GPIOEN_0);
	if(GPIOEN_0_DEF != GPIOEN_0)
	{
		reg = GPIOEN_0 & GPIOEN_0_MASK;
		__raw_writel(reg, remap_addr + PILOT_III_TOP_LEVEL_GPIOEN_0);
	}
	//Pin Mux Control Register
	printk(KERN_DEBUG "Pin Mux Control Register: 0x%08X\n",PINMUXCTL);
	if(PINMUXCTL_DEF != PINMUXCTL)
	{
		reg = PINMUXCTL & PINMUXCTL_MASK;
		__raw_writel(reg, remap_addr + PILOT_III_TOP_LEVEL_PIN_CTRL_PINMUXCTL);
	}
	//GPIO Enable Register 1
	printk(KERN_DEBUG "GPIO Enable Register 1: 0x%08X\n",GPIOEN_1);
	if(GPIOEN_1_DEF != GPIOEN_1)
	{
		reg = GPIOEN_1 & GPIOEN_1_MASK;
		__raw_writel(reg, remap_addr + PILOT_III_TOP_LEVEL_GPIOEN_1);
	}
	//GPIO Enable Register 2
	printk(KERN_DEBUG "GPIO Enable Register 2: 0x%08X\n",GPIOEN_2);
	if(GPIOEN_2_DEF != GPIOEN_2)
	{
		reg = GPIOEN_2 & GPIOEN_2_MASK;
		__raw_writel(reg, remap_addr + PILOT_III_TOP_LEVEL_GPIOEN_2);
	}

	if(remap_addr != NULL)
		iounmap(remap_addr);
	
	return 0;
}

void __exit cleanup_module(void)
{
	/* do nothing */
}

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("platform-specific initialization module for HORNET");
MODULE_LICENSE("GPL");
