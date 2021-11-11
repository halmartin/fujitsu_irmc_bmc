--- linux-3.14.17/arch/arm/kernel/head.S	2014-08-14 07:08:34.000000000 +0530
+++ linux-3.14.17.new/arch/arm/kernel/head.S	2014-09-04 11:49:30.817797292 +0530
@@ -406,6 +406,18 @@
 
 
 
+#ifdef CONFIG_ARCH_HORNET
+/*
+ * This provides a C-API version of the printascii
+ */
+/*ENTRY(EarlyPrintk_Asm)
+	stmfd	sp!, {r0 - r12, lr}
+	bl	EarlyPrintk
+	ldmfd	sp!, {r0 - r12, pc}
+ENDPROC(EarlyPrintk_Asm)*/
+#endif
+
+
 /*
  * Setup common bits before finally enabling the MMU.  Essentially
  * this is just loading the page table pointer and domain access
