--- u-boot-1.1.6/cpu/arm926ejs/interrupts.c	2006-11-02 09:15:01.000000000 -0500
+++ u-boot-1.1.6-PilotIII/cpu/arm926ejs/interrupts.c	2010-04-05 16:47:24.000000000 -0400
@@ -165,14 +165,22 @@
 	bad_mode ();
 }
 
+#if  defined(CONFIG_PILOTIII)
+extern void HandleIntr(void);
+void do_irq (struct pt_regs *pt_regs)
+{
+	HandleIntr();
+}
+#else
 void do_irq (struct pt_regs *pt_regs)
 {
 	printf ("interrupt request\n");
 	show_regs (pt_regs);
 	bad_mode ();
 }
+#endif
 
-#ifdef CONFIG_INTEGRATOR
+#if defined(CONFIG_INTEGRATOR) || defined(CONFIG_PILOTIII)
 
 	/* Timer functionality supplied by Integrator board (AP or CP) */
 
