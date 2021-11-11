--- linux-3.14.17/arch/arm/mach-pilot/include/mach/hardware.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/hardware.h	2014-09-04 11:50:19.729798706 +0530
@@ -0,0 +1,45 @@
+/*
+ *  linux/arch/arm/include/asm/arch-se-pilot3/hardware.h
+ *
+ *  This file contains the hardware definitions of the Integrator.
+ *
+ *  Copyright (C) 1999 ARM Limited.
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
+#ifndef __ASM_ARCH_HARDWARE_H
+#define __ASM_ARCH_HARDWARE_H
+
+#include <asm/sizes.h>
+#include <mach/platform.h>
+
+
+
+/*****************************************************************
+							IO Mapping
+*****************************************************************/
+#define IO_PHYS_BASE	         	 	0x40000000
+#ifdef CONFIG_SOC_SE_PILOT3
+#define IO_BASE	                 		0xf0000000
+#else
+#define IO_BASE	                 		0xfd000000
+#endif
+#define MEM_BASE	                 	0xff000000
+#define MEM_ADDRESS(x)                  ((x) | (MEM_BASE))
+#define IO_ADDRESS(x) 					((x) | (IO_BASE)) 
+#define PHY_ADDRESS(x)                  ((x) & 0x0FFFFFFF)| IO_PHYS_BASE)
+#endif
+
