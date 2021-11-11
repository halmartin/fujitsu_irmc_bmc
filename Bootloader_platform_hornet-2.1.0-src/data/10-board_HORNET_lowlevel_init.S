--- uboot/board/HORNET/lowlevel_init.S	1969-12-31 19:00:00.000000000 -0500
+++ uboot.ok/board/HORNET/lowlevel_init.S	2010-05-21 14:23:42.117302374 -0400
@@ -0,0 +1,76 @@
+/* Low level initialization for the board */
+/* Copyright (c) American Megatrends, Inc */
+
+#include <config.h>
+#include <version.h>
+#include "pilotiii_hw.h"
+.include  "pilot3.equ"
+
+.extern hornet_init
+
+/*----------------------------- Setting up the memory and Flash ----------------------*/
+.globl lowlevel_init
+lowlevel_init:
+
+	/* Save Return PC */
+   	mov 	r10,lr
+
+	ldr	r13,=LMEMSTART+0x5000	@temp stack in local memory yy
+/* Check if it is a warm boot. For warm boot skip memory init */
+	ldr     	r6,=SYSCONTROL            @0x40100100
+   	ldr     	r4, [r6]
+	tst		r4,#0x02
+	beq		skip_mem_init
+
+	/* Memory Initialization */
+	bl			hornet_init
+
+// Moved clearing this to reset code of u-boot and linux
+// This is done so that firmware can get the status of last boot (Cold/Warm)
+//	/* Clear cold boot bit */
+//	ldr     	r6,=SYSCONTROL            @0x40100100
+//   	ldr     	r4, [r6]
+//	ldr			r5,=0xFFFFFFFD
+//	and			r4,r4,r5
+//	str			r4, [r6]
+skip_mem_init:
+	
+/*	bl		flash_config  */
+
+	/* Return Back to caller */
+ 	mov		lr, r10
+	mov		pc, lr
+
+
+/*-----------------------------------------------------------------------------------*/
+/*							FLASH Configuration										 */
+/*-----------------------------------------------------------------------------------*/
+/* IMPORTANT: r10 should not be destroyed in this function*/
+
+flash_config:
+
+	/* Save Return PC */
+	mov		r9,lr
+
+	/* Return Back to caller */
+	mov		lr,r9
+	mov		pc, lr
+
+/* No remap function in PILOTII */
+/*-----------------------------------------------------------------------------------*/
+/*					 Remap Memory -Swap Address of ROM and SDRAM					 */
+/*-----------------------------------------------------------------------------------*/
+.globl remap_memory
+remap_memory:
+
+	/* Save Return PC */
+	mov 	r9,lr
+
+	/* Add your SOC Specific Remap if any */
+
+	/* Return Back to caller */
+    mov 	lr,r9
+	mov		pc, lr
+
+	
+
