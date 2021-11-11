--- linux-3.14.17/arch/arm/mach-pilot/include/mach/serial.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/serial.h	2015-04-22 17:38:59.025429095 +0530
@@ -0,0 +1,49 @@
+/*
+ *  linux/arch/arm/include/asm/arch-xxx/serial.h
+ *
+ *  This serial port is similiar to UART 8250
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
+
+#include <linux/serial_core.h>
+#include <mach/hardware.h>
+
+#define BASE_BAUD	(CONFIG_UART_CLK/16)
+
+#define STD_COM_FLAGS (ASYNC_SKIP_TEST | UPF_LOW_LATENCY)
+
+#ifdef CONFIG_SOC_SE_PILOT3
+#define SERIAL_PORT_DFNS			\
+ {0,BASE_BAUD,SE_UART_2_VA_BASE,IRQ_UART_2,STD_COM_FLAGS,0,UPIO_PORT,0,2},	\
+ {0,BASE_BAUD,SE_UART_3_VA_BASE,IRQ_UART_3,STD_COM_FLAGS,0,UPIO_PORT,0,2},	\
+ {0,BASE_BAUD,SE_UART_4_VA_BASE,IRQ_UART_4,STD_COM_FLAGS,0,UPIO_PORT,0,2},	\
+ {0,BASE_BAUD,SE_VIRTUAL_UART_0_VA_BASE, IRQ_V_UART_0, STD_COM_FLAGS,0,UPIO_PORT,0,2 }, \
+ {0,BASE_BAUD,SE_VIRTUAL_UART_1_VA_BASE, IRQ_V_UART_1, STD_COM_FLAGS,0,UPIO_PORT,0,2 }, 
+#else
+#define SERIAL_PORT_DFNS			\
+{0,BASE_BAUD, SE_UART_2_VA_BASE, IRQ_UART_2,STD_COM_FLAGS,0,UPIO_MEM32,(void __iomem *)SE_UART_2_VA_BASE,2},\
+ {0,BASE_BAUD,SE_UART_3_VA_BASE,IRQ_UART_3,STD_COM_FLAGS,0,UPIO_MEM32,(void __iomem *)SE_UART_3_VA_BASE,2},      \
+ {0,BASE_BAUD,SE_UART_4_VA_BASE,IRQ_UART_4,STD_COM_FLAGS,0,UPIO_MEM32,(void __iomem *)SE_UART_4_VA_BASE,2},      \
+ {0,BASE_BAUD,SE_VIRTUAL_UART_0_VA_BASE, IRQ_V_UART_0, STD_COM_FLAGS,0,UPIO_MEM32,(void __iomem *)SE_VIRTUAL_UART_0_VA_BASE,2 }, \
+ {0,BASE_BAUD,SE_VIRTUAL_UART_1_VA_BASE, IRQ_V_UART_1, STD_COM_FLAGS,0,UPIO_MEM32,(void __iomem *)SE_VIRTUAL_UART_1_VA_BASE,2 },
+#endif
+
+
+
+
