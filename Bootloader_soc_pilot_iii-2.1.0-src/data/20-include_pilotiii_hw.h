--- u-boot-1.1.6/include/pilotiii_hw.h	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/include/pilotiii_hw.h	2010-04-05 16:47:25.000000000 -0400
@@ -0,0 +1,17 @@
+#ifndef _PILOTIII_HW_H_
+#define _PILOTIII_HW_H_
+
+#include <config.h>
+
+/* U-Boot does not use MMU. So no mapping */
+#define IO_ADDRESS(x)	(x)
+#define MEM_ADDRESS(x)  (x)
+
+#include <pilotiii/hwmap.h>
+#include <pilotiii/hwreg.h>
+#include <pilotiii/hwdef.h>
+#include <pilotiii/serreg.h>
+#include <pilotiii/macreg.h>
+
+
+#endif
