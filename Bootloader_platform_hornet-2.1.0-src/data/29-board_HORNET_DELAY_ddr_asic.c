diff -Naur uboot/board/HORNET/ddr_asic.c uboot.new/board/HORNET/ddr_asic.c
--- uboot/board/HORNET/ddr_asic.c	2013-11-26 17:42:30.726727950 +0900
+++ uboot.new/board/HORNET/ddr_asic.c	2013-11-26 17:56:08.603095246 +0900
@@ -65,7 +65,7 @@
 
 void DELAY(tU32 count)
 {
-  tU32 i=0;
+  tVU32 i=0;
   for(i=0;i<count;i++)
     ;
 }// DELAY
