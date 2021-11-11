--- u-boot-1.1.6/vendors/ami/sideband/sidebandstart.c	1970-01-01 08:00:00.000000000 +0800
+++ u-boot-1.1.6-ami/vendors/ami/sideband/sidebandstart.c	2014-04-08 10:55:14.932512583 +0800
@@ -0,0 +1,40 @@
+/****************************************************************
+ ****************************************************************
+ **                                                            **
+ **    (C)Copyright 2005-2006, American Megatrends Inc.        **
+ **                                                            **
+ **            All Rights Reserved.                            **
+ **                                                            **
+ **        6145-F, Northbelt Parkway, Norcross,                **
+ **                                                            **
+ **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
+ **                                                            **
+ ****************************************************************
+ ****************************************************************/
+/****************************************************************
+
+  Author	: JCCHIU
+
+  Module	: SIDEBAND Startup code.
+		  Calls the necessary SIDEBAND functions to enable
+		  SIDEBAND Pass thru.
+			
+  Revision	: 1.0  
+
+  Changelog : 1.0 - Initial Version [SC]
+
+*****************************************************************/
+#include <common.h>
+#ifdef CONFIG_SIDEBAND_SUPPORT
+#include <exports.h>
+#include <net.h>
+#include <sideband.h>
+
+void
+SIDEBAND_Start(void)
+{
+
+    sideband_probe();
+
+}
+#endif
