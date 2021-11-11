--- u-boot-1.1.6/net/eth.c	2006-11-02 09:15:01.000000000 -0500
+++ u-boot-1.1.6-PilotIII/net/eth.c	2010-04-05 16:47:24.000000000 -0400
@@ -54,6 +54,9 @@
 extern int skge_initialize(bd_t*);
 extern int tsec_initialize(bd_t*, int, char *);
 extern int npe_initialize(bd_t *);
+#if defined(CONFIG_PILOTIII)
+extern int 	ast_eth_initialize(bd_t *bis);
+#endif
 
 static struct eth_device *eth_devices, *eth_current;
 
@@ -252,6 +255,9 @@
 	rtl8169_initialize(bis);
 #endif
 
+#if defined(CONFIG_PILOTIII)
+	ast_eth_initialize(bis);
+#endif
 	if (!eth_devices) {
 		puts ("No ethernet found.\n");
 	} else {
