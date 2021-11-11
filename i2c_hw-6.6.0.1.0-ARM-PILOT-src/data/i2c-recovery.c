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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <mach/platform.h>
#include <asm/io.h>
#include <asm/irq.h>

#include "i2c-data.h"
#include "i2c-hardware.h"

extern struct i2c_pilot_ii_data se_pilot_ii_data_ptr[BUS_COUNT];


static bus_recovery_info_T m_bus_recovery_info [BUS_COUNT] =
{
#if (BUS_COUNT>=1)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=2)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=3)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=4)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=5)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=6)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=7)
        { ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=8)
        { ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=9)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
#if (BUS_COUNT>=10)
	{ ENABLE_RECOVERY, DISABLE_SMBUS_RESET, ENABLE_CLOCK_PULSE, DISABLE_FORCE_STOP, DEFAULT_NUM_PULSES, DEFAULT_FREQ },
#endif
};

static int recovery_check_time_ms[BUS_COUNT] = { 0 };  /* Bus recovery check time (in milliseconds) */

static void
set_1_to_i2c_pin_control_reg0 (unsigned long bitmask)
{
	unsigned long i2c_pin_ctrl0;

	i2c_pin_ctrl0 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24);

	i2c_pin_ctrl0 |= bitmask;

	*(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24) = i2c_pin_ctrl0;
}


static void
set_1_to_i2c_pin_control_reg1 (unsigned long bitmask)
{
	unsigned long i2c_pin_ctrl1;

	i2c_pin_ctrl1 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28);

	i2c_pin_ctrl1 |= bitmask;

	*(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28) = i2c_pin_ctrl1;
}

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
static void
set_1_to_i2c_pin_control_reg2 (unsigned long bitmask)
{
        unsigned long i2c_pin_ctrl2;

        i2c_pin_ctrl2 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910);

        i2c_pin_ctrl2 |= bitmask;

        *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910) = i2c_pin_ctrl2;
}
#endif
#if defined SOC_PILOT_IV
static void
set_1_to_i2c_pin_control_reg3 (unsigned long bitmask)
{
        unsigned long i2c_pin_ctrl3;

        i2c_pin_ctrl3 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960);

        i2c_pin_ctrl3 |= bitmask;

        *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960) = i2c_pin_ctrl3;
}
#endif

static void
set_0_to_i2c_pin_control_reg0 (unsigned long bitmask)
{
	unsigned long i2c_pin_ctrl0;

	i2c_pin_ctrl0 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24);

	i2c_pin_ctrl0 &= bitmask;

	*(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24) = i2c_pin_ctrl0;
}


static void
set_0_to_i2c_pin_control_reg1 (unsigned long bitmask)
{
	unsigned long i2c_pin_ctrl1;

	i2c_pin_ctrl1 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28);

	i2c_pin_ctrl1 &= bitmask;

	*(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28) = i2c_pin_ctrl1;
}

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
static void
set_0_to_i2c_pin_control_reg2 (unsigned long bitmask)
{
        unsigned long i2c_pin_ctrl2;

        i2c_pin_ctrl2 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910);

        i2c_pin_ctrl2 &= bitmask;

        *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910) = i2c_pin_ctrl2;
}
#endif

#if defined SOC_PILOT_IV
static void
set_0_to_i2c_pin_control_reg3 (unsigned long bitmask)
{
        unsigned long i2c_pin_ctrl3;

        i2c_pin_ctrl3 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960);

        i2c_pin_ctrl3 &= bitmask;

        *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960) = i2c_pin_ctrl3;
}
#endif

