--- linux-3.14.17/arch/arm/include/asm/soc-pilot/hwdef.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/include/asm/soc-pilot/hwdef.h	2014-09-04 12:01:08.717817462 +0530
@@ -0,0 +1,101 @@
+/*
+ *  This file contains the SE PILOT-III Register definitions
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
+#ifndef     _HWDEF_PILOT_III_
+#define     _HWDEF_PILOT_III_
+
+
+/* -----------------------------------------------------------------
+ *      	         SRAM Registers
+ * -----------------------------------------------------------------
+ */
+
+
+/*
+ * -------------------------------------------------------------
+ *  					SDRAM Registers
+ * -------------------------------------------------------------
+ */
+
+/*--------------------------------------------------------
+ *  Timer definitions
+ * ------------------------------------------------------
+ */
+/* Timer conrol reg */
+#define TIMER_INTR_ENABLE		0x000
+#define TIMER_INTR_DISBALE		0x004
+#define TIMER_PERIODICAL		0x002
+#define TIMER_FREERUNNING		0x000
+#define TIMER_ENABLE			0x001
+#define TIMER_DISBALE			0x000
+
+
+/* --------------------------------------------------------
+ *  Interrupt Controllers
+ * --------------------------------------------------------
+ */
+
+
+/*  --------------------------------------------------------------
+ *   I2S Controllers
+ *  --------------------------------------------------------------
+ */
+
+
+/*  --------------------------------------------------------------
+ *   SSP Controllers
+ *  --------------------------------------------------------------
+ */
+
+
+/*  --------------------------------------------------------------
+ *   I2C Controllers
+ *  --------------------------------------------------------------
+ */
+
+
+/*----------------------------------------------------------------
+ *   DMA Controllers
+ *----------------------------------------------------------------
+ */
+
+
+/*----------------------------------------------------------------
+ *   Serial  Controllers
+ *----------------------------------------------------------------
+ */
+#define PILOT_UART_CLOCK      UART_CLOCK	/* defined in board config */
+#define PILOT_BAUD_115200	    (PILOT_UART_CLOCK /1843200)
+#define PILOT_BAUD_57600	    (PILOT_UART_CLOCK /921600)
+#define PILOT_BAUD_38400	    (PILOT_UART_CLOCK /614400)
+#define PILOT_BAUD_19200	    (PILOT_UART_CLOCK /307200)
+#define PILOT_BAUD_14400	    (PILOT_UART_CLOCK /230400)
+#define PILOT_BAUD_9600			(PILOT_UART_CLOCK /153600)
+
+/*----------------------------------------------------------------
+ *   RTC
+ *----------------------------------------------------------------
+ */
+
+/*----------------------------------------------------------------
+ *   WDT
+ *----------------------------------------------------------------
+ */
+#endif
