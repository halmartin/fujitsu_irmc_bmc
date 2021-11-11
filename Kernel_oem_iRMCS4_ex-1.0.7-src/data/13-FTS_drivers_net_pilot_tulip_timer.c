diff -Naur linux_ORIG/drivers/net/ethernet/pilot_tulip/timer.c linux/drivers/net/ethernet/pilot_tulip/timer.c
--- linux_ORIG/drivers/net/ethernet/pilot_tulip/timer.c	2015-04-09 19:50:55.714389672 +0530
+++ linux/drivers/net/ethernet/pilot_tulip/timer.c	2015-04-09 20:52:37.868747664 +0530
@@ -16,6 +16,22 @@
 
 #include "tulip.h"
 
+int front_lan_phy_index = FRONT_LAN_PHY_INDEX;
+extern struct tulip_private *tp_front;
+/* 
+ * call when 'frontlanphy=' is given on the commandline.
+ */
+static
+int __init frontlanphy_setup(char *str)
+{
+    int phy_index;
+    if (get_option(&str, &phy_index))
+	{
+		front_lan_phy_index = phy_index; 
+	}
+return 1;
+}
+__setup("frontlanphy=", frontlanphy_setup);
 
 void tulip_media_task(struct work_struct *work)
 {
@@ -26,9 +42,25 @@
 #if 0	/* Pilot-II/III */
 	u32 csr12 = ioread32(ioaddr + CSR12);
 #endif	
-	int next_tick = 2*HZ;
+	int next_tick = 3*HZ;
 	unsigned long flags;
 
+       if ((tp->phys[tp->phy_index] - 1) == front_lan_phy_index) 
+		{
+			netif_carrier_on(dev);
+			mod_timer(&tp->timer, RUN_AT(next_tick));
+			return;
+		}
+		else if ((tp_front != NULL) && (tp_front->phys[tp_front->phy_index] != INVALID_PHYID) && (tp_front->phy_linkdown != -1))
+		{
+			if (mii_link_ok (&tp_front->mii))
+			{
+				// Front Lan Detected, so ignore rear lan link changes..
+				mod_timer(&tp->timer, RUN_AT(next_tick));
+				return;
+			}
+		}
+
 #if 0	/* Pilot-II/III */
 	if (tulip_debug > 2) {
 		printk(KERN_DEBUG "%s: Media selection tick, %s, status %8.8x mode"
