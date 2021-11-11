--- linux-3.14.17/drivers/mmc/host/sdhci.c	2014-09-12 16:35:29.049069364 +0000
+++ linux/drivers/mmc/host/sdhci.c	2014-09-12 16:35:43.925069545 +0000
@@ -1061,7 +1061,7 @@
 	mod_timer(&host->timer, timeout);
 
 	host->cmd = cmd;
-	host->busy_handle = 0;
+	//host->busy_handle = 0;
 
 	sdhci_prepare_data(host, cmd);
 
