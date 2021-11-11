--- u-boot-1.1.6/cpu/arm926ejs/PILOTIII/serial.c	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/cpu/arm926ejs/PILOTIII/serial.c	2010-04-05 16:47:24.000000000 -0400
@@ -0,0 +1,130 @@
+/*
+ * (C) Copyright 2002
+ * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
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
+ *
+ */
+
+#include <config.h>
+#include <linux/types.h>
+#include <asm/u-boot.h>
+#include <asm/global_data.h>
+#include "soc_hw.h"
+#include "astserial.h"
+
+DECLARE_GLOBAL_DATA_PTR;
+
+#ifdef CONFIG_SERIAL2
+	UINT32 DebugSerialPort = SE_UART_2_BASE ;
+#elif CONFIG_SERIAL3
+	UINT32 DebugSerialPort = SE_UART_3_BASE ;
+#elif CONFIG_SERIAL4
+	UINT32 DebugSerialPort = SE_UART_4_BASE;
+#else
+	#error "Bad: you didn't configure serial ..."
+#endif
+
+
+void
+serialhandler(void *arg)
+{
+	AST_SerialIntIdentification(DebugSerialPort);
+}
+
+#if 0
+void
+ll_serial_init(void)
+{
+	AST_SerialInit (DebugSerialPort, PILOT_BAUD_115200, PARITY_NONE, 0, 8 );
+	AST_SetSerialFifoCtrl(DebugSerialPort, 1, ENABLE, ENABLE);
+}
+#endif
+
+void
+serial_setbrg(void)
+{
+	unsigned int reg = 0;
+
+	switch (gd->baudrate)
+	{
+		case 9600:
+			reg = PILOT_BAUD_9600;
+			break;
+		case 19200:
+			reg = PILOT_BAUD_19200;
+			break;
+		case 38400:
+			reg = PILOT_BAUD_38400;
+			break;
+		case 57600:
+			reg = PILOT_BAUD_57600;
+			break;
+//#if (SYS_CLK == 22118400)
+		case 115200:
+			reg = PILOT_BAUD_115200;
+			break;
+//#endif
+		default:
+			reg=PILOT_BAUD_38400;
+			break;
+	}
+
+	/* Set Baud Rate */
+	AST_SerialInit (DebugSerialPort, reg, PARITY_NONE, 0, 8 );
+
+	/* Enable FiFo */
+	AST_SetSerialFifoCtrl(DebugSerialPort, 1, ENABLE, ENABLE);
+
+	return;
+}
+
+int
+serial_init(void)
+{
+	serial_setbrg();
+	return 0;
+}
+
+
+int
+serial_getc(void)
+{
+	return AST_GetSerialChar( DebugSerialPort );
+}
+
+
+void
+serial_putc(const char c)
+{
+	AST_PutSerialChar( DebugSerialPort, c );
+   	if(c == '\n')
+	   		AST_PutSerialChar(DebugSerialPort,'\r');
+
+}
+
+int
+serial_tstc(void)
+{
+	return AST_TestSerialForChar(DebugSerialPort);
+}
+
+
+void
+serial_puts (char *s)
+{
+	AST_PutSerialStr(DebugSerialPort,s);
+}
+
