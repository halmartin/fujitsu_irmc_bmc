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
extern int pilot_ii_get_recovery_info (int bus, bus_recovery_info_T* info);

void i2c_pilot_slave_xfer_enable(int bus);
#ifdef SOC_PILOT_IV
int fifo_depth[ BUS_COUNT ] =
{
#if (BUS_COUNT>=1)
   	36, 
#endif
#if (BUS_COUNT>=2)
	36, 
#endif
#if (BUS_COUNT>=3)
	36, 
#endif
#if (BUS_COUNT>=4)
	36, 
#endif
#if (BUS_COUNT>=5)
	36, 
#endif
#if (BUS_COUNT>=6)
	128,
#endif
#if (BUS_COUNT>=7)
	128,
#endif
#if (BUS_COUNT>=8)
	128,
#endif
#if (BUS_COUNT>=9)
	128,
#endif
#if (BUS_COUNT>=10)
	36
#endif

};
#else
int fifo_depth[ BUS_COUNT ] =
{
#if (BUS_COUNT>=1)
   	36, 
#endif
#if (BUS_COUNT>=2)
	36, 
#endif
#if (BUS_COUNT>=3)
	36, 
#endif
#if (BUS_COUNT>=4)
	36, 
#endif
#if (BUS_COUNT>=5)
	36, 
#endif
#if (BUS_COUNT>=6)
	128,
#endif
#if (BUS_COUNT>=7)
	36,
#endif
#if (BUS_COUNT>=8)
	36
#endif

};
#endif

/** Default values in these arrays are for the PILOT_II **/
static u32 i2c_pilot_ii_io_base[ BUS_COUNT ] =
{
#if (BUS_COUNT>=1)
  SE_I2C_0_VA_BASE, 
#endif
#if (BUS_COUNT>=2)
  SE_I2C_1_VA_BASE,
#endif
#if (BUS_COUNT>=3)
  SE_I2C_2_VA_BASE,
#endif
#if (BUS_COUNT>=4)
  SE_I2C_3_VA_BASE, 
#endif
#if (BUS_COUNT>=5)
  SE_I2C_4_VA_BASE, 
#endif
#if (BUS_COUNT>=6)
  SE_I2C_5_VA_BASE,
#endif
#if (BUS_COUNT>=7)
  SE_I2C_6_VA_BASE,
#endif
#if (BUS_COUNT>=8)
  SE_I2C_7_VA_BASE,
#endif
#if (BUS_COUNT>=9)
  SE_I2C_8_VA_BASE,
#endif
#if (BUS_COUNT>=10)
  SE_I2C_9_VA_BASE
#endif
};

static int slave_addr[ BUS_COUNT ] =
  {
#if (BUS_COUNT>=1)
    DEFAULT_SLAVE_0,
#endif
#if (BUS_COUNT>=2)
    DEFAULT_SLAVE_1, 
#endif
#if (BUS_COUNT>=3)
    DEFAULT_SLAVE_2, 
#endif
#if (BUS_COUNT>=4)
    DEFAULT_SLAVE_3,
#endif
#if (BUS_COUNT>=5)
    DEFAULT_SLAVE_4, 
#endif
#if (BUS_COUNT>=6)
    DEFAULT_SLAVE_5,
#endif
    
#if (BUS_COUNT>=7)
    DEFAULT_SLAVE_6,
#endif
    
#if (BUS_COUNT>=8)
    DEFAULT_SLAVE_7,
#endif

#if (BUS_COUNT>=9)
    DEFAULT_SLAVE_8,
#endif

#if (BUS_COUNT>=10)
    DEFAULT_SLAVE_9
#endif
  };

static int slave_enabled[ BUS_COUNT ] = { 0 };	/* Record which slave interfaces are being enabled */

static int busbusy_timeout[ BUS_COUNT ] = { 0 };/* Support overwrite of "wait for bus free" bus busy timeout */


u32 i2c_pilot_ii_read_reg( int bus, u32 offset )
{
    //return( ioread16( i2c_pilot_ii_io_base[ bus ] + offset ) );
    return( (*(volatile short *)( i2c_pilot_ii_io_base[ bus ] + offset ))&(0xffff) );
}

void i2c_pilot_ii_write_reg( int bus, u32 value, u32 offset )
{
    //iowrite16( value, i2c_pilot_ii_io_base[ bus ] + offset );
    *(volatile short *)(i2c_pilot_ii_io_base[ bus ] + offset )=value&0xffff;
}

void i2c_pilot_ii_disable_all_interrupts(int bus)
{
	i2c_pilot_ii_write_reg(bus,0,I2C_INTR_MASK_REG);
}


void i2c_enable_bus(int bus)
{
  unsigned short temp=0;
  /* Enable I2C interface */
  temp=i2c_pilot_ii_read_reg(bus,I2C_ENABLE_REG);
  i2c_pilot_ii_write_reg(bus,(temp|1),I2C_ENABLE_REG);

}

