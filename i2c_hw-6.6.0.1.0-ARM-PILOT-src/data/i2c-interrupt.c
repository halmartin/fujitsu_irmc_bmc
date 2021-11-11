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
extern int irq[ BUS_COUNT ];
extern int fifo_depth[ BUS_COUNT ];
#ifdef INT_MONITOR
struct i2c_pilot_ii_interrupt_monitor int_monitor;
#endif

static void i2cSTOPDET_slave_process(int bus,u32 status);

void
i2cTXEMPTY_slave_process(int bus, u32 status)
{
  unsigned volatile int fifo_status ;
  
  se_pilot_ii_data_ptr[bus].Slave_TX_index = 0;
  fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
  while( (fifo_status & TFNF)&&(se_pilot_ii_data_ptr[bus].Slave_TX_index < se_pilot_ii_data_ptr[bus].Slave_TX_len))
    {
#ifdef INT_MONITOR_DATA
      if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
      {
    	  int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = se_pilot_ii_data_ptr[bus].Slave_TX_data[se_pilot_ii_data_ptr[bus].Slave_TX_index]&0xff;
    	  int_monitor.int_data[int_monitor.start_position].data_count++;
      }
#endif
      i2c_pilot_ii_write_reg(bus,(se_pilot_ii_data_ptr[bus].Slave_TX_data[se_pilot_ii_data_ptr[bus].Slave_TX_index]&0xff) ,I2C_DATA_CMD_REG);
      se_pilot_ii_data_ptr[bus].Slave_TX_index ++;
      fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
    }
  
  
  if ( se_pilot_ii_data_ptr[bus].Slave_TX_index >= se_pilot_ii_data_ptr[bus].Slave_TX_len)
    {
      i2c_pilot_ii_disable_interrupt(bus, TX_EMPTY);
    }
  
  return;
}




static void i2cTXEMPTY_master_process(int bus, u32 status)
{
  unsigned volatile int fifo_status, txflr = 0;
  
  if(se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag==MASTER_XMIT)
    {
      fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
      txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG);
      
      if ((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len))
    	  se_pilot_ii_data_ptr[bus].MasterXmit = 1;
#if defined SOC_PILOT_IV
      while((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len -1))
      {
#else /* Pre Pilot 4 an auto STOP is generated when the TX FIFO is empty */
      while((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len))
	  {
        /* When we want to do a Master transaction that is more than the FIFO length (32 or 128 bytes, based on the bus), then
         * we need to be cautious that we dont send that single master transaction as two separate transactions due to time delay
         * in writing the remaining bytes into the TXFIFO.
         * For eg. in FIFO length 32 - if we are to send 60 bytes Master transcation, we first
         * write 32 bytes into the FIFO, and when the TX interrupt comes back to us, to fill the remaining bytes
         * we check if there is previous data in the FIFO, so that we are sure that this goes out as single master transaction.
         * If the TXFLR is empty, then the controller as alreayd put a STOP after 32 bytes, and then filling the remaining 28 bytes (out of the 60)
         * goes as second master transaction. In order to avoid that, we do the below check... Hopefully you understand.. ;)
         */
	  if( ( (txflr == 0) && (se_pilot_ii_data_ptr[bus].TX_index != 0) ) == 0)
#endif	/* #if defined SOC_PILOT_IV */
	  {
#ifdef INT_MONITOR_DATA
		  if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
	      {
	    	  int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index];
	    	  int_monitor.int_data[int_monitor.start_position].data_count++;
	      }
#endif
		  i2c_pilot_ii_write_reg(bus,se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index],I2C_DATA_CMD_REG);
	  }
	  
	  txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG);
	  fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
	  se_pilot_ii_data_ptr[bus].TX_index ++;
	  }
#if defined SOC_PILOT_IV
		if ((fifo_status & TFNF) && (se_pilot_ii_data_ptr[bus].TX_index >= se_pilot_ii_data_ptr[bus].TX_len - 1))
#else
      if ( se_pilot_ii_data_ptr[bus].TX_index >= se_pilot_ii_data_ptr[bus].TX_len)
#endif
	{
#if defined SOC_PILOT_IV
			i2c_pilot_ii_write_reg(bus, (se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index] | CMDSTOP), I2C_DATA_CMD_REG);
			se_pilot_ii_data_ptr[bus].TX_index ++;
