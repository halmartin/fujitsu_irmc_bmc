--- linux_org/drivers/net/mii.c	2016-05-17 18:00:40.883449512 +0530
+++ linux/drivers/net/mii.c	2016-05-17 18:10:13.235432567 +0530
@@ -111,10 +111,17 @@
 				   ADVERTISED_100baseT_Half)) {
 			ethtool_cmd_speed_set(ecmd, SPEED_100);
 			ecmd->duplex = !!(nego & ADVERTISED_100baseT_Full);
-		} else {
+		} else if(nego & (ADVERTISED_10baseT_Full |
+                                   ADVERTISED_10baseT_Half)){
 			ethtool_cmd_speed_set(ecmd, SPEED_10);
 			ecmd->duplex = !!(nego & ADVERTISED_10baseT_Full);
 		}
+		else
+		{
+			ethtool_cmd_speed_set(ecmd, 0);
+			ecmd->duplex = 255;
+		}
+
 	} else {
 		ecmd->autoneg = AUTONEG_DISABLE;
 