void i2c_disable_bus(int bus)
{
  unsigned short temp=0;
  /* Enable I2C interface */
  temp=i2c_pilot_ii_read_reg(bus,I2C_ENABLE_REG);
  temp&=0xfffe;
  i2c_pilot_ii_write_reg(bus,(temp),I2C_ENABLE_REG);
}





void i2c_pilot_ii_disable_interrupt(int bus, unsigned long mask)
{
  unsigned long current_mask;
  
  current_mask = i2c_pilot_ii_read_reg(bus,I2C_INTR_MASK_REG);
  i2c_pilot_ii_write_reg(bus,(current_mask & ~mask),I2C_INTR_MASK_REG);
  
  return;
}

void i2c_pilot_ii_enable_interrupt(int bus, unsigned long mask)
{
  unsigned long current_mask;
  
  current_mask = i2c_pilot_ii_read_reg(bus,I2C_INTR_MASK_REG);
  i2c_pilot_ii_write_reg(bus,current_mask|mask,I2C_INTR_MASK_REG);
  return;
}

static void 
reset_i2c_pin_control_reg0 (unsigned long bitmask )
{
  volatile unsigned long i2c_pin_ctrl0;
  unsigned int i=0;
  
  i2c_pin_ctrl0 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0);
  
  /* I2C Module Reset, TAR Update without disabling the I2C module */
  i2c_pin_ctrl0 |= bitmask;
  
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0 )= i2c_pin_ctrl0 ;
  for(i=0;i<0x20;i++);
  
  /* Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine */
  i2c_pin_ctrl0 &= ~bitmask;
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0 )= i2c_pin_ctrl0;
}

static void 
reset_i2c_pin_control_reg1 (unsigned long bitmask )
{
  volatile unsigned long i2c_pin_ctrl1;
  unsigned int i=0;
  
  i2c_pin_ctrl1 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT1);
  
  /* I2C Module Reset, TAR Update without disabling the I2C module */
  i2c_pin_ctrl1 |= bitmask;
  
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT1 )= i2c_pin_ctrl1;
  for(i=0;i<0x20;i++);
  
  /* Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine */
  i2c_pin_ctrl1 &= ~bitmask;
  * (unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT1 )= i2c_pin_ctrl1;
}

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
static void 
reset_i2c_pin_control_reg2 (unsigned long bitmask )
{
  volatile unsigned long i2c_pin_ctrl2;
  unsigned int i=0;
  
  i2c_pin_ctrl2 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT2);
  
  /* I2C Module Reset, TAR Update without disabling the I2C module */
  i2c_pin_ctrl2 |= bitmask;
  
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT2 )= i2c_pin_ctrl2;
  for(i=0;i<0x20;i++);
  
  /* Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine */
  i2c_pin_ctrl2 &= ~bitmask;
  * (unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT2 )= i2c_pin_ctrl2;
}
#endif

#if defined SOC_PILOT_IV
static void
reset_i2c_pin_control_reg3 (unsigned long bitmask )
{
  volatile unsigned long i2c_pin_ctrl3;
  unsigned int i=0;

  i2c_pin_ctrl3 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT3);

  /* I2C Module Reset, TAR Update without disabling the I2C module */
  i2c_pin_ctrl3 |= bitmask;

  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT3 )= i2c_pin_ctrl3;
  for(i=0;i<0x20;i++);

  /* Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine */
  i2c_pin_ctrl3 &= ~bitmask;
  * (unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT3 )= i2c_pin_ctrl3;
}
#endif
void i2c_pilot_ii_reset(int bus)
{

  unsigned long i2c0_bitmask= (1<<7);
  unsigned long i2c1_bitmask= (1<<15);
  unsigned long i2c2_bitmask= (1<<23);
  unsigned long i2c3_bitmask= (1<<31);
  unsigned long i2c4_bitmask= (1<<7);
  unsigned long i2c5_bitmask= (1<<15);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
  unsigned long i2c6_bitmask= (1<<7);
  unsigned long i2c7_bitmask= (1<<15);
#endif
#if defined SOC_PILOT_IV
  unsigned long i2c8_bitmask= (1<<7);
  unsigned long i2c9_bitmask= (1<<15);
#endif
  
  /* Hit the reset bit */
  if ( bus == 0)
    reset_i2c_pin_control_reg0(i2c0_bitmask); 
  if ( bus == 1)
    reset_i2c_pin_control_reg0(i2c1_bitmask);
  if ( bus == 2)
    reset_i2c_pin_control_reg0(i2c2_bitmask);
  if ( bus == 3)
    reset_i2c_pin_control_reg0(i2c3_bitmask);
  if ( bus == 4)
    reset_i2c_pin_control_reg1(i2c4_bitmask);
  if ( bus == 5)
    reset_i2c_pin_control_reg1(i2c5_bitmask);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
  if ( bus == 6)
    reset_i2c_pin_control_reg2(i2c6_bitmask);
  if ( bus == 7)
    reset_i2c_pin_control_reg2(i2c7_bitmask);
#endif
#if defined SOC_PILOT_IV
  if ( bus == 8)
    reset_i2c_pin_control_reg3(i2c8_bitmask);
  if ( bus == 9)
    reset_i2c_pin_control_reg3(i2c9_bitmask);
#endif

}


