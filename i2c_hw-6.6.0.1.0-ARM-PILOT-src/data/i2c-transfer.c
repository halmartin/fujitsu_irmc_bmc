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
#include <linux/version.h>
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
#include "i2c-log.h"

extern struct i2c_pilot_ii_data se_pilot_ii_data_ptr[BUS_COUNT];
extern int fifo_depth[ BUS_COUNT ];
#ifdef INT_MONITOR
extern struct i2c_pilot_ii_interrupt_monitor int_monitor;
#endif
#ifdef BUS_BUSY_MONITOR
struct i2c_pilot_ii_bus_busy_monitor bus_busy_monitor;
#endif

extern void i2cTXEMPTY_slave_process(int bus, u32 status);
//#define SYS_RESET			// Emergency reset.

static int
DisplayAbortInfo( struct i2c_adapter *i2c_adap)
{ 
	int abort_source;
	//unsigned int Target;

   	abort_source =  se_pilot_ii_data_ptr[i2c_adap->nr].abort_status;

	//Target=i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_TAR_REG);
	i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_TAR_REG);

	/* Check for Address NACK */
   	if( abort_source & (ABRT_7B_ADDR_NOACK | ABRT_10ADDR1_NOACK | ABRT_10ADDR2_NOACK))
   	{
//   		dev_err( &i2c_adap->dev, "NACK on address transmission to 0x%x\n", Target );
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
   		se_pilot_ii_data_ptr[i2c_adap->nr].stat.nack++;
#endif
  		return( NACKONWR );
	}

   	/* Check for NACK */
   	if( abort_source & (ABRT_TXDATA_NOACK))
   	{
//   		dev_err( &i2c_adap->dev, "NACK on data transmission to 0x%x\n", Target );
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
		se_pilot_ii_data_ptr[i2c_adap->nr].stat.nack++;
#endif
   		return( NACKONWR );
   	}

   	/* Check for lost arbitration */
   	if( abort_source & ABRT_LOST )
   	{
//   		dev_err( &i2c_adap->dev, "Arbitration lost on data transmission\n" );
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
		se_pilot_ii_data_ptr[i2c_adap->nr].stat.arblost++;
#endif
      	return( ARBLOST );
   	}

	/* Check for using disabled Master */
   	if( abort_source & ABRT_MASTER_DIS )
   	{
//   	 	dev_err( &i2c_adap->dev, "Attempted to use disabled Master on data transmission\n" );
   		return( -EREMOTEIO );
   	}

   	/* Check for the master sends a read command in 10bit addressing mode when the restart is disable */
   	if( abort_source & ABRT_10B_RD_NORSTRT )
    {
//   		dev_err( &i2c_adap->dev, "The master send a read command in 10bit addresing mode when the restartis disable on data transmission\n" );
   		return( -EREMOTEIO );
   	}

   	/* Check for the user is trying to send a start byte when the restart is disable */
   	if( abort_source & ABRT_SBYTE_NORSTRT )
   	{
//   	 	dev_err( &i2c_adap->dev, "The user is trying to send a start byte when the restart is disable on data transmission\n" );
   		return( -EREMOTEIO );
   	}

   	/* Check for the user is trying to use the master to send data in High speed mode when restart is disable */
   	if( abort_source & ABRT_HS_NORSTRT )
   	{
//   	 	dev_err( &i2c_adap->dev, "The user is trying to use the master to send data in high speed mode when restart is disable on data transmission\n" );
   		return( -EREMOTEIO );
   	}

   	/* Check for Master has sent a start byte and teh start byte was acknowledged (wrong behavior) */
   	if( abort_source & ABRT_SBYTE_ACKDET )
   	{
//   	 	dev_err( &i2c_adap->dev, "Master has sent a start byte and the start byte was acknowledged on data transmission\n" );
   		return( -EREMOTEIO );
   	}

  	/* Check for Master is in high speed mode and the high speed master code was acknowledged */
   	if( abort_source & ABRT_HS_ACKDET )
   	{
//   		dev_err( &i2c_adap->dev, "Master is in high speed mode and the high speed master code was acknowledged  on data transmission\n" );
   		return( -EREMOTEIO );
   	}

	/* Check for Master sent a general call but the user programmed the byte following the G.Call to be read rom the bus*/
   	if( abort_source & ABRT_GCALL_READ )
	{  
//   		dev_err( &i2c_adap->dev, "Master sent a general call but the user programmed the byte following the G.Call to be read from the bus  on data transmission\n" );
   		return( -EREMOTEIO );
   	}

   	/* Check for Master sent a general call and no slave on the bus responded with an ack*/
   	if( abort_source & ABRT_GCALL_NOACK )
   	{
//   		dev_err( &i2c_adap->dev, "Master sent a general call but and no slave on the bus responded with an ack on data transmission\n" );
   		return( -EREMOTEIO );
   	}

//   	dev_err( &i2c_adap->dev, "INTERNAL ERROR: Unknown Abort Source %x\n",abort_source);
	return (BUSERR);
}

static u32 
i2c_pilot_ii_wait_for_int( int bus , int ms_timeout)
{
  if (wait_event_timeout(se_pilot_ii_data_ptr[bus].pilot_ii_wait,se_pilot_ii_data_ptr[bus].op_status,(msecs_to_jiffies(ms_timeout) ) ) == 0)
	{
   		se_pilot_ii_data_ptr[bus].op_status = i2c_pilot_ii_read_reg( bus, I2C_INTR_STAT_REG );
	}
	return se_pilot_ii_data_ptr[bus].op_status;
}

static int 
pilot_ii_bus_busy_monitor (int bus)
{
	unsigned int current_timestamp = 0;

#ifdef BUS_BUSY_MONITOR
	unsigned int position;

	if (bus_busy_monitor.monitor_count < MAX_BUS_BUSY_MONITOR)
	{
		bus_busy_monitor.monitor_count++;
		position = bus_busy_monitor.monitor_count - 1;
		bus_busy_monitor.start_position = 0;
	}
	else
	{
		position = bus_busy_monitor.start_position;
		bus_busy_monitor.start_position++;
		bus_busy_monitor.start_position %= MAX_BUS_BUSY_MONITOR;
	}
	bus_busy_monitor.data[position].jiffies_value = get_jiffies_64();
	bus_busy_monitor.data[position].bus = bus;
	bus_busy_monitor.data[position].i2c_con = i2c_pilot_ii_read_reg(bus,I2C_CON_REG);
	bus_busy_monitor.data[position].i2c_tar = i2c_pilot_ii_read_reg(bus,I2C_TAR_REG);
	bus_busy_monitor.data[position].i2c_sar = i2c_pilot_ii_read_reg(bus,I2C_SAR_REG);
	bus_busy_monitor.data[position].i2c_intr_stat = i2c_pilot_ii_read_reg(bus,I2C_INTR_STAT_REG);
	bus_busy_monitor.data[position].i2c_rx_tl = i2c_pilot_ii_read_reg(bus,I2C_RX_TL_REG);
	bus_busy_monitor.data[position].i2c_tx_tl = i2c_pilot_ii_read_reg(bus,I2C_TX_TL_REG);
	bus_busy_monitor.data[position].i2c_enable = i2c_pilot_ii_read_reg(bus,I2C_ENABLE_REG);
	bus_busy_monitor.data[position].i2c_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
	bus_busy_monitor.data[position].i2c_txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG);
	bus_busy_monitor.data[position].i2c_rxflr = i2c_pilot_ii_read_reg(bus,I2C_RXFLR_REG);
	bus_busy_monitor.data[position].i2c_tx_abt_src = i2c_pilot_ii_read_reg(bus,I2C_TX_ABORT_SOURCE_REG);
	bus_busy_monitor.data[position].i2c_debug_sts = Get_Clk_Streaching_info(bus);
