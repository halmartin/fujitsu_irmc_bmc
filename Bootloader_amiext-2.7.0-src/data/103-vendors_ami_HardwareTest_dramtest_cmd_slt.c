diff -Naur uboot.org/vendors/ami/HardwareTest/dramtest/cmd_slt.c uboot/vendors/ami/HardwareTest/dramtest/cmd_slt.c
--- uboot.org/vendors/ami/HardwareTest/dramtest/cmd_slt.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/dramtest/cmd_slt.c	2014-01-27 15:09:50.957237126 +0800
@@ -0,0 +1,47 @@
+/*
+ * (C) Copyright 2013 ASPEED Software
+ * SLT in ASPEED's SDK
+ *
+*/
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#include <common.h>
+#include <command.h>
+#define CONFIG_SYS_MAXARGS 16
+
+extern int pll_function(int argc, char *argv[]);
+extern int trap_function(int argc, char *argv[]);
+extern int dram_stress_function(int argc, char *argv[]);
+
+int do_plltest (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
+{
+    return pll_function( argc, argv);
+}
+
+U_BOOT_CMD(
+    plltest,   CONFIG_SYS_MAXARGS, 0,  do_plltest,
+    "plltest - PLLTest [pll mode] [err rate] \n",
+    NULL
+);
+
+int do_traptest (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
+{
+    return trap_function( argc, argv);
+}
+
+U_BOOT_CMD(
+    traptest,   CONFIG_SYS_MAXARGS, 0,  do_traptest,
+    "traptest- Check hardware trap for CPU clock and CPU\\AHB ratio.\n",
+    NULL
+);
+
+int do_dramtest (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
+{
+    return dram_stress_function( argc, argv);
+}
+
+U_BOOT_CMD(
+    dramtest,   CONFIG_SYS_MAXARGS, 0,  do_dramtest,
+    "dramtest- Stress DRAM.\n",
+    NULL
+);
+#endif
