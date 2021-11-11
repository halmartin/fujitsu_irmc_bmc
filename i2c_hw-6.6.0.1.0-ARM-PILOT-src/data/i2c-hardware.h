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

#ifndef _I2C_HARDWARE_H
#define _I2C_HARDWARE_H
#include <linux/version.h>

#define ARM_PROCESSOR			0x01
#define SOFTWARE_RESET_ENABLE	0x02

/* Sensible Defaults */
#define DEFAULT_TIMEOUT     	( 100 )         /* Timeout/delay for bus activities */
#define DEFAULT_RETRIES     	( 3 )           /* Retries on send/receive */
#define DEFAULT_BB_RETRIES  	( 10 )          /* Retries to get a free bus */
#define PILOT_II_SIGNAL_RECEIVED  (0xffff)
#define MAX_MCTP_MSG_SIZE   164

#define I2C_CON_REG				0X00
#define I2C_TAR_REG				0X04
#define I2C_SAR_REG				0X08
#define I2C_HS_MADDR_REG		0X0C
#define I2C_DATA_CMD_REG		0X10
#define I2C_SS_SCL_HCNT_REG		0X14
#define I2C_SS_SCL_LCNT_REG		0X18
#define I2C_FS_SCL_HCNT_REG		0X1C
#define I2C_FS_SCL_LCNT_REG		0X20
#define I2C_HS_SCL_HCNT_REG		0X24
#define I2C_HS_SCL_LCNT_REG		0X28
#define I2C_INTR_STAT_REG		0X2C
#define I2C_INTR_MASK_REG		0X30
#define I2C_RAW_INTR_STAT_REG	0X34
#define I2C_RX_TL_REG			0X38
#define I2C_TX_TL_REG			0X3C
#define I2C_CLR_INTR_REG		0X40
#define I2C_CLR_RX_UNDER_REG	0X44
#define I2C_CLR_RX_OVER_REG  	0X48
#define I2C_CLR_TX_OVER_REG		0X4C
#define I2C_CLR_RD_REQ_REG		0X50
#define I2C_CLR_TX_ABRT_REG		0X54
#define I2C_CLR_RX_DONE_REG		0X58
#define I2C_CLR_ACTIVITY_REG	0X5C
#define I2C_CLR_STOP_DET_REG	0X60
#define I2C_CLR_START_DET_REG	0X64
#define I2C_CLR_GEN_CALL_REG	0X68
#define I2C_ENABLE_REG			0X6C
#define I2C_STATUS_REG			0X70
#define I2C_TXFLR_REG			0X74
#define I2C_RXFLR_REG			0X78
#define I2C_SRESET_REG			0X7C
#define I2C_TX_ABORT_SOURCE_REG	0X80
#define I2C_COMP_PARAMS_REG		0Xf4
#define I2C_COMP_VERSION_REG	0Xf8


//	IC_CON		
#define SLAVE_MODE				(0<<0)
#define MASTER_MODE				(1<<0)	//1=MASTER ENABLED ,0=MASTER DISABLED
#define SPEED					(3<<1)	//mask for speed mode
#define SPEED_100KB				(1<<1)	//100K Bits
#define SPEED_400KB				(2<<1)	//400K Bits
#define SPEED_34MB				(3<<1)	//3.4M Bits
#define IC_10BIT_ADDR_SLAVE		        (1<<3)	//1=10 BIT ADDR SLAVE
#define IC_10BIT_ADDR_MASTER	                (1<<4)	//1=10 BIT ADDR MASTER
#define IC_RESTART_EN			        (1<<5)	//
#define IC_SLAVE_DISABLE		        (1<<6)	//0 =ENABLE 1=DISABLE
#define STOP_DET_FILTER			(1<<7)  // Pilot 4 IC_CON STOP detect filtering bit
#define RXFIFO_CLK_STRETCH		(1<<9)  // Pilot 4 IC_CON RX FIFO Clock stretching bit
#define IC_TAR_ADDR				(0x3ff) //TARGET ADDR field for master transactions		
#define GC_OR_START				(1<<10)
#define SPECIAL					(1<<11) //0 =IGNORE BIT 10