#endif
	/*
	 * The below code will look for bus busy and if it gets continuous bus busy for BUS_BUSY_SECONDS seconds and 
	 * BUS_BUSY_COUNT_LIMIT times then the transfer assumes some problem in the driver and resets by itself.
	 * This is a failsafe mechanism which can be recovered after some time.
	 * It stores the number of resets in proc entry with some registers.
	 */
	current_timestamp = jiffies/HZ;
	if (se_pilot_ii_data_ptr[bus].bus_busy_count == 0)
	{
		se_pilot_ii_data_ptr[bus].bus_busy_count++;
		se_pilot_ii_data_ptr[bus].bus_busy_timestamp = current_timestamp;
	}
	else
	{
		se_pilot_ii_data_ptr[bus].bus_busy_count++;
		if (current_timestamp < se_pilot_ii_data_ptr[bus].bus_busy_timestamp)
		{
			se_pilot_ii_data_ptr[bus].bus_busy_timestamp = current_timestamp; // Someone changed the time backwards. What if they forward the time?????
		}
		else if (((current_timestamp - se_pilot_ii_data_ptr[bus].bus_busy_timestamp) >= BUS_BUSY_SECONDS) || 
				(se_pilot_ii_data_ptr[bus].bus_busy_count >= BUS_BUSY_COUNT_LIMIT))
		{
			/* If slave interface is enabled and active, disable/re-enable the slave to clear the active state */
			u16 icCon = i2c_pilot_ii_read_reg (bus, I2C_CON_REG);

			if (((icCon & IC_SLAVE_DISABLE) == 0) && (Get_Clk_Streaching_info(bus) & 0x1))
			{
				i2c_disable_bus(bus);	/* Disable I2C interface first */

				i2c_pilot_ii_write_reg(bus, icCon | IC_SLAVE_DISABLE, I2C_CON_REG);
				i2c_pilot_ii_write_reg(bus, icCon, I2C_CON_REG);

				i2c_enable_bus(bus);	/* Enable I2C interface */
			}

			i2c_init_internal_data(bus);
#if defined(SYS_RESET) && defined(SOC_PILOT_III)
			printk(KERN_DEBUG "I2C%d: (%llu) Master-Xfer failed. Continuous Bus busy, System reset of i2c bus.\n", bus, get_jiffies_64());
			i2c_sys_reset(bus);
			i2c_init_bus(bus);
#else
			printk(KERN_DEBUG "I2C%d: (%llu) Master-Xfer failed. Continuous Bus busy, Normal reset of i2c bus.\n", bus, get_jiffies_64());
			i2c_init_hardware(bus);
#endif
			//enable time out counters
			i2c_to_counter_en( bus);
			//Enable Time out interrupts
			i2c_time_out_interrupt(bus);
			se_pilot_ii_data_ptr[bus].bus_reset_count++;
			se_pilot_ii_data_ptr[bus].continuous_bus_reset++;
		}
		
	}
	
	printk(KERN_DEBUG "I2C%d: (%llu) Master-Xfer failed. Bus busy count %d, Time (in seconds) : %d\n", 
			bus, get_jiffies_64(), se_pilot_ii_data_ptr[bus].bus_busy_count, 
			(current_timestamp - se_pilot_ii_data_ptr[bus].bus_busy_timestamp));

	return 0;
}

static int 
pilot_ii_bus_timeout_monitor (int bus)
{

	unsigned int current_timestamp = 0;
	/*
	 * The below code will look for bus timeout and if it gets continuous bus timeout for BUS_TIMEOUT_SECONDS seconds and 
	 * BUS_TIMEOUT_COUNT_LIMIT times then the transfer assumes some problem in the driver and resets by itself.
	 * This is a failsafe mechanism which can be recovered after some time.
	 * It stores the number of resets in proc entry with some registers.
	 */
	current_timestamp = jiffies/HZ;
	if (se_pilot_ii_data_ptr[bus].bus_timeout_count == 0)
	{
		se_pilot_ii_data_ptr[bus].bus_timeout_count++;
		se_pilot_ii_data_ptr[bus].bus_timeout_timestamp = current_timestamp;
	}
	else
	{
		se_pilot_ii_data_ptr[bus].bus_timeout_count++;
		if (current_timestamp < se_pilot_ii_data_ptr[bus].bus_timeout_timestamp)
		{
			se_pilot_ii_data_ptr[bus].bus_timeout_timestamp = current_timestamp; // Someone changed the time backwards. What if they forward the time?????
		}
		else if (((current_timestamp - se_pilot_ii_data_ptr[bus].bus_timeout_timestamp) >= BUS_TIMEOUT_SECONDS) || 
				(se_pilot_ii_data_ptr[bus].bus_timeout_count >= BUS_TIMEOUT_COUNT_LIMIT))
		{
			i2c_bus_recovery(bus, 1);    /* Unconditionally force a STOP */
			i2c_init_internal_data(bus);
#if defined(SYS_RESET) && defined(SOC_PILOT_III)
			printk(KERN_DEBUG "I2C%d: (%llu) Continuous Bus Timeout, System reset of i2c bus.\n", bus, get_jiffies_64());
			i2c_sys_reset(bus);
			i2c_init_bus(bus);
#else
			printk(KERN_DEBUG "I2C%d: (%llu) Continuous Bus Timeout, Normal reset of i2c bus.\n", bus, get_jiffies_64());
			i2c_init_hardware(bus);
#endif
			//enable time out counters
			i2c_to_counter_en( bus);
			//Enable Time out interrupts
			i2c_time_out_interrupt(bus);
			se_pilot_ii_data_ptr[bus].bus_timeout_reset_count++;
			se_pilot_ii_data_ptr[bus].continuous_bus_timeout_reset++;
		}
		
	}
	return 0;
}

static int 
pilot_ii_send_bytes( struct i2c_adapter *i2c_adap,
                          struct i2c_msg msg, int messages_left )
{
   	int status;
   	int i;

	for (i=0; i<msg.len; i++)
	  se_pilot_ii_data_ptr[i2c_adap->nr].TX_data[i] = msg.buf[i];

	se_pilot_ii_data_ptr[i2c_adap->nr].TX_len = msg.len;
	se_pilot_ii_data_ptr[i2c_adap->nr].TX_index = 0;
	
	
	se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = MASTER_XMIT;
	
	/* Unmask TX empty interrupt */
	se_pilot_ii_data_ptr[i2c_adap->nr].op_status = 0;
	
