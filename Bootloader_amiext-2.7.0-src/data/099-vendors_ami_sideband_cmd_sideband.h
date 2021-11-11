--- u-boot-1.1.6/vendors/ami/sideband/cmd_sideband.h	1970-01-01 08:00:00.000000000 +0800
+++ u-boot-1.1.6-ami/vendors/ami/sideband/cmd_sideband.h	2014-04-08 10:55:14.936512583 +0800
@@ -0,0 +1,15 @@
+#ifndef __AMI_CMD_SIDEBAND_H__
+#define __AMI_CMD_SIDEBAND_H__
+
+/* U-boot's cmd function start sideband discovery */
+extern int  do_sideband (cmd_tbl_t *, int, int, char *[]);
+
+#ifdef CONFIG_SIDEBAND_SUPPORT
+U_BOOT_CMD(				
+	sideband,	1,	0,	do_sideband,				
+	"sideband- Configure SIDEBAND Interfaces\n",	
+	"    - Configure SIDEBAND Interfaces\n"		
+);
+#endif
+
+#endif
