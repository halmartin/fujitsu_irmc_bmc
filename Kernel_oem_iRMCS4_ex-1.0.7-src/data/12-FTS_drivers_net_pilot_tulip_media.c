--- linux_orig/drivers//net/ethernet/pilot_tulip/media.c	2016-09-19 17:50:51.107189986 +0530
+++ linux/drivers//net/ethernet/pilot_tulip/media.c	2016-09-19 18:14:47.800132488 +0530
@@ -41,6 +41,8 @@
                 0xB4, 0xB8, 0xBC, 0xC0,  0xC4, 0xC8, 0xCC, 0,  0,0,0,0,  0,0,0,0,
                 0,0xD0,0,0,  0,0,0,0,  0,0,0,0, 0, 0xD4, 0xD8, 0xDC, };
 
+        int g_phyreg0 = 0;
+
 	#ifdef CONFIG_SPX_FEATURE_GLOBAL_SHARED_MDIO_MDC_LINES
 	extern spinlock_t miilock;
 	#endif
@@ -471,10 +473,10 @@
         switch (retval)
         {
                 case -2:                /* No PHY */
-                        tp->linkdown = 0;
+                        tp->linkdown = -1;
                         break;
                 case -1:                /* No Link */
-                        if (!tp->linkdown)
+                        if ((tp->linkdown == 0) || (tp->linkdown == -1))
                         {
                                 tp->linkdown = 1;       
                                 LinkChange=1;
@@ -491,7 +493,7 @@
                         }
                         break;
                 default:                /* Status Change or Link UP */
-                        if (tp->linkdown)
+                        if ((tp->linkdown == 1) || (tp->linkdown == -1))
                         {
                                 tp->linkdown = 0;
                                 LinkChange=1;
@@ -512,7 +514,7 @@
 
         if ((org_csr6 != tp->csr6) || (LinkChange))
         {
-                if (tp->linkdown == 0)
+                if ((tp->linkdown == 0) || (tp->linkdown == -1))
                 {
                         if (retval == -2)
                                 printk("%s : Detected Possibe NC-SI Interface\n",dev->name);
@@ -525,6 +527,13 @@
         return 0;
 }
 
+static int __init Read_PhyReg0(char *str)
+{
+       get_option(&str, &g_phyreg0);
+       return 1;
+}
+__setup("phyreg0=", Read_PhyReg0);
+
 void tulip_init_mii (struct net_device *dev, int board_idx, int phy_idx, int phy)
 {
         struct tulip_private *tp = netdev_priv(dev);
@@ -598,30 +607,45 @@
                 tulip_mdio_write (dev, phy, 4, to_advert);
         }
 
-        /* Enable autonegotiation: some boards default to off. */
-        if (tp->default_port == 0) {
-                new_bmcr = mii_reg0 | BMCR_ANENABLE;
-                if (new_bmcr != mii_reg0) {
-                        new_bmcr |= BMCR_ANRESTART;
-                        ane_switch = 1;
-                }
-        }
-        /* ...or disable nway, if forcing media */
-        else {
-                new_bmcr = mii_reg0 & ~BMCR_ANENABLE;
+        if((g_phyreg0 != 0))
+        {
+                /* setting phy_reg0 */
+                printk("Setting phy reg0 value from bootargs----\n");
+                new_bmcr = g_phyreg0;
+
                 if (new_bmcr != mii_reg0)
+                {
                         ane_switch = 1;
+                }
         }
+        else
+        {
 
-        /* clear out bits we never want at this point */
-        new_bmcr &= ~(BMCR_CTST | BMCR_FULLDPLX | BMCR_ISOLATE |
-                      BMCR_PDOWN | BMCR_SPEED100 | BMCR_LOOPBACK |
-                      BMCR_RESET);
-
-        if (tp->full_duplex)
-                new_bmcr |= BMCR_FULLDPLX;
-        if (tulip_media_cap[tp->default_port] & MediaIs100)
-                new_bmcr |= BMCR_SPEED100;
+                /* Enable autonegotiation: some boards default to off. */
+                if (tp->default_port == 0) {
+                        new_bmcr = mii_reg0 | BMCR_ANENABLE;
+                        if (new_bmcr != mii_reg0) {
+                                new_bmcr |= BMCR_ANRESTART;
+                                ane_switch = 1;
+                        }
+                }
+                /* ...or disable nway, if forcing media */
+                else {
+                        new_bmcr = mii_reg0 & ~BMCR_ANENABLE;
+                        if (new_bmcr != mii_reg0)
+                                ane_switch = 1;
+                }
+
+                /* clear out bits we never want at this point */
+                new_bmcr &= ~(BMCR_CTST | BMCR_FULLDPLX | BMCR_ISOLATE |
+                              BMCR_PDOWN | BMCR_SPEED100 | BMCR_LOOPBACK |
+                              BMCR_RESET);
+
+                if (tp->full_duplex)
+                        new_bmcr |= BMCR_FULLDPLX;
+                if (tulip_media_cap[tp->default_port] & MediaIs100)
+                        new_bmcr |= BMCR_SPEED100;
+        }
 
         if (new_bmcr != mii_reg0) {
                 /* some phys need the ANE switch to
@@ -646,7 +670,7 @@
         /* Find the connected MII xcvrs.
            Doing this in open() would allow detecting external xcvrs later,
            but takes much time. */
-        for (phyn = 1; phyn <= 32 && phy_idx < sizeof (tp->phys); phyn++) {
+        for (phyn = 1; phyn <= 32 && phy_idx < ARRAY_SIZE(tp->phys); phyn++) {
                 int phy = phyn & 0x1f;
                 int mii_status = tulip_mdio_read (dev, phy, MII_BMSR);
                 if ((mii_status & 0x8301) == 0x8001 ||