   	i2c_pilot_ii_enable_interrupt(i2c_adap->nr,TX_EMPTY);
	/* Wait for interrupt */

 	status = i2c_pilot_ii_wait_for_int( i2c_adap->nr, (msg.len*300)+3000);

	
	se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = SLAVE;
	
  	i2c_pilot_ii_disable_interrupt(i2c_adap->nr,TX_EMPTY);
	
	/* Check for signal pending */
  	if( status == PILOT_II_SIGNAL_RECEIVED )
	  {
	    //dev_info( &i2c_adap->dev, "send_bytes: Signal pending\n" );
	    return( -ERESTARTSYS );
	  }
	
	if (status & TX_ABRT)
	  return DisplayAbortInfo(i2c_adap);
	
	/* Check for timeout */
	if( status == 0 )
	{
#ifdef INT_MONITOR
		if (i2c_adap->nr == int_monitor.bus)
			int_monitor.start_flag = 0;
#endif
		printk(KERN_DEBUG "I2C%d: master send_bytes: Timed out sending data\n", i2c_adap->nr);
		printk(KERN_DEBUG "I2C%d: master send_bytes: Got status: 0x%08x\n", i2c_adap->nr, status );
		pilot_ii_bus_timeout_monitor(i2c_adap->nr);
		return( -EREMOTEIO );
	}
   	
#ifdef INT_MONITOR
	if (i2c_adap->nr == int_monitor.bus)
		int_monitor.start_flag = 1;
#endif

   	return( 0 );
}

static int 
pilot_ii_receive_bytes( struct i2c_adapter *i2c_adap,
			struct i2c_msg msg, int messages_left )
{
  unsigned long		flags;
  int status;
  int i;
  int bus=i2c_adap->nr;
  int fifo_size=fifo_depth[bus];

  //  printk("%s %x\n",__FUNCTION__, msg.len);
  if(msg.len<=0x0)
    {
      return( -ERESTARTSYS );
    }

  if((i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_RXFLR_REG)))
    {
      return( -EREMOTEIO );
    }
  
  se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_index = 0;
  se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len = msg.len;
  se_pilot_ii_data_ptr[i2c_adap->nr].Master_rd_cmd_index=0x0;
  se_pilot_ii_data_ptr[i2c_adap->nr].TX_len = 0;
  se_pilot_ii_data_ptr[i2c_adap->nr].TX_index=0x0;
  
  se_pilot_ii_data_ptr[i2c_adap->nr].op_status = 0;
  
  //wait if our slave is busy
  // while(Get_Activity_info(i2c_adap->nr)&0x1);
  //  while(i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_RXFLR_REG));
  
  spin_lock_irqsave( &se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock,flags);

  se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = MASTER_RECV;

  if(msg.len<=fifo_size)
    {
      /* send read command */
#if defined SOC_PILOT_IV
        for(i=0; i < msg.len -1; i++)
#else
        for(i=0; i < msg.len ; i++)
#endif
	{
	  i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
	  se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
	}
#if defined SOC_PILOT_IV
		// Program the stop bit
      i2c_pilot_ii_write_reg(bus, (0x100 | CMDSTOP), I2C_DATA_CMD_REG);
      se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
#endif
    }
  else
    {
      //send read cmds
      for(i = 0; i<fifo_size;i++)
	{
	  i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
	  se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
	}
      
      i2c_pilot_ii_enable_interrupt(i2c_adap->nr,TX_EMPTY);
    }
  spin_unlock_irqrestore(&se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock, flags);


  /* Wait for interrupt */
  status = i2c_pilot_ii_wait_for_int(i2c_adap->nr,(msg.len*300)+3000);
  
  se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = SLAVE;
  
  /* Check for signal pending */
  if( status == PILOT_II_SIGNAL_RECEIVED )
    {
      dev_info( &i2c_adap->dev, "pilot_ii_receive_bytes: Signal pending\n" );
      return( -ERESTARTSYS );
    }
  
  if (status & TX_ABRT)
    return DisplayAbortInfo(i2c_adap);
  
  /* check if the receive buffer is empty */
  if(  status & RX_UNDER )
    {
      /* read this register to clear RX_UNDER interrupt */
      i2c_pilot_ii_read_reg(i2c_adap->nr, I2C_CLR_RX_UNDER_REG);
      /* Got empty buffer when read */
      dev_err( &i2c_adap->dev, "pilot_ii_receive_bytes: Empty receiver buffer on data read\n" );
      return( -EREMOTEIO );
    }
  
  /* check if the receive buffer is completely filled and more data arrived */
  if(  status & RX_OVER )
    {
      /* read this register to clear RX_OVER interrupt */
      i2c_pilot_ii_read_reg(i2c_adap->nr, I2C_CLR_RX_OVER_REG);
      dev_err( &i2c_adap->dev, "pilot_ii_receive_bytes: The receiver buffer is completely filled, more data arrived is lost\n" );
      return( -EREMOTEIO );
    }
  
  if( status == 0 )
    {
      dev_err( &i2c_adap->dev, "pilot_ii_receive_bytes: Timed out receiving data\n" );
      dev_err( &i2c_adap->dev, "pilot_ii_receive_bytes: Got status: 0x%08x\n", status );
      printk("rd cmds %d \n",se_pilot_ii_data_ptr[bus].Master_rd_cmd_index);
      return( -EREMOTEIO );
    }
  
  for ( i = 0; i<msg.len; i++)
    msg.buf[i] = se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_data[i];
  
  return( 0 );
}


static int 
send_slave_addr( struct i2c_adapter *i2c_adap, int addr, int flags )
{
   	u16 convalue;
	unsigned long	irqFlags;
	int retryCount = 3;	/* retry bus busy check a finite number of times */

retry:  /* Retry selective bus busy events */
   	if ( i2c_wait_for_bus_free_ext(i2c_adap->nr) != 0 )
   	{
   		printk(KERN_DEBUG "I2C%d:I2C bus busy\n",i2c_adap->nr);
   		return -1;
   	}
   	
    spin_lock_irqsave( &se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock,irqFlags);
#if 1	/* Enhance bus busy checking */
    if ( ( i2c_pilot_ii_read_reg(i2c_adap->nr, I2C_STATUS_REG ) & IC_STATUS_ACTIVITY ) ||
         ( (Get_Clk_Streaching_info(i2c_adap->nr) & 0x01) != 0 ) || 
         ( i2c_is_bus_idle(i2c_adap->nr) == 0 ) )

#else
    if ( i2c_pilot_ii_read_reg(i2c_adap->nr, I2C_STATUS_REG ) & IC_STATUS_ACTIVITY )
#endif
    {
    	spin_unlock_irqrestore(&se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock, irqFlags);

   		/* After successfully waiting for bus to be free a peer master has now
   		 * started a bus transfer. Rather than returning a bus busy failure, retry the operation. */
   		if (retryCount-- > 0) goto retry;
   		printk(KERN_DEBUG "I2C%d:I2C bus busy\n",i2c_adap->nr);
   		return -1;
    }

    //Disable the I2C interface to ensure the TX FIFO is empty for new master transfer.
    i2c_disable_bus(i2c_adap->nr);

    /* Check for 10-bit slave addresses  */
   	if( flags & I2C_M_TEN )
	  {
	    //i2c_pilot_ii_write_reg(i2c_adap->nr,0, I2C_ENABLE_REG);
	    
	    convalue = i2c_pilot_ii_read_reg (i2c_adap->nr, I2C_CON_REG);
	    convalue |= IC_10BIT_ADDR_MASTER;
	    /* Write addressing mode back to control register */
	    i2c_pilot_ii_write_reg (i2c_adap->nr,convalue,I2C_CON_REG);
	    //i2c_pilot_ii_write_reg(i2c_adap->nr,1, I2C_ENABLE_REG);
   	}

   	/* Program TAR register */
	i2c_pilot_ii_write_reg (i2c_adap->nr, addr,I2C_TAR_REG);
    /* RE-enable the I2C interface */
	i2c_enable_bus(i2c_adap->nr);
    
    spin_unlock_irqrestore(&se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock, irqFlags);
	return 0;
}

