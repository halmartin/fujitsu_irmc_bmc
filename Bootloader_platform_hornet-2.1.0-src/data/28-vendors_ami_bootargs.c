--- u-boot-1.1.6/vendors/ami/bootargs.c	2013-03-25 10:47:45.687291061 -0400
+++ uboot/vendors/ami/bootargs.c	2013-03-25 10:22:58.736663532 -0400
@@ -0,0 +1,18 @@
+# include <common.h>
+# include <config.h>
+
+int Get_bootargs(char *bootargs,int rootisinitrd,int RootMtdNo)
+{
+
+	if(rootisinitrd == 1)
+	{
+		sprintf(bootargs,"root=/dev/ramdisk ro ip=none ramdisk_blocksize=4096 ");
+	}
+	else
+	{
+		sprintf(bootargs,"root=/dev/mtdblock%d ro ip=none ",RootMtdNo);
+	}
+		
+	return 0;
+}
+