// I2C Bus Speed Modes
#define I2C_BUS_SPEED_MODE_STANDARD     100
#define I2C_BUS_SPEED_MODE_FAST         400
#define I2C_BUS_SPEED_MODE_HIGH_SPEED   3400
// IC_TAR
#define IC_TAR_10BMASTER		(1<<12)
#define DAT					(0XFF)	//DATA FIELD
#define CMDREAD					(1<<8)  //1=READ,0=WRITE
#define CMDWRITE				(0<<8)
#define CMDSTOP					(1<<9)

//	IC_INTR_STAT,IC_INTR_MASK,IC_INTR_STAT
#define RX_UNDER				(1<<0)
#define RX_OVER					(1<<1)
#define RX_FULL					(1<<2)
#define TX_OVER					(1<<3)
#define TX_EMPTY				(1<<4)
#define RD_REQ					(1<<5)
#define TX_ABRT					(1<<6)
#define RX_DONE					(1<<7)
#define ACTIVITY				(1<<8)
#define STOP_DET				(1<<9)
#define START_DET				(1<<10)
#define GEN_CALL				(1<<11)

//	IC_ENABLE
#define ENABLEI2C				(1<<0)		//1=enable  0 =Disable

//	IC_STATUS
#define IC_STATUS_ACTIVITY		        (1<<0)
#define TFNF					(1<<1)		//Transmit FIFO Not Full
#define TFE					(1<<2)		//Transmit FIFO empty
#define RFNE					(1<<3)		//Transmit FIFO Not Empty
#define RFF					(1<<4)		//Transmit FIFO full


//	IC_SRESET
#define SRESET_ALL				(1<<0)		//Soft reset all
#define SRESET_MASTER			        (1<<1)		//Soft reset Master machines
#define SRESET_SLAVE			        (1<<2)		//Soft reset Slave machines

//	IC_TX_ABRT_SOURCE

#define ABRT_7B_ADDR_NOACK		(1<<0)
#define ABRT_10ADDR1_NOACK		(1<<1)
#define ABRT_10ADDR2_NOACK		(1<<2)
#define ABRT_TXDATA_NOACK		(1<<3)
#define ABRT_GCALL_NOACK		(1<<4)
#define ABRT_GCALL_READ			(1<<5)
#define ABRT_HS_ACKDET			(1<<6)
#define ABRT_SBYTE_ACKDET		(1<<7)
#define ABRT_HS_NORSTRT			(1<<8)
#define ABRT_SBYTE_NORSTRT		(1<<9)
#define ABRT_10B_RD_NORSTRT		(1<<10)
#define ABRT_MASTER_DIS			(1<<11)
#define ABRT_LOST				(1<<12)
#define ABRT_SLVFUSH_TXFIFO		(1<<13)
#define ABRT_SLV_ARBLOST		(1<<14)
#define ABRT_SLVRD_INTX			(1<<15)

//;IC_COMP_PARAM_1

#define RX_BUFFER_DEPTH			8	// 15:8
#define TX_BUFFER_DEPTH			16	// 23:16

#define I2C_CLK_DR_LOW(x)		(1 << (1 + x*8))
#define I2C_DAT_DR_LOW(x)		(1 << (5 + x*8))
#define SET_CLK_NO_DR(x)		(0 << (1 + x*8))
#define SET_DAT_NO_DR(x)		(0 << (5 + x*8))

