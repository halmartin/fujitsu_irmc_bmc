--- linux_orig/drivers//net/ethernet/pilot_tulip/tulip.h	2016-09-19 17:50:51.107189986 +0530
+++ linux/drivers//net/ethernet/pilot_tulip/tulip.h	2016-09-19 18:59:29.766076101 +0530
@@ -323,7 +323,8 @@
 
 #define EEPROM_SIZE 512 	/* 2 << EEPROM_ADDRLEN */
 
-
+#define FRONT_LAN_PHY_INDEX     2   /* Can be modified as configurable if needed */
+#define ENABLE_FRONT_LAN	0
 #define RUN_AT(x) (jiffies + (x))
 
 #define get_u16(ptr) get_unaligned_le16((ptr))
@@ -443,6 +444,8 @@
 	struct net_device *dev;
 	int phy_index;	/* Pilot-II */
 	int linkdown;	/* Pilot-II */
+        int phy_linkdown;   /* Pilot-II */
+        struct timer_list phy_timer;
 	int speed;
 	struct mii_if_info mii; /* Used for Generic MII */
 };
