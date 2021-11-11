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
#ifndef __MISCCTRL_H__
#define __MISCCTRL_H__

#define ENABLE_CHASSIS_INTRUSION			_IOC(_IOC_WRITE,'M',0x100,0x3FFF)
#define REGISTER_CHASSIS_INTRUSION			_IOC(_IOC_WRITE,'M',0x101,0x3FFF)
#define WAIT_FOR_CHASSIS_INTRUSION			_IOC(_IOC_WRITE,'M',0x102,0x3FFF)
#define UNREGISTER_CHASSIS_INTRUSION		_IOC(_IOC_WRITE,'M',0x103,0x3FFF)
#define DISABLE_CHASSIS_INTRUSION			_IOC(_IOC_WRITE,'M',0x104,0x3FFF)
#define GET_INTRUSION_RAW_STATUS			_IOC(_IOC_WRITE,'M',0x105,0x3FFF)
#define SET_PWRBTOUT						_IOC(_IOC_WRITE,'M',0x106,0x3FFF)
#define SET_POWER_BT_LOCK					_IOC(_IOC_WRITE,'M',0x107,0x3FFF)
#define GET_ONCTLnSTATUS					_IOC(_IOC_WRITE,'M',0x108,0x3FFF)
#define GET_PWRGDSTATUS						_IOC(_IOC_WRITE,'M',0x109,0x3FFF)
#define ENABLE_DISABLE_PWRGD_INTR			_IOC(_IOC_WRITE,'M',0x10A,0x3FFF)
#define WAIT_FOR_PWRGD_CHANGE				_IOC(_IOC_WRITE,'M',0x10B,0x3FFF)
#define WAIT_FOR_PWBTIN_INTR				_IOC(_IOC_WRITE,'M',0x10C,0x3FFF)
#define ENABLE_DISABLE_SLP_STATE_INTR		_IOC(_IOC_WRITE,'M',0x10D,0x3FFF)
#define GET_SLP5STATUS						_IOC(_IOC_WRITE,'M',0x10E,0x3FFF)
#define GET_SLP3STATUS						_IOC(_IOC_WRITE,'M',0x10F,0x3FFF)
#define WAIT_FOR_SLP_STATE_INTR				_IOC(_IOC_WRITE,'M',0x110,0x3FFF)
#define ENABLE_PASSTHROUGH					_IOC(_IOC_WRITE,'M',0x111,0x3FFF)
#define DISABLE_PASSTHROUGH					_IOC(_IOC_WRITE,'M',0x112,0x3FFF)
#define SET_PASSTHRU_DATA					_IOC(_IOC_WRITE,'M',0x113,0x3FFF)
#define POWER_ON_OFF						_IOC(_IOC_WRITE,'M',0x114,0x3FFF)
#define GET_SWC_REG_VAL						_IOC(_IOC_WRITE,'M',0x115,0x3FFF)
#define SET_SWC_REG_VAL						_IOC(_IOC_WRITE,'M',0x116,0x3FFF)
#define WAIT_FOR_PASS_THRU_INTR				_IOC(_IOC_WRITE,'M',0x117,0x3FFF)
#define ENABLE_DISABLE_PASS_THRU_INTR		_IOC(_IOC_WRITE,'M',0x118,0x3FFF)
#define ENABLE_DISABLE_PWBTIN_INTR			_IOC(_IOC_WRITE,'M',0x119,0x3FFF)
#define REGISTER_SWC_INTS					_IOC(_IOC_WRITE,'M',0x11A,0x3FFF)
#define UNREGISTER_SWC_INTS					_IOC(_IOC_WRITE,'M',0x11B,0x3FFF)
#define WAIT_FOR_SWC_INT					_IOC(_IOC_WRITE,'M',0x11C,0x3FFF)
#define GEN_SMI_SW_EVENT					_IOC(_IOC_WRITE,'M',0x11D,0x3FFF)
#define GEN_SCI_SW_EVENT					_IOC(_IOC_WRITE,'M',0x11E,0x3FFF)
#define GET_SYS_CLK_CTRL_REG_VAL			_IOC(_IOC_WRITE,'M',0x11F,0x3FFF)
#define SET_SYS_CLK_CTRL_REG_VAL			_IOC(_IOC_WRITE,'M',0x120,0x3FFF)
#define GET_RTC_REG_VAL						_IOC(_IOC_WRITE,'M',0x121,0x3FFF)
#define SET_RTC_REG_VAL						_IOC(_IOC_WRITE,'M',0x122,0x3FFF)
#define BATTERY_BACKED_READ					_IOC(_IOC_WRITE,'M',0x123,0x3FFF)
#define BATTERY_BACKED_WRITE				_IOC(_IOC_WRITE,'M',0x124,0x3FFF)
#define INTRUSION_STATE_ON_INIT				_IOC(_IOC_WRITE,'M',0x125,0x3FFF)
#define ACPI_PWR_OFF_STATE_SYNC				_IOC(_IOC_WRITE,'M',0x126,0x3FFF)