#if defined SOC_PILOT_IV
static int send_slave_addr_bitbang( struct i2c_adapter *i2c_adap, int addr)
{
	int bus = i2c_adap->nr;
	int pulse_period = 5; //100KHz Clock
	int ret = 0;
	int i = 0;
	
	addr = addr << 1;
	
	drive_clock_high(bus);
	drive_data_high(bus);
	pilot_bit_bang_select(bus);
	udelay (pulse_period); 
	
	// Start Condition
	drive_clock_high(bus);
	drive_data_low(bus);
	udelay (pulse_period);

	for (i = 0;i < 7;i ++)
	{
		if ( addr & (0x1 << (7-i)) )
		{
			drive_clock_low(bus);
			drive_data_high(bus);//1
			udelay (pulse_period);
			drive_clock_high(bus);
			udelay (pulse_period);
		}
		else
		{
			drive_clock_low(bus);
			drive_data_low(bus);//0
			udelay (pulse_period);
			drive_clock_high(bus);
			udelay (pulse_period);
		}
	}

	//Write Bit
	drive_clock_low(bus);
	drive_data_low(bus);//0
	udelay (pulse_period);
	drive_clock_high(bus);
	udelay (pulse_period);

	
	//Confirmation
	drive_clock_low(bus);
	drive_data_high(bus);//1
	udelay (pulse_period);
	drive_clock_high(bus);
	udelay (pulse_period);
	
	if (0 != get_sda_status(bus) ) 
		ret = -1;
	
	// Stop Condition
	drive_clock_low(bus);
	drive_data_low(bus);//0
	udelay (pulse_period);
	drive_clock_high(bus);
	udelay (pulse_period);
	
	// Idle
	drive_clock_high(bus);//1
	drive_data_high(bus);
	udelay (pulse_period);
	
	pilot_bit_bang_deselect(bus);
	return ret;
}
#endif