void pilot_bit_bang_deselect(int bus)
{
        unsigned long i2c0_bitmask = ~(unsigned long)(1<<0);
        unsigned long i2c1_bitmask = ~(unsigned long)(1<<8);
        unsigned long i2c2_bitmask = ~(unsigned long)(1<<16);
        unsigned long i2c3_bitmask = ~(unsigned long)(1<<24);
        unsigned long i2c4_bitmask = ~(unsigned long)(1<<0);
        unsigned long i2c5_bitmask = ~(unsigned long)(1<<8);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
        unsigned long i2c6_bitmask = ~(unsigned long)(1<<0);
        unsigned long i2c7_bitmask = ~(unsigned long)(1<<8);
#endif
#if defined SOC_PILOT_IV
        unsigned long i2c8_bitmask = ~(unsigned long)(1<<0);
        unsigned long i2c9_bitmask = ~(unsigned long)(1<<8);
#endif

        if (bus == 0)
                set_0_to_i2c_pin_control_reg0(i2c0_bitmask);

        if (bus == 1)
                set_0_to_i2c_pin_control_reg0(i2c1_bitmask);

        if (bus == 2)
                set_0_to_i2c_pin_control_reg0(i2c2_bitmask);

        if (bus == 3)
                set_0_to_i2c_pin_control_reg0(i2c3_bitmask);

        if (bus == 4)
                set_0_to_i2c_pin_control_reg1(i2c4_bitmask);

        if (bus == 5)
                set_0_to_i2c_pin_control_reg1(i2c5_bitmask);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
        if (bus == 6)
                set_0_to_i2c_pin_control_reg2(i2c6_bitmask);

        if (bus == 7)
                set_0_to_i2c_pin_control_reg2(i2c7_bitmask);
#endif
#if defined SOC_PILOT_IV
        if (bus == 8)
                set_0_to_i2c_pin_control_reg3(i2c8_bitmask);

        if (bus == 9)
                set_0_to_i2c_pin_control_reg3(i2c9_bitmask);
#endif

}

void pilot_bit_bang_select(int bus)
{
	unsigned long i2c0_bitmask = 1<<0;
	unsigned long i2c1_bitmask = 1<<8;
	unsigned long i2c2_bitmask = 1<<16;
	unsigned long i2c3_bitmask = 1<<24;
	unsigned long i2c4_bitmask = 1<<0;
	unsigned long i2c5_bitmask = 1<<8;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	unsigned long i2c6_bitmask = 1<<0;
	unsigned long i2c7_bitmask = 1<<8;
#endif
#if defined SOC_PILOT_IV
	unsigned long i2c8_bitmask = 1<<0;
	unsigned long i2c9_bitmask = 1<<8;
#endif

	if (bus == 0)
		set_1_to_i2c_pin_control_reg0(i2c0_bitmask);

	if (bus == 1)
		set_1_to_i2c_pin_control_reg0(i2c1_bitmask);

	if (bus == 2)
		set_1_to_i2c_pin_control_reg0(i2c2_bitmask);

	if (bus == 3)
		set_1_to_i2c_pin_control_reg0(i2c3_bitmask);
	
	if (bus == 4)
		set_1_to_i2c_pin_control_reg1(i2c4_bitmask);

	if (bus == 5)
		set_1_to_i2c_pin_control_reg1(i2c5_bitmask);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
        if (bus == 6)
                set_1_to_i2c_pin_control_reg2(i2c6_bitmask);

        if (bus == 7)
                set_1_to_i2c_pin_control_reg2(i2c7_bitmask);
#endif
#if defined SOC_PILOT_IV
        if (bus == 8)
                set_1_to_i2c_pin_control_reg3(i2c8_bitmask);

        if (bus == 9)
                set_1_to_i2c_pin_control_reg3(i2c9_bitmask);
#endif
}

void drive_data_low (int bus)
{
	
	unsigned long i2c0_bitmask = 1<<5;
	unsigned long i2c1_bitmask = 1<<13;
	unsigned long i2c2_bitmask = 1<<21;
	unsigned long i2c3_bitmask = 1<<29;
	unsigned long i2c4_bitmask = 1<<5;
	unsigned long i2c5_bitmask = 1<<13;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
        unsigned long i2c6_bitmask = 1<<5;
        unsigned long i2c7_bitmask = 1<<13;
#endif
#if defined SOC_PILOT_IV
        unsigned long i2c8_bitmask = 1<<5;
        unsigned long i2c9_bitmask = 1<<13;
#endif
        
	if (bus == 0)
		set_1_to_i2c_pin_control_reg0(i2c0_bitmask);

	if (bus == 1)
		set_1_to_i2c_pin_control_reg0(i2c1_bitmask);

	if (bus == 2)
		set_1_to_i2c_pin_control_reg0(i2c2_bitmask);

	if (bus == 3)
		set_1_to_i2c_pin_control_reg0(i2c3_bitmask);
	
	if (bus == 4)
		set_1_to_i2c_pin_control_reg1(i2c4_bitmask);

	if (bus == 5)
		set_1_to_i2c_pin_control_reg1(i2c5_bitmask);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if (bus == 6)
		set_1_to_i2c_pin_control_reg2(i2c6_bitmask);

	if (bus == 7)
		set_1_to_i2c_pin_control_reg2(i2c7_bitmask);
#endif
#if defined SOC_PILOT_IV
	if (bus == 8)
		set_1_to_i2c_pin_control_reg3(i2c8_bitmask);

	if (bus == 9)
		set_1_to_i2c_pin_control_reg3(i2c9_bitmask);
#endif
}

