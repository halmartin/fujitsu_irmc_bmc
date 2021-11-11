--- linux/drivers/mmc/card/block.c	2014-08-14 09:38:34.000000000 +0800
+++ linux.new/drivers/mmc/card/block.c	2017-04-17 14:10:50.150162406 +0800
@@ -1145,7 +1145,14 @@
 	 * initial command - such as address errors.  No data
 	 * has been transferred.
 	 */
+#if defined(CONFIG_SOC_SE_PILOT4)	
+	if ((brq->cmd.resp[0] & R1_OUT_OF_RANGE) && !(brq->cmd.resp[0] & (R1_ADDRESS_ERROR | R1_BLOCK_LEN_ERROR | R1_WP_VIOLATION | R1_CC_ERROR | R1_ERROR))) {
+		// If occurs OUT_OF_RANGE only, host does not consider it as error.
+	}
+	else if (brq->cmd.resp[0] & CMD_ERRORS) {
+#else
 	if (brq->cmd.resp[0] & CMD_ERRORS) {
+#endif
 		pr_err("%s: r/w command failed, status = %#x\n",
 		       req->rq_disk->disk_name, brq->cmd.resp[0]);
 		return MMC_BLK_ABORT;
