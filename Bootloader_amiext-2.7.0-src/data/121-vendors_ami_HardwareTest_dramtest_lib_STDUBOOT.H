diff -Naur uboot.org/vendors/ami/HardwareTest/lib/STDUBOOT.H uboot/vendors/ami/HardwareTest/lib/STDUBOOT.H
--- uboot.org/vendors/ami/HardwareTest/lib/STDUBOOT.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/STDUBOOT.H	2014-01-27 15:09:50.977236876 +0800
@@ -0,0 +1,10 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef STDUBOOT_H
+#define STDUBOOT_H
+
+unsigned long int strtoul(char *string, char **endPtr, int base);
+int atoi( char s[] );
+int rand(void);
+
+#endif // End STDUBOOT_H
+#endif