void drive_data_high (int bus)
{
	
	unsigned long i2c0_bitmask = ~(unsigned long)(1<<5);
	unsigned long i2c1_bitmask = ~(unsigned long)(1<<13);
	unsigned long i2c2_bitmask = ~(unsigned long)(1<<21);
	unsigned long i2c3_bitmask = ~(unsigned long)(1<<29);
	unsigned long i2c4_bitmask = ~(unsigned long)(1<<5);
	unsigned long i2c5_bitmask = ~(unsigned long)(1<<13);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	unsigned long i2c6_bitmask = ~(unsigned long)(1<<5);
	unsigned long i2c7_bitmask = ~(unsigned long)(1<<13);
#endif
#if defined SOC_PILOT_IV
	unsigned long i2c8_bitmask = ~(unsigned long)(1<<5);
	unsigned long i2c9_bitmask = ~(unsigned long)(1<<13);
#endif

	if (bus == 0)
		set_0_to_i2c_pin_control_reg0(i2c0_bitmask);

	if (bus == 1)
		set_0_to_i2c_pin_control_reg0(i2c1_bitmask);

	if (bus == 2)
		set_0_to_i2c_pin_control_reg0(i2c2_bitmask);

	if (bus == 3)
		set_0_to_i2c_pin_control_reg0(i2c3_bitmask);
	
	if (bus == 4)
		set_0_to_i2c_pin_control_reg1(i2c4_bitmask);

	if (bus == 5)
		set_0_to_i2c_pin_control_reg1(i2c5_bitmask);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if (bus == 6)
		set_0_to_i2c_pin_control_reg2(i2c6_bitmask);

	if (bus == 7)
		set_0_to_i2c_pin_control_reg2(i2c7_bitmask);
#endif
#if defined SOC_PILOT_IV
	if (bus == 8)
		set_0_to_i2c_pin_control_reg3(i2c8_bitmask);

	if (bus == 9)
		set_0_to_i2c_pin_control_reg3(i2c9_bitmask);
#endif
}

void drive_clock_low (int bus)
{
	
	unsigned long i2c0_bitmask = 1<<1;
	unsigned long i2c1_bitmask = 1<<9;
	unsigned long i2c2_bitmask = 1<<17;
	unsigned long i2c3_bitmask = 1<<25;
	unsigned long i2c4_bitmask = 1<<1;
	unsigned long i2c5_bitmask = 1<<9;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	unsigned long i2c6_bitmask = 1<<1;
	unsigned long i2c7_bitmask = 1<<9;
#endif
#if defined SOC_PILOT_IV
	unsigned long i2c8_bitmask = 1<<1;
	unsigned long i2c9_bitmask = 1<<9;
#endif
        
	if (bus == 0)
		set_1_to_i2c_pin_control_reg0(i2c0_bitmask);

	if (bus == 1)
		set_1_to_i2c_pin_control_reg0(i2c1_bitmask);

	if (bus == 2)
		set_1_to_i2c_pin_control_reg0(i2c2_bitmask);

	if (bus == 3)
		set_1_to_i2c_pin_control_reg0(i2c3_bitmask);
	
	if (bus == 4)
		set_1_to_i2c_pin_control_reg1(i2c4_bitmask);

	if (bus == 5)
		set_1_to_i2c_pin_control_reg1(i2c5_bitmask);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if (bus == 6)
		set_1_to_i2c_pin_control_reg2(i2c6_bitmask);
	
	if (bus == 7)
		set_1_to_i2c_pin_control_reg2(i2c7_bitmask);
#endif
#if defined SOC_PILOT_IV
	if (bus == 8)
		set_1_to_i2c_pin_control_reg3(i2c8_bitmask);
	
	if (bus == 9)
		set_1_to_i2c_pin_control_reg3(i2c9_bitmask);
#endif
}