static void 
update_i2c_i2cpct_reg0 (int bus)
{
  volatile unsigned long i2c_pin_ctrl0;
  unsigned long OR_bitmask = (1 << (4 + (bus*8)));
  i2c_pin_ctrl0 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0);
  
  /* Set the TAR bit */
  i2c_pin_ctrl0 |= OR_bitmask;
  
  /* Clear bit-bang */
  i2c_pin_ctrl0 &= ~(1 << (bus*8));
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0)= i2c_pin_ctrl0;
}

//used for bus 4,5 ,but argument bus should be 0,1
static void 
update_i2c_i2cpct_reg1 (int bus)
{
  volatile unsigned long i2c_pin_ctrl1;
  unsigned long OR_bitmask = (1 << (4 + (bus*8)));
  
  i2c_pin_ctrl1 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT1);
  
  /* Set the TAR bit */
  i2c_pin_ctrl1 |= OR_bitmask;
  
  /* Clear bit-bang */
  i2c_pin_ctrl1 &= ~(1 << (bus*8));
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT1 )= i2c_pin_ctrl1;
}

//used for bus 6,7 ,but argument bus should be 0,1
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
static void
update_i2c_i2cpct_reg2(int bus)
{
  volatile unsigned long i2c_pin_ctrl2;
  unsigned long OR_bitmask = (1 << (4 + (bus*8)));
  
  i2c_pin_ctrl2 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT2);
  
  /* Set the TAR bit */
  i2c_pin_ctrl2 |= OR_bitmask;
  
  /* Clear bit-bang */
  i2c_pin_ctrl2 &= ~(1 << (bus*8));
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT2 )= i2c_pin_ctrl2;
}
#endif
#if defined SOC_PILOT_IV
static void
update_i2c_i2cpct_reg3(int bus)
{
  volatile unsigned long i2c_pin_ctrl3;
  unsigned long OR_bitmask = (1 << (4 + (bus*8)));

  i2c_pin_ctrl3 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT3);

  /* Set the TAR bit */
  i2c_pin_ctrl3 |= OR_bitmask;

  /* Clear bit-bang */
  i2c_pin_ctrl3 &= ~(1 << (bus*8));
  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT3 )= i2c_pin_ctrl3;
}
#endif

static void i2c_pilot_ii_init_i2cpct(int bus)
{
  if(bus < 4)
    update_i2c_i2cpct_reg0(bus);
  else if( (bus>=4) && (bus<=5) )
    update_i2c_i2cpct_reg1(bus-4);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
  else if( (bus>=6) && (bus<=7) )
    update_i2c_i2cpct_reg2(bus-6);
#endif
#if defined SOC_PILOT_IV
  else if( (bus>=8) && (bus<=9) )
    update_i2c_i2cpct_reg3(bus-8);
#endif

}

void i2c_init_bus(int bus)
{
  u32 controlbits;	/* IC_CON is a 32 bit register */
  bus_recovery_info_T bus_info;

  //clear to interrupt status
  i2c_disable_time_out_interrupt(bus);
  
  //disable time out counters
  i2c_disable_to_counters( bus);
  
  
  //programme time out vlaues
  //    i2c_prog_time_out_values(bus,0);
  
  //enable time out counters
  // i2c_to_counter_en( bus);
  
  //Enable Time out interrupts
  // i2c_time_out_interrupt(bus);
  
  switch(bus)
    {
    case 0:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C0_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 1:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C1_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 2:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C2_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 3:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C3_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 4:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C4_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 5:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C5_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 6:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C6_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 7:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C7_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 8:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C8_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
      
    case 9:
      *(volatile char *)(SE_SYS_CLK_VA_BASE+I2C9_P3_FEATURE_EN)=EN_P3_FEATURES;
      break;
    default:
      break;
    }

#ifndef SOC_PILOT_IV	/* not supported in Pilot 4 (per Pilot 4 Specification v0.23) */
  *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_SMB_CTRL_0) = 0xFF;
  *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_SMB_CTRL_1) = 0x1EF7BDEF;
  *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_SMB_CTRL_2) |= 0x1EF;
