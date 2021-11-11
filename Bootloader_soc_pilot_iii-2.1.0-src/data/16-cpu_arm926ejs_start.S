--- uboot/cpu/arm926ejs/start.S	2011-01-21 12:03:01.243394721 -0500
+++ uboot.new/cpu/arm926ejs/start.S	2011-01-21 11:19:28.693395983 -0500
@@ -135,6 +135,14 @@
 	orr	r0,r0,#0xd3
 	msr	cpsr,r0
 
+#if defined(CONFIG_PILOTIII)
+	/* Watchdog reset clears the prefetch bit  */
+	/* So enabling the prefetch bit for faster access */
+	ldr  r0,=0x40200010
+	ldrb r1,[r0]
+	orr  r1,r1,#0x10
+	strb r1,[r0]
+#endif
 	/*
 	 * we do sys-critical inits only at reboot,
 	 * not when booting from ram!
@@ -143,6 +151,14 @@
 	bl	cpu_init_crit
 #endif
 
+#if defined(CONFIG_PILOTIII)
+/* Reset the processor mode, because lowlevel_init mucked with it */
+	mrs	r0,cpsr
+	bic	r0,r0,#0x1f
+	orr	r0,r0,#0xd3
+	msr	cpsr,r0
+#endif
+
 #ifndef CONFIG_SKIP_RELOCATE_UBOOT
 relocate:				/* relocate U-Boot to RAM	    */
 	adr	r0, _start		/* r0 <- current position of code   */
@@ -207,6 +223,13 @@
 	mov	r0, #0
 	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
 	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */
+#if defined(CONFIG_PILOTIII)
+
+01:	mrc	p15,0,r15,c7,c14,3      /*test clean and invalidate d-cache*/
+	bne     01b
+
+	mcr	p15,0,r0,c7,c10,4       /*drain write buffer*/
+#endif
 
 	/*
 	 * disable MMU stuff and caches
