--- linux-3.14.17/arch/arm/mach-pilot/include/mach/platform.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/platform.h	2014-09-04 15:29:27.874178701 +0530
@@ -0,0 +1,43 @@
+/*
+ *  linux/arch/arm/include/asm/arch-se_pilot3/platform.h
+ *
+ *  This file contains the VERBENA SOC specific values
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
+
+#ifndef PLATFORM_H
+#define PLATFORM_H
+#include <mach/hardware.h>
+#define CONFIG_UART_CLK 1843200
+#include <mach/serreg.h>
+
+#include <asm/soc-pilot/hwmap.h>
+#include <asm/soc-pilot/hwreg.h>
+#include <asm/soc-pilot/hwdef.h>
+
+#include <mach/mapdesc.h>
+/*****************************************************************
+						    Flash
+*****************************************************************/
+#define CPE_FLASH_BASE                  0x0
+#define CPE_FLASH_SZ                    0x00800000
+#define CPE_FLASH_VA_BASE               MEM_ADDRESS(CPE_FLASH_BASE)
+
+#endif
+
