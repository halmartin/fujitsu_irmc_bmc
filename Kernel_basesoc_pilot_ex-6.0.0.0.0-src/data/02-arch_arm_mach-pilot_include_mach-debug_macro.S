--- linux-3.14.17/arch/arm/mach-pilot/include/mach/debug-macro.S	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/debug-macro.S	2014-09-04 12:00:46.393816817 +0530
@@ -0,0 +1,38 @@
+/* linux/arch/arm/include/asm/arch-xxx/debug-macro.S
+ *
+ * Debugging macro include header
+ *
+ * Copyright (C) 1994-1999 Russell King
+ *
+ * This program is free software; you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License version 2 as
+ * published by the Free Software Foundation.
+ *
+*/
+
+#include <mach/lowlevel_serial.h>
+
+mmu_enabled:
+		.word	io_mapping_done
+
+		.macro	addruart, rp, rv, temp
+		ldr	\rp, =UART_BASE		@ physical address
+		ldr	\rv, =UART_MMU_BASE	@ virtual address
+		.endm
+
+		.macro	senduart,rd,rx
+		strb	\rd, [\rx, #SERIAL_THR]
+		.endm
+
+		.macro	waituart,rd,rx
+1001:	ldr		\rd, [\rx, #SERIAL_LSR]
+		tst		\rd, #1 << 5			@ SERIAL_LSR_THRE
+		beq		1001b
+		.endm
+
+/* I dont know what busyuart should do. So I used the same as waituart */
+		.macro	busyuart,rd,rx
+1001:	ldr		\rd, [\rx, #SERIAL_LSR]
+		tst		\rd, #1 << 5			@ SERIAL_LSR_THRE
+		beq		1001b
+		.endm
