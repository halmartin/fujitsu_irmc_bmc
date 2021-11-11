--- u-boot-1.1.6/vendors/ami/sideband/cmd_sideband.c	1970-01-01 08:00:00.000000000 +0800
+++ u-boot-1.1.6-ami/vendors/ami/sideband/cmd_sideband.c	2014-04-08 10:55:14.936512583 +0800
@@ -0,0 +1,15 @@
+# include <common.h>
+# include <config.h>
+# include <command.h>
+# include <net.h>
+# include "cmd_sideband.h"
+
+#ifdef CONFIG_SIDEBAND_SUPPORT
+/* U-boot's cmd function enter sideband discovery */
+int  
+do_sideband (cmd_tbl_t *cmdtp, int flag , int argc, char *argv[])
+{
+	NetLoop(SIDEBAND);
+	return 0;
+}
+#endif