#endif
  
  /* Disable all interrupts on the bus */
  i2c_pilot_ii_disable_all_interrupts(bus);
  
  /* Init I2CPCT registers for TAR bit and I2C mode */
    i2c_pilot_ii_init_i2cpct(bus);
  
  /*Disable the I2C interface first to write control register */
  i2c_disable_bus(bus);

  
  
  /* Set the slave address again */
  i2c_pilot_ii_write_reg( bus, slave_addr[bus], I2C_SAR_REG );
  
  /* Set the TX fifo threshold level for interrupt generation */
  i2c_pilot_ii_write_reg (bus, DEFAULT_TX_THRESHOLD,I2C_TX_TL_REG);
  
  /* Set the RX fifo threshold level for interrupt generation */
  i2c_pilot_ii_write_reg (bus, DEFAULT_RX_THRESHOLD,I2C_RX_TL_REG);
  
  
  /* Enable restart when acting as a master */
  controlbits = 0x00 | IC_RESTART_EN;
  pilot_ii_get_recovery_info(bus, &bus_info);
  // Enable Slave when the bus is a ipmb bus.
  /* Enable Slave interface for IPMB ports or multi-master ports enabled from user space */
  if ((slave_enabled[bus] == 0) && (bus_info.enable_recovery == ENABLE_RECOVERY))
      controlbits |= IC_SLAVE_DISABLE;
  
  /* Configure as master */
  controlbits |= MASTER_MODE;
  
#ifdef SOC_PILOT_IV	/* Clock stretching is enabled though the IC_CON register on Pilot 4 */
  controlbits |= RXFIFO_CLK_STRETCH | STOP_DET_FILTER;
#endif

  /* Configure as 100Kbit speed */
  //controlbits |= SPEED_100KB;
  /* Configure as 7 bit  addressing  for master and slave */
 // controlbits |= 0;
  
  /* Set Speed,Addressing,Master */
  i2c_pilot_ii_write_reg( bus,controlbits , I2C_CON_REG );
  
  /* Set the i2c bus freq to 100kHz */
  //i2c_pilot_ii_write_reg(bus, 0xEA, I2C_SS_SCL_HCNT_REG );
  //i2c_pilot_ii_write_reg(bus, 0xFA, I2C_SS_SCL_LCNT_REG );
  
  set_speed(bus,se_pilot_ii_data_ptr[bus].bus_speed);
  
  i2c_pilot_ii_enable_interrupt(bus, STOP_DET|TX_ABRT|RX_OVER|RX_FULL|RD_REQ);
  
  /* Enable I2C interface */
  i2c_enable_bus(bus);

}

void i2c_init_hardware(int bus)
{
  //do s/w reset
  i2c_pilot_ii_reset( bus);
  
  i2c_init_bus(bus); 
  
  if (se_pilot_ii_data_ptr[bus].SlaveTX_Enable == 1)
	i2c_pilot_slave_xfer_enable(bus);

}


int i2c_wait_for_bus_free(int bus)
{
  int	retries = DEFAULT_BB_RETRIES*DEFAULT_BB_RETRIES;
  
  i2c_pilot_ii_read_reg(bus, I2C_CLR_ACTIVITY_REG);
  
  while ( (i2c_pilot_ii_read_reg(bus, I2C_STATUS_REG ) & IC_STATUS_ACTIVITY ) && (--retries > 0))
    {
      i2c_pilot_ii_read_reg(bus, I2C_CLR_ACTIVITY_REG);
      mdelay (1);
    }
  if(retries == 0)
    {
      //printk("I2C%d:I2C bus busy\n",bus);
      return 1;
    }
  
  return 0;
}

/* Extended wait for bus free function */
int i2c_wait_for_bus_free_ext(int bus)
{
  int	retries = DEFAULT_BB_RETRIES*DEFAULT_BB_RETRIES;
  int	fastPoll = 0;	/* poll for bus busy at faster (500us) interval */
  
  /* check for reconfigured bus busy timeout */
  if ((bus < BUS_COUNT) && (busbusy_timeout[bus] != 0))
  {
    retries = busbusy_timeout[bus]; /* Timeout in 1ms retry intervals */
    retries *= 2;	/* double retries for faster polled interval */
    fastPoll = 1;
  }
  
  i2c_pilot_ii_read_reg(bus, I2C_CLR_ACTIVITY_REG);
  
  while ( ((i2c_pilot_ii_read_reg(bus, I2C_STATUS_REG ) & (IC_STATUS_ACTIVITY | RFNE)) ||	/* Include check for RX FIFO not empty */
		  (((Get_Clk_Streaching_info(bus) & 0x01) != 0) || (i2c_is_bus_idle(bus) == 0))) &&
		  (retries -- > 0))
    {
      i2c_pilot_ii_read_reg(bus, I2C_CLR_ACTIVITY_REG);
      (fastPoll) ? udelay(500) : mdelay (1);
    }

  if(retries < 0)
    {
      //printk("I2C%d:I2C bus busy\n",bus);
      return 1;
    }
  
  return 0;
}