void drive_clock_high (int bus)
{
	
	unsigned long i2c0_bitmask = ~(unsigned long)(1<<1);
	unsigned long i2c1_bitmask = ~(unsigned long)(1<<9);
	unsigned long i2c2_bitmask = ~(unsigned long)(1<<17);
	unsigned long i2c3_bitmask = ~(unsigned long)(1<<25);
	unsigned long i2c4_bitmask = ~(unsigned long)(1<<1);
	unsigned long i2c5_bitmask = ~(unsigned long)(1<<9);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	unsigned long i2c6_bitmask = ~(unsigned long)(1<<1);
	unsigned long i2c7_bitmask = ~(unsigned long)(1<<9);
#endif
#if defined SOC_PILOT_IV
	unsigned long i2c8_bitmask = ~(unsigned long)(1<<1);
	unsigned long i2c9_bitmask = ~(unsigned long)(1<<9);
#endif
        
	if (bus == 0)
		set_0_to_i2c_pin_control_reg0(i2c0_bitmask);

	if (bus == 1)
		set_0_to_i2c_pin_control_reg0(i2c1_bitmask);

	if (bus == 2)
		set_0_to_i2c_pin_control_reg0(i2c2_bitmask);

	if (bus == 3)
		set_0_to_i2c_pin_control_reg0(i2c3_bitmask);
	
	if (bus == 4)
		set_0_to_i2c_pin_control_reg1(i2c4_bitmask);

	if (bus == 5)
		set_0_to_i2c_pin_control_reg1(i2c5_bitmask);

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if (bus == 6)
		set_0_to_i2c_pin_control_reg2(i2c6_bitmask);
	
	if (bus == 7)
		set_0_to_i2c_pin_control_reg2(i2c7_bitmask);
#endif
#if defined SOC_PILOT_IV
	if (bus == 8)
		set_0_to_i2c_pin_control_reg3(i2c8_bitmask);
	
	if (bus == 9)
		set_0_to_i2c_pin_control_reg3(i2c9_bitmask);
#endif
}

unsigned long get_clk_status(int bus)
{
	unsigned long retval = 0;

	unsigned long i2c0_bitmask = 1<<2;
	unsigned long i2c1_bitmask = 1<<10;
	unsigned long i2c2_bitmask = 1<<18;
	unsigned long i2c3_bitmask = 1<<26;
	unsigned long i2c4_bitmask = 1<<2;
	unsigned long i2c5_bitmask = 1<<10;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
        unsigned long i2c6_bitmask = 1<<2;
        unsigned long i2c7_bitmask = 1<<10;
#endif
#if defined SOC_PILOT_IV
        unsigned long i2c8_bitmask = 1<<2;
        unsigned long i2c9_bitmask = 1<<10;
#endif
        
	if (bus == 0)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c0_bitmask;

	if (bus == 1)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c1_bitmask;

	if (bus == 2)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c2_bitmask;

	if (bus == 3)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c3_bitmask;

	if (bus == 4)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28)& i2c4_bitmask;

	if (bus == 5)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28)& i2c5_bitmask;

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if (bus == 6)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910)& i2c6_bitmask;
	
	if (bus == 7)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910)& i2c7_bitmask;
#endif
#if  defined SOC_PILOT_IV
	if (bus == 8)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960)& i2c8_bitmask;
	
	if (bus == 9)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960)& i2c9_bitmask;
#endif
        
	return retval;
}

