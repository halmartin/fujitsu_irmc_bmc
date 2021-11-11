--- u-boot-1.1.6/lib_arm/armlinux.c	2006-11-02 09:15:01.000000000 -0500
+++ u-boot-1.1.6-ami/lib_arm/armlinux.c	2012-04-24 14:56:58.994467308 -0400
@@ -75,6 +75,9 @@
 
 extern image_header_t header;	/* from cmd_bootm.c */
 
+#ifdef CFG_YAFU_SUPPORT
+extern int fwupdate(void);
+#endif
 
 void do_bootm_linux (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
 		     ulong addr, ulong *len_ptr, int verify)
@@ -115,7 +118,13 @@
 		if (ntohl (hdr->ih_magic) != IH_MAGIC) {
 			printf ("Bad Magic Number\n");
 			SHOW_BOOT_PROGRESS (-10);
+#ifndef CONFIG_SPX_FEATURE_FAIL_SAFE_BOOTING
+#ifdef CFG_YAFU_SUPPORT 
+			       printf ("Jumping to Firmware Update Mode... waits for 2 minutes for somebody to start firmware upgrade process\n");
+				fwupdate();
+#endif
 			do_reset (cmdtp, flag, argc, argv);
+#endif
 		}
 
 		data = (ulong) & header;
@@ -127,7 +136,13 @@
 		if (crc32 (0, (unsigned char *) data, len) != checksum) {
 			printf ("Bad Header Checksum\n");
 			SHOW_BOOT_PROGRESS (-11);
+#ifndef CONFIG_SPX_FEATURE_FAIL_SAFE_BOOTING
+#ifdef CFG_YAFU_SUPPORT 
+			       printf ("Jumping to Firmware Update Mode... waits for 2 minutes for somebody to start firmware upgrade process\n");
+				fwupdate();
+#endif
 			do_reset (cmdtp, flag, argc, argv);
+#endif
 		}
 
 		SHOW_BOOT_PROGRESS (10);
@@ -152,7 +167,13 @@
 			if (csum != ntohl (hdr->ih_dcrc)) {
 				printf ("Bad Data CRC\n");
 				SHOW_BOOT_PROGRESS (-12);
+#ifndef CONFIG_SPX_FEATURE_FAIL_SAFE_BOOTING
+#ifdef CFG_YAFU_SUPPORT
+				printf ("Jumping to Firmware Update Mode... waits for 2 minutes for somebody to start firmware upgrade process\n");
+				fwupdate();
+#endif
 				do_reset (cmdtp, flag, argc, argv);
+#endif
 			}
 			printf ("OK\n");
 		}
@@ -164,7 +185,13 @@
 		    (hdr->ih_type != IH_TYPE_RAMDISK)) {
 			printf ("No Linux ARM Ramdisk Image\n");
 			SHOW_BOOT_PROGRESS (-13);
+#ifndef CONFIG_SPX_FEATURE_FAIL_SAFE_BOOTING
+#ifdef CFG_YAFU_SUPPORT
+				printf ("Jumping to Firmware Update Mode... waits for 2 minutes for somebody to start firmware upgrade process\n");
+				fwupdate();
+#endif			
 			do_reset (cmdtp, flag, argc, argv);
+#endif
 		}
 
 #if defined(CONFIG_B2) || defined(CONFIG_EVB4510) || defined(CONFIG_ARMADILLO)
