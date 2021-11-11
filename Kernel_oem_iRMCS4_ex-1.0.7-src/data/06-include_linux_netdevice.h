--- linux_old/include/linux/netdevice.h	2015-01-30 13:45:06.222149036 +0530
+++ linux/include/linux/netdevice.h	2015-01-28 16:56:42.778269218 +0530
@@ -1764,6 +1764,9 @@
 #define NETDEV_PRECHANGEMTU	0x0017 /* notify before mtu change happened */
 #define NETDEV_LINK_UP     0x0101  /* Link up Notifier   */
 #define NETDEV_LINK_DOWN   0x0102  /* Link down Notifier */
+#define NETDEV_PHY_LINK_UP      0x0103          /* PHY Link up Notifier   */
+#define NETDEV_PHY_LINK_DOWN    0x0104          /* PHY Link down Notifier */
+
 
 
 int register_netdevice_notifier(struct notifier_block *nb);
@@ -2959,6 +2962,9 @@
 void netdev_features_change(struct net_device *dev);
 void netdev_link_up(struct net_device *dev);
 void netdev_link_down(struct net_device *dev);
+void netdev_phy_link_up(struct net_device *dev);
+void netdev_phy_link_down(struct net_device *dev);
+
 /* Load a device via the kmod */
 void dev_load(struct net *net, const char *name);
 struct rtnl_link_stats64 *dev_get_stats(struct net_device *dev,
