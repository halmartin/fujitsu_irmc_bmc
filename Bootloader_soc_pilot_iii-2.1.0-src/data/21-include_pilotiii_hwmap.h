--- uboot.old/include/pilotiii/hwmap.h	2012-04-19 10:12:58.269548867 -0400
+++ uboot/include/pilotiii/hwmap.h	2012-04-19 10:08:44.008922587 -0400
@@ -0,0 +1,180 @@
+/*
+ *  This file contains the SE PILOT2 Register Base, IRQ and DMA  mappings
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
+#ifndef     _HWREG_PILOTII_H_
+#define     _HWMAP_PILOTII_H_
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
+#define SE_TOP_LEVEL_PIN_CTRL_BASE     	(SE_REGISTER_BASE + 0x0100900)	 	/* 256  */
+#define SE_SCRATCH_RAM_BASE            	(SE_REGISTER_BASE + 0x0101000)	 	/* 24 K */ 	
+#define SE_BOOT_SPI_BASE		       	(SE_REGISTER_BASE + 0x0200000)  	/* 1 MB */ 
+#define SE_DDR2_CTRL_BASE              	(SE_REGISTER_BASE + 0x0300000) 	 	/* 2 MB */
+#define DDR_REG_BASE                    (SE_REGISTER_BASE + 0x0300000)
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
+#define SE_BACKUP_SPI_BASE	            (SE_REGISTER_BASE + 0x0280000)		/*  4 K */
+#define SE_RES_DEB_BASE                 (SE_REGISTER_BASE + 0x0100800)     /* 256  */
+
+
+
+/*--------------- Virtual address of the IO Registers Region  ------------*/
+#define SE_REGISTER_VA_BASE				IO_ADDRESS(SE_REGISTER_BASE)
+#define SE_INTC_VA_BASE					IO_ADDRESS(SE_INTC_BASE)
+#define SE_LPC_ROM_VA_BASE				IO_ADDRESS(SE_LPC_ROM_BASE)
+#define SE_SYS_CLK_VA_BASE				IO_ADDRESS(SE_SYS_CLK_BASE)
+#define SE_SCRATCH_128_VA_BASE			IO_ADDRESS(SE_SCRATCH_128_BASE)
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
+#define SE_BACKUP_SPI_VA_BASE			IO_ADDRESS(SE_BACKUP_SPI_BASE)
+#define SE_RES_DEB_VA_BASE                      IO_ADDRESS(SE_RES_DEB_BASE)
+
+
+/*****************************************************************
+					  IRQ Assignment
+*****************************************************************/
+
+#define NR_IRQS							32	
+#define SE_NR_IRQS                     	32
+#define SE_NR_FIQS                     	32
+#define MAXIRQNUM                       31
+#define MAXFIQNUM                       31
+
+#define IRQ_MAC_A		           		0
+#define IRQ_MAC_B    	                1
+#define IRQ_FGE                     	2
+#define IRQ_USB                       	3
+#define IRQ_SYS_CLK_SWITCH            	27
+#define IRQ_SECURITY					28
+#define IRQ_BMC_SSP_DOORBELL           	30 
+#define IRQ_SSP_BMC_DOORBELL            31
+#define IRQ_GPIO						16
+#define IRQ_SYS_WAKEUP					17
+#define IRQ_KCS_BT_SMIC_MAIL			18
+#define IRQ_TIMER_0                     6
+#define IRQ_TIMER_1                     7
+#define IRQ_BOMBER						29
+#define IRQ_UART_2                      19
+#define IRQ_UART_3						20
+#define IRQ_UART_4                      21
+#define IRQ_I2C_0                       23
+#define IRQ_I2C_1                       24
+#define IRQ_I2C_2                       25	
+#define IRQ_I2C_3                       26	
+#define IRQ_I2C_4                       4
+#define IRQ_I2C_5						5
+#define IRQ_ICMB						22	
+#define IRQ_SUPER_IO					11		// Combined Interrupt
+#define IRQ_BMC							13		// Combined Interrupt
+
+/*****************************************************************
+				    APB DMA channel assignment
+*****************************************************************/
+/* TODO */
+
+#define MAX_SE_ETH		2
+
+#endif
