--- u-boot-1.1.6/vendors/ami/sideband/sideband.h	1970-01-01 08:00:00.000000000 +0800
+++ u-boot-1.1.6-ami/vendors/ami/sideband/sideband.h	2014-04-08 10:55:14.936512583 +0800
@@ -0,0 +1,32 @@
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
+  Module	: SIDEBAND Definitions
+
+  Revision	: 1.0  
+
+  Changelog : 1.0 - Initial Version  [SC]
+
+ *****************************************************************/
+#ifndef __SIDEBAND_H__
+#define __SIDEBAND_H__
+
+#include "types.h"
+
+void SIDEBAND_Start(void);
+void SIDEBANDHandler(unsigned char *Buffer,unsigned Unused1,unsigned Unused2, unsigned Len);
+
+#endif
