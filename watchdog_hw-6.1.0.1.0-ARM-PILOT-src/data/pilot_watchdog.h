/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2016, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * pilot_watchdog.h
 * Watchdog driver Hardware Layer header file for Pilot
 *
 *****************************************************************/
#ifndef _WDT_HW_H_
#define _WDT_HW_H_

#define PILOT_WATCHDOG_REG_BASE   0x40410000
#define PILOT_WATCHDOG_REG_LEN    SZ_4K

#ifdef SOC_PILOT_IV
#define MAX_WDT_DEVICES 4
#else
#define MAX_WDT_DEVICES 1
#endif
#define MIN_WDT_DEVICE 1
#ifdef CONFIG_SPX_FEATURE_GLOBAL_FAILSAFE_WATCHDOG
	static int current_wdt_device = CONFIG_SPX_FEATURE_GLOBAL_FAILSAFE_WATCHDOG;
#else
	static int current_wdt_device = 1;
#endif
static const int min_wdt_num = 1, max_wdt_num = MAX_WDT_DEVICES;
static int wdt_boot_complete = 0;

#ifdef SOC_PILOT_IV
#define PILOT_WATCHDOG_IRQ        91
#else
#define PILOT_WATCHDOG_IRQ        (32 + 14)
#endif


/* Watchdog Timer registers offset */
#define PILOT_WATCHDOG_WDCTL      0x00
#define PILOT_WATCHDOG_WDCNT_LO   0x10
#define PILOT_WATCHDOG_WDCNT_HI   0x14

/* Reset and Debug registers offset */
#define PILOT_RESET_CONTROL_SYSRCR  0x00
#define PILOT_RESET_STATUS_SYSRST   0x04
#define PILOT_WATCHDOG_SYSWRER      0x08
#define PILOT_SOFTWARE_SYSSRER      0x0C

#ifdef SOC_PILOT_IV
/*watchdog 1/2/3 registers*/
#define SYSWCR                  0x80
#define SYSWRERL                0x84
#define SYSWRERH                0x88
#define SYSWCFR                 0x8C
#define SYSRST_STATUS           0xB0
#define COUNT_PERSEC            381
#define PILOT_WATCHDOG_PRETRIGGER_TIMEOUT 20 /* FTSCHG_IRCONA */
/* bits of system reset control register */
#define PILOT_WATCHDOG_WDCTL_TRIGGER_NEW_WDT 0x800000
#define RESET_I2C_INTERFACES_0_TO_7          0xFF00
#define RESET_I2C_INTERFACES_8_TO_9          0x0C
#define RESET_EMMC                           0x20
#define PILOT_WATCHDOG_PRETRIGGER_ENABLE     0x400000
#define PILOT_WATCHDOG_NEW_WDCTL_FIRE        0x20000
#define PRETRIGGER_VALUE_REACHED_STATUS      0x40000
#endif

/* bits of watchdog control register */
#define PILOT_WATCHDOG_WDCTL_TRIGGER      0x80 /* bit 7 */
#define PILOT_WATCHDOG_WDCTL_WDO          0x40 /* bit 6 */
#define PILOT_WATCHDOG_WDCTL_DISABLE      0x08 /* bit 3 */
#define PILOT_WATCHDOG_WDCTL_ACTION       0x04 /* bit 2 */
#define PILOT_WATCHDOG_WDCTL_FIRE         0x02 /* bit 1 */
#define PILOT_WATCHDOG_WDCTL_RUN          0x01 /* bit 0 */

/* bits of system reset control register */ 
#define PILOT_WATCHDOG_RESET_ENABLE       0x01 /* bit 0 */
#define PILOT_SOFTWARE_RESET_ENABLE       0x02 /* bit 1 */

/*Different BMC Modules to reset */
#define RESET_ARM_PROCESSOR  0x00000001         /* bit 0 */
#define RESET_I2C_INTERFACES 0x0000FF00         /* bit 8-15 */
#define RESET_PWM_TACH       0x00400000         /* bit 22 */

/*      Register value */
#define PILOT_WATCHDOG_SYSWRER_VALUE      0x00

#define PILOT_WATCHDOG_DRIVER_NAME "pilot_watchdog"

static void *pilot_watchdog_virt_base;
static struct watchdog_core_funcs_t *watchdog_core_ops;
static int pilot_watchdog_hal_id;
#endif /* _WDT_HW_H_ */
