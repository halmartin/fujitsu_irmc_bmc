--- linux-3.14.17/arch/arm/mach-pilot/include/mach/timex.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/timex.h	2014-09-04 12:02:30.237819818 +0530
@@ -0,0 +1,26 @@
+/*
+ *  linux/arch/arm/include/asm/arch-xxx/timex.h
+ *
+ *  Architecture timex specifications
+ *
+ *  Copyright (C) 1999 ARM Limited
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
+#include <mach/platform.h>
+
+//#define CLOCK_TICK_RATE			(25*1000*1000*2)	// I don't know why this does not work
+#define CLOCK_TICK_RATE			(25*1000*1000)		// This works
