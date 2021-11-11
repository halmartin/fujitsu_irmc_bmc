--- /usr/src/dev/killme/build/hornet/Build/bootloader/uboot/board/HORNET/u-boot.lds	2011-10-25 13:55:33.000000000 +0800
+++ uboot/board/HORNET/u-boot.lds	2011-10-20 22:24:51.000000000 +0800
@@ -40,14 +40,14 @@
 	  net/libnet.a (.text)
 	}
 
-	. = 0x87F0FF80;
+	. = 0x847FF800;
 
 	.fmh 	:
 	{
 		board/HORNET/fmh.o (.bss)
 	} 
 
-	. = 0x87F10000;
+	. = 0x84800000;
 
 	.text2 		:
 	{