int i2c_pilot_ii_disable_slave(int bus)
{
  u32 ctrl_bits;
  unsigned long flags;
  
  slave_enabled [ bus ] = 0;	/* Save the slave is disabled for future use */

  // Check if slave is already disabled to avoid an unnecessary wait
  ctrl_bits = i2c_pilot_ii_read_reg(bus,I2C_CON_REG);
  if (ctrl_bits & IC_SLAVE_DISABLE) {
	  return 0;
  }

  if(i2c_wait_for_bus_free(bus) != 0)
    {
      printk("ERROR: I2C%d: Disable slave failed due to bus-busy\n",bus);
      return 1;
    }
  
  local_save_flags(flags);
  local_irq_disable();
  
  ctrl_bits = i2c_pilot_ii_read_reg(bus,I2C_CON_REG);
  if (!(ctrl_bits & IC_SLAVE_DISABLE))
    {
      i2c_disable_bus(bus);
      ctrl_bits |= IC_SLAVE_DISABLE;
      i2c_pilot_ii_write_reg( bus,ctrl_bits,I2C_CON_REG );
      i2c_enable_bus(bus);
      //      i2c_pilot_ii_write_reg( bus,1,I2C_ENABLE_REG );
    }
  
  local_irq_restore(flags);
  
  return 0;
}

int i2c_pilot_ii_enable_slave(int bus)
{
  u32 ctrl_bits;
  unsigned long flags;
  
  slave_enabled [ bus ] = 1;	/* Save the slave is enabled for future use */

  // Check if slave is already enabled to avoid an unnecessary wait */
  ctrl_bits = i2c_pilot_ii_read_reg(bus,I2C_CON_REG);
  if (!(ctrl_bits & IC_SLAVE_DISABLE))
  {
	  return 0;
  }
  if(i2c_wait_for_bus_free(bus) != 0)
    {
      printk("ERROR: I2C%d: Enable slave failed due to bus-busy\n",bus);
      return 1;
    }
  
  local_save_flags(flags);
  local_irq_disable();
  
  if(se_pilot_ii_data_ptr[bus].SlaveRX_Entries != MAX_FIFO_LEN)
    {
      se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE;
      
      ctrl_bits = i2c_pilot_ii_read_reg(bus,I2C_CON_REG);
      if (ctrl_bits & IC_SLAVE_DISABLE)
	{
	  i2c_disable_bus(bus);
	  ctrl_bits &= ~IC_SLAVE_DISABLE;
	  i2c_pilot_ii_write_reg( bus,ctrl_bits,I2C_CON_REG );
	  i2c_enable_bus(bus);
	  //	  i2c_pilot_ii_write_reg( bus,1,I2C_ENABLE_REG );
	}
    }
  local_irq_restore(flags);
  
  return 0;
}


void i2c_pilot_ii_set_slave( int bus, int slave )
{
 // unsigned int temp=0;
  /* Save the slave address for future uses */
  slave_addr[ bus ] = slave >> 1;
  
  /* Disable I2C interface first */
  i2c_disable_bus(bus);
  
  /* set slave mode */
  i2c_pilot_ii_write_reg( bus, slave >> 1 , I2C_SAR_REG );
  
  /* Enable I2C interface */
  i2c_enable_bus(bus);

  // enable slave if not yet done 
  i2c_pilot_ii_enable_slave( bus ); 	 
}

int i2c_pilot_ii_get_slave( int bus )
{
  return i2c_pilot_ii_read_reg( bus, I2C_SAR_REG );
}

int i2c_pilot_ii_get_raw_intr_sts( int bus )
{
  return i2c_pilot_ii_read_reg( bus, I2C_RAW_INTR_STAT_REG);
}


//Disable Time out counters
void i2c_disable_to_counters(int bus)
{
  /* support I2C ports 8 and 9 */
  if ((bus >= 0) && (bus <= 7))
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_EN)&=(~(I2C_TO_BITS<<(bus*4)));
  else if ((bus == 8) || (bus == 9))
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_EN_1)&=(~(I2C_TO_BITS<<((bus-8)*4)));
}	


//Enable Time out counters,h/w will set corresponding status bits 
//if i2c bus is held low for long time.
void i2c_to_counter_en(int bus)
{
  /* support I2C ports 8 and 9 */
  if ((bus >= 0) && (bus <= 7))
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_EN)|=(I2C_TO_BITS<<(bus*4));
  else if ((bus == 8) || (bus == 9))
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_EN_1)|=(I2C_TO_BITS<<((bus-8)*4));
}	

//Enable Time out interrupts,if i2c bus is held low for more than programmed 
//time out time then interrupt will be generated for s/w to do bus reset.
void i2c_time_out_interrupt(int bus)
{
  bus_recovery_info_T bus_info;
  
  if ((pilot_ii_get_recovery_info(bus, &bus_info) != 0) || (bus_info.enable_recovery == ENABLE_RECOVERY)) {
    return;
  }
  /* support I2C ports 8 and 9 */
  if ((bus >= 0) && (bus <= 7))
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_EN)|=(I2C_TO_BITS<< (bus*4) );
  else if ((bus == 8) || (bus == 9))
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_EN_1)|=(I2C_TO_BITS<< ((bus-8)*4));
}


