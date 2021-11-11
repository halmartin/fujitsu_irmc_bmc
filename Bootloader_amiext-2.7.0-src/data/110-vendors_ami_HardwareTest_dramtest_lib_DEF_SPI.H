diff -Naur uboot.org/vendors/ami/HardwareTest/lib/DEF_SPI.H uboot/vendors/ami/HardwareTest/lib/DEF_SPI.H
--- uboot.org/vendors/ami/HardwareTest/lib/DEF_SPI.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/DEF_SPI.H	2014-01-27 15:09:50.969236976 +0800
@@ -0,0 +1,27 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef DEF_SPI_H
+#define DEF_SPI_H
+
+#include "TYPEDEF.H"
+#include "SWFUNC.H"
+
+typedef struct _DEVICE_PCI_INFO
+{
+    USHORT    usVendorID;
+    USHORT    usDeviceID;
+    ULONG     ulPCIConfigurationBaseAddress;
+    ULONG     ulPhysicalBaseAddress;
+    ULONG     ulMMIOBaseAddress;
+    USHORT    usRelocateIO;
+} DEVICE_PCI_INFO;
+
+//VIDEO Engine Info
+typedef struct _VIDEO_ENGINE_INFO {
+    USHORT             iEngVersion;
+    DEVICE_PCI_INFO    VGAPCIInfo;
+} VIDEO_ENGINE_INFO;
+
+BOOLEAN  GetDevicePCIInfo (VIDEO_ENGINE_INFO *VideoEngineInfo);
+
+#endif // DEF_SPI_H
+#endif
