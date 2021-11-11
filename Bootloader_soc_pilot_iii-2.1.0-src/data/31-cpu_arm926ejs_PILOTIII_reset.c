--- uboot.old/cpu/arm926ejs/PILOTIII/reset.c	2011-11-25 17:43:23.642519896 +0800
+++ uboot/cpu/arm926ejs/PILOTIII/reset.c	2012-12-06 17:06:30.425983732 +0800
@@ -4,6 +4,8 @@
 #include <linux/types.h>
 #include "soc_hw.h"
 
+#define SPI_CLK_MAX     0x111
+#define SPI_CLK_DEFAULT 0x444
 
 
 void
@@ -13,6 +15,9 @@
 	void (*resetloc)(void);
 	volatile unsigned long resetspi;
 
+	/* Bring down SPI Clock */
+	*(volatile unsigned long*)(0x40100120) &= ~SPI_CLK_MAX;
+	*(volatile unsigned long*)(0x40100120) |= SPI_CLK_DEFAULT;
 
 	/* After SPI write/erase, SPI goes to a wierd state and first read
 	   after this returns all 0xff. Furthur read are ok. So in this wierd
@@ -26,8 +31,8 @@
 	
 	printf("Resetting ...\n");
 
-	/* clear the cold reset bit */
-	*((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x00)) &= 0xFFFFFFFD;
+	/* clear the cold reset bit and address remap bit */
+	*((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x00)) &= 0xFFFFFFFC;
 
 	/* Watch dog reset does power on reset which will reinitialize DDR thus
 	   blanking out video output. So cannot use WDT. Instead do warn reset */
