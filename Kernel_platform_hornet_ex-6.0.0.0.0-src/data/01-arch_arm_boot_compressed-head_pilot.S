--- linux-3.14.17/arch/arm/boot/compressed/head-pilot.S	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/boot/compressed/head-pilot.S	2014-09-04 11:48:31.669795583 +0530
@@ -0,0 +1,5 @@
+#include <asm/mach-types.h>
+
+		.section	".start", "ax"
+		mov	r7, 	#(MACH_TYPE_HORNET & 0x00ff)
+		orr	r7, r7,	#(MACH_TYPE_HORNET & 0xff00)
