--- linux-3.14.17/arch/arm/mach-pilot/include/mach/uncompress.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/uncompress.h	2014-09-04 12:02:39.821820095 +0530
@@ -0,0 +1,70 @@
+/*
+ * linux/arch/arm/include/asm/arch-xxx/uncompress.h
+ *
+ * Copyright (C) 2005 American Megatrends Inc.
+ */
+
+#include <mach/hardware.h>
+#include <mach/lowlevel_serial.h>
+
+#define flush()  do { } while(0)
+
+
+static void
+putc(const char c)
+{
+	volatile unsigned int status=0;
+
+	/* Wait for Ready */
+	do
+   {
+    	status = *(volatile unsigned char *)(UART_BASE+SERIAL_LSR);
+    }
+    while (!((status & SERIAL_LSR_THRE)==SERIAL_LSR_THRE) );
+
+	/* Write Character */
+    *(volatile unsigned char *)(UART_BASE+SERIAL_THR) = c;
+
+	return;
+}
+
+
+#if 0
+static void
+putstr(const char *s)
+{
+	while (*s)
+	{
+		putc(*s);
+		if (*s == '\n')
+			putc('\r');
+		s++;
+	}
+}
+#endif
+
+static __inline__ void
+arch_decomp_setup(void)
+{
+	/* Enable DLAB, 8 Bits and One Stop Bit */
+    *(volatile unsigned char *)(UART_BASE+SERIAL_LCR) = 0x83;
+
+	/* Set the Baud Rate */
+    *(volatile unsigned char *)(UART_BASE+SERIAL_DLM) = 
+							((CONFIG_UART_CLK/(BAUD_RATE *16)) & 0xFF00) >> 8;
+    *(volatile unsigned char *)(UART_BASE+SERIAL_DLL) = 
+							((CONFIG_UART_CLK/(BAUD_RATE *16)) & 0x00FF);
+
+	/* Disable  DLAB, 8 Bits and One Stop Bit */
+    *(volatile unsigned char *)(UART_BASE+SERIAL_LCR) = 0x03;
+
+	/* Enable FIFO */
+    *(volatile unsigned char *)(UART_BASE+SERIAL_FCR) = 0xC1;
+
+	return;
+}
+
+/*
+ * nothing to do
+ */
+#define arch_decomp_wdog()
