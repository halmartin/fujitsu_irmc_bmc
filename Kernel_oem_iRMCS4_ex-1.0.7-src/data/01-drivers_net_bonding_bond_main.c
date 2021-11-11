--- linux_old/drivers/net/bonding/bond_main.c	2015-01-30 13:43:55.422147333 +0530
+++ linux/drivers/net/bonding/bond_main.c	2015-01-28 20:06:59.000000000 +0530
@@ -2050,6 +2050,7 @@
 				slave->speed == SPEED_UNKNOWN ? 0 : slave->speed,
 				slave->duplex ? "full" : "half");
 
+            netif_carrier_on(slave->dev);
 
 			if (rtnl_trylock())
             {
