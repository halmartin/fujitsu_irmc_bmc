--- linux-3.14.17/arch/arm/include/asm/soc-pilot/hwmap.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/include/asm/soc-pilot/hwmap.h	2014-09-04 12:01:17.029817702 +0530
@@ -0,0 +1,285 @@
+/*
+ *  This file contains the SE PILOT3 Register Base, IRQ and DMA  mappings
+ *
+ *  Copyright (C) 2009 ServerEngines
+ *  Based on AMI's similar file
+ *
+ *  Copyright (C) 2005 American Megatrends Inc
+ *
+ * This program is free software; you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation; either version 2 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program; if not, write to the Free Software
+ * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
+ */
+
+#ifndef     _HWREG_PILOT3_H_
+#define     _HWMAP_PILOT3_H_
+
+
+/********************************************************************
+   Internal Register Mapping (0x40000000 to 0x40AFFFFF) 
+*********************************************************************/
+#define SE_REGISTER_BASE				0x40000000
+
+#define SE_INTC_BASE        		  	(SE_REGISTER_BASE + 0x0000000)    	/* 1 MB */
+#define SE_LPC_ROM_BASE                	(SE_REGISTER_BASE + 0x0100000)	 	/* 256  */
+#define SE_SYS_CLK_BASE                	(SE_REGISTER_BASE + 0x0100100)	 	/* 256  */
+#define SE_SCRATCH_128_BASE            	(SE_REGISTER_BASE + 0x0100200)    	/* 256  */ 
+#define SE_TOP_LVL_PIN_BASE             (SE_REGISTER_BASE + 0x0100900)    	/* 256  */ 
+#define SE_SCRATCH_RAM_BASE            	(SE_REGISTER_BASE + 0x0101000)	 	/* 24 K */ 	
+#define SE_BOOT_SPI_BASE		       	(SE_REGISTER_BASE + 0x0200000)  	/* 1 MB */ 
+#define SE_DDR2_CTRL_BASE              	(SE_REGISTER_BASE + 0x0300000) 	 	/* 2 MB */
+#define SE_MAC_A_BASE	           	   	(SE_REGISTER_BASE + 0x0500000)    	/* 1 MB */
+#define SE_MAC_B_BASE                  	(SE_REGISTER_BASE + 0x0600000)	 	/* 1 MB */
+#define SE_USB_0_BASE				   	(SE_REGISTER_BASE + 0x0700000) 	 	/* 1 MB */
+#define SE_USB_1_BASE                  	(SE_REGISTER_BASE + 0x0800000)	 	/* 1 MB */
+#define SE_FGE_BASE					   	(SE_REGISTER_BASE + 0x0900000)	 	/* 1 MB */	
+#define SE_SECURITY_BASE			   	(SE_REGISTER_BASE + 0x0A00000)  	/* 1 MB */
+#define SE_AHB_DMA_BASE			       	(SE_REGISTER_BASE + 0x0400000)	 	/* 64 K */	
+//#define SE_AHB_SYS_APB_BRIDGE        	(SE_REGISTER_BASE + 0x0410000)
+//#define SE_AHB_SIO_BRIDGE 	 	   	(SE_REGISTER_BASE + 0x0420000)
+//#define SE_AHB_PER_APB_BRIDGE 	   	(SE_REGISTER_BASE + 0x0430000)	
+#define SE_GPIO_BASE				   	(SE_REGISTER_BASE + 0x0420000)	 	/*  4 K */	
+#define SE_SIO_UART_0_BASE			   	(SE_REGISTER_BASE + 0x0421000)	 	/*  4 K */
+#define SE_SIO_UART_1_BASE			   	(SE_REGISTER_BASE + 0x0422000)    	/*  4 K */
+#define SE_SYS_WAKEUP_BASE			   	(SE_REGISTER_BASE + 0x0425000)    	/*  4 K */
+#define SE_KCS1_KCS2_BASE			   	(SE_REGISTER_BASE + 0x0426000)    	/*  512 */
+#define SE_KCS3_BASE				   	(SE_REGISTER_BASE + 0x0426200)    	/*  256 */
+#define SE_KCS4_BASE			       	(SE_REGISTER_BASE + 0x0426300)	 	/*  256 */
+#define SE_KCS5_BT_SMIC_MAIL_BASE	   	(SE_REGISTER_BASE + 0x0426400)		/*  256 */
+#define SE_PILOT_SPEC_BASE             	(SE_REGISTER_BASE + 0x0426500)		/* 3K - 256 */	
+#define SE_WDT_MSFT_BASE  	           	(SE_REGISTER_BASE + 0x0427000)		/*  4 K */
+#define SE_RTC_BASE		        	   	(SE_REGISTER_BASE + 0x0428000)		/*  4 K */  
+#define SE_HOST_SPI_BASE		       	(SE_REGISTER_BASE + 0x0429000)	    /*  256 */
+#define SE_TAP_BASE                    	(SE_REGISTER_BASE + 0x0429100)		/*  256 */
+#define SE_WDT_BMC_BASE                	(SE_REGISTER_BASE + 0x0410000)		/*  4 K */		
+#define SE_TIMER_BASE				   	(SE_REGISTER_BASE + 0x0411000)		/*  4 K */
+#define SE_PS2_BASE					   	(SE_REGISTER_BASE + 0x0412000)		/*  4 K */
+#define SE_FAN_TACH_PWM_BASE			(SE_REGISTER_BASE + 0x0413000)		/*  4 K */
+#define SE_ADC_THERMAL_BASE				(SE_REGISTER_BASE + 0x0414000)	 	/*  4 K */
+#define SE_VIRTUAL_UART_0_BASE			(SE_REGISTER_BASE + 0x0415000)		/*  4 K */
+#define SE_VIRTUAL_UART_1_BASE			(SE_REGISTER_BASE + 0x0416000)		/*  4 K */
+#define SE_BOMBER_BASE                  (SE_REGISTER_BASE + 0x0417000) 	 	/*  4 K */
+#define SE_PECI_BASE                 	(SE_REGISTER_BASE + 0x0418000)		/* 32 K */
+#define SE_UART_2_BASE	                (SE_REGISTER_BASE + 0x0430000)		/*  4 K */
+#define SE_UART_3_BASE	                (SE_REGISTER_BASE + 0x0431000)		/*  4 K */
+#define SE_UART_4_BASE	                (SE_REGISTER_BASE + 0x0432000)		/*  4 K */
+#define SE_I2C_0_BASE	                (SE_REGISTER_BASE + 0x0433000)		/*  4 K */
+#define SE_I2C_1_BASE	                (SE_REGISTER_BASE + 0x0434000)		/*  4 K */
+#define SE_I2C_2_BASE	                (SE_REGISTER_BASE + 0x0435000)		/*  4 K */
+#define SE_I2C_3_BASE	                (SE_REGISTER_BASE + 0x0436000)		/*  4 K */
+#define SE_I2C_4_BASE	                (SE_REGISTER_BASE + 0x0437000)		/*  4 K */
+#define SE_I2C_5_BASE	                (SE_REGISTER_BASE + 0x0438000)		/*  4 K */
+#define SE_I2C_6_BASE	                (SE_REGISTER_BASE + 0x0439000)		/*  4 K */
+#define SE_I2C_7_BASE	                (SE_REGISTER_BASE + 0x043A000)		/*  4 K */
+#define SE_BACKUP_SPI_BASE	            (SE_REGISTER_BASE + 0x0280000)		/*  4 K */
+#define SE_RES_DEB_BASE                 (SE_REGISTER_BASE + 0x0100800)     /* 256  */
+
+/*--------------- Virtual address of the IO Registers Region  ------------*/
+#define SE_REGISTER_VA_BASE				IO_ADDRESS(SE_REGISTER_BASE)
+#define SE_INTC_VA_BASE					IO_ADDRESS(SE_INTC_BASE)
+#define SE_LPC_ROM_VA_BASE				IO_ADDRESS(SE_LPC_ROM_BASE)
+#define SE_SYS_CLK_VA_BASE				IO_ADDRESS(SE_SYS_CLK_BASE)
+#define SE_SCRATCH_128_VA_BASE			IO_ADDRESS(SE_SCRATCH_128_BASE)
+#define SE_TOP_LVL_PIN_VA_BASE          IO_ADDRESS(SE_TOP_LVL_PIN_BASE) 
+#define SE_SCRATCH_RAM_VA_BASE			IO_ADDRESS(SE_SCRATCH_RAM_BASE)
+#define SE_BOOT_SPI_VA_BASE				IO_ADDRESS(SE_BOOT_SPI_BASE)
+#define SE_DDR2_CTRL_VA_BASE			IO_ADDRESS(SE_DDR2_CTRL_BASE)
+#define SE_MAC_A_VA_BASE				IO_ADDRESS(SE_MAC_A_BASE)
+#define SE_MAC_B_VA_BASE				IO_ADDRESS(SE_MAC_B_BASE)
+#define SE_USB_0_VA_BASE				IO_ADDRESS(SE_USB_0_BASE)
+#define SE_USB_1_VA_BASE				IO_ADDRESS(SE_USB_1_BASE)
+#define SE_FGE_VA_BASE					IO_ADDRESS(SE_FGE_BASE)
+#define SE_SECURITY_VA_BASE				IO_ADDRESS(SE_SECURITY_BASE)
+#define SE_AHB_DMA_VA_BASE				IO_ADDRESS(SE_AHB_DMA_BASE)
+#define SE_GPIO_VA_BASE					IO_ADDRESS(SE_GPIO_BASE)
+#define SE_SIO_UART_0_VA_BASE			IO_ADDRESS(SE_SIO_UART_0_BASE)
+#define SE_SIO_UART_1_VA_BASE			IO_ADDRESS(SE_SIO_UART_1_BASE)
+#define SE_SYS_WAKEUP_VA_BASE			IO_ADDRESS(SE_SYS_WAKEUP_BASE)
+#define SE_KCS1_KCS2_VA_BASE			IO_ADDRESS(SE_KCS1_KCS2_BASE)
+#define SE_KCS3_VA_BASE					IO_ADDRESS(SE_KCS3_BASE)
+#define SE_KCS4_VA_BASE					IO_ADDRESS(SE_KCS4_BASE)
+#define SE_KCS5_BT_SMIC_MAIL_VA_BASE	IO_ADDRESS(SE_KCS5_BT_SMIC_MAIL_BASE)
+#define SE_PILOT_SPEC_VA_BASE			IO_ADDRESS(SE_PILOT_SPEC_BASE)
+#define SE_WDT_MSFT_VA_BASE				IO_ADDRESS(SE_WDT_MSFT_BASE)
+#define SE_RTC_VA_BASE					IO_ADDRESS(SE_RTC_BASE)
+#define SE_HOST_SPI_VA_BASE				IO_ADDRESS(SE_HOST_SPI_BASE)
+#define SE_TAP_VA_BASE					IO_ADDRESS(SE_TAP_BASE)
+#define SE_WDT_BMC_VA_BASE				IO_ADDRESS(SE_WDT_BMC_BASE)
+#define SE_TIMER_VA_BASE				IO_ADDRESS(SE_TIMER_BASE)
+#define SE_PS2_VA_BASE					IO_ADDRESS(SE_PS2_BASE)
+#define SE_FAN_TACH_PWM_VA_BASE			IO_ADDRESS(SE_FAN_TACH_PWM_BASE)
+#define SE_ADC_THERMAL_VA_BASE			IO_ADDRESS(SE_ADC_THERMAL_BASE)
+#define SE_VIRTUAL_UART_0_VA_BASE		IO_ADDRESS(SE_VIRTUAL_UART_0_BASE)
+#define SE_VIRTUAL_UART_1_VA_BASE		IO_ADDRESS(SE_VIRTUAL_UART_1_BASE)
+#define SE_BOMBER_VA_BASE				IO_ADDRESS(SE_BOMBER_BASE)
+#define SE_PECI_VA_BASE					IO_ADDRESS(SE_PECI_BASE)
+#define SE_UART_2_VA_BASE				IO_ADDRESS(SE_UART_2_BASE)
+#define SE_UART_3_VA_BASE				IO_ADDRESS(SE_UART_3_BASE)
+#define SE_UART_4_VA_BASE				IO_ADDRESS(SE_UART_4_BASE)
+#define SE_I2C_0_VA_BASE				IO_ADDRESS(SE_I2C_0_BASE)
+#define SE_I2C_1_VA_BASE				IO_ADDRESS(SE_I2C_1_BASE)
+#define SE_I2C_2_VA_BASE				IO_ADDRESS(SE_I2C_2_BASE)
+#define SE_I2C_3_VA_BASE				IO_ADDRESS(SE_I2C_3_BASE)
+#define SE_I2C_4_VA_BASE				IO_ADDRESS(SE_I2C_4_BASE)
+#define SE_I2C_5_VA_BASE				IO_ADDRESS(SE_I2C_5_BASE)
+#define SE_I2C_6_VA_BASE				IO_ADDRESS(SE_I2C_6_BASE)
+#define SE_I2C_7_VA_BASE				IO_ADDRESS(SE_I2C_7_BASE)
+#define SE_BACKUP_SPI_VA_BASE			IO_ADDRESS(SE_BACKUP_SPI_BASE)
+#define SE_RES_DEB_VA_BASE             IO_ADDRESS(SE_RES_DEB_BASE)
+
+/*****************************************************************
+					  IRQ Assignment
+*****************************************************************/
+
+#define NR_IRQS							96  
+#define SE_NR_IRQS                     	96  
+#define MAXIRQNUM                       95  
+#define COMBINED_IRQ_START				32
+#define COMBINED2_IRQ_START				64  
+
+#define IRQ_MAC_A		           		0
+#define IRQ_MAC_B    	                1
+#define IRQ_FGE                     	2
+#define IRQ_USB_1_0                     3
+#define IRQ_I2C_4                       4
+#define IRQ_I2C_5						5
+#define IRQ_TIMER_0                     6
+#define IRQ_TIMER_1                     7
+
+#define IRQ_COMBINED_8					8			//Don't Use
+#define IRQ_COMBINED_9					9			//Don't Use
+#define IRQ_USB_2_0						10			
+#define IRQ_COMBINED_11					11			//Don't Use
+#define IRQ_COMBINED_12					12			//Don't Use
+#define IRQ_COMBINED_13					13			//Don't Use
+#define IRQ_COMBINED_14					14			//Don't Use
+#define IRQ_COMBINED_15					15			//Don't Use
+
+#define IRQ_GPIO						16
+#define IRQ_SYS_WAKEUP					17
+#define IRQ_KCS_BT_SMIC_MAIL			18
+#define IRQ_UART_2                      19
+#define IRQ_UART_3						20
+#define IRQ_UART_4                      21
+#define IRQ_ICMB						22	
+#define IRQ_I2C_0                       23
+#define IRQ_I2C_1                       24
+#define IRQ_I2C_2                       25	
+#define IRQ_I2C_3                       26	
+#define IRQ_SYS_CLK_SWITCH            	27
+#define IRQ_SECURITY					28
+#define IRQ_BOMBER						29
+#define IRQ_BMC_SSP_DOORBELL           	30 
+#define IRQ_SSP_BMC_DOORBELL            31
+
+#define IRQ_COMBINED_BIT0				(32+0)		// Don't Use
+#define IRQ_COMBINED_BIT1				(32+1)		// Don't Use
+#define IRQ_COMBINED_BIT2				(32+2)		// Don't Use
+#define IRQ_COMBINED_BIT3_RESERVED		(32+3)		// Don't Use
+#define IRQ_COMBINED_BIT4				(32+4)		// Don't Use
+#define IRQ_SIO_WDT						(32+5)
+#define IRQ_SIO_PSR						(32+6)
+#define IRQ_SIO_SWC						(32+7)
+#define IRQ_V_UART_0					(32+8)
+#define IRQ_V_UART_1					(32+9)
+#define IRQ_COMBINED_BIT10				(32+10)		// Don't Use
+#define IRQ_COMBINED_BIT11				(32+11)		// Don't Use
+#define IRQ_TIMER_2						(32+12)		
+#define IRQ_TIMER_3						(32+13)		
+#define IRQ_BMC_WDT						(32+14)
+#define IRQ_RTC							(32+15)
+#define IRQ_PS2							(32+16)
+#define IRQ_PWM							(32+17)
+#define IRQ_PECI						(32+18)
+#define IRQ_DDR2						(32+19)
+#define IRQ_SPI							(32+20)
+#define IRQ_COMBINED_BIT21				(32+21)		// Don't Use
+#define IRQ_COMBINED_BIT22				(32+22)		// Don't Use
+#define IRQ_COMBINED_BIT23				(32+23)		// Don't Use
+#define IRQ_COMBINED_BIT24				(32+24)		// Don't Use
+#define IRQ_COMBINED_BIT25				(32+25)		// Don't Use
+#define IRQ_COMBINED_BIT26				(32+26)		// Don't Use
+#define IRQ_COMBINED_BIT27				(32+27)		// Don't Use
+#define IRQ_COMBINED_BIT28				(32+28)		// Don't Use
+#define IRQ_COMBINED_BIT29				(32+29)		// Don't Use
+#define IRQ_COMBINED_BIT30				(32+30)		// Don't Use
+#define IRQ_CLOCK_COMTROL	   			(32+31)		
+
+//All these below interrupts represent the combined interrupts in CIRQCTLHI register of Pilot3 spec
+#define IRQ_CRYPTO_ENGINE				(64+0)
+#define IRQ_BOMBER_INT					(64+1)
+#define IRQ_SSP_TO_ARM					(64+2)
+#define IRQ_ARM_TO_SSP					(64+3)
+#define IRQ_USB_COMBIRQ					(64+4)  	
+#define IRQ_SDHC						(64+5)
+#define IRQ_NAND						(64+6)
+#define IRQ_AHB2PCI						(64+7)
+#define IRQ_TIMER_4						(64+8)
+#define IRQ_TIMER_5						(64+8)
+#define IRQ_TIMER_6						(64+10)
+#define IRQ_I2C_6						(64+11)
+#define IRQ_I2C_7						(64+12)
+#define IRQ_PCIE_TEST					(64+13)
+#define IRQ_HW_DBG						(64+14)
+#define IRQ_RESERVED					(64+15) 	//Don't use
+
+/*The below interrupts are SSP interrupts so "Don't use" */
+#define IRQ_RESERVED_16					(64+16)
+#define IRQ_RESERVED_17					(64+17)
+#define IRQ_RESERVED_18             	(64+18)
+#define IRQ_RESERVED_19                 (64+19)
+#define IRQ_RESERVED_20					(64+20)
+#define IRQ_RESERVED_21                 (64+21)
+#define IRQ_RESERVED_22                 (64+22)
+#define IRQ_RESERVED_23                 (64+23)
+#define IRQ_RESERVED_24                 (64+24)
+#define IRQ_RESERVED_25                 (64+25)
+#define IRQ_RESERVED_26                 (64+26)
+#define IRQ_RESERVED_27                 (64+27)
+#define IRQ_RESERVED_28                 (64+28)
+#define IRQ_RESERVED_29                 (64+29)
+#define IRQ_RESERVED_30					(64+30)
+#define IRQ_RESERVED_31					(64+31)
+
+
+/* List of invalid Irq - Using this will fail in Request IRQ */
+#define INVALID_IRQ_MAP                                 \
+{                                                                               \
+8,9,11,12,13,14,15,32,33,34,35,36,42,43,\
+44,45,53,54,55,56,57,58,59,60,61,62,63,  \
+79,80,81,82, \
+83,84,85,86,87,88,89,90,91,92,93,94,95  \
+}
+
+/* Mapping of Actual IRQ (8 to 15) to the Bits in Combined IRQ Control/Status Register */
+#define COMBINED_IRQ_MAP  							\
+{ 													\
+  0x00000003,0x00000004,0x00000000,0x000007F0,		\
+  0x00003800,0x001FC000,0x01E00000,0xFE000000,		\
+}							
+  	
+/* Mapping of Actual IRQ (8 to 15) to the Bits in Combined IRQ Control Register High & Combined IRQ Status Register High  */
+/*CIRQCTLHI & CIRQSTSHI*/
+#define COMBINED2_IRQ_MAP				\
+{							\
+  0x00000000,0x00000000,0x00000010,0x00000000,		\
+  0x00000700,0x000000EF,0x00006000,0x00001800,		\
+}
+/*****************************************************************
+				    APB DMA channel assignment
+*****************************************************************/
+/* TODO */
+
+#define MAX_SE_ETH		2
+
+#endif
