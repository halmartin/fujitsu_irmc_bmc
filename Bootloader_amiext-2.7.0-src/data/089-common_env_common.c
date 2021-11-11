--- uboot_old/common/env_common.c	2014-12-23 04:11:01.467369283 -0500
+++ uboot/common/env_common.c	2014-12-23 04:24:09.621460844 -0500
@@ -138,7 +138,22 @@
 	"pcidelay="	MK_STR(CONFIG_PCI_BOOTDELAY)	"\0"
 #endif
 #ifdef  CONFIG_EXTRA_ENV_SETTINGS
-	CONFIG_EXTRA_ENV_SETTINGS
+	CONFIG_EXTRA_ENV_SETTINGS                       "\0"
+#endif
+#ifdef CONFIG_BOOT_SELECTOR
+        "bootselector="  MK_STR(CONFIG_BOOT_SELECTOR)   "\0"
+#endif
+#ifdef CONFIG_MOST_RECENTLY_PROG_FW
+        "recentlyprogfw="  MK_STR(CONFIG_MOST_RECENTLY_PROG_FW)  "\0"
+#endif
+#ifdef CONFIG_MEMTEST_ENABLE
+        "do_memtest="  MK_STR(CONFIG_MEMTEST_ENABLE)    "\0"
+#endif
+#ifdef CONFIG_MEMTEST_RESULT
+        "memtest_pass="  CONFIG_MEMTEST_RESULT    "\0"
+#endif
+#ifdef CONFIG_SPX_FEATURE_BMC_FIRMWARE_AUTO_RECOVERY
+        "bootretry="  MK_STR(CONFIG_SPX_FEATURE_BMC_FIRMWARE_REBOOT_RETRY_COUNT)    "\0"
 #endif
 	"\0"
 };
