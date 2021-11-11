
/*********************************************************
 **                                                     **
 **    (C)Copyright 2009-2015, American Megatrends Inc. **
 **                                                     **
 **            All Rights Reserved.                     **
 **                                                     **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,      **
 **                                                     **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.  **
 **                                                     **
 ********************************************************/


#ifndef __GPIO_MOD__
#define __GPIO_MOD__

#define GPIO_SIZE                               4096
#define GPIO_BASE_OFFS                          SE_GPIO_BASE

/***** GPIO Port Register Offset *****/
#define GPIO_GPDO_OFFSET                        0x08
#define GPIO_GPDI_OFFSET                        0x09
#define GPIO_IMEN_OFFSET                        0x0A
#define GPIO_EVST_OFFSET                        0x0B
#define GPIO_DBC0_OFFSET                        0x0C
#define GPIO_DBC1_OFFSET                        0x0D
#define GPIO_GRPSEL_OFFSET                      0x0E
#define GPIO_SINF_CTRL_REG_OFFSET               0xF0
#define GPIO_CMB_INTRSTAT_GPISR0_OFFSET         0xF1
#define GPIO_CMB_INTRSTAT_GPISR1_OFFSET         0xF2
#define GPIO_SGPIO_CLK_HP_REG_OFFSET            0xF3
#define GPIO_SGPIO_CLK_LP_REG_OFFSET            0xF4
#if defined(SOC_PILOT_IV) || defined(SOC_PILOT_III)
#define GPIO_CMB_INTRSTAT_GPISR2_OFFSET         0xF5
#define GPIO_CMB_INTRSTAT_GPISR3_OFFSET         0xF6
#define GPIO_PORT_CAP_CTRL_REG_OFFSET           0xF7 //SGPIO
#endif
#ifdef SOC_PILOT_IV
#define GPIO_SG2SIC_OFFSET                      0x2A0
#define GPIO_SG2ISR0_OFFSET                     0x2A1
#define GPIO_SG2ISR1_OFFSET                     0x2A2
#define GPIO_S2CKHIGH_OFFSET                    0x2A3
#define GPIO_S2CKLOW_OFFSET                     0x2A4
#endif

/***** GPCFG Register Bits' Mask *****/
#define GPIO_EVENT_TO_SMI_ENB                   0x80
#define GPIO_EVENT_TO_IRQ_ENB                   0x40
#define GPIO_EVENT_DEBOUNCE_ENB                 0x20
#define GPIO_EVENT_POLARITY                     0x10
#define GPIO_EVENT_TYPE                         0x08
#define GPIO_PUPD_CNTRL                         0x06
#define GPIO_OUTPUT_ENB                         0x01
// Pull-up/Pull-down Control bits.
#define GPIO_ENABLE_INTERNAL_PULL_DOWN          0x00
#define GPIO_ENABLE_INTERNAL_PULL_UP            0x04
#define GPIO_NO_PULL_UP_DOWN                    0x06

#if defined(SOC_PILOT_IV)
#define MAX_GPIO                                368
#define MAX_GPIO_PORTS                          46 // 0-45, skip 14,15
#define NAME                                    "gpio-pilot-iv"
#elif defined(SOC_PILOT_III)
#define MAX_GPIO                                240
#define MAX_GPIO_PORTS                          30
#define NAME                                    "gpio-pilot-iii"
#else
#define MAX_GPIO                                96
#define MAX_GPIO_PORTS                          12
#define NAME                                    "gpio-pilot-ii"
#endif
#define MAX_GPIO_IDX           	 		(MAX_GPIO - 1)
#define GPIO_START_IDX    	                0
#define GPIO_MAJOR_NO				101

#define GPIO_HW_DEV_NAME        		"gpio_hw"
#define MAX_GPIO_HW_PORT		1

#if 0
#define ERR(format, args...)    printk(PREFIX CERR  "ERR  " format CNORM "\n", ##args)
#define WARN(format, args...)   printk(PREFIX CWARN "WARN " format CNORM "\n", ##args)
#define INFO(format, args...)   printk(PREFIX CINFO "INFO " format CNORM "\n", ##args)
#define DBG(format, args...)    printk(PREFIX CDBG  "DBG  " format CNORM "\n", ##args)
#else
#define ERR(format, args...)
#define INFO(format, args...)
#define DBG(format, args...)
#endif

/* Gpio Device Private data */
typedef struct {
    void     *pilot_gpio_base;
}  __attribute__((packed)) gpio_dev_t;


/*  function declarations */
int pilot_readgpio      ( void *gpin_data  );
int pilot_writegpio     ( void *gpin_data  );
int pilot_getdir        ( void *gpin_data  );
int pilot_setdir        ( void *gpin_data  );
int pilot_getpol        ( void *gpin_data  );
int pilot_setpol        ( void *gpin_data  );
int pilot_getpull_up_down        ( void *gpin_data  );
int pilot_setpull_up_down        ( void *gpin_data  );
int pilot_regsensorints  ( void* gpin_data, uint32_t total_interrupt_sensors, void* sensor_itr );
int pilot_unregsensorints   ( void *gpin_data  );
int pilot_regchassisints   ( void* gpin_data, uint32_t total_chassis_interrupts,  void* chassis_itr );
int pilot_unregchassisints  ( void *gpin_data  );
int pilot_EnableGpioToggle (  void* ToggleData);
int pilot_set_gpio_property( unsigned short gpionum,  unsigned char property, unsigned char value);
int pilot_get_gpio_property( unsigned short gpionum, unsigned char property, unsigned char *value);
int set_sgpio_data(unsigned short sgpio, unsigned char value);
int get_sgpio_data(unsigned short sgpio, unsigned char *value);
int enable_gpio_event(unsigned short gpionum);
int disable_gpio_event(unsigned short gpionum);
int clear_gpio_event(unsigned short gpionum);
int get_gpio_event(unsigned short gpionum, unsigned char *value);
int pilot_getdebounce (void *gpin_data);
int pilot_setdebounce (void *gpin_data);
int pilot_getdebounce_clock (void *gpin_data);
int pilot_setdebounce_clock (void *gpin_data);
int pilot_reg_reading_on_ints (unsigned short gpionum, unsigned long va_address);
int pilot_get_reading_on_ints (unsigned short gpionum, unsigned char *value);
int pilot_readgpios  (void *gpio_list_data_info, unsigned int count);
int pilot_writegpios (void *gpio_list_data_info, unsigned int count);
int pilot_getgpiosdir  (void *gpio_list_data_info, unsigned int count);
int pilot_setgpiosdir (void *gpio_list_data_info, unsigned int count);
// Prototype for the pulse function
int pilot_writegpio_pulsedata ( void *gpin_data  );

#endif //__GPIO_MOD__






