--- linux-3.14.17/arch/arm/mach-pilot/include/mach/system.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/system.h	2014-09-04 12:02:23.273819617 +0530
@@ -0,0 +1,103 @@
+/*
+ *  linux/arch/arm/include/asm/arch-xxx/system.h
+ *
+ *  Copyright (C) 1999 ARM Limited
+ *  Copyright (C) 2000 Deep Blue Solutions Ltd
+ *	Copyright (C) 2005 American Megatrends Inc
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
+#ifndef __ASM_ARCH_SYSTEM_H
+#define __ASM_ARCH_SYSTEM_H
+
+#include <mach/platform.h>
+
+#define SPI_CLK_MAX     0x111
+#define SPI_CLK_DEFAULT 0x444
+
+/* Some Debug Functions */
+void CheckPrintk(char *);
+void CheckPoint(unsigned long val);
+
+static inline void arch_idle(void)
+{
+	/*
+	 * This should do all the clock switching
+	 * and wait for interrupt tricks
+	 */
+	cpu_do_idle();
+}
+
+static inline void arch_reset(char mode)
+{
+//	void (*resetloc)(void);
+	volatile unsigned long resetspi;
+
+	local_irq_disable();
+	
+        /* Disable all interrupts */
+        *((volatile unsigned long*) Pilot2_Irq_intmask_Reg) = 0xFFFFFFFF;
+        *((volatile unsigned long*) Pilot2_Irq_inten_Reg)   = 0;
+        *((volatile unsigned long*) Pilot2_Combined_Irq_Ctrl_Reg) = 0x0;
+
+        *((volatile unsigned long*) Pilot2_Fiq_intmask_Reg) = 0xFFFFFFFF;
+        *((volatile unsigned long*) Pilot2_Fiq_inten_Reg)   = 0;
+        *((volatile unsigned long*) Pilot2_Combined_Fiq_Ctrl_Reg) = 0x0;
+
+        *((volatile unsigned long*) Pilot2_Irq_Ctrl_Hi_Reg) = 0x0;
+        *((volatile unsigned long*) Pilot2_Fiq_Ctrl_Hi_Reg) = 0x0;
+
+	/* Bring down SPI clock */
+	*(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+0x20) &= ~SPI_CLK_MAX;
+	*(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+0x20) |= SPI_CLK_DEFAULT;
+
+        /**((volatile unsigned long*)(SE_INTC_BASE+0x08) ) = 0xFFFFFFFF;
+        *((volatile unsigned long*)(SE_INTC_BASE+0x00) )   = 0;
+        *((volatile unsigned long*) (SE_SYS_CLK_BASE+0x38)) = 0x0;
+
+        *((volatile unsigned long*) (SE_INTC_BASE+0xC4)) = 0xFFFFFFFF;
+        *((volatile unsigned long*) (SE_INTC_BASE+0xC0))   = 0;
+        *((volatile unsigned long*) (SE_SYS_CLK_BASE + 0x3C)) = 0x0;
+
+        *((volatile unsigned long*) (SE_SYS_CLK_BASE+0x60)) = 0x0;
+        *((volatile unsigned long*) (SE_SYS_CLK_BASE + 0x64)) = 0x0;*/
+
+	/* Bring down SPI clock */
+	*(volatile unsigned long*)(SE_SYS_CLK_BASE+0x20) &= ~SPI_CLK_MAX;
+	*(volatile unsigned long*)(SE_SYS_CLK_BASE+0x20) |= SPI_CLK_DEFAULT;
+
+	/* After SPI write/erase, SPI goes to a wierd state and first read
+	   after this returns all 0xff. Furthur read are ok. So in this wierd
+	   state, reset (Jump to 0) will lead to fault. So do a dummy read here 
+	   to clear the wierd state
+	*/
+	resetspi =*((volatile unsigned long *)0);
+	if (resetspi == 0xFFFFFFFF)
+		printk("Setting SPI to Read Mode \n");
+
+	/* clear the cold reset bit */
+	printk("arch_reset() called with mode %d \n",mode);
+
+	// clear the cold reset bit
+	*((volatile unsigned long *)(SE_SYS_CLK_BASE + 0x00)) &= 0xFFFFFFFC;
+	cpu_reset(0x0);
+
+//	resetloc=(unsigned int *)0x0;
+//	(*resetloc)();
+	printk("arch_reset() failed \n");
+	while (1);
+}
+
+#endif