int pilot_ii_master_xfer(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs, int num )
{
  int i = 0;
	//int j = 0;
  int retval = 0;
  int status;
	unsigned long	flags;
	int bus=i2c_adap->nr;
	int fifo_size=fifo_depth[bus];
	unsigned volatile int fifo_status=0x0;

	if (se_pilot_ii_data_ptr[i2c_adap->nr].SlaveTX_Enable)
	{
		printk("ERROR: I2C%d: bus for slave xfer, disallow master transfer\n", i2c_adap->nr); 
		return( BUSERR ); 
	}	
	
	se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len = 0x0;
	se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_index = 0x0;
	se_pilot_ii_data_ptr[i2c_adap->nr].Master_rd_cmd_index=0x0;
	
	se_pilot_ii_data_ptr[i2c_adap->nr].TX_len = 0;
	se_pilot_ii_data_ptr[i2c_adap->nr].TX_index=0x0;
	
	se_pilot_ii_data_ptr[bus].MasterTruncated = 0;	/* Handle truncated master transfers as errors */


	/* Check if bus busy */
	if(i2c_wait_for_bus_free_ext(i2c_adap->nr) == 1)
	{
		/* Bus busy. Perform a recovery */
		i2c_bus_recovery(i2c_adap->nr, 0);  /* Conditionally force a STOP when enabled */
		{
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
			se_pilot_ii_data_ptr[i2c_adap->nr].stat.error++;
#endif
#ifdef INT_MONITOR
			if (i2c_adap->nr == int_monitor.bus)
				int_monitor.start_flag = 0;
#endif
			pilot_ii_bus_busy_monitor(i2c_adap->nr);
			return -EIO;
		}
	}
	/* Loop across all the messages */
	/* Send the destination slave address onto the bus */

	//clear any abort status set from prior transactions	
	 se_pilot_ii_data_ptr[i2c_adap->nr].abort_status=0x0;
	if(num == 1)
	{
	  for (i=0;i<num;i++)
	    {
#if defined SOC_PILOT_IV
		  if(!(msgs[i].flags & I2C_M_RD) && (msgs[i].len == 0))
		  {
			  // In order to support I2C scan functionality, I2C controller should have capability of doing zero byte write transaction. 
			  // Because limitation of I2C controller of Pilot4 SoC which can not do zero byte write transaction, 
			  // implement scan by sending slave address in bit-bang.
			  retval = send_slave_addr_bitbang(i2c_adap, msgs[i].addr);
			  return retval;
		  }
#else
		  if(!(msgs[i].flags & I2C_M_RD) && (msgs[i].len == 0))
		  {
			  printk(KERN_WARNING "I2C Zero Byte write is not supported\n");
			  return -EIO;
		  }
#endif
	      retval = send_slave_addr(i2c_adap, msgs[i].addr, msgs[i].flags);
	      if(retval != 0) {
	    	  log_i2c_plain(i2c_adap, (char *)&msgs[i].addr, 1);  	    	  
	    	  return -EIO;
	      }
	      
	      if (msgs[i].flags & I2C_M_RD)
		retval = pilot_ii_receive_bytes(i2c_adap, msgs[i], num-(i+1));
	      else
		retval = pilot_ii_send_bytes(i2c_adap, msgs[i],num-(i+1));
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
			se_pilot_ii_data_ptr[i2c_adap->nr].stat.master_tx++;
			se_pilot_ii_data_ptr[i2c_adap->nr].stat.count++;
			// Do overflow protection with lower limit to reduce integration time and time to reach an predefined error ratio
	        if (se_pilot_ii_data_ptr[i2c_adap->nr].stat.count > 10000) {
	        	se_pilot_ii_data_ptr[i2c_adap->nr].stat.count = (se_pilot_ii_data_ptr[i2c_adap->nr].stat.count + 9) / 10;
	        	se_pilot_ii_data_ptr[i2c_adap->nr].stat.error = (se_pilot_ii_data_ptr[i2c_adap->nr].stat.error + 9) / 10;
	        }
#endif
		if(retval != 0)
		{
			if(msgs[i].flags & I2C_M_RD)
			{
				log_i2c_err(i2c_adap,"Master-Read failed %x\n",retval);
			}
			else
			{
				log_i2c_err(i2c_adap,"Master-Write failed %x\n",retval);
			}
		}

		if (msgs[i].flags & I2C_M_RD)	
			log_i2c(i2c_adap, &msgs[i], LOG_I2C_MR);
		else
			log_i2c(i2c_adap, &msgs[i], LOG_I2C_MW);
			
	    }
	  if (retval == 0)
	  {
		  se_pilot_ii_data_ptr[i2c_adap->nr].bus_busy_count = 0;
		  se_pilot_ii_data_ptr[i2c_adap->nr].continuous_bus_reset = 0; // The bus is ok now, so initialize the continuous counter.
		  se_pilot_ii_data_ptr[i2c_adap->nr].bus_timeout_count = 0;
		  se_pilot_ii_data_ptr[i2c_adap->nr].continuous_bus_timeout_reset = 0;
		  retval = i;
	  }
	  return(retval);
	}
	
	
	/* ????????????????? */
	if (num != 2)
	  {
	    se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = SLAVE;
	    return (-EINVAL);
	  }
	
	/* ---------------Num == 2  Repeated Start Condition --------------------------*/
	
    se_pilot_ii_data_ptr[i2c_adap->nr].op_status = 0;	
	
	retval = send_slave_addr(i2c_adap, msgs[0].addr, msgs[0].flags);
	if(retval != 0)
	  return -EIO;
	

	if (msgs[1].flags & I2C_M_RD)
	{
	    //MASTER WRITE-MASTER READ RESTART CASE
	    /* set RX buffer len as requested bytes lengh */
	    se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len = msgs[1].len;
	    se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_index = 0;
	    se_pilot_ii_data_ptr[i2c_adap->nr].Master_rd_cmd_index=0x0;

	    se_pilot_ii_data_ptr[i2c_adap->nr].op_status = 0;
	    
	    //wait if our slave is busy
	    //	    while(Get_Activity_info(i2c_adap->nr)&0x1);
	    //while(i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_RXFLR_REG));
	    if((i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_RXFLR_REG)))
	      {
		return( -EREMOTEIO );
	      }
	    
	    
	    spin_lock_irqsave( &se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock,flags);
	    se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = RESTART_MWMR;
	    
        if (se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) {
        /*  Set the RX fifo threshold at 0 so that 1 read generates RX_FULL interrupt generation */
         i2c_pilot_ii_write_reg (bus, 0 ,I2C_RX_TL_REG);
        }
	    
	    if(msgs[0].len<fifo_size)
	    {
	        //send write data
	        for (i = 0; i<msgs[0].len;i++)
	            i2c_pilot_ii_write_reg(i2c_adap->nr, msgs[0].buf[i], I2C_DATA_CMD_REG);
		
		
	        se_pilot_ii_data_ptr[i2c_adap->nr].TX_len = msgs[0].len;
	        se_pilot_ii_data_ptr[i2c_adap->nr].TX_index = msgs[0].len;

	        if ((se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) && se_pilot_ii_data_ptr[bus].Master_rd_cmd_index==0) {
	            // reinitialize MasterRX_len irrespective of PEC of SMBus
#if defined SOC_PILOT_IV  
	                se_pilot_ii_data_ptr[bus].MasterRX_len = 2;
#else
	                se_pilot_ii_data_ptr[bus].MasterRX_len = 1; 
#endif
	        }
	        
	        /* send read command */
	        fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
#if defined SOC_PILOT_IV 
	        while( (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index < se_pilot_ii_data_ptr[bus].MasterRX_len - 1)&&(fifo_status &TFNF) )
#else
	        while( (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index < se_pilot_ii_data_ptr[bus].MasterRX_len)&&(fifo_status &TFNF) )
#endif
	        {
	            if ((se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) && se_pilot_ii_data_ptr[bus].Master_rd_cmd_index ==0)  {
#if !defined(SOC_PILOT_IV)         //PILOT_IV is SMP, RX_FULL is already enabled on the other CPU       
	                i2c_pilot_ii_enable_interrupt(i2c_adap->nr,RX_FULL);
#endif
	            }    
	            i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
	            se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
	            fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
	        }
#if defined SOC_PILOT_IV 
	        if ((se_pilot_ii_data_ptr[bus].Master_rd_cmd_index == se_pilot_ii_data_ptr[bus].MasterRX_len - 1) && (fifo_status &TFNF))
	        {
                if (!((se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) && se_pilot_ii_data_ptr[bus].Master_rd_cmd_index==1)) {
                    i2c_pilot_ii_write_reg(bus, (0x100 | CMDSTOP), I2C_DATA_CMD_REG);
                    se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
                }
	        }
#endif
	        spin_unlock_irqrestore(&se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock, flags);
	        if((se_pilot_ii_data_ptr[bus].Master_rd_cmd_index<se_pilot_ii_data_ptr[bus].MasterRX_len) || 
	                ((se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) && se_pilot_ii_data_ptr[bus].Master_rd_cmd_index==1))
	        {
	            i2c_pilot_ii_enable_interrupt(i2c_adap->nr,TX_EMPTY);
	        }
	         
	    }
	    else
	    {
            for (i=0; i<msgs[0].len; i++)                
                se_pilot_ii_data_ptr[i2c_adap->nr].TX_data[i] = msgs[0].buf[i];

            se_pilot_ii_data_ptr[i2c_adap->nr].TX_len = msgs[0].len;
            se_pilot_ii_data_ptr[i2c_adap->nr].TX_index = 0;
            
            spin_unlock_irqrestore(&se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock, flags); 
		
            i2c_pilot_ii_enable_interrupt(i2c_adap->nr,TX_EMPTY);
	    }
	    if (se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) {
	        msgs[1].len = se_pilot_ii_data_ptr[bus].MasterRX_len;
	    }
	    status = i2c_pilot_ii_wait_for_int(i2c_adap->nr,(msgs[1].len*300) + 600);		// Included delay 0f 600ms as it makes timeout while flashing.
	
	    se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = SLAVE;
	  }
	else
	  {
	    //MASTER READ-MASTER-WRITE-RE-START-CASE
	    /* set RX buffer len as requested bytes lengh */
	    se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len = msgs[0].len;
	    se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_index = 0;
	    se_pilot_ii_data_ptr[i2c_adap->nr].Master_rd_cmd_index=0x0;

	    se_pilot_ii_data_ptr[i2c_adap->nr].TX_len = msgs[1].len;
	    se_pilot_ii_data_ptr[i2c_adap->nr].TX_index = 0x0;
	    
	    for (i=0; i<msgs[1].len; i++)
	      se_pilot_ii_data_ptr[i2c_adap->nr].TX_data[i] = msgs[1].buf[i];
	
	    
	    se_pilot_ii_data_ptr[i2c_adap->nr].op_status = 0;
	    
	    //wait if our slave is busy
	    //	    while(Get_Activity_info(i2c_adap->nr)&0x1);
	    //while(i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_RXFLR_REG));
	    if((i2c_pilot_ii_read_reg(i2c_adap->nr,I2C_RXFLR_REG)))
	      {
		return( -EREMOTEIO );
	      }

	    
	    spin_lock_irqsave( &se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock,flags);
	    se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = RESTART_MRMW;
	    
	    
	    if(msgs[0].len<fifo_size)
	      {
		/* send read command */
		for(i = 0; i<msgs[0].len;i++)
		  {
		    i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
		    se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
		  }
		fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
#if defined SOC_PILOT_IV
        while((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len - 1))
#else
		while((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len))
#endif
		  {
		    i2c_pilot_ii_write_reg(bus,se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index],I2C_DATA_CMD_REG);
		    se_pilot_ii_data_ptr[bus].TX_index ++;
		    fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
		  }
#if defined SOC_PILOT_IV
			if ((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index == se_pilot_ii_data_ptr[bus].TX_len - 1))
			{
				i2c_pilot_ii_write_reg(bus, ((se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index]) | CMDSTOP), 
						I2C_DATA_CMD_REG);
				se_pilot_ii_data_ptr[bus].TX_index++;
			}
			if(se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len)
			{
				i2c_pilot_ii_enable_interrupt(i2c_adap->nr,TX_EMPTY);
			}
