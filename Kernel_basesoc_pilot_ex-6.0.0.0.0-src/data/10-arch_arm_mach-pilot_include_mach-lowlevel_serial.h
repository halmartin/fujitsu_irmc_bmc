--- linux-3.14.17/arch/arm/mach-pilot/include/mach/lowlevel_serial.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/lowlevel_serial.h	2014-09-04 12:01:46.089818542 +0530
@@ -0,0 +1,29 @@
+/*
+ * linux/arch/arm/include/asm/arch-xxx/lowlevel_serial.h
+ *
+ * Copyright (C) 2005 American Megatrends Inc.
+ */
+
+/* Used during bootup till the serial driver is initialized */
+/* Included in debug_macro.S and uncompress.h */
+#include  <mach/platform.h>
+
+#define UART_BASE  	SE_UART_2_BASE			/* Before MMU */
+#define UART_MMU_BASE 	SE_UART_2_VA_BASE		/* After MMU (actually after paging init)*/
+#define BAUD_RATE CONFIG_SPX_FEATURE_BAUDRATE_CONSOLE_TTY
+
+
+/* Serial Registers */
+#define SERIAL_THR	0x00
+#define SERIAL_RBR	0x00
+#define SERIAL_IER	0x04
+#define SERIAL_IIR  	0x08
+#define SERIAL_FCR  	0x08
+#define SERIAL_LCR  	0x0C
+#define SERIAL_MCR	0x10
+#define SERIAL_LSR  	0x14
+#define SERIAL_MSR	0x18
+#define SERIAL_SPR	0x1C
+
+
+
