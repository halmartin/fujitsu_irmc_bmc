diff -Naur uboot.org/vendors/ami/HardwareTest/lib/IO.H uboot/vendors/ami/HardwareTest/lib/IO.H
--- uboot.org/vendors/ami/HardwareTest/lib/IO.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/IO.H	2014-01-27 15:09:50.961237076 +0800
@@ -0,0 +1,28 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef IO_H
+#define IO_H
+
+#include "SWFUNC.H"
+
+//
+// Macro
+//
+#if 	defined(LinuxAP)
+    #define delay(val)	usleep(val*1000)
+    #define ob(p,d) outb(d,p)
+    #define ib(p) inb(p)
+#else
+    #define ob(p,d)	outp(p,d)
+    #define ib(p) inp(p)
+#endif
+
+#ifdef USE_LPC
+void open_aspeed_sio_password(void);
+void enable_aspeed_LDU(BYTE jldu_number);
+int findlpcport(BYTE jldu_number);
+#endif
+
+void WriteSOC_DD(ULONG addr, ULONG data);
+ULONG ReadSOC_DD(ULONG addr);
+#endif
+#endif