/* IOCTL Commands and structure */
typedef enum {
	POWER_GOOD_INTR,
	PASS_THRU_INTR,
	PWRBTNIN_INTR,
	SLP_STATE_INTR
}eIntrType;

typedef struct _swc_interrupt
{
	int int_num;
	int int_source;
	int int_type;
	unsigned char int_input_data;
	unsigned char int_output_data;
} swc_interrupt_t;

typedef struct miscctrlcmd 
{
	unsigned char Index;
	unsigned int data;
	unsigned int timeout;
} MISCCTRLCMD;

/******This is an internal structure in the driver which is used for maintaining qs******/
typedef struct _pending_swc_interrupt
{
	int int_num;
	unsigned char  output_data;
	struct _pending_swc_interrupt *next;
}  __attribute__((packed)) pending_swc_interrupt_t;

typedef struct _swc_interrupt_data
{
	unsigned int total_interrupts;
	swc_interrupt_t *int_swc_info;
}  __attribute__((packed)) swc_interrupt_data_t;


typedef MISCCTRLCMD miscctrl_ioctl_data;

#define MAX_BAT_BACKED_REG 32

#define MISCCTRL_CTL_FILE	"/dev/miscctrl"


/* Device Properties */
#define MISCCTRL_DEVICE_MAJOR	102
#define MISCCTRL_DEVNAME		"MISCCTRL"

/* Pilot-ii RTC Registers */
#define RTC_STAT_REG	0x10
#define RTC_ICR_REG		0x14
#define RTC_BBR_BASE	0x40

/* Pilot-ii SWM Registers */
#define WKP_STS0_REG        0x00
#define WKP_STS1_REG        0x01
#define WKP_EN0_REG         0x02
#define WKP_EN1_REG         0x03
#define WKP_CFG0_REG        0x04
#define WKP_CFG1_REG        0x05
#define WKP_SMIEN0_REG      0x06
#define SW_CTL_REG          0x09
#define SWC_STAT0_REG       0x0A
#define SWC_STAT1_REG       0x0B
#define SLP_ST_REG          0x26
#define PASS_CTL_REG        0x27
#define PASS_STS_REG        0x28
#define GPE1_STS0_REG       0x2F
#define GPE1_STS1_REG       0x30
#define GPE1_EN0_REG        0x31
#define GPE1_EN1_REG        0x32
#define PWBTIN_STS_REG      0x37

/* Pilot-ii PASS-THRU Register */
#define PASS1_CFG 0x34
#define PASS2_CFG 0x35
#define PASS3_CFG 0x36

/* Pilot-ii specific registers */
#define LPC_CTRL0_REG    0x10
#define LPC_CTRL1_REG    0x11

#define PRS_SIOCFG7_REG  0x27
#define SIOCFG7_CLK_DSBL ((unsigned char)1<<7)

//For access the required Bits in Registers
#define CHASSIS_INTRUSION_STATE_CHANGE	(0x1<<2)
#define ENABLE_CHASSIS_INTRUSION_INTR	(0x1<<2)
#define CHASSIS_INTRUSION_RAW_STATUS    (0x1<<3)

