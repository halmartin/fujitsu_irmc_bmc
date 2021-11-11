--- linux_orig/drivers//net/ethernet/pilot_tulip/tulip_core.c	2016-09-19 17:50:51.107189986 +0530
+++ linux/drivers//net/ethernet/pilot_tulip/tulip_core.c	2016-09-19 19:01:23.392814388 +0530
@@ -37,6 +37,8 @@
 #include <linux/ethtool.h>
 #include <linux/mii.h>
 #include <linux/eth_over.h>
+#include <linux/rtnetlink.h>
+#include <net/ipconfig.h>
 
 #ifdef CONFIG_SPARC
 #include <asm/prom.h>
@@ -55,6 +57,7 @@
 
 
 struct net_device *pilot3_devices[2];
+struct tulip_private *tp_front = NULL;
 #define MAX_UNITS 8
 /* Used to pass the full-duplex flag, etc. */
 static int full_duplex[MAX_UNITS];
@@ -135,6 +138,7 @@
 #else
 int tulip_debug = 1;
 #endif
+extern int front_lan_phy_index;
 
 #ifdef CONFIG_SPX_FEATURE_GLOBAL_SHARED_MDIO_MDC_LINES
 //to be used in media.c
@@ -150,6 +154,151 @@
 		schedule_work(&tp->media_work);
 }
 
+static void tulip_phy_link_monitor(unsigned long data)
+{
+    struct tulip_private *tp = (struct tulip_private *)data;
+    struct net_device *dev = tp->dev;
+    int interface = 0;
+    unsigned short phyid;
+    unsigned int org_csr6 = tp->csr6;
+    unsigned int org_full_duplex = tp->full_duplex;
+    unsigned int org_speed = tp->speed;
+
+    if ((tp_front == NULL) || (tp_front->phys[tp_front->phy_index] == INVALID_PHYID))
+    {
+        // No Front LAN Module, so ignore PHY link Monitor
+        if (tp->phys[tp->phy_index] == INVALID_PHYID)
+        {
+#ifdef CONFIG_PILOT3_MAC1_PHY_PRESENT
+            tp->phys[tp->phy_index] = CONFIG_PILOT3_MAC1_PHYID + 1;
+            tp->mii.phy_id = CONFIG_PILOT3_MAC1_PHYID + 1;
+#endif
+            phyid = tulip_mdio_read(dev, tp->mii.phy_id, MII_PHYSID1);
+            if ((phyid == 0) || (phyid == 0xFFFF))  /* Possible NCSI */
+            {
+                tp->phys[tp->phy_index] = INVALID_PHYID;
+                tp->mii.phy_id = INVALID_PHYID;
+            }
+            else
+            {
+                printk ("Front LAN Card is plugged in\n");
+            }
+        }
+        mod_timer(&tp->phy_timer, RUN_AT(3*HZ));
+        return;
+    }
+
+    interface = pilot_up (tp->dev, 0);
+    switch (interface)
+    {
+    case -2: // NO PHY, card removed.
+        if (tp->phy_linkdown != -1)
+        {
+            printk("%s: (PHY_link_monitor) LAN Card is "
+                    "removed for PHY Index %d\n",
+                    dev->name, (tp->phys[tp->phy_index] - 1));
+            if (tp->phy_linkdown == 0)
+            {
+                if (rtnl_trylock())
+                {
+                    netdev_phy_link_down(dev);
+                    rtnl_unlock();
+                }
+                else
+                {
+                    netdev_phy_link_down(dev);
+                }
+            }
+            tp->phy_linkdown = -1;
+        }
+        break;
+    case -1: /* No Link or Link Down */
+        if ((tp->phy_linkdown == 0) || (tp->phy_linkdown == -1))
+        {
+            tp->phy_linkdown = 1;
+            printk("%s: (PHY_link_monitor) Link is down for PHY Index %d\n",
+                    dev->name, (tp->phys[tp->phy_index] - 1));
+            if (rtnl_trylock())
+            {
+                netdev_phy_link_down(dev);
+                rtnl_unlock();
+            }
+            else
+            {
+                netdev_phy_link_down(dev);
+            }
+        }
+        break;
+    case 0:            /* Status Change or Link UP */
+        if ((tp->phy_linkdown == 1) || (tp->phy_linkdown == -1))
+        {
+            tp->phy_linkdown = 0;
+            printk("%s: (PHY_link_monitor) Link is Up for PHY Index %d\n",
+                    dev->name, (tp->phys[tp->phy_index] - 1));
+            if (rtnl_trylock())
+            {
+                netdev_phy_link_up(dev);
+                rtnl_unlock();
+            }
+            else
+            {
+                netdev_phy_link_up(dev);
+            }
+        }
+        break;
+    }
+
+    /* need to update CSR6 register for every speed and full-duplex settings*/
+    if ( ((tp->phys[tp->phy_index] - 1) != front_lan_phy_index) &&
+       ((tp->csr6 != org_csr6) ||
+       (tp->speed != org_speed) ||
+       (tp->full_duplex != org_full_duplex)) )
+    {
+       tp->csr6 = org_csr6;
+       tp->speed = org_speed;
+       tp->full_duplex = org_full_duplex;
+    }
+
+    /* Adjust the speed and duplex for the front lan to communicate.
+     * Check the front lan link
+     * Get the speed and duplex for front lan
+     * Check for the differences.
+     * */
+    if ( (tp_front != NULL) &&
+         ((tp->phys[tp->phy_index] - 1) != front_lan_phy_index) &&
+        (pilot_up (tp_front->dev, 0) == 0) &&
+         ((tp->csr6 != tp_front->csr6) ||
+         (tp->speed != tp_front->speed) ||
+        (tp->full_duplex != tp_front->full_duplex)) )
+    {
+        // Assign all the settings of front lan to rear lan.
+        tp->csr6 = tp_front->csr6;
+        tp->speed = tp_front->speed;
+        tp->full_duplex = tp_front->full_duplex;
+       printk("%s : Detected %dMbps %s Duplex\n",dev->name,tp->speed,(tp->full_duplex)?"Full":"Half");
+        tulip_restart_rxtx(tp);
+    }
+    else
+    {
+       if ((org_csr6 != tp->csr6) && ((tp->linkdown == 0) || (tp->linkdown == -1)))
+       {
+               printk("%s : Detected %dMbps %s Duplex\n",dev->name,tp->speed,(tp->full_duplex)?"Full":"Half");
+               tulip_restart_rxtx(tp);
+       }
+    }
+
+    if ( (tp_front != NULL) &&
+         ((tp->phys[tp->phy_index] - 1) != front_lan_phy_index) &&
+         (mii_link_ok (&tp->mii) == 0) )
+    {
+        netif_carrier_on(dev);
+    }
+
+    mod_timer(&tp->phy_timer, RUN_AT(3*HZ));
+}
+
+
+
 /*
  * This table use during operation for capabilities and media timer.
  *
@@ -319,12 +468,6 @@
 	tp->csr6 = ((0x1<<1) | (0x1<<30) | (0x1<<21) | (0x1 << 9));
 	tp->full_duplex = 1;
 
-	/* Check if we have valid PHY */
-	if (tp->mii.phy_id == INVALID_PHYID)	/* Possible NCSI */
-	{	
-		return -2;
-	}
-	
 	/* Confirm if we really have a MII PHY by reading Manufacturer ID  */
 #ifdef CONFIG_SOC_SE_PILOT3
     	phyid = tulip_mdio_read(dev, tp->mii.phy_id, MII_PHYSID1);
