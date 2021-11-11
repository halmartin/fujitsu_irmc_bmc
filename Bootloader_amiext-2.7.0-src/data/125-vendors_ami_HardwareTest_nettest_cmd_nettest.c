diff -Naur uboot.org/vendors/ami/HardwareTest/nettest/cmd_nettest.c uboot/vendors/ami/HardwareTest/nettest/cmd_nettest.c
--- uboot.org/vendors/ami/HardwareTest/nettest/cmd_nettest.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/nettest/cmd_nettest.c	2014-01-27 15:09:50.953237176 +0800
@@ -0,0 +1,39 @@
+/*
+ * (C) Copyright 2013 ASPEED Software
+ * MAC Manufacture Test in ASPEED's SDK
+ *
+*/
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#include <common.h>
+#include <command.h>
+#include <COMMINF.H>
+#define CONFIG_SYS_MAXARGS 16
+
+#ifdef SLT_UBOOT
+extern int main_function(int argc, char *argv[]);
+
+int do_mactest (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
+{
+    ModeSwitch = MODE_DEDICATED;
+    return main_function( argc, argv);
+}
+
+int do_ncsitest (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
+{
+    ModeSwitch = MODE_NSCI;
+    return main_function( argc, argv);
+}
+
+U_BOOT_CMD(
+    mactest,    CONFIG_SYS_MAXARGS, 0,  do_mactest,
+    "mactest - Dedicated LAN test program \n",
+    NULL
+);
+U_BOOT_CMD(
+    ncsitest,    CONFIG_SYS_MAXARGS, 0,  do_ncsitest,
+    "ncsitest- Share LAN (NC-SI) test program \n",
+    NULL
+);
+
+#endif // End SLT_UBOOT
+#endif
