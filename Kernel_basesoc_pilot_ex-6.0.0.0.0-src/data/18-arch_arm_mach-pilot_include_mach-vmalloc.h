--- linux-3.14.17/arch/arm/mach-pilot/include/mach/vmalloc.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/vmalloc.h	2014-09-04 12:02:53.497820490 +0530
@@ -0,0 +1,20 @@
+/*
+ *  linux/arch/arm/include/asm/arch-xxx/vmalloc.h
+ *
+ *  Copyright (C) 2000 Russell King.
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
+#define VMALLOC_END       (PAGE_OFFSET + 0x20000000)	/* Increased from (PAGE_OFFSET + 0x19000000) to support 512MBytes of RAM */