#endif
	  i2c_pilot_ii_disable_interrupt(bus, TX_EMPTY);
	}
    }


  //Master Reads
  else if(se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag == MASTER_RECV)
    {
      fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
#if defined SOC_PILOT_IV
	  while( (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index<se_pilot_ii_data_ptr[bus].MasterRX_len - 1)&&(fifo_status &TFNF) )
	{
#else /* Pre Pilot 4 an auto STOP is generated when the TX FIFO is empty */
      while( (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index<se_pilot_ii_data_ptr[bus].MasterRX_len)&&(fifo_status &TFNF) )
	{
	 /* Check is TX FIFO empty and STOP has already been generated (see above) */
	 if ((txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG)) == 0)
	 {
	   /* Truncated Master Transfer */
	   se_pilot_ii_data_ptr[bus].MasterTruncated = 1;
	 }
	 else
#endif	/* #if defined SOC_PILOT_IV */
	 {
#ifdef INT_MONITOR_DATA
	  if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
      {
    	  int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = 0x100;
    	  int_monitor.int_data[int_monitor.start_position].data_count++;
      }
#endif
	  i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
	 }  /* TX FIFO empty check */
	  se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
	  fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
	}

#if defined SOC_PILOT_IV
      if((fifo_status & TFNF) && (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index >= se_pilot_ii_data_ptr[bus].MasterRX_len -1))
#else
      if(se_pilot_ii_data_ptr[bus].Master_rd_cmd_index>=se_pilot_ii_data_ptr[bus].MasterRX_len)
#endif	  
	{
#if defined SOC_PILOT_IV
		i2c_pilot_ii_write_reg(bus, (0x100 | CMDSTOP), I2C_DATA_CMD_REG);
		se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
#endif
		i2c_pilot_ii_disable_interrupt(bus, TX_EMPTY);
	}
      
    }
  //Restart MWR
  else if(se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag == RESTART_MWMR)
  {
      if( se_pilot_ii_data_ptr[bus].TX_index< se_pilot_ii_data_ptr[bus].TX_len)
	{
	  fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
	  while((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len))
	    {
#if !defined SOC_PILOT_IV	/* Pre Pilot 4 an auto STOP is generated when the TX FIFO is empty */
	     /* Check is TX FIFO empty and STOP has already been generated (see above) */
	     if (((txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG)) == 0) && (se_pilot_ii_data_ptr[bus].TX_index != 0))
	     {
	         /* Truncated Master Transfer */
	         /* when txflr equals to 0, it doesn't appear to have generated STOP 
	          * For safe play, the No MasterTruncated case is only applied in SMBus Block Read and Block Procedure Call
	          * Further investigation needs to be done */
	         
	         if (se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1)
	             i2c_pilot_ii_write_reg(bus,se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index],I2C_DATA_CMD_REG);
	         else
	             se_pilot_ii_data_ptr[bus].MasterTruncated = 1;
	         
	     }
	     else
#endif	/* #if !defined SOC_PILOT_IV */
	     {
#ifdef INT_MONITOR_DATA
		  if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
	      {
	    	  int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index];
	    	  int_monitor.int_data[int_monitor.start_position].data_count++;
	      }
#endif
		  i2c_pilot_ii_write_reg(bus,se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index],I2C_DATA_CMD_REG);
	     }  /* TX FIFO empty check */
	      se_pilot_ii_data_ptr[bus].TX_index ++;
	      fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
	    }
	}
      else
	{
      if ((se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) && se_pilot_ii_data_ptr[bus].Master_rd_cmd_index==0) {
          // reinitialize MasterRX_len irrespective of PEC of SMBus
#if defined SOC_PILOT_IV  
              se_pilot_ii_data_ptr[bus].MasterRX_len = 2;
#else
              se_pilot_ii_data_ptr[bus].MasterRX_len = 1; 
#endif
      }
	  fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
#if defined SOC_PILOT_IV
	  while( (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index<se_pilot_ii_data_ptr[bus].MasterRX_len - 1) && (fifo_status &TFNF) )
	    {
#else /* Pre Pilot 4 an auto STOP is generated when the TX FIFO is empty */
	  while( (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index<se_pilot_ii_data_ptr[bus].MasterRX_len)&&(fifo_status &TFNF) )
	    {
	     /* Check is TX FIFO empty and STOP has already been generated (see above) */
	     if (((txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG)) == 0) && (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index !=0))
	     {	         
	         /* Truncated Master Transfer */
             if (se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1)
                 i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
             else
                 se_pilot_ii_data_ptr[bus].MasterTruncated = 1;
	     }
	     else
#endif	/* #if defined SOC_PILOT_IV */
	     {
#ifdef INT_MONITOR_DATA
		  if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
	      {
	    	  int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = 0x100;
	    	  int_monitor.int_data[int_monitor.start_position].data_count++;
	      }
#endif
	      i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
	     }  /* TX FIFO empty check */
	      se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
	      fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
	    }
#if defined SOC_PILOT_IV
	  if((fifo_status & TFNF) && (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index >= se_pilot_ii_data_ptr[bus].MasterRX_len - 1))
#else
	  if(se_pilot_ii_data_ptr[bus].Master_rd_cmd_index>=se_pilot_ii_data_ptr[bus].MasterRX_len)
#endif	  
	    {
#if defined SOC_PILOT_IV
	      if (!((se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) && se_pilot_ii_data_ptr[bus].Master_rd_cmd_index==1)) {
			i2c_pilot_ii_write_reg(bus, (0x100 | CMDSTOP), I2C_DATA_CMD_REG);
			se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
	      }
	      i2c_pilot_ii_disable_interrupt(bus, TX_EMPTY);
#endif	      
	    }
	}
    }
  //Restart MRW
  else if( (se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag==RESTART_MRMW) )
    {
      if(se_pilot_ii_data_ptr[bus].Master_rd_cmd_index<se_pilot_ii_data_ptr[bus].MasterRX_len)
	{
	  fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
	  while( (se_pilot_ii_data_ptr[bus].Master_rd_cmd_index<se_pilot_ii_data_ptr[bus].MasterRX_len)&&(fifo_status &TFNF) )
	    {
#if !defined SOC_PILOT_IV	/* Pre Pilot 4 an auto STOP is generated when the TX FIFO is empty */
	     /* Check is TX FIFO empty and STOP has already been generated (see above) */
	     if ((txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG)) == 0)
	     {
	       /* Truncated Master Transfer */
	       se_pilot_ii_data_ptr[bus].MasterTruncated = 1;
	     }
	     else
#endif	/* #if !defined SOC_PILOT_IV */
	     {
#ifdef INT_MONITOR_DATA
		  if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
	      {
	    	  int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = 0x100;
	    	  int_monitor.int_data[int_monitor.start_position].data_count++;
	      }
#endif
	      i2c_pilot_ii_write_reg(bus, 0x100, I2C_DATA_CMD_REG);
	     }  /* TX FIFO empty check */
	      se_pilot_ii_data_ptr[bus].Master_rd_cmd_index++;
	      fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
	    }
	}
      else
	{
	  fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG);
