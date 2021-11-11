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

#ifndef _I2C_DATA_H
#define _I2C_DATA_H

#include <linux/i2c.h>

#define TRANSFERSIZE 1024
#define MAX_FIFO_LEN 32
#define BUS_BUSY_MONITOR
#ifdef BUS_BUSY_MONITOR
#define MAX_BUS_BUSY_MONITOR	200
#endif
#define INT_MONITOR
#ifdef INT_MONITOR
#define INT_MONITOR_DATA
#define MAX_INTERRUPT_MONITOR	200
#endif



#if defined SOC_PILOT_IV //|| defined SOC_PILOT_III
#define BUS_COUNT  10
#elif defined SOC_PILOT_III
#define BUS_COUNT  8// Default - All 8 SE i2c bus are useable 
#else
#define BUS_COUNT  6
#endif

#define DEFAULT_SLAVE_0     ( 0x10 )
#define DEFAULT_SLAVE_1     ( 0x10 )
#define DEFAULT_SLAVE_2     ( 0x10 )
#define DEFAULT_SLAVE_3     ( 0x10 )
#define DEFAULT_SLAVE_4     ( 0x10 )
#define DEFAULT_SLAVE_5     ( 0x10 )
#define DEFAULT_SLAVE_6     ( 0x10 )
#define DEFAULT_SLAVE_7     ( 0x10 )
#ifdef SOC_PILOT_IV
#define DEFAULT_SLAVE_8     ( 0x10 )  
#define DEFAULT_SLAVE_9     ( 0x10 )  
#endif

#define DEFAULT_RX_THRESHOLD 17 //RX interrupt after 1 bytes in RX Fifo
#define DEFAULT_TX_THRESHOLD 24 //TX interrupt when TX fifo is empty
#define SLAVETX_RX_THRESHOLD 35
#define SLAVETX_TX_THRESHOLD 35

#define MASTER 0x10
#define SLAVE 0x00
#define MASTER_RECV 0x10
#define MASTER_XMIT 0x11
#define SLAVE_XMIT  0x12
#define RESTART_MRMW 0x13  
#define RESTART_MWMR 0x14

#define SLAVETX_MAX_RES_SIZE	34	//Here defined size for SSIF application: Length, Data[1..32], PEC 

/* Error codes in compliance with IPMI */
#define ARBLOST		0x80000081
#define BUSERR		0x80000082
#define NACKONWR	0x80000083
#define TRUNKRD		0x80000084

#define TMEXT     1
#define TSEXT     2
#define TTimeout  4  

#define BUS_BUSY_COUNT_LIMIT	20
#define BUS_BUSY_SECONDS		5

#define BUS_TIMEOUT_COUNT_LIMIT	10
#define BUS_TIMEOUT_SECONDS		10

#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_LOG
#define I2C_LOG_FILTER_COUNT_MAX 6

#define I2C_LOG_NONE	0
#define I2C_LOG_ALL	1
#define I2C_LOG_FILTER	2

struct log_i2c_filter
{
	unsigned addr;
	unsigned mode;
	unsigned offset;
	unsigned val;
};

struct log_i2c_conf
{
	u8 log_enabled;
	struct log_i2c_filter filters[I2C_LOG_FILTER_COUNT_MAX];
};
#endif // CONFIG_SPX_FEATURE_ENABLE_I2C_LOG

#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
typedef struct {
	unsigned master_tx;
	unsigned slave_rx;
	unsigned arblost;
	unsigned nack;
	unsigned recover;
	unsigned error;
	unsigned count;
} i2c_stat_t;
#endif // CONFIG_SPX_FEATURE_ENABLE_I2C_STAT

struct i2c_pilot_ii_data
{
	unsigned char TX_data[TRANSFERSIZE];
	int TX_len;
	int TX_index;

	unsigned char Slave_TX_data[TRANSFERSIZE];
	int Slave_TX_len;
	int Slave_TX_index;

    unsigned char MCTPRX_data[MAX_FIFO_LEN][TRANSFERSIZE];
    int MCTPRX_Writer;
    int MCTPRX_Reader;
    int MCTPRX_Len[MAX_FIFO_LEN];
    int MCTPRX_Entries;
 
	unsigned char MasterRX_data[TRANSFERSIZE];
	int MasterRX_len;
	int MasterRX_index;
        int Master_rd_cmd_index;

	unsigned char Linear_SlaveRX_data[TRANSFERSIZE];
	int Linear_SlaveRX_len;
	int Linear_SlaveRX_index;

	unsigned char SlaveRX_data[MAX_FIFO_LEN][TRANSFERSIZE];
	int SlaveRX_len[MAX_FIFO_LEN];
	int SlaveRX_index[MAX_FIFO_LEN];
	
