--- linux-3.14.17/arch/arm/mach-pilot/include/mach/memory.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/memory.h	2014-09-04 12:02:00.653818963 +0530
@@ -0,0 +1,35 @@
+/*
+ *  linux/arch/arm/include/asm/arch-xxx/memory.h
+ *
+ *  Copyright (C) 1999 ARM Limited
+ *  Copyright (C) 2005 American Megatrends Inc.
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
+#ifndef __ASM_ARCH_MEMORY_H
+#define __ASM_ARCH_MEMORY_H
+
+/*
+ * Physical DRAM offset.
+ */
+#define PHYS_OFFSET	CONFIG_PHYS_OFFSET
+#define BUS_OFFSET	CONFIG_PHYS_OFFSET
+
+#define __virt_to_bus(x)	((x) - PAGE_OFFSET + BUS_OFFSET)
+#define __bus_to_virt(x)	((x) - BUS_OFFSET + PAGE_OFFSET)
+
+#define __virt_to_phys(x)	((x) - PAGE_OFFSET + PHYS_OFFSET)
+#define __phys_to_virt(x)	((x) - PHYS_OFFSET + PAGE_OFFSET)
+#endif
