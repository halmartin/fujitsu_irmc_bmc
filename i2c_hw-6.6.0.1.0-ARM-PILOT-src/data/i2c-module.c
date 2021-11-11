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
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h> /* For Ioctl definitions */
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/stddef.h>
#include <linux/seq_file.h>
#endif

#include "i2c-data.h"
#include "i2c-hardware.h"
#include "i2c-log.h"

#define I2C_ID              ( 1 )

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
u32 irq_vadd[BUS_COUNT]= 
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
SE_I2C_9_VA_BASE,
#endif
};
#endif

#ifdef SOC_PILOT_IV 
int irq[ BUS_COUNT ] =
{
#if (BUS_COUNT>=1)
   	IRQ_I2C_0, 
#endif
#if (BUS_COUNT>=2)
	IRQ_I2C_1, 
#endif
#if (BUS_COUNT>=3)
	IRQ_I2C_2, 
#endif
#if (BUS_COUNT>=4)
	IRQ_I2C_3, 
#endif
#if (BUS_COUNT>=5)
	IRQ_I2C_4, 
#endif
#if (BUS_COUNT>=6)
	IRQ_I2C_5,
#endif
#if (BUS_COUNT>=7)
	IRQ_I2C_6,
#endif
#if (BUS_COUNT>=8)
    IRQ_I2C_7,
#endif
#if (BUS_COUNT>=9)
    IRQ_I2C_8,
#endif
#if (BUS_COUNT>=10)
    IRQ_I2C_9
#endif
  };