@@ -333,8 +476,6 @@
 #endif
 	if ((phyid == 0) || (phyid == 0xFFFF))	/* Possible NCSI */
 	{
-		tp->phys[tp->phy_index] = INVALID_PHYID;
-		tp->mii.phy_id = INVALID_PHYID;
 		return -2;
 	}
 
@@ -1343,7 +1484,7 @@
 	return 0;			
 }	
 
-static const struct ethtool_ops ops = {
+static struct ethtool_ops ops = {
 	.get_drvinfo = tulip_get_drvinfo,
 	.get_link	 = tulip_get_link,
 	.nway_reset	 = tulip_nway_reset,
@@ -1969,6 +2110,7 @@
 	tp->base_addr = ioaddr;
 //	tp->revision = chip_rev;
 //	tp->csr0 = csr0 ;
+	tp->linkdown = -1;
 	tp->csr0 = (0x20<<8);
 	spin_lock_init(&tp->lock);
 	spin_lock_init(&tp->mii_lock);
@@ -2381,6 +2523,25 @@
 #endif
 	/* put the chip in snooze mode until opened */
 	tulip_set_power_state (tp, 0, 1);
+        if ((ENABLE_FRONT_LAN) && (root_server_addr == __constant_htonl(INADDR_NONE))) //Check whether the kernel is booting with NFS. 
+        {
+               /* Get back-up of front lan */
+               if (((tp->phys[tp->phy_index] - 1) == front_lan_phy_index) ||
+                       (tp->phys[tp->phy_index] == INVALID_PHYID))
+               {
+                                       tp_front = tp;
+               }
+
+               // Initially make the link as down.
+               tp->phy_linkdown = -1;
+               /* Add timer to monitor all PHY for link status change */
+               init_timer(&tp->phy_timer);
+               tp->phy_timer.data = (unsigned long)tp;
+               tp->phy_timer.function = tulip_phy_link_monitor;
+              // Given enough delay for the first time, so that netmon driver will get loaded. 
+               tp->phy_timer.expires = RUN_AT(60*HZ);
+               add_timer(&tp->phy_timer);
+       }
 
 	return 0;
 
@@ -2482,7 +2643,8 @@
 		return;
 
 	tp = netdev_priv(dev);
-    ioaddr = tp->base_addr;
+	del_timer_sync (&tp->phy_timer);
+	ioaddr = tp->base_addr;
 	unregister_netdev(dev);
 //	pci_free_consistent (pdev,
 //			     sizeof (struct tulip_rx_desc) * RX_RING_SIZE +