#if defined SOC_PILOT_IV
	  while((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len - 1))
	    {
#else /* Pre Pilot 4 an auto STOP is generated when the TX FIFO is empty */
	  while((fifo_status & TFNF)&& (se_pilot_ii_data_ptr[bus].TX_index < se_pilot_ii_data_ptr[bus].TX_len))
	    {
	     /* Check is TX FIFO empty and STOP has already been generated (see above) */
	     if ((txflr = i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG)) == 0)
	     {
	      /* Truncated Master Transfer */
	      se_pilot_ii_data_ptr[bus].MasterTruncated = 1;
	     }
	     else
#endif	/* #if defined SOC_PILOT_IV */
	     {
#ifdef INT_MONITOR_DATA
		  if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
	      {
	    	  int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index];
	    	  int_monitor.int_data[int_monitor.start_position].data_count++;
	      }
#endif
	      i2c_pilot_ii_write_reg(bus,se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index],I2C_DATA_CMD_REG);
	     }  /* TX FIFO empty check */
	      se_pilot_ii_data_ptr[bus].TX_index ++;
	      fifo_status = i2c_pilot_ii_read_reg(bus,I2C_STATUS_REG); 
	    }
#if defined SOC_PILOT_IV	  
	  if ((fifo_status & TFNF) && (se_pilot_ii_data_ptr[bus].TX_index >= se_pilot_ii_data_ptr[bus].TX_len - 1))
#else
	  if ( se_pilot_ii_data_ptr[bus].TX_index >= se_pilot_ii_data_ptr[bus].TX_len)
#endif	  
	    {
#if defined SOC_PILOT_IV	
			i2c_pilot_ii_write_reg(bus, (se_pilot_ii_data_ptr[bus].TX_data[se_pilot_ii_data_ptr[bus].TX_index] | CMDSTOP), I2C_DATA_CMD_REG);
			se_pilot_ii_data_ptr[bus].TX_index ++;
#endif
	      i2c_pilot_ii_disable_interrupt(bus, TX_EMPTY);
	    }
	}
    }
  else
    {
      i2c_pilot_ii_disable_interrupt(bus, TX_EMPTY);
    }
  
  return;
}


static int 
i2cRXFULL_master_process(int bus,u32 status)
{
	unsigned int		i;
	unsigned int RxFlr;
 	RxFlr = i2c_pilot_ii_read_reg(bus,I2C_RXFLR_REG);

	for(i=0;i<RxFlr;i++)
	  {
	    /* Fix I2C bus monitoring data capturing */
	    se_pilot_ii_data_ptr[bus].MasterRX_data[se_pilot_ii_data_ptr[bus].MasterRX_index] =i2c_pilot_ii_read_reg(bus,I2C_DATA_CMD_REG);
#ifdef INT_MONITOR_DATA
	    if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
	    {
	    	int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = se_pilot_ii_data_ptr[bus].MasterRX_data[se_pilot_ii_data_ptr[bus].MasterRX_index];
	    	int_monitor.int_data[int_monitor.start_position].data_count++;
	    }
#endif
	    //se_pilot_ii_data_ptr[bus].MasterRX_data[se_pilot_ii_data_ptr[bus].MasterRX_index] =i2c_pilot_ii_read_reg(bus,I2C_DATA_CMD_REG);
	    se_pilot_ii_data_ptr[bus].MasterRX_index ++;
	  }
	
    if ((se_pilot_ii_data_ptr[bus].block_read == 1 || se_pilot_ii_data_ptr[bus].block_proc_call == 1) && se_pilot_ii_data_ptr[bus].MasterRX_index==1)
    {
        se_pilot_ii_data_ptr[bus].MasterRX_len = se_pilot_ii_data_ptr[bus].MasterRX_data[0] +1;
        if (se_pilot_ii_data_ptr[bus].master_read_with_PEC)
            se_pilot_ii_data_ptr[bus].MasterRX_len += 1;
        /* Set the RX fifo threshold level for interrupt generation */
        i2c_pilot_ii_write_reg (bus, DEFAULT_RX_THRESHOLD,I2C_RX_TL_REG);
        /* For Tx_len is greater than Fifo depth, TX_EMPTY is first disabled in i2cTXEMPTY_master_process just before 1st read 
         * where real MasterRX_len is obtained. Re-enabling TX_EMPTY to issue remaining read commands*/
        if (se_pilot_ii_data_ptr[bus].MasterRX_index < se_pilot_ii_data_ptr[bus].MasterRX_len )
            i2c_pilot_ii_enable_interrupt(bus,TX_EMPTY);
    }   
	return 0;
}

static int 
i2cRXFULL_slave_process(int bus,u32 status)
{
  unsigned int		i;
  unsigned int RxFlr;
  
 	RxFlr = i2c_pilot_ii_read_reg(bus,I2C_RXFLR_REG);  

 	/* Leave a byte in RX FIFO to avoid concatenation of data from back-to-back
 	 * slave transfers if this is the end of a slave transfer and STOP is not yet asserted */
 	if ((RxFlr > 0) && ((status & (RX_FULL | STOP_DET)) == RX_FULL)) RxFlr--;

	for(i=0;i<RxFlr;i++)
	  {
	    /* Low risk but check for a buffer overflow and discard remaining data if full */
	    if (se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index >= TRANSFERSIZE)
	    {
	    	i2c_pilot_ii_read_reg(bus,I2C_DATA_CMD_REG);
	    	continue;
	    }

		/* Fix I2C bus monitoring data capturing */
	    se_pilot_ii_data_ptr[bus].Linear_SlaveRX_data[se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index] =i2c_pilot_ii_read_reg(bus,I2C_DATA_CMD_REG);
#ifdef INT_MONITOR_DATA
	    if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
	    {
	    	int_monitor.int_data[int_monitor.start_position].data_bytes[int_monitor.int_data[int_monitor.start_position].data_count] = se_pilot_ii_data_ptr[bus].Linear_SlaveRX_data[se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index];
	    	int_monitor.int_data[int_monitor.start_position].data_count++;
	    }
#endif
	    se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index++;
	  }
	return 0;
}

#ifdef CONFIG_SPX_FEATURE_MCTP_SUPPORT
static void (*mctp_slave_complete_callback)(int bus) = NULL;

