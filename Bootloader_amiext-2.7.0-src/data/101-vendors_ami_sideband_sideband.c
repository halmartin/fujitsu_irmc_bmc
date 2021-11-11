--- u-boot-1.1.6/vendors/ami/sideband/sideband.c	1970-01-01 08:00:00.000000000 +0800
+++ u-boot-1.1.6-ami/vendors/ami/sideband/sideband.c	2014-04-08 10:55:14.912512583 +0800
@@ -0,0 +1,36 @@
+/****************************************************************
+ ****************************************************************
+ **                                                            **
+ **    (C)Copyright 2005-2007, American Megatrends Inc.        **
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
+  Author	: JCCHIU
+
+  Module	: SIDEBAND Core API
+
+  Revision	: 1.0  
+
+  Changelog : 1.0 - Initial Version  [SC]
+
+ *****************************************************************/
+#include <common.h>
+#include <exports.h>
+#include <net.h>
+#ifdef CONFIG_SIDEBAND_SUPPORT
+#include "sideband.h"
+
+void
+SIDEBANDHandler(unsigned char *Buffer,unsigned Unused1,unsigned Unused2, unsigned Len)
+{
+	printf("SIDEBANDHandler\n");
+}
+
+#endif