#endif
	      }
	    else
	      {
		/* send read command */
		for(i =0; i<fifo_size;i++)
		  {
		    i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
		    se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
		  }
	      }
	    i2c_pilot_ii_enable_interrupt(i2c_adap->nr,TX_EMPTY);
	    
	    spin_unlock_irqrestore(&se_pilot_ii_data_ptr[i2c_adap->nr].transfer_lock, flags);
	    
	    
	    status = i2c_pilot_ii_wait_for_int(i2c_adap->nr,(msgs[1].len*300)+3000);
	
	    se_pilot_ii_data_ptr[i2c_adap->nr].master_xmit_recv_mode_flag = SLAVE;
	  }

	
	
	/* Check for signal pending */
  	if( status == PILOT_II_SIGNAL_RECEIVED )
   	{
		//dev_info( &i2c_adap->dev, "send_bytes: Signal pending\n" );
		return( -ERESTARTSYS );
   	}

	if (status & TX_ABRT)	
	{
		return DisplayAbortInfo(i2c_adap);
	}

   	/* Check for timeout */
   	if( status == 0 )
	{
       	printk(KERN_DEBUG "I2C%d: pilot_ii_repeated start: Timed out sending data\n", i2c_adap->nr);
       	printk(KERN_DEBUG "rd_cmds %d \n",se_pilot_ii_data_ptr[i2c_adap->nr].Master_rd_cmd_index);
       	pilot_ii_bus_timeout_monitor(i2c_adap->nr);
       	return( -EREMOTEIO );
   	}

        
	/* check if the receive buffer is empty */
   	if(  status & RX_UNDER )
   	{
	  /* read this register to clear RX_UNDER interrupt */
	  i2c_pilot_ii_read_reg(i2c_adap->nr, I2C_CLR_RX_UNDER_REG);
	  dev_err( &i2c_adap->dev, "pilot_ii_repeated_start: Empty receiver buffer on data read\n" );
	  return( -EREMOTEIO );
   	}
	
	
	/* check if the receive buffer is completely filled and more data arrived */
   	if(  status & RX_OVER )
	  {
	    /* read this register to clear RX_OVER interrupt */
	    i2c_pilot_ii_read_reg(i2c_adap->nr, I2C_CLR_RX_OVER_REG);
	    dev_err( &i2c_adap->dev, "pilot_ii_repeated_start: The receiver buffer is completely filled, more data arrived is lost\n" );
	    return( -EREMOTEIO );
	  }
	if (msgs[1].flags & I2C_M_RD)
	  {
	    if (se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) {
	        msgs[1].len = se_pilot_ii_data_ptr[bus].MasterRX_len;
	    }
	    for ( i = 0; i<msgs[1].len; i++)
	      msgs[1].buf[i] = se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_data[i];

	  }
	else
	  {
	    for ( i = 0; i<msgs[0].len; i++)
	      {
		msgs[0].buf[i] = se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_data[i];
	      }
	  }
	for (i = 0; i < num; ++i) {
		if (msgs[i].flags & I2C_M_RD)
			log_i2c(i2c_adap, &msgs[i], LOG_I2C_MR);
		else
			log_i2c(i2c_adap, &msgs[i], LOG_I2C_MW);
	
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
		se_pilot_ii_data_ptr[i2c_adap->nr].stat.master_tx++;
		se_pilot_ii_data_ptr[i2c_adap->nr].stat.count++;
		// Do overflow protection with lower limit to reduce integration time and time to reach an predefined error ratio
        if (se_pilot_ii_data_ptr[i2c_adap->nr].stat.count > 10000) {
        	se_pilot_ii_data_ptr[i2c_adap->nr].stat.count = (se_pilot_ii_data_ptr[i2c_adap->nr].stat.count + 9) / 10;
        	se_pilot_ii_data_ptr[i2c_adap->nr].stat.error = (se_pilot_ii_data_ptr[i2c_adap->nr].stat.error + 9) / 10;
        }
#endif
	}
	
	
	se_pilot_ii_data_ptr[i2c_adap->nr].bus_busy_count = 0;
	se_pilot_ii_data_ptr[i2c_adap->nr].continuous_bus_reset = 0; // The bus is ok now, so initialize the continuous counter.
	se_pilot_ii_data_ptr[i2c_adap->nr].bus_timeout_count = 0;
	se_pilot_ii_data_ptr[i2c_adap->nr].continuous_bus_timeout_reset = 0;

	return(num);
}


int pilot_ii_smb_xfer( struct i2c_adapter *i2c_adap, u16 addr, unsigned short flags,
                            char read_write, u8 command, int size, union i2c_smbus_data * data)
                    
