--- hwreg.h	2013-03-15 13:15:45.361568025 +0530
+++ uboot/include/pilotiii/hwreg.h	2013-03-15 12:54:20.200674447 +0530
@@ -0,0 +1,274 @@
+/*
+ *  This file contains the SE PILOT-II Register locations
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
+#ifndef     _HWREG_PILOT_III_H_
+#define     _HWREG_PILOT_III_H_
+
+/* -----------------------------------------------------------------
+ *                   System and Clock Control Registers
+ * -----------------------------------------------------------------
+ */
+#define PCISID  (SE_SYS_CLK_VA_BASE + 0x4C)
+
+/* -----------------------------------------------------------------
+ *                   Pilot Specific Registers
+ * -----------------------------------------------------------------
+ */
+#define PCCT_REG  (SE_PILOT_SPEC_VA_BASE + 0x45) /*Port Capture Contorl Reg*/
+/* -----------------------------------------------------------------
+ *                   SRAM Registers
+ * -----------------------------------------------------------------
+ */
+
+ /* -------------------------------------------------------------
+ *                      DDR Controller Registers
+ * -------------------------------------------------------------
+ */
+
+#define SDRAM_MEMINIT           (PILOTII_SDRAMC_BASE + 0x00)
+#define SDRAM_DQSDEL            (PILOTII_SDRAMC_BASE + 0x04)
+#define SDRAM_DSSYSES           (PILOTII_SDRAMC_BASE + 0x08)
+#define SDRAM_DERRRRDADD        (PILOTII_SDRAMC_BASE + 0x0C)
+#define SDRAM_MEMTIMING         (PILOTII_SDRAMC_BASE + 0x10)
+#define SDRAM_REFRESH_POWER_DOWN        (PILOTII_SDRAMC_BASE + 0x14)
+#define SDRAM_COMP_MEMCONFIG        (PILOTII_SDRAMC_BASE + 0x18)
+#define SDRAM_DERRINJADD        (PILOTII_SDRAMC_BASE + 0x1C)
+#define SDRAM_MEM_AHB_QBITS     (PILOTII_SDRAMC_BASE + 0x20)
+#define SDRAM_DSYND         (PILOTII_SDRAMC_BASE + 0x24)
+#define SDRAM_ERRV_TRHI         (PILOTII_SDRAMC_BASE + 0x28)
+#define SDRAM_ERRV_TRLO         (PILOTII_SDRAMC_BASE + 0x2C)
+#define SDRAM_STRAHB_DMA_ARB        (PILOTII_SDRAMC_BASE + 0x30)
+#define SDRAM_STRAFB_DMAQ       (PILOTII_SDRAMC_BASE + 0x34)
+#define SDRAM_DMA_GM            (PILOTII_SDRAMC_BASE + 0x38)
+#define SDRAM_DUA_GM            (PILOTII_SDRAMC_BASE + 0x3C)
+#define SDRAM_TOP_CTRL_REG      (PILOTII_SDRAMC_BASE + 0x60)
+
+
+/* -------------------------------------------------------------------
+ *                      Timer Registers
+ * -------------------------------------------------------------------
+ */
+#define TIMER1_LOAD_REG                (SE_TIMER_BASE + 0x0)
+#define TIMER1_COUNT_REG               (SE_TIMER_BASE + 0x4)
+#define TIMER1_CONTROL_REG             (SE_TIMER_BASE + 0x8)
+
+
+#define TIMER2_LOAD_REG                (SE_TIMER_BASE + 0x14)
+#define TIMER2_COUNT_REG               (SE_TIMER_BASE + 0x18)
+#define TIMER2_CONTROL_REG             (SE_TIMER_BASE + 0x1C)
+
+#define TIMER3_LOAD_REG                (SE_TIMER_BASE + 0x28)
+#define TIMER3_COUNT_REG               (SE_TIMER_BASE + 0x2C)
+#define TIMER3_CONTROL_REG             (SE_TIMER_BASE + 0x30)
+
+/* --------------------------------------------------------------------
+ *                          GPIO Registers
+ * --------------------------------------------------------------------
+ */
+#define GPIO_PORT0_OFFSET           0x00
+#define GPIO_PORT1_OFFSET           0x10
+#define GPIO_PORT2_OFFSET           0x20
+#define GPIO_PORT3_OFFSET           0x30
+#define GPIO_PORT4_OFFSET           0x40
+#define GPIO_PORT5_OFFSET           0x50
+#define GPIO_PORT6_OFFSET           0x60
+#define GPIO_PORT7_OFFSET           0x70
+#define GPIO_PORT8_OFFSET           0x80
+#define GPIO_PORT9_OFFSET           0x90
+#define GPIO_PORT10_OFFSET          0xA0
+#define GPIO_PORT11_OFFSET          0xB0
+
+// GPIO Port 0 
+#define GP0_GPCFG0           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x00) // Port 0, Pin 0 Config
+#define GP0_GPCFG1           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x01) // Port 0, Pin 1 Config
+#define GP0_GPCFG2           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x02) // Port 0, Pin 2 Config
+#define GP0_GPCFG3           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x03) // Port 0, Pin 3 Config
+#define GP0_GPCFG4           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x04) // Port 0, Pin 4 Config
+#define GP0_GPCFG5           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x05) // Port 0, Pin 5 Config
+#define GP0_GPCFG6           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x06) // Port 0, Pin 6 Config
+#define GP0_GPCFG7           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x07) // Port 0, Pin 7 Config
+#define GP0_GPD0             (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x08) // Port 0, Data Out
+#define GP0_GPDI             (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x09) // Port 0, Data In
+#define GP0_GPIMEN           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x0A) // Port 0, Interrupt Mask Enable 
+#define GP0_GPEVST           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x0B) // Port 0, Event Status
+#define GP0_GPDBC0           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x0C) // Port 0, Debounce Clock Control 0 (bits 3:0)
+#define GP0_GPDBC1           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x0D) // Port 0, Debounce Clock Control 1 (bits 7:4)
+#define GP0_GRPSEL           (SE_GPIO_VA_BASE + GPIO_PORT0_OFFSET + 0x0E) // Port 0, Host Group Select
+
+// GPIO Port 1
+#define GP1_GPCFG0           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x00) // Port 1, Pin 0 Config
+#define GP1_GPCFG1           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x01) // Port 1, Pin 1 Config
+#define GP1_GPCFG2           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x02) // Port 1, Pin 2 Config
+#define GP1_GPCFG3           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x03) // Port 1, Pin 3 Config
+#define GP1_GPCFG4           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x04) // Port 1, Pin 4 Config
+#define GP1_GPCFG5           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x05) // Port 1, Pin 5 Config
+#define GP1_GPCFG6           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x06) // Port 1, Pin 6 Config
+#define GP1_GPCFG7           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x07) // Port 1, Pin 7 Config
+#define GP1_GPD0             (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x08) // Port 1, Data Out
+#define GP1_GPDI             (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x09) // Port 1, Data In
+#define GP1_GPIMEN           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x0A) // Port 1, Interrupt Mask Enable 
+#define GP1_GPEVST           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x0B) // Port 1, Event Status
+#define GP1_GPDBC0           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x0C) // Port 1, Debounce Clock Control 0 (bits 3:0)
+#define GP1_GPDBC1           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x0D) // Port 1, Debounce Clock Control 1 (bits 7:4)
+#define GP1_GRPSEL           (SE_GPIO_VA_BASE + GPIO_PORT1_OFFSET + 0x0E) // Port 1, Host Group Select
+
+// GPIO Port 2
+#define GP2_GPCFG0           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x00) // Port 2, Pin 0 Config
+#define GP2_GPCFG1           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x01) // Port 2, Pin 1 Config
+#define GP2_GPCFG2           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x02) // Port 2, Pin 2 Config
+#define GP2_GPCFG3           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x03) // Port 2, Pin 3 Config
+#define GP2_GPCFG4           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x04) // Port 2, Pin 4 Config
+#define GP2_GPCFG5           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x05) // Port 2, Pin 5 Config
+#define GP2_GPCFG6           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x06) // Port 2, Pin 6 Config
+#define GP2_GPCFG7           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x07) // Port 2, Pin 7 Config
+#define GP2_GPD0             (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x08) // Port 2, Data Out
+#define GP2_GPDI             (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x09) // Port 2, Data In
+#define GP2_GPIMEN           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x0A) // Port 2, Interrupt Mask Enable 
+#define GP2_GPEVST           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x0B) // Port 2, Event Status
+#define GP2_GPDBC0           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x0C) // Port 2, Debounce Clock Control 0 (bits 3:0)
+#define GP2_GPDBC1           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x0D) // Port 2, Debounce Clock Control 1 (bits 7:4)
+#define GP2_GRPSEL           (SE_GPIO_VA_BASE + GPIO_PORT2_OFFSET + 0x0E) // Port 2, Host Group Select
+
+// GPIO Port 3
+#define GP3_GPCFG0           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x00) // Port 3, Pin 0 Config
+#define GP3_GPCFG1           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x01) // Port 3, Pin 1 Config
+#define GP3_GPCFG2           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x02) // Port 3, Pin 2 Config
+#define GP3_GPCFG3           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x03) // Port 3, Pin 3 Config
+#define GP3_GPCFG4           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x04) // Port 3, Pin 4 Config
+#define GP3_GPCFG5           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x05) // Port 3, Pin 5 Config
+#define GP3_GPCFG6           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x06) // Port 3, Pin 6 Config
+#define GP3_GPCFG7           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x07) // Port 3, Pin 7 Config
+#define GP3_GPD0             (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x08) // Port 3, Data Out
+#define GP3_GPDI             (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x09) // Port 3, Data In
+#define GP3_GPIMEN           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x0A) // Port 3, Interrupt Mask Enable 
+#define GP3_GPEVST           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x0B) // Port 3, Event Status
+#define GP3_GPDBC0           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x0C) // Port 3, Debounce Clock Control 0 (bits 3:0)
+#define GP3_GPDBC1           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x0D) // Port 3, Debounce Clock Control 1 (bits 7:4)
+#define GP3_GRPSEL           (SE_GPIO_VA_BASE + GPIO_PORT3_OFFSET + 0x0E) // Port 3, Host Group Select
+
+#define GPSIC                (SE_GPIO_VA_BASE + 0xF0) // GPIO Serial Interface Control Register
+#define GPISR0               (SE_GPIO_VA_BASE + 0xF1) // GPIO Combined Interrupt Status Register 0
+#define GPISR1               (SE_GPIO_VA_BASE + 0xF2) // GPIO Combined Interrupt Status Register 1
+#define SCKHIGH              (SE_GPIO_VA_BASE + 0xF3) // SGPIO Serial Clock High Period
+#define SCKLOW               (SE_GPIO_VA_BASE + 0xF4) // SGPIO Serial Clock Low Period
+
+/* -----------------------------------------------------------------
+ *               PWM and Fan Tach Controller Registers
+ * -----------------------------------------------------------------
+*/
+#define PWMCFG              (SE_FAN_TACH_PWM_VA_BASE + 0x00)
+#define FMCFG               (SE_FAN_TACH_PWM_VA_BASE + 0x04)
+#define PWOCF               (SE_FAN_TACH_PWM_VA_BASE + 0x08)
+#define FTISR               (SE_FAN_TACH_PWM_VA_BASE + 0x0C)
+#define PWMCRCSR            (SE_FAN_TACH_PWM_VA_BASE + 0x10)
+#define PWCRVR0             (SE_FAN_TACH_PWM_VA_BASE + 0x14)
+#define PWCRVR1             (SE_FAN_TACH_PWM_VA_BASE + 0x18)
+#define PWCRVR2             (SE_FAN_TACH_PWM_VA_BASE + 0x1C)
+#define PWCRVR3             (SE_FAN_TACH_PWM_VA_BASE + 0x20)
+// 0x24 -> 0x3C Reserved
+#define PWPSR0              (SE_FAN_TACH_PWM_VA_BASE + 0x40)
+#define PWDCR0              (SE_FAN_TACH_PWM_VA_BASE + 0x44)
+#define PWPSR1              (SE_FAN_TACH_PWM_VA_BASE + 0x48)
+#define PWDCR1              (SE_FAN_TACH_PWM_VA_BASE + 0x4C)
+#define PWPSR2              (SE_FAN_TACH_PWM_VA_BASE + 0x50)
+#define PWDCR2              (SE_FAN_TACH_PWM_VA_BASE + 0x54)
+#define PWPSR3              (SE_FAN_TACH_PWM_VA_BASE + 0x58)
+#define PWDCR3              (SE_FAN_TACH_PWM_VA_BASE + 0x5C)
+
+#define FMTHR0              (SE_FAN_TACH_PWM_VA_BASE + 0x60)
+#define FMSPR0              (SE_FAN_TACH_PWM_VA_BASE + 0x64)
+#define FMCSR0              (SE_FAN_TACH_PWM_VA_BASE + 0x68)
+// Fill-in the remainder of the Fan Tach Registers if required.
+
+
+/* -----------------------------------------------------------------
+ *               Interrupt Controller Register
+ * -----------------------------------------------------------------
+*/
+#define Pilot2_Irq_inten_Reg        (SE_INTC_VA_BASE + 0x00)
+#define Pilot2_Irq_intmask_Reg      (SE_INTC_VA_BASE + 0x08)
+#define Pilot2_Irq_intforce_Reg     (SE_INTC_VA_BASE + 0x10)
+#define Pilot2_Irq_rawstatus_Reg    (SE_INTC_VA_BASE + 0X18)
+#define Pilot2_Irq_status_Reg       (SE_INTC_VA_BASE + 0x20)
+#define Pilot2_Irq_maskstatus_Reg   (SE_INTC_VA_BASE + 0x28)
+#define Pilot2_Irq_finalstatus_Reg  (SE_INTC_VA_BASE + 0x30)
+#define Pilot2_Irq_vector_Reg       (SE_INTC_VA_BASE + 0x38)
+#define Pilot2_Irq_vector_0_Reg     (SE_INTC_VA_BASE + 0x40)
+#define Pilot2_Irq_vector_1_Reg     (SE_INTC_VA_BASE + 0x48)
+#define Pilot2_Irq_vector_2_Reg     (SE_INTC_VA_BASE + 0x50)
+#define Pilot2_Irq_vector_3_Reg     (SE_INTC_VA_BASE + 0x58)
+#define Pilot2_Irq_vector_4_Reg     (SE_INTC_VA_BASE + 0x60)
+#define Pilot2_Irq_vector_5_Reg     (SE_INTC_VA_BASE + 0x68)
+#define Pilot2_Irq_vector_6_Reg     (SE_INTC_VA_BASE + 0x70)
+#define Pilot2_Irq_vector_7_Reg     (SE_INTC_VA_BASE + 0x78)
+#define Pilot2_Irq_vector_8_Reg     (SE_INTC_VA_BASE + 0x80)
+#define Pilot2_Irq_vector_9_Reg     (SE_INTC_VA_BASE + 0x88)
+#define Pilot2_Irq_vector_10_Reg    (SE_INTC_VA_BASE + 0x90)
+#define Pilot2_Irq_vector_11_Reg    (SE_INTC_VA_BASE + 0x98)
+#define Pilot2_Irq_vector_12_Reg    (SE_INTC_VA_BASE + 0xA0)
+#define Pilot2_Irq_vector_13_Reg    (SE_INTC_VA_BASE + 0xA8)
+#define Pilot2_Irq_vector_14_Reg    (SE_INTC_VA_BASE + 0xB0)
+#define Pilot2_Irq_vector_15_Reg    (SE_INTC_VA_BASE + 0xB8)
+#define Pilot2_Fiq_inten_Reg        (SE_INTC_VA_BASE + 0xC0)
+#define Pilot2_Fiq_intmask_Reg      (SE_INTC_VA_BASE + 0xC4)
+#define Pilot2_Fiq_intforce_Reg     (SE_INTC_VA_BASE + 0xC8)
+#define Pilot2_Fiq_rawstatus_Reg    (SE_INTC_VA_BASE + 0xCC)
+#define Pilot2_Fiq_status_Reg       (SE_INTC_VA_BASE + 0xD0)
+#define Pilot2_Fiq_finalstatus_Reg  (SE_INTC_VA_BASE + 0xD4)
+#define Pilot2_Irq_plevel_Reg       (SE_INTC_VA_BASE + 0xD8)
+#define Pilot2_Irq_plevel0_Reg      (SE_INTC_VA_BASE + 0xE8)
+
+/*-------------------------------------------------------------
+ *            SSP Controllers Registers
+ *  -----------------------------------------------------------
+ */
+
+
+/*---------------------------------------------------------------
+ *   I2C Controllers Register
+ *  ------------------------------------------------------------
+ */
+
+/*------------------------------------------------------------------
+ *   DMA Controllers Registers
+ *  ----------------------------------------------------------------
+ */
+
+/*------------------------------------------------------------------
+ *               RTC Register Locations
+ *------------------------------------------------------------------*/
+
+
+/*------------------------------------------------------------------
+ *               WDT Register Locations
+ *------------------------------------------------------------------*/
+#define WDT_LOAD_REG0       (SE_WDT_BMC_BASE + 0x10)
+#define WDT_LOAD_REG1       (SE_WDT_BMC_BASE + 0x14)
+#define WDT_CONTROL_REG     (SE_WDT_BMC_BASE + 0x00)
+
+/*-------------------------------------------------------------------
+ *     Reset and Debug Control Registers 
+ *------------------------------------------------------------------*/
+#define SYS_RST_CTRL_REG (SE_RES_DEB_BASE + 0x00)
+#define SYS_RST_STAT_REG (SE_RES_DEB_BASE + 0X04)
+#define SYS_WDT_RST_EN   (SE_RES_DEB_BASE + 0x08)
+#define SYS_SOFT_RST_EN  (SE_RES_DEB_BASE + 0x0C)
+
+#endif
