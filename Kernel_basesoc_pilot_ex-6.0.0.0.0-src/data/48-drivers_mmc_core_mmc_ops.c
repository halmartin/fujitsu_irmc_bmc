--- linux/drivers/mmc/core/mmc_ops.c	2014-08-14 09:38:34.000000000 +0800
+++ linux.new/drivers/mmc/core/mmc_ops.c	2015-06-16 16:56:51.869116000 +0800
@@ -413,9 +413,11 @@
 {
 	int err;
 	struct mmc_command cmd = {0};
-	unsigned long timeout;
 	u32 status = 0;
+#if !defined(CONFIG_SOC_SE_PILOT4)
+	unsigned long timeout;
 	bool ignore_crc = false;
+#endif
 
 	BUG_ON(!card);
 	BUG_ON(!card->host);
@@ -425,21 +427,28 @@
 		  (index << 16) |
 		  (value << 8) |
 		  set;
+#if defined(CONFIG_SOC_SE_PILOT4)
+	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
+#else
 	cmd.flags = MMC_CMD_AC;
 	if (use_busy_signal)
 		cmd.flags |= MMC_RSP_SPI_R1B | MMC_RSP_R1B;
 	else
 		cmd.flags |= MMC_RSP_SPI_R1 | MMC_RSP_R1;
-
+#endif
+		
 
 	cmd.cmd_timeout_ms = timeout_ms;
+#if !defined(CONFIG_SOC_SE_PILOT4)
 	if (index == EXT_CSD_SANITIZE_START)
 		cmd.sanitize_busy = true;
-
+#endif
+		
 	err = mmc_wait_for_cmd(card->host, &cmd, MMC_CMD_RETRIES);
 	if (err)
 		return err;
 
+#if !defined(CONFIG_SOC_SE_PILOT4)
 	/* No need to check card status in case of unblocking command */
 	if (!use_busy_signal)
 		return 0;
@@ -454,17 +463,25 @@
 		ignore_crc = true;
 
 	timeout = jiffies + msecs_to_jiffies(MMC_OPS_TIMEOUT_MS);
+#endif
 	do {
+#if defined(CONFIG_SOC_SE_PILOT4)
+		err = mmc_send_status(card, &status);
+		if (err)
+			return err;		
+#else		
 		if (send_status) {
 			err = __mmc_send_status(card, &status, ignore_crc);
 			if (err)
 				return err;
 		}
+#endif		
 		if (card->host->caps & MMC_CAP_WAIT_WHILE_BUSY)
 			break;
 		if (mmc_host_is_spi(card->host))
 			break;
-
+			
+#if !defined(CONFIG_SOC_SE_PILOT4)
 		/*
 		 * We are not allowed to issue a status command and the host
 		 * does'nt support MMC_CAP_WAIT_WHILE_BUSY, then we can only
@@ -481,6 +498,7 @@
 				mmc_hostname(card->host), __func__);
 			return -ETIMEDOUT;
 		}
+#endif
 	} while (R1_CURRENT_STATE(status) == R1_STATE_PRG);
 
 	if (mmc_host_is_spi(card->host)) {