unsigned long get_sda_status(int bus)
{
	unsigned long retval = 0;

	unsigned long i2c0_bitmask = 1<<6;
	unsigned long i2c1_bitmask = 1<<14;
	unsigned long i2c2_bitmask = 1<<22;
	unsigned long i2c3_bitmask = 1<<30;
	unsigned long i2c4_bitmask = 1<<6;
	unsigned long i2c5_bitmask = 1<<14;
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	unsigned long i2c6_bitmask = 1<<6;
	unsigned long i2c7_bitmask = 1<<14;
#endif
#if defined SOC_PILOT_IV
	unsigned long i2c8_bitmask = 1<<6;
	unsigned long i2c9_bitmask = 1<<14;
#endif
        
	if (bus == 0)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c0_bitmask;

	if (bus == 1)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c1_bitmask;

	if (bus == 2)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c2_bitmask;

	if (bus == 3)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x24)& i2c3_bitmask;

	if (bus == 4)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28)& i2c4_bitmask;

	if (bus == 5)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x28)& i2c5_bitmask;

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	if (bus == 6)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910)& i2c6_bitmask;
	
	if (bus == 7)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x910)& i2c7_bitmask;
#endif
#if defined SOC_PILOT_IV
	if (bus == 8)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960)& i2c8_bitmask;
	
	if (bus == 9)
		retval = *(unsigned long *)(SE_SYS_CLK_VA_BASE + 0x960)& i2c9_bitmask;
#endif

	return retval;
}

static int perform_slave_recovery(int bus, int force_stop) /* Unconditionally force a STOP when enabled */
{
	int pulse_period = 5;
	int pulse_num;

	printk(KERN_DEBUG "%s() bus %d, is_generate_clock_pulses_enabled %u\n",
		__FUNCTION__, bus, m_bus_recovery_info[bus].is_generate_clock_pulses_enabled);

	pulse_period = (int) (500000/(m_bus_recovery_info[bus].frequency));

	/*see if Generate Clock pulses option is enabled */		
	if (m_bus_recovery_info[bus].is_generate_clock_pulses_enabled)
	{
		//printk("I2C%d: Slave recovery-Generate Clock Pulses...\n",bus);

		pulse_num= m_bus_recovery_info[bus].num_clock_pulses;
		//while (m_bus_recovery_info[bus].num_clock_pulses > 0) 
		while (pulse_num > 0) 

		{
			//m_bus_recovery_info[bus].num_clock_pulses -= 1;
			pulse_num -= 1;

			/* make SCL low */ 
			drive_clock_low (bus);

			udelay (pulse_period); 

			/* make SCL high */
			drive_clock_high (bus);

			udelay (pulse_period);
			if (0 != (get_sda_status(bus))) 
			{
				//printk("I2C%d: Slave recovery-Bus Recovered\n",bus);
				break;
			}
		}
	}

	if (force_stop || (0 == get_sda_status(bus))||(0 == get_clk_status(bus)))   /* Unconditionally force a STOP */
	{
		if (m_bus_recovery_info[bus].is_force_stop_enabled)
		{
			//printk("I2C%d: Slave recovery-Creating Forced STOP Condition\n",bus);

			/* make Data Low */
			drive_data_low(bus);
			
			/* make SCL high */
			drive_clock_high (bus);

			/* make Data High */
			drive_data_high (bus);

			udelay (5);
		}
	}

	/* If any of the lines is still low, it's a failure */
	if ((0 == get_sda_status(bus))||(0 == get_clk_status(bus)))
	{
		if (0 == get_sda_status(bus))
			se_pilot_ii_data_ptr[bus].i2c_link_state = I2C_UNABLE_DRIVE_DATA_HIGH;
		if (0 == get_clk_status(bus))
			se_pilot_ii_data_ptr[bus].i2c_link_state = I2C_UNABLE_DRIVE_CLK_HIGH;
		return 1;
	}

	return 0;
}

static int perform_pilot_ii_recovery(int bus)
{
	i2c_init_internal_data(bus);
	i2c_pilot_ii_reset(bus);
	i2c_init_hardware(bus);
	//enable time out counters
	i2c_to_counter_en( bus);
	//Enable Time out interrupts
	i2c_time_out_interrupt(bus);

	if ((0 == get_sda_status(bus))||(0 == get_clk_status(bus)))
	{
		//printk("ERROR: I2C%d: Pilot-ii recovery failed\n",bus);
		return 1;
	}

	//printk("I2C%d: Pilot-ii recovery succeeded\n",bus);
	return 0;
}

int pilot_ii_i2c_bus_recovery(struct i2c_adapter *i2c_adap)
{
	return i2c_bus_recovery(i2c_adap->nr, 0);	/* conditionally force STOP */ 
}

#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
extern struct i2c_pilot_ii_data se_pilot_ii_data_ptr[];
#endif