void i2c_set_mctp_complete_callback(void (*func)(int bus))
{
	mctp_slave_complete_callback = func;
}

EXPORT_SYMBOL_GPL(i2c_set_mctp_complete_callback);
#endif


static void 
i2cSTOPDET_slave_process_no_pipe_read(int bus,u32 status)
{
    int FifoPtr;
    unsigned char *DataBuffer;
	unsigned long length = 0, flags;
#ifdef CONFIG_SPX_FEATURE_MCTP_SUPPORT
	int endOfMsg = 0;
#endif

	spin_lock_irqsave( &se_pilot_ii_data_ptr[bus].data_lock, flags );

    if (((se_pilot_ii_data_ptr[bus].SlaveRX_Entries == MAX_FIFO_LEN) && (se_pilot_ii_data_ptr[bus].MCTPRX_Entries == MAX_FIFO_LEN))||(se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index==0x0))
    {
        /* Return back */
        spin_unlock_irqrestore( &se_pilot_ii_data_ptr[bus].data_lock, flags );
        return;
    }

    if((se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index >= 7) && ((se_pilot_ii_data_ptr[bus].Linear_SlaveRX_data[2] & 0x01) || (se_pilot_ii_data_ptr[bus].Linear_SlaveRX_data[0] == 0x0F) ) && (se_pilot_ii_data_ptr[bus].SlaveTX_Enable == 0)) 
    {

#ifdef CONFIG_SPX_FEATURE_MCTP_SUPPORT

    	if (se_pilot_ii_data_ptr[bus].Linear_SlaveRX_data[6] & 0x40)
    		endOfMsg = 1;

    	if(se_pilot_ii_data_ptr[bus].MCTPRX_Entries == MAX_FIFO_LEN)
        {
            se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index = 0;
            spin_unlock_irqrestore( &se_pilot_ii_data_ptr[bus].data_lock, flags );
            return; 
        }
        FifoPtr = se_pilot_ii_data_ptr[bus].MCTPRX_Writer;
        DataBuffer = se_pilot_ii_data_ptr[bus].MCTPRX_data[FifoPtr];
        length = se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index;

        DataBuffer[0] = i2c_pilot_ii_read_reg(bus,I2C_SAR_REG) << 1;

        if(length)
            memcpy(&DataBuffer[1],se_pilot_ii_data_ptr[bus].Linear_SlaveRX_data,length);

        se_pilot_ii_data_ptr[bus].MCTPRX_Len[FifoPtr] = length + 1;
        se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index = 0;

        if((++se_pilot_ii_data_ptr[bus].MCTPRX_Writer) == MAX_FIFO_LEN)
            se_pilot_ii_data_ptr[bus].MCTPRX_Writer = 0;

        se_pilot_ii_data_ptr[bus].MCTPRX_Entries++;

        /* Check if buffer full. If so disable slave */
        if ((se_pilot_ii_data_ptr[bus].MCTPRX_Entries == MAX_FIFO_LEN) && (se_pilot_ii_data_ptr[bus].SlaveRX_Entries == MAX_FIFO_LEN))
        {
            i2c_pilot_ii_disable_slave(bus);
        }

        spin_unlock_irqrestore( &se_pilot_ii_data_ptr[bus].data_lock, flags );
        wake_up_interruptible(&se_pilot_ii_data_ptr[bus].pilot_ii_mctp_data_wait);
        
        if (endOfMsg && (mctp_slave_complete_callback != NULL)) {
        	mctp_slave_complete_callback(bus);
        }
#else
        spin_unlock_irqrestore( &se_pilot_ii_data_ptr[bus].data_lock, flags );
#endif
        
        return;
    }
    else
    {
        if (se_pilot_ii_data_ptr[bus].SlaveRX_Entries == MAX_FIFO_LEN)
        {
            se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index = 0;
            spin_unlock_irqrestore( &se_pilot_ii_data_ptr[bus].data_lock, flags );
            return;
        }
        FifoPtr = se_pilot_ii_data_ptr[bus].SlaveRX_Writer;

        se_pilot_ii_data_ptr[bus].SlaveRX_index[FifoPtr] = 0;

        DataBuffer = se_pilot_ii_data_ptr[bus].SlaveRX_data[FifoPtr];
        length = se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index;

        /* First byte of buffer should be filled with slave address */
        DataBuffer[se_pilot_ii_data_ptr[bus].SlaveRX_index[FifoPtr]++] =i2c_pilot_ii_read_reg(bus,I2C_SAR_REG) << 1;


        /* Read the Length and oopy to buffer */
        if(length)
            memcpy(&DataBuffer[1],se_pilot_ii_data_ptr[bus].Linear_SlaveRX_data,length);
        se_pilot_ii_data_ptr[bus].SlaveRX_index[FifoPtr] = se_pilot_ii_data_ptr[bus].SlaveRX_len[FifoPtr]= length+1;
        se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index = 0;

        if ((++se_pilot_ii_data_ptr[bus].SlaveRX_Writer) == MAX_FIFO_LEN)
            se_pilot_ii_data_ptr[bus].SlaveRX_Writer = 0;

        se_pilot_ii_data_ptr[bus].SlaveRX_Entries++;

        /* Don't disable slave, but instead discard new transactions (see above) */
#if 0
        /* Check if buffer full. If so disable slave */
        if ((se_pilot_ii_data_ptr[bus].SlaveRX_Entries == MAX_FIFO_LEN) && (se_pilot_ii_data_ptr[bus].MCTPRX_Entries == MAX_FIFO_LEN))
        {
            i2c_pilot_ii_disable_slave(bus);
        }
#endif

        spin_unlock_irqrestore( &se_pilot_ii_data_ptr[bus].data_lock, flags );		
        wake_up_interruptible(&se_pilot_ii_data_ptr[bus].pilot_ii_slave_data_wait);

        return;
    }
}