//I2C Time Outs
#define   SMB_MTO      (1)
#define   SMB_STO      (1<<1)
#define   SMB_GTO      (1<<2)
#define   SMB_TFIFO_TO (1<<3)
#define   I2C_TO_BITS   ((SMB_TFIFO_TO)|(SMB_MTO)|(SMB_STO)|(SMB_GTO))
//#define   I2C_TO_BITS   ((SMB_TFIFO_TO)|(SMB_STO)|(SMB_GTO))  
#define   MCTP_I2C_RECV_TIMEOUT 2000  /* 2000 milliseconds */

//P3_Features
#define  EN_STOP_DET           (1)
#define  EN_BUS_HANG_FIX       (1<<1)
#define  EN_CLK_STREACHING     (1<<2)
#define  EN_TXFIFO_TO          (1<<3)

#define  EN_P3_FEATURES       ((EN_TXFIFO_TO)|(EN_CLK_STREACHING)|(EN_BUS_HANG_FIX)|(EN_STOP_DET))

  
//I2C registers inside system control block
#define GIPO2EN_I2C7   ((1<<23)|(1<<24))
#define GIPO2EN_I2C6   ((1<<20)|(1<<21))

#define PINMUXCTL_I2C7  ((1<<20)|(1<<21))
#define PINMUXCTL_I2C6  ((1<<18)|(1<<19))  
  
#define I2CPCT0         0x24
#define I2CPCT1         0x28
#define TOPLEVEL_PINCTRL 0x800
#define GPIO0EN          (TOPLEVEL_PINCTRL+0x0)
#define PINMUXCTL         (TOPLEVEL_PINCTRL+0x4)
#define GPIO1EN           (TOPLEVEL_PINCTRL+0x8)
#define GPIO2EN           (TOPLEVEL_PINCTRL+0xc)

  
  
#define I2C_SMB_CTRL_0  0x900
#define I2C_SMB_CTRL_1  0x904
#define I2C_SMB_CTRL_2  0x908
#define I2CPCT2         0x910
#define I2C0_TO_Counter 0x918
#define I2C1_TO_Counter 0x91c
#define I2C2_TO_Counter 0x920
#define I2C3_TO_Counter 0x924
#define I2C4_TO_Counter 0x928
#define I2C5_TO_Counter 0x92c
#define I2C6_TO_Counter 0x930
#define I2C7_TO_Counter 0x934
#define I2C8_TO_Counter 0x964
#define I2C9_TO_Counter 0x968
#define I2C_TO_INTR_EN  0x938
#define I2C_TO_INTR_EN_1 0x96C  // Pilot 4 register
#define I2C_COUNTER_STS_1  0x974
#define I2C_TO_EN       0x93C
#define I2C_TO_EN_1     0x970  // Pilot 4 register
#define I2C_TO_INTR_STS 0x940
#define I2C_DBG_STS     0x944
#define I2C_DBG_STS_1   0x978  // Pilot 4 register
#define I2C0_P3_FEATURE_EN 0x948
#define I2C1_P3_FEATURE_EN 0x949
#define I2C2_P3_FEATURE_EN 0x94A
#define I2C3_P3_FEATURE_EN 0x94B
#define I2C4_P3_FEATURE_EN 0x94C
#define I2C5_P3_FEATURE_EN 0x94D
#define I2C6_P3_FEATURE_EN 0x94E
#define I2C7_P3_FEATURE_EN 0x94F
#define I2C8_P3_FEATURE_EN 0x97C
#define I2C9_P3_FEATURE_EN 0x97E
#define I2CPCT3         0x960
#define I2C_ENFIL          0x950
#define SYSRCR  0x700
#define SYSRST  0x704
#define SYSWRER 0x708
#define SYSSRER 0x70C
 
  
  
  
u32 i2c_pilot_ii_read_reg( int bus, u32 offset );
void i2c_pilot_ii_write_reg( int bus, u32 value, u32 offset );