//Clear Intr Sts and Disbale Time out interrupts
void i2c_disable_time_out_interrupt(int bus)
{
  /* support I2C ports 8 and 9 */
  if ((bus >= 0) && (bus <= 7))
  {
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (I2C_TO_BITS<<(bus*4)) ;
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_EN)&=(~(I2C_TO_BITS<<(bus*4)));
  }
  else if ((bus == 8) || (bus == 9))
  {
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (I2C_TO_BITS<<((bus-8)*4)) ;
    *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_EN_1)&=(~(I2C_TO_BITS<<((bus-8)*4)));
  }
}

//Default values are compatible with SMBus,except that Ttime,
//Ttime out for IPMB is 3msec were as for SMBUs is 35msec
void i2c_prog_time_out_values(int bus,int mode)
{
  int Ttimeout=0x0;
  
  
  if(mode==1) //smbus
    {
      Ttimeout=(4*35)+20;  //msecs
      //      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_SMB_CTRL_0)|= (1<<bus);  //enable SMBus data-hold's
    }
  else if(mode==2)  //IPMB
    {
      Ttimeout=(4*3)+4;  //msecs
    }
  else
    {
      i2c_disable_time_out_interrupt(bus);
      i2c_disable_to_counters( bus);
      return;  //I2C 
    }
  
  //clear to interrupt status and disable interrupts
  i2c_disable_time_out_interrupt(bus);
  
  switch(bus)
    {
    case 0:
      //      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C0_TO_Counter)=(0xffffffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C0_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C0_TO_Counter)|=(Ttimeout<<16);
      break;
      
    case 1:
      //  *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C1_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C1_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C1_TO_Counter)|=(Ttimeout<<16);

      break;
      
    case 2:
      //  *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C2_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C2_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C2_TO_Counter)|=(Ttimeout<<16);
      
      break;
      
    case 3:
      //  *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C3_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C3_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C3_TO_Counter)|=(Ttimeout<<16);
      
      break;
      
    case 4:
      //  *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C4_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C4_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C4_TO_Counter)|=(Ttimeout<<16);
      
      break;
      
    case 5:
      // *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C5_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C5_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C5_TO_Counter)|=(Ttimeout<<16);
      
      break;
      
    case 6:
      // *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C6_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C6_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C6_TO_Counter)|=(Ttimeout<<16);
      
      break;
      
    case 7:
      //      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C7_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C7_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C7_TO_Counter)|=(Ttimeout<<16);
      
      break;
    case 8:
      //      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C8_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C8_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C8_TO_Counter)|=(Ttimeout<<16);
      
      break;
      
    case 9:
      //      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C9_TO_Counter)=(0xffffffff);
      
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C9_TO_Counter)&=(0xff00ffff);
      *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C9_TO_Counter)|=(Ttimeout<<16);
      
      break;
    default:
      break;
    }
  //enable time out counters
  i2c_to_counter_en( bus);
  
 //Enable Time out interrupts
  i2c_time_out_interrupt(bus);

}

/*      PILOT 4 Specification:
 *      ======================
 *      SCL_High_time = (HCNT + Filter_clocks + 7)*baseCLK_time + SCL_Fall_time
 *  =>  HCNT = (SCL_High_time - SCL_Fall_time)/baseCLK_time - Filter_clocks - 7
 *  
 *      SCL_Low_time = (LCNT + 1)*baseCLK_time - SCL_Fall_time + SCL_Rise_time
 *  =>  LCNT = (SCL_Low_time - SCL_Rise_time + SCL_Fall_time)/baseCLK_time - 1
 *
 *      PILOT 3 Specification:
 *      ======================
 *      HCNT = (Desired I2C clock high time) / (Internal Serial clock period)
 *      LCNT = (Desired SCL LOW time) / (Internal Serial clock period)
 *      
 *  Based on typical loading conditions program known values as per the spec.,
 *  otherwise calculate.
 */
#define   SCL_RISE_TIME_NS      100         // Nanoseconds.
#define   SCL_FALL_TIME_NS       10         // Nanoseconds.
#define   FILTER_CLOCKS           1         // No. of filter clock cycles.

int calcHCNT( int speed_kHz, int SCL_HighTime_ns, int highSpeed_i2c )
{
  int hcnt;
  int baseCLK_time_ns = highSpeed_i2c ? 10 : 20;
#if defined SOC_PILOT_IV
  hcnt = (SCL_HighTime_ns - SCL_FALL_TIME_NS)/baseCLK_time_ns - FILTER_CLOCKS - 7;
  if     ( speed_kHz ==  100 ) hcnt = highSpeed_i2c ? 0x1C6 : 0xE3;
  else if( speed_kHz ==  400 ) hcnt = highSpeed_i2c ?  0x4C : 0x26;
  else if( speed_kHz == 3400 ) hcnt = 0x06;
#else
  if     ( speed_kHz ==  400 ) hcnt = highSpeed_i2c ?  0x4B : 0x23;
  else                         hcnt = SCL_HighTime_ns / baseCLK_time_ns;
#endif
  return hcnt;
}

