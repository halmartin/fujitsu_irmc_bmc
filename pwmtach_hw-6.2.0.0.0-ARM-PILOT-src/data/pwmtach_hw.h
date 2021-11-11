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

/*
 * PwmTach Common Driver Header File
 *
 * Copyright (C) 2009-2015 American Megatrends Inc.
 *
 * This driver provides hardware layer header file for PWMTACH driver.
 */


#ifndef __PWMTACH_HW_H__
#define __PWMTACH_HW_H__

/* Update for Pilot III */
#if defined SOC_PILOT_III || defined SOC_PILOT_IV
	#define PWMTACHEXTOFF		0x100
	#define NOFEXTPWMCHN		4
	#define NOFEXTFTMCHN		8
	#define MAXPWMCHN_PILOT		3
	#define MAXFTMCHN_PILOT		7

	#define NUMPWM				8
	#define NUMFT				16
#else
	#define NUMPWM				4
	#define NUMFT				8
#endif

#define PWMBASE         SE_FAN_TACH_PWM_BASE
#define PWMTACH_SIZE	(4 * 1024) // 4K

#define PWCFGOFF           0x00
#define FMCFGOFF           0x04
#define PWOCFOFF           0x08
#define FTISROFF           0x0C
#define PWCRCSROFF         0x10
#define PWCRVR0OFF         0x14

#define PWPSR0OFF          0x40
#define PWDCR0OFF          0x44

#define FMTHR0OFF          0x60
#define FMSPR0OFF          0x64
#define FMCSR0OFF          0x68

#define FMPFSR0OFF         0xC0
#define FMPFSR1OFF         0xC4


/* PWM Control OTS configuration register fields definition */
#define FAN0_ENB_ON_OTS					1<<0
#define FAN1_ENB_ON_OTS					1<<1
#define FAN2_ENB_ON_OTS					1<<2
#define FAN3_ENB_ON_OTS					1<<3
#define PWM0_ENB_DIV_BY_128_64			1<<4
#define PWM1_ENB_DIV_BY_128_64			1<<5
#define PWM2_ENB_DIV_BY_128_64			1<<6
#define PWM3_ENB_DIV_BY_128_64			1<<7

/* PWM Counter Resolution Control/Status register fields definition */
#define PWM0_ENB_PROG_CNTR_RES			1<<0
#define PWM1_ENB_PROG_CNTR_RES			1<<2
#define PWM2_ENB_PROG_CNTR_RES			1<<4
#define PWM3_ENB_PROG_CNTR_RES			1<<6
#define PWM0_PROG_CNTR_RES_ERR			1<<1
#define PWM1_PROG_CNTR_RES_ERR			1<<3
#define PWM2_PROG_CNTR_RES_ERR			1<<5
#define PWM3_PROG_CNTR_RES_ERR			1<<7

/* PWM Control Configuration register fields definition */
#define PWM0_CTRL_ENABLE				1<<0
#define PWM1_CTRL_ENABLE				1<<2
#define PWM2_CTRL_ENABLE				1<<4
#define PWM3_CTRL_ENABLE				1<<6
#define PWM0_INVERT_ENABLE				1<<1
#define PWM1_INVERT_ENABLE				1<<3
#define PWM2_INVERT_ENABLE				1<<5
#define PWM3_INVERT_ENABLE				1<<7
#define PWM_CTRL_ENABLE_ALL				(PWM0_CTRL_ENABLE|PWM1_CTRL_ENABLE|PWM2_CTRL_ENABLE|PWM3_CTRL_ENABLE)

/* PWM Pre-Scale register fields definition */
#define SELECT_200KHZ_CLK				(1<<7)
#define SELECT_8MHZ_CLK					(0<<7)

#define CLK_SELECT					SELECT_200KHZ_CLK

/* Fan TACH Monitor Configuration register fields definition */
#define FT0_MONITOR_ENABLE				1<<0
#define FT1_MONITOR_ENABLE				1<<1
#define FT2_MONITOR_ENABLE				1<<2
#define FT3_MONITOR_ENABLE				1<<3
#define FT4_MONITOR_ENABLE				1<<4
#define FT5_MONITOR_ENABLE				1<<5
#define FT6_MONITOR_ENABLE				1<<6
#define FT7_MONITOR_ENABLE				1<<7
#define FT_MONITOR_ENB_ALL				(FT0_MONITOR_ENABLE|FT1_MONITOR_ENABLE|FT2_MONITOR_ENABLE|FT3_MONITOR_ENABLE|FT4_MONITOR_ENABLE|FT5_MONITOR_ENABLE|FT6_MONITOR_ENABLE|FT7_MONITOR_ENABLE)

/* Fan TACH Monitor Control/Status register fields definition */
#define CLK_SELECT_16KHZ				0x00
#define CLK_SELECT_8KHZ					0x40
#define CLK_SELECT_4KHZ					0x80
#define CLK_SELECT_2KHZ					0xC0
#define FILTER_DISABLE					1<<5
#define INTERRUPT_ENABLE				1<<4
#define FTIN_TOGGLE_ERR					1<<3
#define OVERFLOW_ERR					1<<2
#define OVERTHRESHOLD_ERR				1<<1
#define SPEED_READY					1<<0

#define CLK_FREQ_500HZ		(500)
#define CLK_FREQ_2KHZ		(2*1000)
#define CLK_FREQ_4KHZ		(4*1000)
#define CLK_FREQ_8KHZ		(8*1000)
#define CLK_FREQ_16KHZ		(16*1000)
#define CLK_FREQ_200KHZ		(200*1000)
#define CLK_FREQ_8_3_MHZ	(8333*1000)


#define PWM_CLK_FREQ	CLK_FREQ_200KHZ

#if defined SOC_PILOT_III || defined SOC_PILOT_IV
/* 00: 8 KHz 
 * 01: 4 KHz (default) 
 * 10: 2 KHz 
 * 11: 500 Hz 
 */
#define CLK_SELECT_00	CLK_FREQ_8KHZ
#define CLK_SELECT_01	CLK_FREQ_4KHZ
#define CLK_SELECT_10	CLK_FREQ_2KHZ
#define CLK_SELECT_11	CLK_FREQ_500HZ
#else
/* 00: 16 KHz  
 * 01: 8 KHz (default) 
 * 10: 4 KHz  
 * 11: 2 KHz 
 */
#define CLK_SELECT_00	CLK_FREQ_16KHZ
#define CLK_SELECT_01	CLK_FREQ_8KHZ
#define CLK_SELECT_10	CLK_FREQ_4KHZ
#define CLK_SELECT_11	CLK_FREQ_2KHZ
#endif

#endif  // __PWMTACH_HW_H__


