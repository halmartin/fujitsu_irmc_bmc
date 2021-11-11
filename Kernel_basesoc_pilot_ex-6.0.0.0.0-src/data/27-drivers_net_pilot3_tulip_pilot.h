--- linux-3.14.17/drivers/net/ethernet/pilot_tulip/pilot.h	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/drivers/net/ethernet/pilot_tulip/pilot.h	2014-09-04 12:04:18.369822943 +0530
@@ -0,0 +1,17 @@
+#define INTERFACE_ETH_A  0
+#define INTERFACE_ETH_B  1
+
+#ifdef CONFIG_SOC_SE_PILOT3
+#define SE_INTERFACE_ETH_A_IRQ 0
+#define SE_INTERFACE_ETH_B_IRQ 1
+#else
+#define SE_INTERFACE_ETH_A_IRQ 32
+#define SE_INTERFACE_ETH_B_IRQ 33
+#endif
+
+#define IOADDR_INTERFACE_ETH_A (IO_ADDRESS(0x40500000))
+#define IOADDR_INTERFACE_ETH_B (IO_ADDRESS(0x40600000))
+
+#define PILOT3_TAP_SETTING (0x4<<17)
+#define INVALID_PHYID		0x7F	/* Signed char */
+
