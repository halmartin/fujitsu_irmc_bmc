--- linux-3.14.17/arch/arm/mach-pilot/Makefile.boot	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/Makefile.boot	2014-09-04 11:52:28.069802415 +0530
@@ -0,0 +1,7 @@
+  zreladdr-y   := 0x80808000
+params_phys-y           := 0x80800100
+initrd_phys-y           := 0x80C00000
+
+
+
+
