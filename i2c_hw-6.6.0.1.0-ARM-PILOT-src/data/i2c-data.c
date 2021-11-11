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

struct i2c_pilot_ii_data se_pilot_ii_data_ptr[BUS_COUNT];

void i2c_init_internal_data(int bus)
{
  int i;
  static bool kernel_init_done[BUS_COUNT] = {false,};
  
  /* Initialize locks, queues and variables */
    if (!kernel_init_done[bus])
    {
	spin_lock_init( &se_pilot_ii_data_ptr[bus].data_lock );
	spin_lock_init( &se_pilot_ii_data_ptr[bus].i2c_irq_lock );
	spin_lock_init( &se_pilot_ii_data_ptr[bus].to_irq_lock );
	spin_lock_init( &se_pilot_ii_data_ptr[bus].transfer_lock );
        init_waitqueue_head( &(se_pilot_ii_data_ptr[bus].pilot_ii_wait)); 
	init_waitqueue_head( &(se_pilot_ii_data_ptr[bus].pilot_ii_slave_data_wait));
	init_waitqueue_head( &(se_pilot_ii_data_ptr[bus].pilot_ii_mctp_data_wait));
        kernel_init_done[bus] = true;
    }
	se_pilot_ii_data_ptr[bus].op_status = 0;
	se_pilot_ii_data_ptr[bus].abort_status = 0;

	se_pilot_ii_data_ptr[bus].TX_len = 0;
	se_pilot_ii_data_ptr[bus].TX_index = 0;

	se_pilot_ii_data_ptr[bus].MasterRX_len = 0;
	se_pilot_ii_data_ptr[bus].MasterRX_index = 0;
	se_pilot_ii_data_ptr[bus].Master_rd_cmd_index = 0;

    se_pilot_ii_data_ptr[bus].block_read = 0;
    se_pilot_ii_data_ptr[bus].block_proc_call= 0;
    se_pilot_ii_data_ptr[bus].host_notify_flag = 0;
    se_pilot_ii_data_ptr[bus].master_read_with_PEC = 0;

	se_pilot_ii_data_ptr[bus].Linear_SlaveRX_len = 0;
	se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index = 0;

	for(i=0;i<MAX_FIFO_LEN;i++)
	{
		se_pilot_ii_data_ptr[bus].SlaveRX_len[i] = 0;
		se_pilot_ii_data_ptr[bus].SlaveRX_index[i] = 0;
	}
	se_pilot_ii_data_ptr[bus].SlaveRX_Writer = 0;
	se_pilot_ii_data_ptr[bus].SlaveRX_Reader = 0;
	se_pilot_ii_data_ptr[bus].SlaveRX_Entries = 0;
	
	se_pilot_ii_data_ptr[bus].SlaveTX_Enable = 0;
	
	se_pilot_ii_data_ptr[bus].MasterXmit = 0;

	se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag = SLAVE;

	se_pilot_ii_data_ptr[bus].start_detected = 0;
	se_pilot_ii_data_ptr[bus].RD_REQ_Pending = 0x0;
	se_pilot_ii_data_ptr[bus].i2c_link_state = 0;
	
	se_pilot_ii_data_ptr[bus].bus_busy_count = 0;
	se_pilot_ii_data_ptr[bus].bus_busy_timestamp = 0;
	
	se_pilot_ii_data_ptr[bus].bus_timeout_count = 0;
	se_pilot_ii_data_ptr[bus].bus_timeout_timestamp = 0;
}