// Based on typical loading conditions program known values as per the spec,
// Otherwise calculate.
int calcLCNT( int speed_kHz, int SCL_LowTime_ns, int highSpeed_i2c )
{
  int lcnt;
  int baseCLK_time_ns = highSpeed_i2c ? 10 : 20;
#if defined SOC_PILOT_IV
  lcnt = (SCL_LowTime_ns - SCL_RISE_TIME_NS + SCL_FALL_TIME_NS)/baseCLK_time_ns - 1;
  if     ( speed_kHz ==  100 ) lcnt = highSpeed_i2c ? 0x20C : 0x106;
  else if( speed_kHz ==  400 ) lcnt = highSpeed_i2c ?  0x92 :  0x49;
  else if( speed_kHz == 3400 ) lcnt = 0x10;
#else
  if      ( speed_kHz ==  400 ) lcnt = highSpeed_i2c ?  0xA2 :  0x4D;
  else                          lcnt = SCL_LowTime_ns / baseCLK_time_ns;
#endif
  return lcnt;
}

int isHighSpeedI2C( int bus )
{
#if defined SOC_PILOT_IV
  return ( bus==5 || bus==6 || bus==7 || bus==8 ) ? 1 : 0;
#else
  return ( bus==5 ) ? 1 : 0;
#endif
}

//speed in terms of Khz
int set_speed( int bus, int speed_kHz )
{
  int speed_mode, high_speed_i2c, SCL_Period_ns;
  int hcnt_val, lcnt_val;
  unsigned int lcnt_regs[] = { 0x00,                I2C_SS_SCL_LCNT_REG,
                               I2C_FS_SCL_LCNT_REG, I2C_HS_SCL_LCNT_REG };
  unsigned int hcnt_regs[] = { 0x00,                I2C_SS_SCL_HCNT_REG,
                               I2C_FS_SCL_HCNT_REG, I2C_HS_SCL_HCNT_REG };
  //
  // Calculate the speed mode.
  //
  high_speed_i2c = isHighSpeedI2C( bus );
  if( speed_kHz <= 0 )  {             // Error
    return 1;
  }
  else if( speed_kHz <= 100 )  {      // SS = standard speed.
    speed_mode = 1;
  }
  else if( speed_kHz <= 400 )  {      // FS = fast speed.
    speed_mode = 2;
  }
  else if( high_speed_i2c ) {         // HS = high speed.
    speed_mode = 3;
  } else {                            // Error
    return 1;
  }

  //
  // Calculate the timing values.
  // SCL_Period / [ns] = (1/(speed_kHz*1000))/1e-9 = 1e6/speed_kHz;
  //
  SCL_Period_ns  = (1000000 + speed_kHz/2) / speed_kHz;   // Rround up.
    
  hcnt_val = calcHCNT( speed_kHz, SCL_Period_ns/2, high_speed_i2c );
  lcnt_val = calcLCNT( speed_kHz, SCL_Period_ns/2, high_speed_i2c );
    
  //
  // Write calculated values to appropriate hardware registers
  // - speed_mode value range is [1|2|3].
  //
  i2c_disable_bus( bus );
  {
    unsigned short x = i2c_pilot_ii_read_reg( bus, I2C_CON_REG ) & 0xfff9;
    i2c_pilot_ii_write_reg( bus, x | (speed_mode<<1), I2C_CON_REG );

    i2c_pilot_ii_write_reg( bus, lcnt_val, lcnt_regs[speed_mode] );
    i2c_pilot_ii_write_reg( bus, hcnt_val, hcnt_regs[speed_mode] );
  }
  i2c_enable_bus( bus );

  // Save new speed setting.
  se_pilot_ii_data_ptr[bus].bus_speed = speed_kHz;
     
  return 0;
}

unsigned int Get_Activity_info(int bus)
{
  unsigned int activity=0;

  /* support I2C ports 8 and 9 */
  if ((bus >= 0) && (bus <= 7))
    activity = (*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_DBG_STS))>>(bus*4);
  else if ((bus == 8) || (bus == 9))
    activity = (*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_DBG_STS_1))>>((bus-8)*4);
  activity=(activity&0xf);

  return(activity);
  
}