#else
int irq[ BUS_COUNT ] =
{
#if (BUS_COUNT>=1)
        IRQ_I2C_0,
#endif
#if (BUS_COUNT>=2)
        IRQ_I2C_1,
#endif
#if (BUS_COUNT>=3)
        IRQ_I2C_2,
#endif
#if (BUS_COUNT>=4)
        IRQ_I2C_3,
#endif
#if (BUS_COUNT>=5)
        IRQ_I2C_4,
#endif
#if (BUS_COUNT>=6)
        IRQ_I2C_5,
#endif
#if (BUS_COUNT>=7)
        IRQ_I2C_6,
#endif
#if (BUS_COUNT>=8)
        IRQ_I2C_7
#endif
};
#endif

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
typedef irqreturn_t (* HANDLER)	( int this_irq, void *dev_id);
#else
typedef irqreturn_t (* HANDLER)	( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
#endif

static HANDLER irq_handlers[BUS_COUNT] = 
{
#if (BUS_COUNT>=1)
   	i2c_handler_0, 
#endif
#if (BUS_COUNT>=2)
	i2c_handler_1, 
#endif
#if (BUS_COUNT>=3)
	i2c_handler_2, 
#endif
#if (BUS_COUNT>=4)
	i2c_handler_3, 
#endif
#if (BUS_COUNT>=5)
	i2c_handler_4, 
#endif
#if (BUS_COUNT>=6)
	i2c_handler_5,
#endif
#if (BUS_COUNT>=7)
	i2c_handler_6,
#endif
#if (BUS_COUNT>=8)
	i2c_handler_7,
#endif
#if (BUS_COUNT>=9)
    i2c_handler_8,
#endif
#if (BUS_COUNT>=10)
    i2c_handler_9
#endif
};
static struct i2c_adapter se_pilot_ii_ops[ BUS_COUNT ];
extern struct i2c_pilot_ii_data se_pilot_ii_data_ptr[BUS_COUNT];
#ifdef INT_MONITOR
extern struct i2c_pilot_ii_interrupt_monitor int_monitor;
#define SHOW_MONITOR_INFO 0xFE
#endif
#ifdef BUS_BUSY_MONITOR
extern struct i2c_pilot_ii_bus_busy_monitor bus_busy_monitor;
#define SHOW_BUS_BUSY_INFO 0xFD
#endif

static int bus_count = BUS_COUNT;
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
#define I2CSTAT_PROC_DIR "i2c-stat"
#define SHOW_DEBUG_INFO 0xFF 
#define SHOW_BUS_RESET_INFO		0xFC

typedef struct {
	char *name;
	unsigned long stat_offset;
} i2c_stat_proc_descr_t;

typedef struct {
	char *name;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
	int (*read) (char *buffer, char **buffer_location, off_t offset,int buffer_length, int *eof, void *p);
	int (*write) (struct file *file, const char __user *buffer,unsigned long buffer_length, void *p);
#else
	int (*read)(struct file *file,char __user *buffer, size_t buffer_length,loff_t *offset);
	int (*write)(struct file *file,const char *buffer,size_t buffer_length,loff_t *offp);
#endif
} i2c_reg_proc_descr_t;

static i2c_stat_proc_descr_t i2c_stat_proc_descr[] = {
	{ "master_tx", offsetof(i2c_stat_t, master_tx) },
	{ "slave_rx", offsetof(i2c_stat_t, slave_rx) },
	{ "arblost", offsetof(i2c_stat_t, arblost) },
	{ "nack", offsetof(i2c_stat_t, nack) },
	{ "recover", offsetof(i2c_stat_t, recover) },
	{ "error", offsetof(i2c_stat_t, error) },
	{ "count", offsetof(i2c_stat_t, count) },
	{ "continous_bus_reset", SHOW_BUS_RESET_INFO },
	{ "debug", SHOW_DEBUG_INFO },
#ifdef INT_MONITOR
	{ "monitor", SHOW_MONITOR_INFO },
#endif
#ifdef BUS_BUSY_MONITOR
	{ "bus_busy_info", SHOW_BUS_BUSY_INFO },
#endif
	{ 0, 0 }
};

typedef struct i2c_stat_proc_data {
	int bus;
	i2c_stat_proc_descr_t *descr;
	struct i2c_stat_proc_data *next;
} i2c_stat_proc_data_t;

typedef struct i2c_reg_proc_data {
	int bus;
	char* descr;
	struct i2c_reg_proc_data *next;
} i2c_reg_proc_data_t;

static i2c_stat_proc_data_t *i2c_stat_proc_data_head;
static i2c_reg_proc_data_t *i2c_reg_proc_data_head;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	static int i2c_reg_proc_readreg(struct file *file,char __user *buffer, size_t buffer_length,loff_t *offset)
#else
	static int i2c_reg_proc_readreg(char *buffer, char **buffer_location,off_t offset, int buffer_length, int *eof, void *p)
#endif	
{
	int len = 0;
	i2c_stat_proc_data_t *data = NULL;
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	data=PDE_DATA(file_inode(file));
#else
	data = p;
#endif
	if(data == NULL)
	    return 0;	
		
	len +=  snprintf(buffer, buffer_length,       "I2C_CON_REG         : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_CON_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_TAR_REG         : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_TAR_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_SAR_REG         : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_SAR_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_INTR_STAT_REG   : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_INTR_STAT_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_RX_TL_REG       : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_RX_TL_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_TX_TL_REG       : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_TX_TL_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_ENABLE_REG      : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_ENABLE_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_STATUS_REG      : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_STATUS_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_TXFLR_REG       : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_TXFLR_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_RXFLR_REG       : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_RXFLR_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_TX_ABT_SRC_REG  : 0x%X\n",i2c_pilot_ii_read_reg(data->bus,I2C_TX_ABORT_SOURCE_REG));
	len +=  snprintf(&buffer[len], buffer_length, "I2C_DEBUG_STS_REG   : 0x%X\n",Get_Clk_Streaching_info(data->bus));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	*offset+=len;
#endif
	return len;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	static int i2c_reg_proc_reset_read(struct file *file,char __user *buffer, size_t buffer_length,loff_t *offset)
#else
	static int i2c_reg_proc_reset_read(char *buffer, char **buffer_location,off_t offset, int buffer_length, int *eof, void *p)
#endif
{
	int len=0;
	char tmp[64]={0};
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	if (*offset != 0)
		return 0;	
#endif

	len = snprintf(tmp, buffer_length, "0\n");
	
    if (copy_to_user(buffer,tmp, len))
                return -EFAULT;

	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	*offset+=len;
#endif

	return len;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	static int i2c_reg_proc_reset_write(struct file *file,const char *buffer,size_t buffer_length,loff_t *offp)
#else
	static int i2c_reg_proc_reset_write(struct file *file, const char __user *buffer,unsigned long buffer_length, void *p)
#endif
{

	i2c_stat_proc_data_t *data = NULL;
		
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	data=PDE_DATA(file_inode(file));
#else	
	data = p;
#endif
        if(data == NULL)
	    return 0;	

	if(buffer_length > 0)
	{
		if('1' == buffer[0])
		{
			/*reset internal variable and the bus*/
			i2c_init_internal_data(data->bus);
			i2c_init_hardware(data->bus);
			//enable time out counters
			i2c_to_counter_en(data->bus);
			//Enable Time out interrupts
			i2c_time_out_interrupt(data->bus);
		}
	}
	return buffer_length;
}

#ifdef INT_MONITOR
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	static int i2c_reg_proc_monitor_bus_read(struct file *file,char __user *buffer, size_t buffer_length,loff_t *offset)
#else
	static int i2c_reg_proc_monitor_bus_read(char *buffer, char **buffer_location,off_t offset, int buffer_length, int *eof, void *p)
#endif	
{
	int bus,len=0;
	unsigned long int flags;
	i2c_stat_proc_data_t *data = NULL;
	char tmp[64]={0};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	if (*offset != 0)
		return 0;

	data=PDE_DATA(file_inode(file));
#else
	data = p;
#endif

	if (data == NULL)
	    return 0;
	
	spin_lock_irqsave( &se_pilot_ii_data_ptr[data->bus].i2c_irq_lock, flags);
	bus = int_monitor.bus;
	spin_unlock_irqrestore( &se_pilot_ii_data_ptr[data->bus].i2c_irq_lock, flags);
	len = snprintf(tmp, buffer_length, "%i\n", bus);
	
	if (copy_to_user(buffer,tmp, len))
            return -EFAULT;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	*offset=len;
#endif
	return len;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	static int i2c_reg_proc_monitor_bus_write(struct file *file,const char *buffer,size_t buffer_length,loff_t *offp)
#else
	static int i2c_reg_proc_monitor_bus_write(struct file *file, const char __user *buffer,	unsigned long buffer_length, void *p)
#endif	
{	
	unsigned long int flags;
	i2c_stat_proc_data_t *data = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	data=PDE_DATA(file_inode(file));
#else
	data = p;
#endif

	if (data == NULL)
	    return 0;

	if(buffer_length > 0)
	{
		if('1' == buffer[0])
		{
			spin_lock_irqsave( &se_pilot_ii_data_ptr[data->bus].i2c_irq_lock, flags);
			int_monitor.bus = data->bus;
			spin_unlock_irqrestore( &se_pilot_ii_data_ptr[data->bus].i2c_irq_lock, flags);
		}
		else if (('0' == buffer[0]) && (int_monitor.bus == data->bus))
		{
			spin_lock_irqsave( &se_pilot_ii_data_ptr[data->bus].i2c_irq_lock, flags);
			int_monitor.bus = 255;
			spin_unlock_irqrestore( &se_pilot_ii_data_ptr[data->bus].i2c_irq_lock, flags);
		}
	}
	return buffer_length;
}
#endif

static i2c_reg_proc_descr_t i2c_reg_proc_descr[] = {
        { "readreg", i2c_reg_proc_readreg , 0 },
        { "reset", i2c_reg_proc_reset_read, i2c_reg_proc_reset_write },
#ifdef INT_MONITOR
        { "monitorbus", i2c_reg_proc_monitor_bus_read, i2c_reg_proc_monitor_bus_write },
#endif
        { 0, 0, 0}
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
static int i2c_stat_proc_read(struct file *file,char __user *buffer, size_t buffer_length,loff_t *offset)
#else
static int i2c_stat_proc_read(char *buffer, char **buffer_location,off_t offset, int buffer_length, int *eof, void *p)
#endif
{	

	int bus,len=0;
	i2c_stat_proc_descr_t *descr = NULL;
	i2c_stat_t *stat;
	unsigned val;
	i2c_stat_proc_data_t *data = NULL;
	char tmp[750]={0};
	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	data=PDE_DATA(file_inode(file));
	if (*offset > 0)
	{
	    return 0;
	}	
#else	
	data = p;
	*eof = 1;

	if (offset > 0)
	    return 0;
#endif

	if (data == NULL)
	{
 	    return 0;
	}	

	bus = data->bus;
	descr = data->descr;
	stat = &(se_pilot_ii_data_ptr[bus].stat);
	if(descr->stat_offset == SHOW_DEBUG_INFO) {
		len = snprintf(tmp, buffer_length,
				"TX_len: %i\n"
				"TX_index: %i\n"
				"Slave_TX_len: %i\n"
				"Slave_TX_index: %i\n"
				"MasterRX_len: %i\n"
				"MasterRX_index: %i\n"
				"Master_rd_cmd_index: %i\n"
				"Linear_SlaveRX_len: %i\n"
				"Linear_SlaveRX_index: %i\n"
				"SlaveRX_Writer: %i\n"
				"SlaveRX_Reader: %i\n"
				"SlaveRX_Entries: %i\n"
				"op_status: %i\n"
				"abort_status: %i\n"
				"master_xmit_recv_mode_flag: %i\n"
				"start_detected: %i\n"
				"Time_Out: %i\n"
				"RD_REQ_Pending: %i\n"
				"bus_speed: %i\n"
				"Master Xmit: %i\n"
				"bus_busy_reset_count: %i\n"
				"continuous_bus_busy_reset: %i\n"
				"bus_timeout_reset_count: %i\n"
				"continuous_bus_timeout_reset: %i\n"
				"missing_stop_counter: %i\n"
				"rxfull_stop_counter: %i\n"
				"recover_start_counter: %i\n"
				"recover_stop_counter: %i\n"
				"recover_rxfull_counter: %i\n"
				"recover_clock_stretch: %i\n"
#ifdef INT_MONITOR
				"Monitor_status: %i\n",
#else
				,
#endif
				se_pilot_ii_data_ptr[bus].TX_len,
				se_pilot_ii_data_ptr[bus].TX_index,
				se_pilot_ii_data_ptr[bus].Slave_TX_len,
				se_pilot_ii_data_ptr[bus].Slave_TX_index,
				se_pilot_ii_data_ptr[bus].MasterRX_len,
				se_pilot_ii_data_ptr[bus].MasterRX_index,
				se_pilot_ii_data_ptr[bus].Master_rd_cmd_index,
				se_pilot_ii_data_ptr[bus].Linear_SlaveRX_len,
				se_pilot_ii_data_ptr[bus].Linear_SlaveRX_index,
				se_pilot_ii_data_ptr[bus].SlaveRX_Writer,
				se_pilot_ii_data_ptr[bus].SlaveRX_Reader,
				se_pilot_ii_data_ptr[bus].SlaveRX_Entries,
				se_pilot_ii_data_ptr[bus].op_status,
				se_pilot_ii_data_ptr[bus].abort_status,
				se_pilot_ii_data_ptr[bus].master_xmit_recv_mode_flag,
				se_pilot_ii_data_ptr[bus].start_detected,
				se_pilot_ii_data_ptr[bus].Time_Out,
				se_pilot_ii_data_ptr[bus].RD_REQ_Pending,
				se_pilot_ii_data_ptr[bus].bus_speed,
				se_pilot_ii_data_ptr[bus].MasterXmit,
				se_pilot_ii_data_ptr[bus].bus_reset_count,
				se_pilot_ii_data_ptr[bus].continuous_bus_reset,
				se_pilot_ii_data_ptr[bus].bus_timeout_reset_count,
				se_pilot_ii_data_ptr[bus].continuous_bus_timeout_reset,
				se_pilot_ii_data_ptr[bus].missing_stop_counter,
				se_pilot_ii_data_ptr[bus].rxfull_stop_counter,
				se_pilot_ii_data_ptr[bus].recover_start_counter,
				se_pilot_ii_data_ptr[bus].recover_stop_counter,
				se_pilot_ii_data_ptr[bus].recover_rxfull_counter,
				se_pilot_ii_data_ptr[bus].recover_clock_stretch
#ifdef INT_MONITOR
				,int_monitor.start_flag
#endif
				);
	}
	else if (descr->stat_offset == SHOW_BUS_RESET_INFO)
	{
		len = snprintf(tmp, buffer_length, "%u\n", 
				(se_pilot_ii_data_ptr[bus].continuous_bus_timeout_reset + 
				 se_pilot_ii_data_ptr[bus].continuous_bus_reset) / 3); // 3 is for retries
	}
	else {
		val = *(unsigned*)((char*)stat + descr->stat_offset);
		len = snprintf(tmp, buffer_length, "%u\n", val);		
	}

    if (copy_to_user(buffer,tmp, len))
        return -EFAULT;

	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
		*offset+=len;
	#endif

	return len;
}

static char _str[PATH_MAX];

#ifdef INT_MONITOR

/**
 * This function is called at the beginning of a sequence.
 * ie, when:
 *	- the /proc file is read (first time)
 *	- after the function stop (end of sequence)
 *
 */
static void *monitor_seq_start(struct seq_file *seq_file, loff_t *pos)
{
	static unsigned int rec_send = 0;
	static int bus = 0;
	static unsigned long flags = 0;
	if ( *pos == 0 )
	{	
		/* start the sequence */
		/* fix Kernel errors reading the I2C bus monitor data */
		if (int_monitor.bus < BUS_COUNT)
			spin_lock_irqsave( &se_pilot_ii_data_ptr[int_monitor.bus].i2c_irq_lock, flags);
		rec_send = int_monitor.monitor_count;
		if (rec_send == 0)
			return NULL;
		bus = int_monitor.bus;
		int_monitor.bus = 0xFF;	//Stop the monitor for a while.
		if (bus < BUS_COUNT)
			spin_unlock_irqrestore(&se_pilot_ii_data_ptr[bus].i2c_irq_lock, flags);	/* release spin lock after temporarily disabling bus monitoring */
		rec_send = int_monitor.monitor_count;
		return &rec_send;
	}
	else
	{
		if ((rec_send <= 0) || ((unsigned int)(*pos) >= int_monitor.monitor_count))
		{
			
			(*pos) = 0;
			if (bus < BUS_COUNT)
            {
				spin_lock_irqsave( &se_pilot_ii_data_ptr[bus].i2c_irq_lock, flags);	/* Get spin lock before re-enabling bus monitoring */
				int_monitor.bus = bus;	// activate the monitor again.
				spin_unlock_irqrestore(&se_pilot_ii_data_ptr[int_monitor.bus].i2c_irq_lock, flags);
            }
			return NULL;
		}
		else
		{
			rec_send = int_monitor.monitor_count - (*pos);
			return &rec_send;
		}
	}
}

/**
 * This function is called after the beginning of a sequence.
 * It's called untill the return is NULL (this ends the sequence).
 *
 */
static void *monitor_seq_next(struct seq_file *seq_file, void *v, loff_t *pos)
{
	unsigned int *tmp_v = (unsigned int *)v;
	void *ret = NULL;
	
	if ((*tmp_v) != 0)
		ret = v;
	
	(*tmp_v)--;
	(*pos)++;
	
	return ret;
}

/**
 * This function is called at the end of a sequence
 * 
 */
static void monitor_seq_stop(struct seq_file *seq_file, void *v)
{
	return;
}

/**
 * This function is called for each "step" of a sequence
 *
 */
static int monitor_seq_show(struct seq_file *seq_file, void *v)
{
	loff_t *spos = (loff_t *) v;
	unsigned int count, retval = 0;
	int j;

	if ((unsigned int)(*spos) == int_monitor.monitor_count)
		retval += seq_printf (seq_file, "S.No  Jiffies          Bus  Raw  Served  Status  TX_abrt_src  TXFLR  RXFLR  Snd/Rcv    Data[]\n");
	
	if ((unsigned int)(*spos) == 0)
		return retval;
	
	count = (int_monitor.start_position + (int_monitor.monitor_count - ((unsigned int)(*spos)))) % int_monitor.monitor_count;
	retval += seq_printf (seq_file, "%4i  %15llu  %03X  %3X  %6X  %6X  %11X  %5X  %5X  %7X    ", 
			int_monitor.monitor_count - ((unsigned int)(*spos)),
			int_monitor.int_data[count].jiffies_value,
			int_monitor.int_data[count].bus,
			int_monitor.int_data[count].raw_int_stauts,
			int_monitor.int_data[count].int_serviced,
			int_monitor.int_data[count].status_reg,
			int_monitor.int_data[count].tx_abrt_source_reg,
			int_monitor.int_data[count].txflr_reg,
			int_monitor.int_data[count].rxflr_reg,
			int_monitor.int_data[count].send_receive);
	for (j = 0; j < int_monitor.int_data[count].data_count; j++)
	{
		retval += seq_printf (seq_file, " %2X", int_monitor.int_data[count].data_bytes[j]);
	}
	retval += seq_printf (seq_file, "\n");
	return retval;
}

/**
 * This structure gather "function" to manage the sequence
 *
 */
static struct seq_operations monitor_seq_ops = {
	.start = monitor_seq_start,
	.next  = monitor_seq_next,
	.stop  = monitor_seq_stop,
	.show  = monitor_seq_show
};

/**
 * Open the seq-file.
 */
static int monitor_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &monitor_seq_ops);
};

/**
 * This structure is to manage the /proc seq-file
 *
 */
static struct file_operations monitor_file_ops = {
	.owner   = THIS_MODULE,
	.open    = monitor_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
#endif


#ifdef BUS_BUSY_MONITOR

/**
 * This function is called at the beginning of a sequence.
 * ie, when:
 *	- the /proc file is read (first time)
 *	- after the function stop (end of sequence)
 *
 */
static void *bus_busy_seq_start(struct seq_file *seq_file, loff_t *pos)
{
	static unsigned int send = 0;
	if ( *pos == 0 )
	{	
		/* start the sequence */
		send = bus_busy_monitor.monitor_count;
		if (send == 0)
			return NULL;
		return &send;
	}
	else
	{
		if ((send <= 0) || ((unsigned int)(*pos) >= bus_busy_monitor.monitor_count))
		{
			
			(*pos) = 0;
			return NULL;
		}
		else
		{
			send = bus_busy_monitor.monitor_count - (*pos);
			return &send;
		}
	}
}

/**
 * This function is called after the beginning of a sequence.
 * It's called untill the return is NULL (this ends the sequence).
 *
 */
static void *bus_busy_seq_next(struct seq_file *seq_file, void *v, loff_t *pos)
{
	unsigned int *tmp_v = (unsigned int *)v;
	void *ret = NULL;
	
	if ((*tmp_v) != 0)
		ret = v;
	
	(*tmp_v)--;
	(*pos)++;
	
	return ret;
}

/**
 * This function is called at the end of a sequence
 * 
 */
static void bus_busy_seq_stop(struct seq_file *seq_file, void *v)
{
	return;
}

/**
 * This function is called for each "step" of a sequence
 *
 */
static int bus_busy_seq_show(struct seq_file *seq_file, void *v)
{
	loff_t *spos = (loff_t *) v;
	unsigned int count, retval = 0;

	if ((unsigned int)(*spos) == bus_busy_monitor.monitor_count)
		retval += seq_printf (seq_file, "S.No  Jiffies          Bus  CON  TAR  SAR  INTR_STAT  RX_TL  TX_TL  ENABLE  STATUS  TXFLR  RXFLR  TX_ABT_SRC  DEBUG_STS\n");
	
	if ((unsigned int)(*spos) == 0)
		return retval;
	
	count = (bus_busy_monitor.start_position + (bus_busy_monitor.monitor_count - ((unsigned int)(*spos)))) % bus_busy_monitor.monitor_count;
	retval += seq_printf (seq_file, "%4i  %15llu  %03X  %3X  %3X  %3X  %9X  %5X  %5X  %6X  %6X  %5X  %5X  %10X  %9X\n", 
			bus_busy_monitor.monitor_count - ((unsigned int)(*spos)),
			bus_busy_monitor.data[count].jiffies_value,
			bus_busy_monitor.data[count].bus,
			bus_busy_monitor.data[count].i2c_con,
			bus_busy_monitor.data[count].i2c_tar,
			bus_busy_monitor.data[count].i2c_sar,
			bus_busy_monitor.data[count].i2c_intr_stat,
			bus_busy_monitor.data[count].i2c_rx_tl,
			bus_busy_monitor.data[count].i2c_tx_tl,
			bus_busy_monitor.data[count].i2c_enable,
			bus_busy_monitor.data[count].i2c_status,
			bus_busy_monitor.data[count].i2c_txflr,
			bus_busy_monitor.data[count].i2c_rxflr,
			bus_busy_monitor.data[count].i2c_tx_abt_src,
			bus_busy_monitor.data[count].i2c_debug_sts);
	return retval;
}

/**
 * This structure gather "function" to manage the sequence
 *
 */
static struct seq_operations bus_busy_seq_ops = {
	.start = bus_busy_seq_start,
	.next  = bus_busy_seq_next,
	.stop  = bus_busy_seq_stop,
	.show  = bus_busy_seq_show
};

/**
 * Open the seq-file.
 */
static int bus_busy_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &bus_busy_seq_ops);
};

/**
 * This structure is to manage the /proc seq-file
 *
 */
static struct file_operations bus_busy_file_ops = {
	.owner   = THIS_MODULE,
	.open    = bus_busy_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};
#endif
	
static void i2c_stat_proc_init(void)
{
	struct proc_dir_entry *pde=NULL, *i2cstat_proc_dir=NULL, *bus_dir=NULL, *entry=NULL;
	int i=0;
	i2c_stat_proc_descr_t *descr=NULL;
	i2c_reg_proc_descr_t *regdes=NULL;
	i2c_stat_proc_data_t *data=NULL;
	i2c_reg_proc_data_t *regdata=NULL;
	mode_t mode = S_IFREG;
	
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
	struct file_operations *proc_fops = NULL;
#endif
	
	i2cstat_proc_dir = proc_mkdir(I2CSTAT_PROC_DIR, NULL);
	if (i2cstat_proc_dir == NULL) {
		printk(KERN_ERR "%s() failed to create /proc/%s\n",
			__FUNCTION__, I2CSTAT_PROC_DIR);
		return;
	}
	
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))	
	i2cstat_proc_dir->owner	= THIS_MODULE;
	i2cstat_proc_dir->uid 	= 0;
	i2cstat_proc_dir->gid 	= 0;
#endif
				
	for (i = 0; i < BUS_COUNT; ++i) {
		snprintf(_str, sizeof(_str), "%d", i);

		bus_dir = proc_mkdir(_str, i2cstat_proc_dir);
		if (bus_dir == NULL) {
			printk(KERN_ERR "%s() failed to create /proc/%s/%s\n",
				__FUNCTION__, I2CSTAT_PROC_DIR, _str);
			continue;
		}
		
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
		bus_dir->owner = THIS_MODULE;
		bus_dir->uid = 0;
		bus_dir->gid = 0;
#endif

		descr = i2c_stat_proc_descr;
		while (descr->name != 0) {
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
			proc_fops = kmalloc(sizeof(struct file_operations),GFP_KERNEL);
			if(proc_fops == NULL)
			{
				printk(KERN_ERR "%s() failed to allocate memory\n",__FUNCTION__);
				return;		
			}
			memset(proc_fops,0,sizeof(struct file_operations));
			proc_fops->owner = THIS_MODULE;
			
			data = kmalloc(sizeof(*data), GFP_KERNEL);
			if (data)
			{
				data->bus = i;
				data->descr = descr;
				data->next = i2c_stat_proc_data_head;
				i2c_stat_proc_data_head = data;
			}else {
				remove_proc_entry(descr->name, bus_dir);
				printk(KERN_ERR "%s() failed to allocate memory\n",__FUNCTION__);
			}
			#ifdef INT_MONITOR
					if (descr->stat_offset == SHOW_MONITOR_INFO)
					{
						pde = proc_create_data(descr->name,mode,bus_dir,&monitor_file_ops,data);
					}	
					else
			#endif
			#ifdef BUS_BUSY_MONITOR
					if (descr->stat_offset == SHOW_BUS_BUSY_INFO)
					{
						pde = proc_create_data(descr->name,mode,bus_dir,&bus_busy_file_ops,data);
					}
					else					
			#endif
					{
						memset(proc_fops,0,sizeof(struct file_operations));
						proc_fops->owner = THIS_MODULE;
						proc_fops->read = i2c_stat_proc_read;											
						mode = 0644;
						pde = proc_create_data(descr->name,mode,bus_dir,proc_fops,data);
					}
		#else
			pde = create_proc_entry(descr->name, 0444, bus_dir);
		#endif
			if (pde) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))				
				data = kmalloc(sizeof(*data), GFP_KERNEL);
				if (data) {
					data->bus = i;
					data->descr = descr;
					data->next = i2c_stat_proc_data_head;
					i2c_stat_proc_data_head = data;

				#ifdef INT_MONITOR
					if (descr->stat_offset == SHOW_MONITOR_INFO)
					{
						pde->proc_fops = &monitor_file_ops;
					}	
					else
				#endif
				#ifdef BUS_BUSY_MONITOR
					if (descr->stat_offset == SHOW_BUS_BUSY_INFO)
					{	
						pde->proc_fops = &bus_busy_file_ops;
					}
					else
				#endif
					{
						pde->read_proc	= i2c_stat_proc_read;
					}
					pde->owner	= THIS_MODULE;
					pde->mode 	= S_IFREG | S_IRUGO;
					pde->uid 	= 0;
					pde->gid 	= 0;
					pde->data	= data;
				} //end of kmalloc to data  
				else
				{
					remove_proc_entry(descr->name, bus_dir);
					printk(KERN_ERR "%s() failed to allocate memory\n",__FUNCTION__);
				}
#endif				
			} //end of proc_create if true
			else
			{
				printk(KERN_ERR "%s() failed to create /proc/%s/%s/%s\n",
					__FUNCTION__, I2CSTAT_PROC_DIR, _str, descr->name);
			}
			++descr;
		} //end of while

		regdes = i2c_reg_proc_descr;
		while (regdes->name != 0) {
			regdata = kmalloc(sizeof(*regdata),GFP_KERNEL);
			if (regdata) 
			{				
				regdata->bus = i;
				regdata->descr = regdes->name;
				regdata->next = i2c_reg_proc_data_head;
				i2c_reg_proc_data_head = regdata;
			}
			else 
			{
				printk(KERN_ERR "%s() failed to allocate memory\n",__FUNCTION__);
				remove_proc_entry(regdes->name,bus_dir);
			}
			
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
			entry = create_proc_entry(regdes->name, 0644, bus_dir);
#else
			proc_fops = kmalloc(sizeof(struct file_operations),GFP_KERNEL);
			if(proc_fops == NULL)
			{
				printk(KERN_ERR "%s() failed to allocate memory\n",__FUNCTION__);
				return;		
			}
			memset(proc_fops,0,sizeof(struct file_operations));
			proc_fops->owner = THIS_MODULE;
			if(regdes->read != 0)
			{
				proc_fops->read = regdes->read;
			}
			if(regdes->write != 0)
			{
				proc_fops->write = regdes->write;
			}
			entry = proc_create_data(regdes->name,0644,bus_dir,proc_fops,regdata);
#endif
			if(entry) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))					
					if(regdes->read != 0)
					{
						entry->read_proc = regdes->read;
					}
					if(regdes->write != 0)
					{
						entry->write_proc = regdes->write;
					}
					entry->owner	= THIS_MODULE;
					entry->mode	= S_IFREG | S_IRUGO;
					entry->uid	= 0;
					entry->gid	= 0;
					entry->data	= regdata;
