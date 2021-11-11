--- u-boot-1.1.6.old/common/main.c	2012-07-26 23:12:39.923715457 -0400
+++ uboot/common/main.c	2012-07-26 16:09:52.593090089 -0400
@@ -237,7 +237,7 @@
 #ifdef CONFIG_MENUPROMPT
 	printf(CONFIG_MENUPROMPT, bootdelay);
 #else
-	printf("Hit any key to stop autoboot: %2d ", bootdelay);
+	printf("Hit Esc key to stop autoboot: %2d ", bootdelay);
 #endif
 
 #if defined CONFIG_ZERO_BOOTDELAY_CHECK