unsigned int Get_Clk_Streaching_info(int bus)
{
  unsigned int Clk_Streaching=0;

  /* support I2C ports 8 and 9 */
  if ((bus >= 0) && (bus <= 7))
    Clk_Streaching = (*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_DBG_STS))>>(bus*4);
  else if ((bus == 8) || (bus == 9))
    Clk_Streaching = (*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_DBG_STS_1))>>((bus-8)*4);
  Clk_Streaching=(Clk_Streaching&0xf);

  return(Clk_Streaching);
  
}

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
int i2c_sys_reset(int bus)
{
	unsigned long syswrer = 0,syssrer = 0;
	unsigned long sysrcr=0; // Variable to store the original sysrcr value
	unsigned long bimask = 0x100;

	syswrer = *(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSWRER);
        
	if(syswrer & ARM_PROCESSOR)
	{
		/*ARM Processer reset is enabled. disable it*/
		*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSWRER) = (syswrer & 0xFFFFFFFE);
	}

	syssrer = *(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSSRER);

	if(syssrer & ARM_PROCESSOR)
	{
		*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSSRER) = syssrer & 0xFFFFFFFE;
	}

	/*Clear the Reset register*/
	*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSRST) = 0x7;

	/*Enable the I2C module to be reset*/
	*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSSRER) = (bimask << bus);

	sysrcr = *(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSRCR); // keep original value of sysrcr
	/*Enable Software reset*/
	*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSRCR) = SOFTWARE_RESET_ENABLE ;

	i2c_init_bus(bus);

	/* Restore variables to their original value */
	
	/* udelay is required because the reset signal is sent with 500 us delay */
	udelay(500); 
	*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSRST) = 0x2;
	/* Restore sysrcr, syswrer and sysrrer values */
	*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSWRER) = syswrer;
	*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSSRER) = syssrer;
	*(volatile unsigned long *)(SE_SYS_CLK_VA_BASE+SYSRCR)  = sysrcr;

	return 0;

}
#endif

/* Set "wait for bus free" bus busy timeout (in ms) */
int i2c_set_busbusy_timeout(int bus, int timeout)
{
	if (bus < BUS_COUNT)
	{
		busbusy_timeout[bus] = timeout;
	}

	return 0;
}

int i2c_is_bus_idle(int bus)
{
	int loops = 10;
	unsigned int offset = 0;
	int shift = 0;

	switch (bus) {
		case 0:
		case 1:
		case 2:
		case 3:
			offset = I2CPCT0;
			shift = bus * 8;
			break;
		case 4:
		case 5:
			offset = I2CPCT1;
			shift = (bus - 4) * 8;
			break;
		case 6:
		case 7:
			offset = I2CPCT2;
			shift = (bus - 6) * 8;
			break;
		/* support I2C ports 8 and 9 */
		case 8:
		case 9:
			offset = I2CPCT3;
			shift = (bus - 8) * 8;
			break;
		default:
			return 1;
	}

	while (loops-- > 0) {
		if ((((*(unsigned long *)(SE_SYS_CLK_VA_BASE + offset)) >> shift) & 0x44) != 0x44) {
			return 0;
		}
	}
	
	return 1;
}

void i2c_pilot_slave_xfer_enable(int bus) 
{ 
	u32 ctrl_bits = 0;

	/* clear interrups */ 
	i2c_pilot_ii_disable_all_interrupts(bus);
	i2c_pilot_ii_read_reg(bus, I2C_CLR_INTR_REG);	
	/*Disable the I2C interface first to write control register */
	i2c_disable_bus(bus);
	
	/* Disable Master function */
	ctrl_bits = i2c_pilot_ii_read_reg(bus,I2C_CON_REG); 
	ctrl_bits &= ~( MASTER_MODE); 
	ctrl_bits |= SLAVE_MODE;
	ctrl_bits &= ~IC_SLAVE_DISABLE;
	i2c_pilot_ii_write_reg( bus,ctrl_bits,I2C_CON_REG);

	udelay(100); 

	/* Set the TX fifo threshold level for interrupt generation */
	i2c_pilot_ii_write_reg (bus, SLAVETX_TX_THRESHOLD,I2C_TX_TL_REG);
	/* Set the RX fifo threshold level for interrupt generation */
	i2c_pilot_ii_write_reg (bus, SLAVETX_RX_THRESHOLD,I2C_RX_TL_REG);		
	

	se_pilot_ii_data_ptr[bus].SlaveTX_Enable = 1; 
	se_pilot_ii_data_ptr[bus].TX_len = 0; 
	se_pilot_ii_data_ptr[bus].TX_index = 0; 
		 
	se_pilot_ii_data_ptr[bus].MasterRX_len = 0; 
	se_pilot_ii_data_ptr[bus].MasterRX_index = 0; 
	
	se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE_XMIT;
	
	/* Enable interrupts */ 
	i2c_pilot_ii_enable_interrupt(bus, STOP_DET|TX_ABRT|RX_OVER|RX_FULL|RX_DONE|RD_REQ);

		
	/* Enable I2C interface */
	i2c_enable_bus(bus);		
}