#endif
			}
			else {
							printk(KERN_ERR "%s() failed to create /proc/%s/%s/%s\n",
								__FUNCTION__, I2CSTAT_PROC_DIR, _str, descr->name);
			}
			++regdes;
		} //end of while
	}//end of for bus count iterations - for loop
}

static void i2c_stat_proc_clear(void)
{
	i2c_stat_proc_data_t *data;
	i2c_reg_proc_data_t *regdata;
	int i;

	while (i2c_stat_proc_data_head) {
		data = i2c_stat_proc_data_head;
		i2c_stat_proc_data_head = data->next;

		snprintf(_str, sizeof(_str), "%s/%d/%s",
			I2CSTAT_PROC_DIR, data->bus, data->descr->name);

		remove_proc_entry(_str, NULL);

		kfree(data);
	}

	while (i2c_reg_proc_data_head) {
		regdata = i2c_reg_proc_data_head;
		i2c_reg_proc_data_head = regdata->next;

		snprintf(_str, sizeof(_str), "%s/%d/%s",
			I2CSTAT_PROC_DIR, regdata->bus, regdata->descr);

		remove_proc_entry(_str, NULL);

		kfree(regdata);
	}

	for (i = 0; i < BUS_COUNT; ++i) {
		snprintf(_str, sizeof(_str), "%s/%d",
			I2CSTAT_PROC_DIR, i);
		remove_proc_entry(_str, NULL);
	}

	remove_proc_entry(I2CSTAT_PROC_DIR, NULL);
}
#endif // CONFIG_SPX_FEATURE_ENABLE_I2C_STAT