static void 
i2cSTOPDET_slave_process(int bus,u32 status)
{
	i2cRXFULL_slave_process(bus, status);
	i2cSTOPDET_slave_process_no_pipe_read (bus, status);
	return;

}

static void pilot_ii_handler(int bus)
{
    unsigned long		flags;
    unsigned int Txflr = 0, Rxflr = 0, int_flag = 0;
    unsigned int rxfull_clkstretch = 0; /* Record RX clock stretching */
    u32 status;
    static int rxfull_flag = 0;
    u32 fifo_size=fifo_depth[bus];
    u32 M_Rx_Len=se_pilot_ii_data_ptr[bus].MasterRX_len;
    u32 M_Tx_Len=se_pilot_ii_data_ptr[bus].TX_len;
    u32 OP_MODE=se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag;
    static int missing_stop_int[BUS_COUNT + 1] = {0};
#ifdef INT_MONITOR
    struct i2c_pilot_ii_interrupt_data *int_data = NULL;
    unsigned int monitor_flag = 0;
#endif
  
    spin_lock_irqsave( &se_pilot_ii_data_ptr[bus].i2c_irq_lock, flags);
    // printk("H %d\n",bus);

    /* Read interrupt status */
    status = i2c_pilot_ii_read_reg( bus, I2C_INTR_STAT_REG );
    Txflr = i2c_pilot_ii_read_reg(bus, I2C_TXFLR_REG);
    Rxflr = i2c_pilot_ii_read_reg(bus, I2C_RXFLR_REG);
#ifdef INT_MONITOR
    if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
    {
        if (int_monitor.monitor_count < MAX_INTERRUPT_MONITOR)
        {
            int_monitor.monitor_count++;
            int_monitor.start_position = int_monitor.monitor_count - 1;
            monitor_flag = 1;
        }
        memset (&int_monitor.int_data[int_monitor.start_position], 0, sizeof (struct i2c_pilot_ii_interrupt_data));
        int_data = &int_monitor.int_data[int_monitor.start_position];
        int_data->bus = bus;
        int_data->raw_int_stauts = i2c_pilot_ii_read_reg( bus, I2C_RAW_INTR_STAT_REG );
        int_data->status_reg = i2c_pilot_ii_read_reg( bus, I2C_STATUS_REG );
        int_data->tx_abrt_source_reg = i2c_pilot_ii_read_reg( bus, I2C_TX_ABORT_SOURCE_REG );
        int_data->rxflr_reg = Rxflr;
        int_data->jiffies_value = get_jiffies_64();
        int_data->int_serviced = status;
        int_data->send_receive = OP_MODE;
    }
#endif
	/* RX_DONE */
    if(status & RX_DONE)
    {
        //printk("I2C%d: RX_DONE\n",bus);
        i2c_pilot_ii_read_reg(bus, I2C_CLR_RX_DONE_REG);
    }
	
    /* STOP_DET generated */ 
    if(status & STOP_DET)
    {
        /* Clear the stop interrupt as soon as possible in the interrupt handle,
         * as there is a chance that we are missing the STOP_DET interrupt when 
         * Transmit and receive works in parallel.
         */
        if (!(status & START_DET))
            i2c_pilot_ii_read_reg(bus, I2C_CLR_START_DET_REG); // Clear the start as this stop may be for Master and slave.
        i2c_pilot_ii_read_reg(bus, I2C_CLR_STOP_DET_REG);
    }
    
    if (status & START_DET)
    {
        i2c_pilot_ii_disable_interrupt(bus, START_DET);
        i2c_pilot_ii_read_reg(bus, I2C_CLR_START_DET_REG);
        
        if (missing_stop_int[bus] == 1)
        {
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
            if ((Get_Clk_Streaching_info(bus)&0x4) == 0x4)
            {
                se_pilot_ii_data_ptr[bus].recover_clock_stretch++;
            }
#endif
            missing_stop_int[bus] = 0;
            // This interrupt will occur only when we miss stop interrupts for some case.
            if (rxfull_flag)
            {
                // case : RXFULL + STOP (Master & Slave), then next start no need to read the pipe. 
                rxfull_flag = 0;
                i2cSTOPDET_slave_process_no_pipe_read(bus,status);
            }
            else
            {
                // case : STOP (Master & Slave), then next start can induce the STOP_DET also will read the pipe.
                i2cSTOPDET_slave_process(bus,status);
            }
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
            se_pilot_ii_data_ptr[bus].recover_start_counter++;
#endif
            //printk (KERN_DEBUG "i2c[%d] : Start, %x, %x, %x, %x, %x\n", bus, Rxflr, Txflr, status, se_pilot_ii_data_ptr[bus].MasterXmit, OP_MODE);
        }
    }
    
    if(status & RX_OVER)
    {
        //printk("ERROR: I2C%d: RX_OVER\n",bus);
        i2c_pilot_ii_read_reg(bus, I2C_CLR_RX_OVER_REG);
    }
  
    Txflr = i2c_pilot_ii_read_reg(bus, I2C_TXFLR_REG);
    Rxflr = i2c_pilot_ii_read_reg(bus, I2C_RXFLR_REG);
    /* RX_FULL */
    if(status & RX_FULL)
    {
        if (missing_stop_int[bus] == 1)
        {
            //printk (KERN_DEBUG "i2c[%d] : RX_FULL, %x, %x, %x, %x, %x\n", bus, Rxflr, Txflr, status, se_pilot_ii_data_ptr[bus].MasterXmit, OP_MODE);
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
            se_pilot_ii_data_ptr[bus].recover_rxfull_counter++;
#endif
            i2c_pilot_ii_disable_interrupt(bus, START_DET);
            missing_stop_int[bus] = 0;
            int_flag = 1;
            rxfull_flag = 0;
        }
      
        if(OP_MODE == MASTER_RECV)
        {
            if( ( (Txflr<fifo_size)&&(Txflr<M_Rx_Len) )&&(( ( (M_Rx_Len-Txflr)==1)&&((Get_Clk_Streaching_info(bus)&0x6)==0x6) )==0x0)  )
            {
                i2cRXFULL_master_process(bus,status);
                int_flag = 0;
            }
        }
        else if( (OP_MODE == RESTART_MWMR)&&(Txflr<M_Rx_Len) )
        {
            i2cRXFULL_master_process(bus,status);
            int_flag = 0;
        }
        else if( (OP_MODE == RESTART_MRMW) )
        {
            if( ( (Txflr<fifo_size)&&(Txflr<(M_Rx_Len+M_Tx_Len)) )&&(( ( (M_Rx_Len-Txflr)==1)&&((Get_Clk_Streaching_info(bus)&0x6)==0x6) )==0x0)  )
            {
                i2cRXFULL_master_process(bus,status);
                int_flag = 0;
            }
        }
        else if ((status & STOP_DET) == 0)	/* Defer reading slave data to STOP handler, if set */
        {
            /* check if slave clock stretching is enabled indicating back-to-back transfers were detected */
            if ((Get_Clk_Streaching_info(bus) & 0x5) == 0x5)  {
                rxfull_clkstretch = 1;
            }
            i2cRXFULL_slave_process(bus,status);
            int_flag = 0;
        }
    }
  
    /* TX_EMPTY generated as master*/
    if(status & TX_EMPTY)
    {
        if (se_pilot_ii_data_ptr[bus].SlaveTX_Enable)
		{
			se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE;
		}
        if(se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag == SLAVE_XMIT)
        {
            i2cTXEMPTY_slave_process(bus,status);
        }
        else if ((status & TX_ABRT) == 0) // only if there is no master abort
        {
            i2cTXEMPTY_master_process(bus,status);
        }
    }
  
    Txflr=i2c_pilot_ii_read_reg(bus,I2C_TXFLR_REG);
    if (!(status & RX_FULL))
    {
        Rxflr = i2c_pilot_ii_read_reg(bus, I2C_RXFLR_REG);
    }
     
    /* STOP_DET generated */ 
    if (status & STOP_DET)
    {
        if (missing_stop_int[bus] == 1)
        {
            //printk (KERN_DEBUG "i2c[%d] : Stop, %x, %x, %x, %x, %x\n", bus, Rxflr, Txflr, status, se_pilot_ii_data_ptr[bus].MasterXmit, OP_MODE);
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
            se_pilot_ii_data_ptr[bus].recover_stop_counter++;
#endif
            i2c_pilot_ii_disable_interrupt(bus, START_DET);
            missing_stop_int[bus] = 0;
            int_flag = 1;
            rxfull_flag = 0;
        }
        se_pilot_ii_data_ptr[bus].RD_REQ_Pending=0x0;

      
        if(  ( (OP_MODE == MASTER_RECV)||(OP_MODE== RESTART_MRMW)||(OP_MODE==RESTART_MWMR))&&\
               (Txflr<M_Rx_Len) && ( ( ((M_Rx_Len-Txflr)==1)&& ((Get_Clk_Streaching_info(bus)&0x6)==0x6) )==0x0 )   )
        {
            i2cRXFULL_master_process(bus, status);
            se_pilot_ii_data_ptr[bus].op_status=status;
            /* A truncated master transfer is an error, fake an ABORT */
            if (se_pilot_ii_data_ptr[bus].MasterTruncated != 0) se_pilot_ii_data_ptr[bus].op_status |= TX_ABRT;
            se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE;
            wake_up( &se_pilot_ii_data_ptr[bus].pilot_ii_wait );
        }
        else if( ( (OP_MODE==MASTER_XMIT)||(OP_MODE==SLAVE_XMIT)||(OP_MODE==RESTART_MRMW))&&\
                 (se_pilot_ii_data_ptr[bus].TX_index >= M_Tx_Len) && (Txflr == 0) )
        {
            se_pilot_ii_data_ptr[bus].op_status=status;
            /* A truncated master transfer is an error, fake an ABORT */
            if (se_pilot_ii_data_ptr[bus].MasterTruncated != 0) se_pilot_ii_data_ptr[bus].op_status |= TX_ABRT;
            se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE;
            wake_up( &se_pilot_ii_data_ptr[bus].pilot_ii_wait );
        }
        if( ((se_pilot_ii_data_ptr[bus].MasterXmit == 1) && (OP_MODE == MASTER_XMIT) && (Txflr == 0) && (se_pilot_ii_data_ptr[bus].TX_index >= M_Tx_Len)) == 0 )
        {
            i2cSTOPDET_slave_process(bus,status);
            int_flag = 0;
        }
        /* If RX FIFO is not empty and activity bit is clear or RX clock stretching
         * is enabled (consecutive RX transfers detected) then process the slave STOP detect */
        else if ((Rxflr > 0) && (((i2c_pilot_ii_read_reg( bus, I2C_STATUS_REG ) & IC_STATUS_ACTIVITY) == 0) ||
                                 (rxfull_clkstretch || ((Get_Clk_Streaching_info(bus) & 0x5) == 0x5))))
        {
        	se_pilot_ii_data_ptr[bus].MasterXmit = 0;
        	i2cSTOPDET_slave_process(bus,status);
        	int_flag = 0;
        }
        else
        {
            se_pilot_ii_data_ptr[bus].MasterXmit = 0;
            if (int_flag == 1)
            {
                i2cSTOPDET_slave_process(bus,status);
                int_flag = 0;
            }
            /* Missing STOP only if activity is not set */
            else if ((Txflr == 0) && (Rxflr > 0) && ((i2c_pilot_ii_read_reg( bus, I2C_STATUS_REG ) & IC_STATUS_ACTIVITY) == 0))
            {
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
                if (status & RX_FULL)
                {
                    se_pilot_ii_data_ptr[bus].rxfull_stop_counter++;
                    rxfull_flag = 1;
                }
                se_pilot_ii_data_ptr[bus].missing_stop_counter++;
#endif
                missing_stop_int[bus] = 1;
                /* There is a chance that missing STOP interrupt in two parallel transactions, 
                 * so Enabling START interrupt to detect the missing STOP. This is to avoid checksum error.
                 * After Enabling START interrupt, it will be disabled when any of the following interrupt occurred,
                 * 1. RX_FULL - No missing STOP, so no special handling.
                 * 2. STOP - No missing STOP, so no special handling.
                 * 3. START - Real missing start interrupt and STOP will be handled in that case. 
                 */ 
                i2c_pilot_ii_enable_interrupt(bus, START_DET);
                //printk (KERN_DEBUG "i2c[%d] : MI, %x, %x, %x, %x\n", bus, Rxflr, Txflr, status, OP_MODE);
            }
        }
    }

    /* If no one read the RX-FIFO then the below function will read,
     * but this should never reach in any case just a fall back.
     */
    if (int_flag == 1)
    {
        printk (KERN_DEBUG "i2c%d : Missing interrupt error happened, but this should never occur...\n", bus);
        i2cSTOPDET_slave_process(bus,status);
    }

    //RD_REQ handler should be always after stop handler
    //there could be re-start case started by master as soon as we emptied the fifo
    //to get packet demarcation we need to read fifo after moving previous packet.
    if(status & RD_REQ)
    {
        se_pilot_ii_data_ptr[bus].RD_REQ_Pending=1;
        i2c_pilot_ii_read_reg(bus, I2C_CLR_RD_REQ_REG);
        i2cSTOPDET_slave_process(bus,status); 
        if (se_pilot_ii_data_ptr[bus].SlaveTX_Enable)
		{
			se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE_XMIT;
		}
        if(se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag == SLAVE_XMIT)
        {
            if (!se_pilot_ii_data_ptr[bus].SlaveTX_Enable)
			{
				i2cTXEMPTY_slave_process(bus,status);
				se_pilot_ii_data_ptr[bus].RD_REQ_Pending=0x0;
            
				if(se_pilot_ii_data_ptr[bus].Slave_TX_len>se_pilot_ii_data_ptr[bus].Slave_TX_index)
				{
					i2c_pilot_ii_enable_interrupt(bus,TX_EMPTY);
				}
			}
        }
        else
        {
            // If any case BMC gets a master read request then that read request 
            // will block the bus for further transaction, so BMC will send a dummy 
            // byte to clear the bus. Sending dummy byte 0x58 (X).
            i2c_pilot_ii_write_reg(bus, 0x0058 , I2C_DATA_CMD_REG);
            se_pilot_ii_data_ptr[bus].RD_REQ_Pending = 0x0;
        }
    }
  
    /* TX_ABRT geenrated as master */
    if(status & TX_ABRT)
    {
        se_pilot_ii_data_ptr[bus].abort_status= i2c_pilot_ii_read_reg(bus, I2C_TX_ABORT_SOURCE_REG );
        i2c_pilot_ii_read_reg(bus, I2C_CLR_TX_ABRT_REG );
        //printk("I2C%d: Received TX ABORT = 0x%x\n",bus,se_pilot_ii_data_ptr[bus].abort_status);
        /*Reset the MasterXmit Flag*/
        if(OP_MODE == MASTER_XMIT)
            se_pilot_ii_data_ptr[bus].MasterXmit = 0;
        
        i2c_pilot_ii_disable_interrupt(bus, TX_EMPTY);
        se_pilot_ii_data_ptr[bus].op_status = status;
        se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE;
        wake_up( &se_pilot_ii_data_ptr[bus].pilot_ii_wait );
    }

#ifdef INT_MONITOR
    if ((bus == int_monitor.bus) && (int_monitor.start_flag == 1))
    {
        int_data->txflr_reg = i2c_pilot_ii_read_reg( bus, I2C_TXFLR_REG );
        if (monitor_flag == 1)
        {
            int_monitor.start_position = 0;
        }
        else
        {
            int_monitor.start_position++;
            int_monitor.start_position %= MAX_INTERRUPT_MONITOR;
        }
    }
#endif  

    spin_unlock_irqrestore(&se_pilot_ii_data_ptr[bus].i2c_irq_lock, flags);

    return;
}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
irqreturn_t 
i2c_handler_0( int this_irq, void *dev_id)
{
	pilot_ii_handler(0);
	return IRQ_HANDLED;
}