void i2c_init_hardware(int bus);
int i2c_wait_for_bus_free(int bus);
int i2c_wait_for_bus_free_ext(int bus); /* Extended wait for bus free */
int i2c_bus_recovery(int bus, int force_stop); /* Unconditionally force a STOP when enabled */
int i2c_pilot_ii_disable_slave(int bus);
int i2c_pilot_ii_enable_slave(int bus);
void i2c_pilot_ii_disable_all_interrupts(int bus);

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(2,6,24)) 
irqreturn_t i2c_handler_0( int this_irq, void *dev_id);
irqreturn_t i2c_handler_1( int this_irq, void *dev_id);
irqreturn_t i2c_handler_2( int this_irq, void *dev_id);
irqreturn_t i2c_handler_3( int this_irq, void *dev_id);
irqreturn_t i2c_handler_4( int this_irq, void *dev_id);
irqreturn_t i2c_handler_5( int this_irq, void *dev_id);
irqreturn_t i2c_handler_6( int this_irq, void *dev_id);
irqreturn_t i2c_handler_7( int this_irq, void *dev_id);
irqreturn_t i2c_handler_8( int this_irq, void *dev_id);
irqreturn_t i2c_handler_9( int this_irq, void *dev_id);
#else
irqreturn_t i2c_handler_0( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_1( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_2( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_3( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_4( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_5( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_6( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_7( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_8( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
irqreturn_t i2c_handler_9( int this_irq, void *dev_id,
                                 struct pt_regs *regs );
#endif
irqreturn_t i2c_TimeOut_Handler( int this_irq, void *dev_id);


void i2c_enable_bus(int bus);
void i2c_disable_bus(int bus);
void i2c_pilot_ii_disable_interrupt(int bus, unsigned long mask);
void i2c_pilot_ii_enable_interrupt(int bus, unsigned long mask);

int pilot_ii_master_xfer(struct i2c_adapter *i2c_adap, struct i2c_msg *msgs, int num );
int pilot_ii_smb_xfer( struct i2c_adapter *i2c_adap, u16 addr, unsigned short flags,
                            char read_write, u8 command, int size, union i2c_smbus_data * data);
int pilot_ii_slave_recv( struct i2c_adapter *i2c_adap, char *buf, int num );
int pilot_ii_mctp_recv(struct i2c_adapter *i2c_adap,char *buf);


int i2c_bus_recovery(int bus, int force_stop); /* Unconditionally force a STOP when enabled */
int pilot_ii_i2c_bus_recovery(struct i2c_adapter *i2c_adap);

//Bit-bang funtion
void pilot_bit_bang_select(int bus);
void pilot_bit_bang_deselect(int bus);
void drive_clock_low (int bus);
void drive_clock_high (int bus);
void drive_data_low (int bus);
void drive_data_high (int bus);
unsigned long get_clk_status(int bus);
unsigned long get_sda_status(int bus);

void i2c_pilot_ii_set_slave( int bus, int slave );
int i2c_pilot_ii_get_slave( int bus );
int i2c_pilot_ii_get_raw_intr_sts( int bus );
void i2c_pilot_ii_reset(int bus);
void i2c_to_counter_en(int bus);
void Common_I2c_Timeout(int bus, int status);
void Clock_Stretch_Timeout(int bus, int status);	/* SMBus clock low interrupt */
void i2c_time_out_interrupt(int bus);
void i2c_prog_time_out_values(int bus,int mode);
void i2c_disable_time_out_interrupt(int bus);
void i2c_disable_to_counters(int bus);
int set_speed(int bus,int speed);
unsigned int Get_Activity_info(int bus);
unsigned int Get_Clk_Streaching_info(int bus);
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
int i2c_sys_reset(int bus);
#endif
int pilot_ii_slave_send( struct i2c_adapter *i2c_adap, char *buf, int num ); 
void i2c_pilot_slave_xfer_enable(int bus);
int i2c_set_busbusy_timeout(int bus, int timeout);  /* Set "wait for bus free" bus busy timeout */
int i2c_is_bus_idle(int but);	/* Check is I2C bus idle */

#endif

