--- uboot/cpu/arm926ejs/PILOTIII/reset.c	1969-12-31 19:00:00.000000000 -0500
+++ uboot.new/cpu/arm926ejs/PILOTIII/reset.c	2010-05-24 22:58:51.283486421 -0400
@@ -0,0 +1,47 @@
+/* Pilot-III  WDT Routines for u-boot reboot */
+
+#include <common.h>
+#include <linux/types.h>
+#include "soc_hw.h"
+
+
+
+void
+reset_cpu  (ulong addr)
+{
+
+	void (*resetloc)(void);
+	volatile unsigned long resetspi;
+
+
+	/* After SPI write/erase, SPI goes to a wierd state and first read
+	   after this returns all 0xff. Furthur read are ok. So in this wierd
+	   state, reset (Jump to 0) will lead to fault. So do a dummy read here 
+	   to clear the wierd state 
+	*/
+	resetspi =*((volatile unsigned long *)0);
+	if (resetspi == 0xFFFFFFFF)
+		printf("Setting SPI to Read Mode \n");
+
+	
+	printf("Resetting ...\n");
+
+	/* clear the cold reset bit */
+	*((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x00)) &= 0xFFFFFFFD;
+
+	/* Watch dog reset does power on reset which will reinitialize DDR thus
+	   blanking out video output. So cannot use WDT. Instead do warn reset */
+	resetloc = 0;
+	(*resetloc)();
+
+	/* Needs the WDT output line to be connected to reset */
+	/*
+	*(unsigned long *)(WDT_CONTROL_REG) = 0x1 ; 		// set watchdog run
+	*(unsigned char *)(WDT_LOAD_REG0) = 0x1;		// load the count
+	*(unsigned char *)(WDT_LOAD_REG1) = 0x0;		// load the count
+	*(unsigned long *)(WDT_CONTROL_REG) = 0x81 ; 		// set watchdog trigger
+	while (1);
+	*/
+	
+}
+