	int SlaveRX_Writer;
	int SlaveRX_Reader;
	int SlaveRX_Entries;

	int SlaveTX_Enable;
	
	spinlock_t data_lock;
	spinlock_t i2c_irq_lock;
	spinlock_t to_irq_lock;
	spinlock_t transfer_lock;

	volatile u32 op_status;
	volatile u32 abort_status;
	int MasterXmit;
	int MasterTruncated;    /* Record when a truncated master transfer occurs */

	int master_xmit_recv_mode_flag;

	wait_queue_head_t pilot_ii_wait;
	wait_queue_head_t pilot_ii_slave_data_wait;
	wait_queue_head_t pilot_ii_mctp_data_wait;

	int start_detected;
        int Time_Out;
        int RD_REQ_Pending;
        int bus_speed;
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_LOG
	struct log_i2c_conf log_conf;
#endif
#ifdef CONFIG_SPX_FEATURE_ENABLE_I2C_STAT
	i2c_stat_t stat;
	unsigned recover_start_counter;
	unsigned recover_stop_counter;
	unsigned recover_rxfull_counter;
	unsigned recover_clock_stretch;
	unsigned rxfull_stop_counter;
	unsigned missing_stop_counter;
#endif

	int bus_busy_count;
	unsigned int bus_busy_timestamp;
	unsigned int bus_reset_count;
	unsigned int continuous_bus_reset;
	
	int bus_timeout_count;
	unsigned int bus_timeout_timestamp;
	unsigned int bus_timeout_reset_count;
	unsigned int continuous_bus_timeout_reset;
	
    int block_read;
    int block_proc_call;
    int host_notify_flag;
    int master_read_with_PEC;   //for I2C_SMBUS_BLOCK_DATA and I2C_SMBUS_BLOCK_PROC_CALL

	unsigned short i2c_link_state;
};

#ifdef INT_MONITOR
struct i2c_pilot_ii_interrupt_data
{
	unsigned long long int jiffies_value;
	unsigned int bus;
	unsigned int raw_int_stauts;
	unsigned int int_serviced;
	unsigned int send_receive;
	unsigned int data_count;
	unsigned int data_bytes[128];
	unsigned int status_reg;
	unsigned int tx_abrt_source_reg;
	unsigned int rxflr_reg;
	unsigned int txflr_reg;
};

struct i2c_pilot_ii_interrupt_monitor
{
	struct i2c_pilot_ii_interrupt_data int_data[MAX_INTERRUPT_MONITOR];
	int bus;
	unsigned int start_flag;
	unsigned int start_position;
	unsigned int monitor_count;
};
#endif

#ifdef BUS_BUSY_MONITOR
struct i2c_pilot_ii_bus_busy_data
{
	unsigned long long int jiffies_value;
	unsigned char bus;
	unsigned char i2c_con;
	unsigned char i2c_tar;
	unsigned char i2c_sar;
	unsigned char i2c_intr_stat;
	unsigned char i2c_rx_tl;
	unsigned char i2c_tx_tl;
	unsigned char i2c_enable;
	unsigned char i2c_status;
	unsigned char i2c_txflr;
	unsigned char i2c_rxflr;
	unsigned char i2c_tx_abt_src;
	unsigned char i2c_debug_sts;
};

struct i2c_pilot_ii_bus_busy_monitor
{
	struct i2c_pilot_ii_bus_busy_data data[MAX_BUS_BUSY_MONITOR];
	unsigned int start_position;
	unsigned int monitor_count;
};
#endif

int i2c_pilot_ii_add_bus( struct i2c_adapter * );
int i2c_pilot_ii_del_bus( struct i2c_adapter * );

void i2c_init_internal_data(int bus);


/*
* I2C BUS states 
*/

#define I2C_LINK_NO_FAILURE						0x00  //
#define I2C_UNABLE_DRIVE_CLK_HIGH				0x01  //
#define I2C_UNABLE_DRIVE_DATA_HIGH				0x02  //
#define I2C_UNABLE_DRIVE_CLK_LOW				0x03
#define I2C_UNABLE_DRIVE_DATA_LOW				0x04
#define I2C_CLOCK_LOW_TIMEOUT_CLK_HIGH			0x05   
#define I2C_UNDER_TEST							0x06  //  
#define I2C_UNDIAGNOSED_COMM_FAILURE			0x07  // 

/*
* I2c Test Operations
*/
#define I2C_TEST_BUS_BUSY					0x01
#define I2C_TEST_BUS_RECOVERABLE				0x02

typedef struct
{
       int i2cLinkState;
 } i2c_link_state_T;

typedef struct
{
 	int operation;
	int result;
 } i2c_bus_test_T;



#endif
