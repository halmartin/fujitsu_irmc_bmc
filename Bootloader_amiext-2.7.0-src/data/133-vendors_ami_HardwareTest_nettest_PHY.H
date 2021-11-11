diff -Naur uboot.org/vendors/ami/HardwareTest/nettest/PHY.H uboot/vendors/ami/HardwareTest/nettest/PHY.H
--- uboot.org/vendors/ami/HardwareTest/nettest/PHY.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/nettest/PHY.H	2014-01-27 15:09:50.949237227 +0800
@@ -0,0 +1,48 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef PHY_H
+#define PHY_H
+
+//
+// Define
+//
+#define Enable_SearchPHYID             //[ON] (Search vlid PHY ID)
+#define Enable_CheckZeroPHYID          //[ON] (Check PHY ID with value 0)
+
+#ifdef Enable_CheckZeroPHYID
+  #define PHY_IS_VALID( dat )      ( ( (dat & 0xffff) != 0xffff ) && ( ( dat & 0xffff ) != 0x0 ) )
+#else
+  #define PHY_IS_VALID( dat )      ( ( dat & 0xffff) != 0xffff )
+#endif
+
+// Define PHY basic register
+#define PHY_REG_BMCR    0x00 // Basic Mode Control Register
+#define PHY_REG_BMSR    0x01 // Basic Mode Status Register
+#define PHY_REG_ID_1    0x02
+#define PHY_REG_ID_2    0x03
+#define PHY_ANER        0x06 // Auto-negotiation Expansion Register
+#define PHY_GBCR        0x09 // 1000Base-T Control Register
+#define PHY_SR          0x11 // PHY Specific Status Register
+#define PHY_INER        0x12 // Interrupt Enable Register
+
+#define TIME_OUT_PHY_RW           10000
+#define TIME_OUT_PHY_Rst          10000
+
+#define PHYID3_Mask                0xfc00         //0xffc0
+
+/* --- Note for SettingPHY chip ---
+void phy_xxxx (int loop_phy) {
+
+	if ( BurstEnable ) {
+        // IEEE test
+	} 
+	else if (loop_phy) {
+        // Internal loop back
+	} 
+	else {
+        // external loop back
+	}
+}
+----------------------------------- */
+
+#endif // PHY_H
+#endif