#define PWRBTNOUT_HIGH (0x1<<6)
#define  PWRBTNOUT_LOW  0xBF
#define PWRONCTLSTATUS (0x10)
#define PWRGDSTATUS (0x8)
#define PWRGDINTSTATUS 0x2
#define SLPS5_STATUS (0x2)
#define SLPS3_STATUS (0x1)
#define SLP_INTR_STATUS 0x04
#define ENABLE_SLP_STATE_INTR 0x20
#define  ENABLE_PASSTHRUIN1 0xFE
#define ENABLE_PASSTHRUIN2 0xFB
#define ENABLE_PASSTHRUIN3 0xEF
#define ENABLE_PWRGD_INTR 0x1
#define DISABLE_PWRGD_INTR 0xFE 
#define ENABLE_PASS_THRU_IRQ 0x40
#define POWER_GOOD_BIT 0x04
#define PASS_THRU_STATUS_BIT 0x38
#define PWR_BT_LOCK_POS 5
#define PWBTIN_STATUS_BIT 0x03
#define ENABLE_PWR_BT_LOCK 0x20
#define ENABLE_PWRBTOUT 0x40

/* LPC Control Macro */
#define ENABLE_GLOBAL_SMI_EVENT         0x20    // 5th bit for Global SMI.
#define ENABLE_GLOBAL_SCI_EVENT         0x10    // 4th bit for Global SCI.
#define SET_SW_EVENT_STS                0x40    // 6th bit for Software Event Status.
#define GEN_SW_EVENT                    0x10    // 4th bir for Generate Software event. 

#define ENABLE_GLOBAL_EVENT             0x01
#define ENABLE_LOCAL_EVENT              0x02
#define GENERATE_EVENT                  0x04

#define POWER_CHANGE_NO_WAIT 0
#define POWER_CHANGE_WAIT_FOR_PWRGOOD 1

/* System & Clock Control */
#define SYSCFG          0x00        // System Configuration Register
#define SCLKSTS         0x04        // System Clock Status Register
#define SCLKCTL         0x08        // System Clock Control Register
#define STRPSTS         0x0C        // System Strap Option Status Register
#define DDRPLLCTL       0x10        // DDR PLL Control Register
#define SYSPLLCTL       0x14        // SYSPLL Control Register
#define SYSMICSCLKCTL   0x20        // System Miscellaneous Clock Control Register
#define I2CPCT0         0x24        // I2C Pin Control Register 0
#define I2CPCT1         0x28        // I2C Pin Control Register 1
#define UPCTL           0x2C        // UART Pin Control Register
#define IOENGCCFG       0x30        // I/O Enable & Graphics Controller Control Register
#define MISCTL          0x34        // I2C RST Control Register
#define CIRQCTL         0x38        // Combined IRQ Interrupt Control Register
#define CFIQCTL         0x3C        // Combined FIQ Interrupt Control Register
#define CIRQSTS         0x40        // Combined IRQ Interrupt Status Register
#define CFIQSTS         0x44        // Combined FIQ Interrupt Status Register
#define PCIDID          0x48        // PCI System Vendor and Device ID Register
// #define PCISID          0x4C        // PCI Subsystem Vendor and Device ID Register  - Already defiend in hwreg.h
// #define SEDID           0x50        // Server Engines Device ID Register - - Already defiend in hwreg.h
#define PIDBG0          0x54        // MAC and Graphic Control Register
#define PIDBG1          0x58        // Pilot Debug1 Register
#define RSTCTL          0x5C        // Pilot Top level Reset Control Register
#define CIRQCTLHI       0x60        // Upper Combined IRQ Control Register [46:32]
#define CFIQCTLHI       0x64        // FIQ Control Register High for FIQ [46:32]
#define CIRQSTSHI       0x68        // IRQ Status Register High for IRQ[46:32]
#define CFIQSTSHISSP    0x6C        // FIQ Status Register High for FIQ[46:32]
#define CIRQCTLSSP      0x70        // IRQ Control register for SSP
#define CFIQCTLSSP      0x74        // FIQ Control register for SSP
#define CIRQSTSSSP      0x78        // IRQ Status register for SSP
#define CFIQSTSSSP      0x7C        // FIQ Status register for SSP
#define CIRQSTSHISSP    0x80        // IRQ Status register for SSP [46:32]
#define MACRGMIICTL     0x88        // MAC RGMII Control Register

int install_swc_handler(int (*pt2Func)(void), unsigned char intr_type);
int uninstall_swc_handler(int (*pt2Func)(void), unsigned char intr_type);
#endif
