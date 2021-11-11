--- linux_old/net/core/dev.c	2015-01-30 13:45:25.394149497 +0530
+++ linux/net/core/dev.c	2015-01-28 16:38:00.046242213 +0530
@@ -1165,6 +1165,17 @@
 	call_netdevice_notifiers(NETDEV_LINK_DOWN, dev);
 }
 EXPORT_SYMBOL(netdev_link_down);
+void netdev_phy_link_up(struct net_device *dev)
+{
+    call_netdevice_notifiers(NETDEV_PHY_LINK_UP, dev);
+}
+EXPORT_SYMBOL(netdev_phy_link_up);
+
+void netdev_phy_link_down(struct net_device *dev)
+{
+    call_netdevice_notifiers(NETDEV_PHY_LINK_DOWN, dev);
+}
+EXPORT_SYMBOL(netdev_phy_link_down);
 
 /**
  *	dev_set_alias - change ifalias of a device