{


    unsigned char msgbuf0[I2C_SMBUS_BLOCK_MAX+3]; //msg0
    unsigned char msgbuf1[I2C_SMBUS_BLOCK_MAX+2]; //msg1
    int num = read_write == I2C_SMBUS_READ?2:1;
    struct i2c_msg msg[2] = { { addr, flags, 1, msgbuf0 }, 
                              { addr, flags | I2C_M_RD, 0, msgbuf1 }
                            };
    int i;

    int retval = 0;
    int ret = 0;
    
    u8 partial_pec = 0;

    msgbuf0[0] = command;
    switch(size) {
    case I2C_SMBUS_QUICK:
        msg[0].len = 0;
        /* Special case: The read/write field is used as data */
        msg[0].flags = flags | (read_write==I2C_SMBUS_READ)?I2C_M_RD:0;
        msg[0].addr = addr;
        num = 1;
        se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
        se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        break;
    case I2C_SMBUS_BYTE:
        if (read_write == I2C_SMBUS_READ) {
            /* Special case: only a read! */
            msg[0].flags = I2C_M_RD | flags;
            msg[0].addr = addr;
            msg[0].len = 1; //???????????????????????????????
            num = 1;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        }
        else
        {
            msg[0].flags = 0 | flags;
            msg[0].addr = addr;
            num = 1;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        }
        break;
    case I2C_SMBUS_BYTE_DATA:
        if (read_write == I2C_SMBUS_READ)
        {
            msg[0].len = 1;
            msg[0].addr = addr;
            msg[0].flags = 0 | flags;
            
            msg[1].len = 1;
            msg[1].addr = addr;
            msg[1].flags = I2C_M_RD | flags;

            num = 2;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        }
            
        else {
            msg[0].addr = addr;
            msg[0].flags = 0 | flags;
            msg[0].len = 2;
            msgbuf0[1] = data->byte;
            num = 1;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        }
        break;
    case I2C_SMBUS_WORD_DATA:
        if (read_write == I2C_SMBUS_READ)
        {
            msg[0].len=1;
            msg[0].addr = addr;
            msg[0].flags = 0 | flags;
            
            msg[1].len = 2;
            msg[1].addr = addr;
            msg[1].flags = I2C_M_RD | flags;
            num = 2;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        }
        else {
            msg[0].len=3;
            msg[0].addr = addr;
            msg[0].flags = 0 | flags;
            msgbuf0[1] = data->word & 0xff;
            msgbuf0[2] = (data->word >> 8) & 0xff;
            num = 1;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        }
        break;
    case I2C_SMBUS_PROC_CALL:
        num = 2; /* Special case */
        read_write = I2C_SMBUS_READ;
        msg[0].len = 3;
        msg[0].addr = addr;
        msg[0].flags = 0 | flags;
        msgbuf0[1] = data->word & 0xff;
        msgbuf0[2] = (data->word >> 8) & 0xff;
        msg[1].len = 2;
        msg[1].addr = addr;
        msg[1].flags = I2C_M_RD | flags;
        se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
        se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        break;
    case I2C_SMBUS_BLOCK_DATA:
        if (read_write == I2C_SMBUS_READ) {

            
            msg[0].len = 1;
            msg[0].addr = addr;
            msg[0].flags = 0 | flags;
            
            //real length should be send by device in the first received byte, set length to 2 such that 1st read generates RX_FULL interrupt 
#if defined SOC_PILOT_IV
            msg[1].len = 2;
#else
            msg[1].len = 1;
#endif
            msg[1].addr = addr;
            msg[1].flags = I2C_M_RD | flags;
            num = 2;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 1;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].master_read_with_PEC = flags & I2C_CLIENT_PEC? 1:0;
        }
        else {
            msg[0].len = data->block[0] + 2;
            if (msg[0].len > I2C_SMBUS_BLOCK_MAX + 2) {
                dev_err(&i2c_adap->dev, "smbus_access called with "
                       "invalid block write size (%d)\n",
                       data->block[0]);
                return -1;
            }
            for (i = 1; i < msg[0].len; i++)
                msgbuf0[i] = data->block[i-1];
            num = 1;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
            se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 0;
        }
        break;
    case I2C_SMBUS_BLOCK_PROC_CALL:
        num = 2; /* Special case */
        read_write = I2C_SMBUS_READ;
        
        msg[0].len = data->block[0] + 2;
        msg[0].addr = addr;
        msg[0].flags = 0 | flags;
        
        

        if (msg[0].len < 1) {
            dev_err(&i2c_adap->dev, "smbus_access called with "
                       "invalid block write size (%d)\n",
                       data->block[0]);
            return -1;
        }
        for (i = 1; i < msg[0].len; i++)
            msgbuf0[i] = data->block[i-1];

        //real length should be send by device in the first received byte, set length to 2 such that 1st read generates RX_FULL interrupt
#if defined SOC_PILOT_IV        
        msg[1].len = 2;
#else
        msg[1].len =1;
#endif
        msg[1].addr = addr;
        msg[1].flags = I2C_M_RD | flags;
        se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0;
        se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call= 1;
        se_pilot_ii_data_ptr[i2c_adap->nr].master_read_with_PEC = flags & I2C_CLIENT_PEC? 1:0;
        break;
    case I2C_SMBUS_I2C_BLOCK_DATA:
        #if 0
        if (read_write == I2C_SMBUS_READ) {
            msg[1].len = I2C_SMBUS_BLOCK_MAX;
            msg[1].addr = addr;
            msg[1].flags = I2C_M_RD | flags;
            
        } else {
            msg[0].flags = 0 | flags;
            msg[0].addr = addr;
            msg[0].len = data->block[0] + 1;
            if (msg[0].len > I2C_SMBUS_BLOCK_MAX + 1) {
                dev_err(&i2c_adap->dev, "i2c_smbus_xfer_emulated called with "
                       "invalid block write size (%d)\n",
                       data->block[0]);
                return -1;
            }
            for (i = 1; i <= data->block[0]; i++)
                msgbuf0[i] = data->block[i];
        }
        #endif
        break;
    default:
        dev_err(&i2c_adap->dev, "smbus_access called with invalid size (%d)\n",
               size);
        return -1;
    }

    
    
    i = ((flags & I2C_CLIENT_PEC) && size != I2C_SMBUS_QUICK
                          && size != I2C_SMBUS_I2C_BLOCK_DATA); 

    
    if (i) {
        /* Compute PEC if first message is a write */
        if (!(msg[0].flags & I2C_M_RD)) {
            if (num == 1) /* Write only */
                i2c_smbus_add_pec(&msg[0]);
            else /* Write followed by read */
                    partial_pec = i2c_smbus_msg_pec(0, &msg[0]);
        }
            /* Ask for PEC if last message is a read */
        if (msg[num-1].flags & I2C_M_RD)
            msg[num-1].len++;
    }

    
    
    if (size == 0 )
    {
        se_pilot_ii_data_ptr[i2c_adap->nr].TX_len = 0;
        se_pilot_ii_data_ptr[i2c_adap->nr].TX_index = 0;
        
        se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len = 0;
        se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_index = 0;
        //retval = send_slave_addr(i2c_adap, msg[0].addr, msg[0].flags);
        dev_err(&i2c_adap->dev, "smbus_access does not support size (%d)\n",
                       size);
            return -1;
                
    }

    else
    {
        retval = pilot_ii_master_xfer(i2c_adap, msg, num);

        /* these are flags used by smbus, clear them after Smbus transfer finish*/
        se_pilot_ii_data_ptr[i2c_adap->nr].block_read = 0; 
        se_pilot_ii_data_ptr[i2c_adap->nr].block_proc_call = 0;
        se_pilot_ii_data_ptr[i2c_adap->nr].master_read_with_PEC = 0;
        /* Set the RX fifo threshold level for interrupt generation */
        i2c_pilot_ii_write_reg (i2c_adap->nr, DEFAULT_RX_THRESHOLD,I2C_RX_TL_REG);
        if (retval < 0)
            return -1;
        
    }

    
    
    
    /* Check PEC if last message is a read */
    if (i && (msg[num-1].flags & I2C_M_RD)) {
        //printk(" msg[num-1] data1 = %x data2 = %x, data3=%x\n",msgbuf1[0], msgbuf1[1], msgbuf1[2]);
        if (((size == I2C_SMBUS_BLOCK_DATA) || (size == I2C_SMBUS_BLOCK_PROC_CALL)) && ((flags & I2C_CLIENT_PEC)))
            msg[num-1].len = se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len;
        if (i2c_smbus_check_pec(partial_pec, &msg[num-1]) < 0) {
            printk("%s: Line %d: Mode %d, SMBus PEC error\n", __FUNCTION__, __LINE__, size);
            return -1;
        }
    }

    
    

    if (read_write == I2C_SMBUS_READ)
        switch(size) {
            case I2C_SMBUS_BYTE:
                data->byte = msgbuf0[0];
                ret = 1;
                break;
            case I2C_SMBUS_BYTE_DATA:
                data->byte = msgbuf1[0];
                ret = 1;
                break;
            case I2C_SMBUS_WORD_DATA: 
                data->word = msgbuf1[1]<<8 | msgbuf1[0];
                ret = 2;
                break;
            case I2C_SMBUS_PROC_CALL:
                data->word = msgbuf1[1]<< 8 | msgbuf1[0] ;
                ret = 2;
                break;
            case I2C_SMBUS_BLOCK_DATA:

                
                for (i = 0; i < se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len; i++)
                {
                    data->block[i] = msgbuf1[i];
                    
                }
                ret = (data->block[0]==(I2C_SMBUS_BLOCK_MAX+2))?0:se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len;
                break;
            case I2C_SMBUS_BLOCK_PROC_CALL:
                for (i = 0; i < se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len; i++)
                {
                    data->block[i] = msgbuf1[i];
                    
                }
                ret = (data->block[0]==(I2C_SMBUS_BLOCK_MAX+2))?0:se_pilot_ii_data_ptr[i2c_adap->nr].MasterRX_len;
                break;
            case I2C_SMBUS_I2C_BLOCK_DATA:
                #if 0
                /* fixed at 32 for now */
                data->block[0] = I2C_SMBUS_BLOCK_MAX;
                for (i = 0; i < I2C_SMBUS_BLOCK_MAX; i++)
                    data->block[i+1] = msgbuf1[i];
                #endif
                ret =I2C_SMBUS_BLOCK_MAX;
                break;
        }

    
    return ret;
}