int i2c_bus_recovery(int bus, int force_stop)  /* Unconditionally force a STOP when enabled */
{
	int SDAStatusCheckTimes;
	int CLKStatusCheckTimes;
	unsigned long SDAStatus;
	unsigned long CLKStatus;
	int bus_status = 0;

	if (m_bus_recovery_info[bus].enable_recovery == DISABLE_RECOVERY)
		return -ENOSYS;
	
	se_pilot_ii_data_ptr[bus].i2c_link_state = I2C_UNDER_TEST;
	
	if((get_sda_status(bus) == 0) && (get_clk_status(bus) == 0))
	{
		return -EIO;
	}

	/* Set Data output line so that it's not driven by bit-bang */
	drive_data_high(bus);

	/* Set Clock output line so that it's not driven by bit-bang */
	drive_clock_high(bus);

	/* Set bit bang. Disconnect ourselves from the bus */
	pilot_bit_bang_select(bus);

	SDAStatusCheckTimes = (recovery_check_time_ms[bus] != 0) ? ((recovery_check_time_ms[bus] * 1000) / 25) : 100; /* 25uS ticks */
	while (SDAStatusCheckTimes)
	{
		touch_softlockup_watchdog();
		SDAStatus = get_sda_status(bus);
		if (0 != SDAStatus) { break; }
		else SDAStatusCheckTimes--;
		udelay (25);
	}
	
	CLKStatusCheckTimes = (recovery_check_time_ms[bus] != 0) ? ((recovery_check_time_ms[bus] * 1000) / 25) : 100;   /* 25uS ticks */ 
	while (CLKStatusCheckTimes)
	{
		touch_softlockup_watchdog();
		CLKStatus = get_clk_status(bus);
		if (0 != CLKStatus) { break; }
		else CLKStatusCheckTimes--;
		udelay (25);
	}

	if (force_stop || (SDAStatusCheckTimes == 0) || (CLKStatusCheckTimes == 0)) /* Unconditionally force a STOP */
	{
		printk("I2C%d: data or clock low by another device\n",bus);
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
		se_pilot_ii_data_ptr[bus].stat.recover++;
#endif
		/* data is pulled low by a slave on the bus */
		bus_status = perform_slave_recovery(bus, force_stop);   /* Unconditionally force a STOP */
		if(bus_status != 0)
		{
			printk("ERROR: I2C%d: Slave recovery did not succeed\n",bus);
			pilot_bit_bang_deselect(bus);
			return -EIO;
		}
	}

	pilot_bit_bang_deselect(bus);

	/****** 
	 ******	Pilot-II controller sometimes pull down the signals (Known Errata). 
	 ****** So perform controller recovery (basically reset)
     ******/   

	SDAStatusCheckTimes = 100;
	while (SDAStatusCheckTimes)
	{
		touch_softlockup_watchdog();
		SDAStatus = get_sda_status(bus);
		if (0 != SDAStatus) { break; }
		else SDAStatusCheckTimes--;
		udelay (25);
	}
	
	CLKStatusCheckTimes = 100;
	while (CLKStatusCheckTimes)
	{
		touch_softlockup_watchdog();
		CLKStatus = get_clk_status(bus);
		if (0 != CLKStatus) { break; }
		else CLKStatusCheckTimes--;
		udelay (25);
	}

	if ((SDAStatusCheckTimes == 0) || (CLKStatusCheckTimes == 0))
	{
		printk("I2C%d: Recover Pilot-II: Data or Clock is stuck by Pilot-II\n",bus);
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
		se_pilot_ii_data_ptr[bus].stat.recover++;
#endif
		bus_status = perform_pilot_ii_recovery(bus);
		if(bus_status != 0)
		{
			printk("ERROR: I2C%d: Pilot-ii recovery did not succeed\n",bus);
			return -EIO;
		}
	}
	else
	{
		se_pilot_ii_data_ptr[bus].i2c_link_state = I2C_LINK_NO_FAILURE;
	}

	return 0;
}

