--- linux-3.14.17/arch/arm/mach-pilot/include/mach/mapdesc.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/mapdesc.h	2014-09-04 12:01:53.965818770 +0530
@@ -0,0 +1,7 @@
+#ifndef PILOT_MAP_DESC
+
+#define PILOT_MAP_DESC		\
+ {SE_REGISTER_VA_BASE,	__phys_to_pfn(SE_REGISTER_BASE),	SZ_16M,		MT_DEVICE }
+
+#endif
+