int pilot_ii_slave_recv( struct i2c_adapter *i2c_adap,
                    char *buf,int num)
{
  int len=-1;
  unsigned long flags;
  int FifoPtr;
  //  int i=0;

  /* Copy to user space buffer */ 
  if (!wait_event_interruptible(se_pilot_ii_data_ptr[i2c_adap->nr].pilot_ii_slave_data_wait, (se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_Entries > 0)) )
    {
	spin_lock_irqsave( &se_pilot_ii_data_ptr[i2c_adap->nr ].data_lock , flags);
 
      FifoPtr = se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_Reader;
      len = se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_len[FifoPtr];
      if (len > 0)
	{
      len = (len > num)?num:len;
	  memcpy(buf,se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_data[FifoPtr],len);
	  se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_len[FifoPtr] = 0;
	}
      else
	{
	  len  = -1;
	}


      if ((++se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_Reader) == MAX_FIFO_LEN)
	se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_Reader = 0;
      se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_Entries--;

	/*Enable the slave mode again, after making room for next packet in Rx Buffer.
	 * Because I2C Slave mode will be disabled for bus when Rx Buffer is full.
	 */
	if(se_pilot_ii_data_ptr[i2c_adap->nr].SlaveRX_Entries == (MAX_FIFO_LEN - 1))
        {
                i2c_pilot_ii_enable_slave(i2c_adap->nr);
        }

	  if(len > 0)
	  {
		  log_i2c_plain(i2c_adap, buf, len);		
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
		  se_pilot_ii_data_ptr[i2c_adap->nr].stat.slave_rx++;
			se_pilot_ii_data_ptr[i2c_adap->nr].stat.count++;
			// Do overflow protection with lower limit to reduce integration time and time to reach an predefined error ratio
	        if (se_pilot_ii_data_ptr[i2c_adap->nr].stat.count > 10000) {
	        	se_pilot_ii_data_ptr[i2c_adap->nr].stat.count = (se_pilot_ii_data_ptr[i2c_adap->nr].stat.count + 9) / 10;
	        	se_pilot_ii_data_ptr[i2c_adap->nr].stat.error = (se_pilot_ii_data_ptr[i2c_adap->nr].stat.error + 9) / 10;
	        }
#endif
	}
	 spin_unlock_irqrestore( &se_pilot_ii_data_ptr[i2c_adap->nr ].data_lock , flags);	
  }

  return (len==MAX_IPMB_MSG_SIZE)?0:len;
}

int pilot_ii_mctp_recv(struct i2c_adapter *i2c_adap,char *buf)
{
    int len=-1;
    unsigned long flags;
    int FifoPtr;

retry:

    spin_lock_irqsave( &se_pilot_ii_data_ptr[i2c_adap->nr ].data_lock , flags);
    if (se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Entries)
    {
        FifoPtr = se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Reader;
        len = se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Len[FifoPtr];
        if (len > 0)
        {
          memcpy(buf,se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_data[FifoPtr],len);
          se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Len[FifoPtr] = 0;
        }
        else
        {
          len  = -1;
        }

        if ((++se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Reader) == MAX_FIFO_LEN)
            se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Reader = 0;

        se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Entries--;

        if(se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Entries == (MAX_FIFO_LEN - 1))
        {
                i2c_pilot_ii_enable_slave(i2c_adap->nr);
        }
        if(len > 0)
        {
                log_i2c_plain(i2c_adap, buf, len);
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
                se_pilot_ii_data_ptr[i2c_adap->nr].stat.slave_rx++;
#endif
        }

        spin_unlock_irqrestore( &se_pilot_ii_data_ptr[i2c_adap->nr ].data_lock , flags);
    }
    else
    {
        spin_unlock_irqrestore( &se_pilot_ii_data_ptr[i2c_adap->nr ].data_lock , flags);
        if (wait_event_interruptible_timeout(se_pilot_ii_data_ptr[i2c_adap->nr].pilot_ii_mctp_data_wait,(se_pilot_ii_data_ptr[i2c_adap->nr].MCTPRX_Entries > 0),msecs_to_jiffies(MCTP_I2C_RECV_TIMEOUT)) <= 0)
           return -1;
        goto retry;
    }

    return len;
}

int pilot_ii_slave_send( struct i2c_adapter *i2c_adap, 
                    char *buf, int num )
{ 
	int size = 0; 
	int i; 

	/* Copy from user space */ 
	size = buf[0]; 
	size += 2;
	
	se_pilot_ii_data_ptr[i2c_adap->nr].Slave_TX_len = SLAVETX_MAX_RES_SIZE;

	for (i=0; i < SLAVETX_MAX_RES_SIZE; i++)
	{
		if (i < size)
			se_pilot_ii_data_ptr[i2c_adap->nr].Slave_TX_data[i] = buf[i];
		else
			se_pilot_ii_data_ptr[i2c_adap->nr].Slave_TX_data[i] = 0xff;
	}
 
	//Response data is ready
	i2cTXEMPTY_slave_process(i2c_adap->nr, 0);
	i2c_pilot_ii_enable_interrupt(i2c_adap->nr,TX_EMPTY);	
	se_pilot_ii_data_ptr[i2c_adap->nr].RD_REQ_Pending = 0x0;

	kfree(buf); 
	
	return size;
} 