int pilot_ii_set_recovery_info (int bus, bus_recovery_info_T* info)
{
	if (bus >= BUS_COUNT) 
		return -1;

	memcpy (&m_bus_recovery_info [bus], info, sizeof (bus_recovery_info_T));
	// Enable Slave when the bus is a ipmb bus.
	if (info->enable_recovery == DISABLE_RECOVERY) { /* Enable clock stretch interrupts for IPMB's */
		i2c_pilot_ii_enable_slave(bus);
		i2c_prog_time_out_values(bus, 1);
	}
	
	return 0;	
}


int pilot_ii_get_recovery_info (int bus, bus_recovery_info_T* info)
{
	if (bus >= BUS_COUNT) 
		return -1;

	memcpy (info, &m_bus_recovery_info [bus], sizeof (bus_recovery_info_T));
	return 0;	
}

int i2c_is_bus_hung(int bus)			//JM_RECOVERY
{
	int StatusCheckTimes;
	unsigned long Status;

	//printk("I2C%d: checking for bus hang...",bus);

	/* Check if data line stuck low for 2.5ms */
	StatusCheckTimes = 100;
	while (StatusCheckTimes)
	{
		touch_softlockup_watchdog();
		//Status = (i2c_as_read_reg( bus,I2C_CMD_STATUS_REG) & SAMPLE_SDA_LINE) ;
		Status = get_sda_status(bus);
		if (0 != Status) 
		{
			se_pilot_ii_data_ptr[bus].i2c_link_state =  I2C_LINK_NO_FAILURE;
			break; 
		}
		else 
		{
			
			StatusCheckTimes--;
		}
		udelay (25);
	}
	if (StatusCheckTimes == 0)
	{
		//printk("data stuck low\n");
		
		se_pilot_ii_data_ptr[bus].i2c_link_state = I2C_UNABLE_DRIVE_DATA_HIGH;
		return 1; /* data stuck low */
	}
	
	/* Check if clock line stuck low for 2.5ms */
	StatusCheckTimes = 100;
	while (StatusCheckTimes)
	{
		touch_softlockup_watchdog();
		//Status = (i2c_as_read_reg( bus,I2C_CMD_STATUS_REG) & SAMPLE_SCL_LINE);
		Status = get_clk_status(bus);
		if (0 != Status) 
		{
			se_pilot_ii_data_ptr[bus].i2c_link_state =  I2C_LINK_NO_FAILURE;
			break;
		}
		else 
		{
			StatusCheckTimes--;
		}
		udelay (25);
	}
	if (StatusCheckTimes == 0)
	{
		//printk("clock stuck low");
		se_pilot_ii_data_ptr[bus].i2c_link_state = I2C_UNABLE_DRIVE_CLK_HIGH;
		return 1; /* clock stuck low */
	}
	return 0;
}

int
i2c_bus_test (int bus, i2c_bus_test_T* info)
{
	
	i2c_bus_test_T *i2cTestData = info;

	/* Initilaize the result to zero. (Bus free)*/
	i2cTestData->result = I2C_LINK_NO_FAILURE;
	
	if ((i2cTestData->operation  & I2C_TEST_BUS_BUSY) == I2C_TEST_BUS_BUSY )
	{
		if ( i2c_is_bus_hung( bus) == 0 )
		{
			/*Bus is free */
			i2cTestData->result =( i2cTestData->result  &  (~I2C_TEST_BUS_BUSY));
		}
		else 
		{
			/*Bus hung*/
			i2cTestData->result = i2cTestData->result  |I2C_TEST_BUS_BUSY;
		}
	}

	if ( ( i2cTestData->operation  & I2C_TEST_BUS_RECOVERABLE )  ==  I2C_TEST_BUS_RECOVERABLE )
	{
		if ( i2c_bus_recovery( bus , 0) == 0 )  /* Conditionally force a STOP */
		{
			i2cTestData->result =( i2cTestData->result  &  (~I2C_TEST_BUS_RECOVERABLE));
		}
		else 
		{
			i2cTestData->result = i2cTestData->result  |I2C_TEST_BUS_RECOVERABLE;
		}
	}
 
	return 0;	
}


/* Set I2C bus recovery stuck bus timeout (in milliseconds */ 
int set_i2c_bus_recovery_check_time(int bus, int time_ms)
{
    if (bus < BUS_COUNT)
    {
        recovery_check_time_ms[bus] = time_ms;
        return 0;
    }
    return -1;
}