extern int pilot_ii_get_recovery_info (int bus, bus_recovery_info_T* info);
extern int pilot_ii_set_recovery_info (int bus, bus_recovery_info_T* info);
extern int i2c_bus_test (int bus, i2c_bus_test_T* info);
extern int set_i2c_bus_recovery_check_time(int bus, int time_ms);

static int __init 
i2c_pilot_ii_init( void )
{
    int i;

  /* Initialize driver's itnernal data structures */
  for( i = 0; i < bus_count; i++ )
    {
	  
	  i2c_init_internal_data(i);
	  se_pilot_ii_data_ptr[i].Time_Out = 0x0; //keeping it out side as"init_internal_data"
	  se_pilot_ii_data_ptr[i].bus_speed=100; //default we are keeping 100k

  	  //Initialize the bus busy status and reset counters.
	  se_pilot_ii_data_ptr[ i ].bus_busy_count = 0;
	  se_pilot_ii_data_ptr[ i ].bus_busy_timestamp = 0;
	  se_pilot_ii_data_ptr[ i ].bus_reset_count = 0; // Reset Counter for i2c which will not be initialized again.
	  se_pilot_ii_data_ptr[ i ].continuous_bus_reset = 0; // Continuous Reset Counter.
	  se_pilot_ii_data_ptr[ i ].bus_timeout_reset_count = 0;
	  se_pilot_ii_data_ptr[ i ].continuous_bus_timeout_reset = 0;
	  
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
	  se_pilot_ii_data_ptr[i].recover_start_counter = 0;
	  se_pilot_ii_data_ptr[i].recover_stop_counter = 0;
	  se_pilot_ii_data_ptr[i].recover_rxfull_counter = 0;
	  se_pilot_ii_data_ptr[i].recover_clock_stretch = 0;
	  se_pilot_ii_data_ptr[i].rxfull_stop_counter = 0;
	  se_pilot_ii_data_ptr[i].missing_stop_counter = 0;
	  
#endif
    }
#ifdef INT_MONITOR
  int_monitor.start_position = 0;
  int_monitor.monitor_count = 0;
  int_monitor.start_flag= 1;
  int_monitor.bus = 0xFF;
#endif
#ifdef BUS_BUSY_MONITOR
  bus_busy_monitor.start_position = 0;
  bus_busy_monitor.monitor_count = 0;
#endif
  
  /* Request irq */
  for( i = 0; i < bus_count; i++ )
    {
	  
	  i2c_pilot_ii_disable_all_interrupts(i);
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
         if( request_irq( irq[ i ], irq_handlers[i], IRQF_DISABLED, "i2c_pilot_ii", (void *)irq_vadd[i]) < 0 )
#else
         if( request_irq( irq[ i ], irq_handlers[i], SA_INTERRUPT, "i2c_pilot_ii", NULL ) < 0 )
#endif
	    {
	      printk( KERN_ERR "Request for irq %d failed\n", irq[ i ] );
	      return( -ENODEV );
	    }
    }
  
  /* Initialize the hardware */
  for( i = 0; i < bus_count; i++ )
    {

	  i2c_init_hardware(i);
	  
	  //moving following lines from init_harware,as this func will be called from time
	  //out handler
	  //programme time out vlaues
	  i2c_prog_time_out_values(i,1);
      
    }
  
  /* Register SMBus timeout interrupt handler */
  if( request_irq( IRQ_ICMB, i2c_TimeOut_Handler, IRQF_DISABLED, "i2c_pilot_to", NULL ) < 0 )
  {
      printk( KERN_ERR "Request for IRQ_ICMB (SMBus Timeout) failed\n");
      return( -ENODEV );
  }
  
  /* Register ourselves with the upper layers */
  for( i = 0; i < bus_count; i++ )
    {
    

      /* Set up i2c_adapter structs */
      se_pilot_ii_ops[ i ].owner = THIS_MODULE;
      se_pilot_ii_ops[ i ].class = I2C_CLASS_HWMON;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
      se_pilot_ii_ops[ i ].id = I2C_ID;
#endif
      se_pilot_ii_ops[ i ].algo_data = &se_pilot_ii_data_ptr[i];
      se_pilot_ii_ops[ i ].nr = i;
      sprintf( se_pilot_ii_ops[ i ].name, "i2c-pilot_ii-%d", i );
      
      /* Add the bus via the algorithm code */
      if( i2c_pilot_ii_add_bus( &se_pilot_ii_ops[ i ] ) != 0 )
	{
	  printk( KERN_ERR "Cannot add bus %d to algorithm layer\n", i );
	  return( -ENODEV );
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
      printk( KERN_INFO "Registered bus id: %s\n", se_pilot_ii_ops[ i ].dev.bus_id );
#else
      printk( KERN_INFO "Registered bus id: %s\n", kobject_name(&se_pilot_ii_ops[ i ].dev.kobj));
#endif
	
    }
  
  /* Enable the bus to start working */
  for( i = 0; i < bus_count; i++ )
    {
      i2c_enable_bus(i);
      
    }
  
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
    i2c_stat_proc_init();
#endif
  return 0;
}

static void 
i2c_pilot_ii_exit( void )
{
  int i;
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
	i2c_stat_proc_clear();
#endif
  for( i = 0; i < bus_count; i++ )
   {
     i2c_disable_bus(i);
     
     i2c_pilot_ii_del_bus( &se_pilot_ii_ops[ i ] );
     
     i2c_pilot_ii_disable_all_interrupts(i);
     
     i2c_disable_time_out_interrupt(i);
     
     /* Disable the irq */
     disable_irq( irq[ i ] );
     
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
     free_irq( irq[ i ], (void *)irq_vadd[i]);
#else
     free_irq( irq[ i ], 0);
#endif
   }
  
  disable_irq(IRQ_ICMB );
  free_irq( IRQ_ICMB , NULL);
  
  return;
}

/* Return bitfield describing our capabilities */
static u32 
pilot_ii_func( struct i2c_adapter *adap )
{
    return( I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL );
}



/* Algorithm level handling of ioctls and other control commands */
static int 
pilot_ii_control( struct i2c_adapter *i2c_adap, unsigned long cmd,
                       unsigned long arg )
{
    int retval = 0;
    bus_recovery_info_T	bus_recovery_info;
    i2c_bus_test_T testData;
    i2c_link_state_T linkData;
#ifdef CONFIG_SPX_FEATURE_MCTP_SUPPORT
    char *tmp;
#endif            

    switch( cmd )
    {
        case I2C_SET_HOST_ADDR:
            i2c_pilot_ii_set_slave( i2c_adap->nr, arg );
            break;

        case I2C_GET_HOST_ADDR:
            retval = i2c_pilot_ii_get_slave( i2c_adap->nr );
            break;

        case I2C_RESET:
            //	printk("I2C%d: Resetting the controller\n",i2c_adap->nr);
            /* Reset the controller */
            i2c_init_internal_data(i2c_adap->nr);
            i2c_init_hardware(i2c_adap->nr);
            //enable time out counters
            i2c_to_counter_en(i2c_adap->nr);
            //Enable Time out interrupts
            i2c_time_out_interrupt(i2c_adap->nr);
            break;

      
      
		case I2C_SET_REC_INFO:
			if (copy_from_user(&bus_recovery_info, (void*)arg, sizeof(bus_recovery_info_T))) 
				return -EFAULT;
			if (0 != pilot_ii_set_recovery_info (i2c_adap->nr, &bus_recovery_info)) 
			{
				dev_err (&i2c_adap->dev, "pilot_ii_control: set recovery info failed\n");
			}
			break;
		case I2C_GET_REC_INFO:
			if (0 != pilot_ii_get_recovery_info (i2c_adap->nr, &bus_recovery_info)) 
			{
				dev_err (&i2c_adap->dev, "pilot_ii_control: get recovery info failed\n");
			}
			if (copy_to_user((void*)arg, &bus_recovery_info, sizeof(bus_recovery_info_T))) 
				return -EFAULT;
			break;
		case I2C_SET_SPEED:
			retval = set_speed(i2c_adap->nr, arg);
			break;
		
		case I2C_GET_LINK_STATE:
			linkData.i2cLinkState = se_pilot_ii_data_ptr[i2c_adap->nr].i2c_link_state; 
			if (copy_to_user((void*)arg, &linkData, sizeof(i2c_link_state_T))) 
				retval  =  -EFAULT;

			break;			

		case I2C_BUS_TEST:
			if  (copy_from_user (&testData, (void*) arg, sizeof(i2c_bus_test_T)))
				return -EFAULT;

			if (0 != i2c_bus_test (i2c_adap->nr, &testData)) 
			{
				dev_err (&i2c_adap->dev, "i2c-ioctl: I2C_BUS_TEST failed\n");
			}
			if (copy_to_user((void*)arg, &testData, sizeof(i2c_bus_test_T))) 
				retval =  -EFAULT;
			break;

        case I2C_MCTP_READ:
#ifdef CONFIG_SPX_FEATURE_MCTP_SUPPORT
            tmp = kmalloc(MAX_MCTP_MSG_SIZE,GFP_KERNEL);
            if(!tmp)
                return -ENOMEM;
            retval = pilot_ii_mctp_recv(i2c_adap,tmp);
            if(retval > 0)
                retval = copy_to_user((char*)arg,tmp,retval)? -EFAULT:retval;
            kfree(tmp);
#else
            retval = -ENOTTY;
#endif            
            
            break;
        case I2C_SYS_RESET:
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
                i2c_init_internal_data(i2c_adap->nr);
                i2c_sys_reset(i2c_adap->nr);
                break;
#endif
        /* Set "wait for bus free" bus busy timeout */
        case I2C_SET_BB_TIMEOUT:
            retval = i2c_set_busbusy_timeout(i2c_adap->nr, arg);
            break;

        /* Disable I2C port Slave Interface */
        case I2C_DISABLE_SLAVE:
            retval = i2c_pilot_ii_disable_slave(i2c_adap->nr);
            break;

            /* Set I2C bus recovery stuck signal check time (in milliseconds */ 
        case I2C_RECOV_CHECK_MS: 
            retval = set_i2c_bus_recovery_check_time(i2c_adap->nr, arg); 
            break;
            
        case ENABLE_SSIF:
           	//printk("I2C%d: Enable slave xfer for SSIF\n",i2c_adap->nr);
    	    i2c_pilot_slave_xfer_enable(i2c_adap->nr); 
		    break;
			
        default:
            dev_err( &i2c_adap->dev, "pilot_ii_control: Unknown ioctl command\n" );
            retval = -ENOTTY;
    }

    return( retval );
}


static struct i2c_algorithm pilot_ii_algo =
{
    .master_xfer = pilot_ii_master_xfer,
    .smbus_xfer = pilot_ii_smb_xfer,
    .functionality = pilot_ii_func,
    .i2c_hw_ioctl = pilot_ii_control,
    .slave_recv = pilot_ii_slave_recv,
	.slave_send = pilot_ii_slave_send,
     .i2c_recovery = pilot_ii_i2c_bus_recovery,
};

int 
i2c_pilot_ii_add_bus( struct i2c_adapter *i2c_adap )
{
    /* Register new adapter */
    i2c_adap->algo = &pilot_ii_algo;

    /* Set some sensible depilot_iiults */
    i2c_adap->timeout = DEFAULT_TIMEOUT;
    i2c_adap->retries = DEFAULT_RETRIES;

    i2c_add_adapter( i2c_adap );
    printk("I2C%d: Hardware routines registered\n",i2c_adap->nr);
    log_i2c_proc_open( i2c_adap );

    return(0);
}

int 
i2c_pilot_ii_del_bus( struct i2c_adapter *i2c_adap )
{
    log_i2c_proc_release( i2c_adap );
    i2c_del_adapter( i2c_adap );
    return 0;
}

MODULE_AUTHOR( "American Megatrends" );
MODULE_DESCRIPTION( "Pilot I2C Algorithm" );
MODULE_LICENSE( "GPL" );

module_init( i2c_pilot_ii_init );
module_exit( i2c_pilot_ii_exit );
