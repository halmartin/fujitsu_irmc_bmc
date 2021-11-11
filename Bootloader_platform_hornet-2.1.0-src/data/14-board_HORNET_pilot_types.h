--- uboot/board/HORNET/pilot_types.h	1969-12-31 19:00:00.000000000 -0500
+++ uboot.ok/board/HORNET/pilot_types.h	2010-05-21 14:23:42.117302374 -0400
@@ -0,0 +1,19 @@
+
+#ifndef __PILOTTYPES
+#define __PILOTTYPES
+typedef unsigned int              tU32;
+typedef unsigned short            tU16;
+typedef unsigned char             tU8;
+
+typedef unsigned int *            tPU32;
+typedef unsigned short *          tPU16;
+typedef unsigned char *           tPU8;
+
+typedef volatile unsigned int     tVU32;
+typedef volatile unsigned short   tVU16;
+typedef volatile unsigned char    tVU8;
+
+typedef volatile unsigned int *   tPVU32;
+typedef volatile unsigned short * tPVU16;
+typedef volatile unsigned char *  tPVU8;
+#endif