irqreturn_t 
i2c_handler_1( int this_irq, void *dev_id)
{
	pilot_ii_handler(1);
	return IRQ_HANDLED;
}

irqreturn_t 
i2c_handler_2( int this_irq, void *dev_id)
{
	pilot_ii_handler(2);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_3( int this_irq, void *dev_id)
{
	pilot_ii_handler(3);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_4( int this_irq, void *dev_id)
{
	pilot_ii_handler(4);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_5( int this_irq, void *dev_id)
{
	pilot_ii_handler(5);
	return IRQ_HANDLED;
}

irqreturn_t 
i2c_handler_6( int this_irq, void *dev_id)
{
	pilot_ii_handler(6);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_7( int this_irq, void *dev_id)
{
	pilot_ii_handler(7);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_8( int this_irq, void *dev_id)
{
	pilot_ii_handler(8);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_9( int this_irq, void *dev_id)
{
	pilot_ii_handler(9);
	return IRQ_HANDLED;
}
#else
irqreturn_t 
i2c_handler_0( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(0);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_1( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(1);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_2( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(2);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_3( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(3);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_4( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(4);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_5( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(5);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_6( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(6);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_7( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(7);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_8( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(8);
	return IRQ_HANDLED;
}
irqreturn_t 
i2c_handler_9( int this_irq, void *dev_id,
                                 struct pt_regs *regs )
{
	pilot_ii_handler(9);
	return IRQ_HANDLED;
}
#endif



irqreturn_t
i2c_TimeOut_Handler( int this_irq, void *dev_id)
{
    unsigned int i;
    unsigned int status=0;
  
    

    //    printk("I2c Time out Intr status %x\n", *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) );
    
    for (i = 0;  i < BUS_COUNT;  i++)
      {
        if ((i ==8) || (i ==9))
            status = (*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE + I2C_COUNTER_STS_1) >> ((i-8)*4) ) & 0xf;
        else
            status = (*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) >>(i*4) ) & 0xf;
        if (status)
        /* Implement alternate "clock stretch" interrupt handler */
            Clock_Stretch_Timeout(i, status);
		//	  Common_I2c_Timeout(i , status);

      }

    return IRQ_HANDLED;
}

void Common_I2c_Timeout(int bus, int status)
{

 /* Read interrupt status */
  unsigned int irq_status = 0xffffffff; //i2c_pilot_ii_read_reg( bus, I2C_INTR_STAT_REG );
  unsigned long  flags;

  spin_lock_irqsave( &se_pilot_ii_data_ptr[bus].to_irq_lock , flags);
  if (status & I2C_TO_BITS)
    {
      i2c_init_internal_data(bus);
      i2c_init_hardware(bus);
      printk("I2c%d Ctrl Is Reseted\n",bus);
      
      //enable time out counters
      i2c_to_counter_en( bus);
      
      //Enable Time out interrupts
      i2c_time_out_interrupt(bus);
    }
  
  if( status & 0x1)
    {
      /* support I2C ports 8 and 9 */
      if ((bus >= 0) && (bus <= 7))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_MTO<<(bus*4));
      else if ((bus == 8) || (bus == 9))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_MTO<<((bus-8)*4));
      se_pilot_ii_data_ptr[bus].Time_Out |= TMEXT;
      printk("I2c%d MEXT expired\n",bus);
    }
  
  if( status & 0x2)
    {
      /* support I2C ports 8 and 9 */
      if ((bus >= 0) && (bus <= 7))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_STO<<(bus*4));
      else if ((bus == 8) || (bus == 9))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_STO<<((bus-8)*4));
      se_pilot_ii_data_ptr[bus].Time_Out |= TSEXT;
      printk("I2c%d SEXT is Expired\n",bus);
    }
  
  if( status & 0x4)
    {
      /* support I2C ports 8 and 9 */
      if ((bus >= 0) && (bus <= 7))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_GTO<<(bus*4));
      else if ((bus == 8) || (bus == 9))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_GTO<<((bus-8)*4));
      se_pilot_ii_data_ptr[bus].Time_Out |=TTimeout;
      printk("I2c%d TTimeOut\n",bus);
    }
  
  if( status & 0x8)
    {
      /* support I2C ports 8 and 9 */
      if ((bus >= 0) && (bus <= 7))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_TFIFO_TO<<(bus*4));
      else if ((bus == 8) || (bus == 9))
        *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_TFIFO_TO<<((bus-8)*4));
      printk("I2c%d FIFO Timed Out\n",bus);
    }
  
  se_pilot_ii_data_ptr[bus].op_status = irq_status;
  wake_up( &se_pilot_ii_data_ptr[bus].pilot_ii_wait );

  spin_unlock_irqrestore(&se_pilot_ii_data_ptr[bus].to_irq_lock, flags);

  
}

/* Clock stretch interrupt handler */
void Clock_Stretch_Timeout(int bus, int status)
{
    unsigned long flags = 0;
    unsigned int rxFlr = 0;
    unsigned int clkStretch = 0;
    unsigned int intrStatus = 0;

    spin_lock_irqsave( &se_pilot_ii_data_ptr[bus].i2c_irq_lock, flags);

    if( status & 0x1)
    {
        /* support I2C ports 8 and 9 */
        if ((bus >= 0) && (bus <= 7))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_MTO<<(bus*4));
        else if ((bus == 8) || (bus == 9))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_MTO<<((bus-8)*4));
    }

    if( status & 0x2)
    {
       /* support I2C ports 8 and 9 */
        if ((bus >= 0) && (bus <= 7))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_STO<<(bus*4));
        else if ((bus == 8) || (bus == 9))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_STO<<((bus-8)*4));
    }

    if( status & 0x4)
    {
        /* Check is this clock stretch interrupt the result of a "missed" STOP on 
         * a back-to-back slave transfer. I.e. we have left data in the RX FIFO. */
        clkStretch = Get_Clk_Streaching_info(bus);
        rxFlr = i2c_pilot_ii_read_reg(bus, I2C_RXFLR_REG);
        intrStatus = i2c_pilot_ii_read_reg( bus, I2C_INTR_STAT_REG );
       
        if ((se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag == SLAVE) &&	/* SLAVE mode */
            ((clkStretch & 0x5) == 0x5) &&										/* Port is clock stretching for a slave transfer */
            ((intrStatus & (RX_FULL | STOP_DET)) == 0) &&						/* No pending RX FULL or STOP interrupts */
            (rxFlr > 0))														/* RX FIFO not empty */
        {
            i2cSTOPDET_slave_process(bus, intrStatus);
        }

        /* support I2C ports 8 and 9 */
        if ((bus >= 0) && (bus <= 7))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_GTO<<(bus*4));
        else if ((bus == 8) || (bus == 9))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_GTO<<((bus-8)*4));
    }

    if( status & 0x8)
    {
        /* support I2C ports 8 and 9 */
        if ((bus >= 0) && (bus <= 7))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_TO_INTR_STS) = (SMB_TFIFO_TO<<(bus*4));
        else if ((bus == 8) || (bus == 9))
            *(volatile unsigned int *)(SE_SYS_CLK_VA_BASE+I2C_COUNTER_STS_1) = (SMB_TFIFO_TO<<((bus-8)*4));
    }

    spin_unlock_irqrestore(&se_pilot_ii_data_ptr[bus].i2c_irq_lock, flags);
}
