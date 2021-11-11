diff -Naur uboot.org/vendors/ami/HardwareTest/lib/LIB_SPI.H uboot/vendors/ami/HardwareTest/lib/LIB_SPI.H
--- uboot.org/vendors/ami/HardwareTest/lib/LIB_SPI.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/LIB_SPI.H	2014-01-27 15:09:50.969236976 +0800
@@ -0,0 +1,15 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef LIB_SPI_H
+#define LIB_SPI_H
+
+#ifdef SPI_BUS
+    // MMIO Functions
+    VOID       MOutwm (ULONG, ULONG, USHORT);
+    VOID       MOutdwm (ULONG, ULONG, ULONG);
+    ULONG      MIndwm (ULONG, ULONG);
+    
+    void spim_init(int cs);
+#endif
+
+#endif // LIB_SPI_H
+#endif
