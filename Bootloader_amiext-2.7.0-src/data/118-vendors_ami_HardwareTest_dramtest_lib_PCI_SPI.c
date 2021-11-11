diff -Naur uboot.org/vendors/ami/HardwareTest/lib/PCI_SPI.c uboot/vendors/ami/HardwareTest/lib/PCI_SPI.c
--- uboot.org/vendors/ami/HardwareTest/lib/PCI_SPI.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/PCI_SPI.c	2014-01-27 15:09:50.961237076 +0800
@@ -0,0 +1,64 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#define PCI_SPI_C
+static const char ThisFile[] = "PCI_SPI.c";
+
+#include "SWFUNC.H"
+#include <common.h>
+#include <command.h>
+#include "DEF_SPI.H"
+#include "LIB.H"
+#include "TYPEDEF.H"
+
+#ifdef SPI_BUS
+ULONG  GetPCIInfo (DEVICE_PCI_INFO  *VGAPCIInfo)
+{
+    ULONG ulPCIBaseAddress, MMIOBaseAddress, LinearAddressBase, busnum, data;
+
+    ulPCIBaseAddress = FindPCIDevice (0x1A03, 0x2000, ACTIVE);
+    busnum = 0;
+    while (ulPCIBaseAddress == 0 && busnum < 256) {
+        ulPCIBaseAddress = FindPCIDevice (0x1A03, 0x2000, busnum);
+        if (ulPCIBaseAddress == 0) {
+            ulPCIBaseAddress = FindPCIDevice (0x1688, 0x2000, busnum);
+        }
+        if (ulPCIBaseAddress == 0) {
+            ulPCIBaseAddress = FindPCIDevice (0x1A03, 0x1160, busnum);
+        }
+        if (ulPCIBaseAddress == 0) {
+            ulPCIBaseAddress = FindPCIDevice (0x1A03, 0x1180, busnum);
+        }
+        busnum++;
+    }
+    printf ("ulPCIBaseAddress = %lx\n", ulPCIBaseAddress);
+    if (ulPCIBaseAddress != 0) {
+        VGAPCIInfo->ulPCIConfigurationBaseAddress = ulPCIBaseAddress;
+        VGAPCIInfo->usVendorID = ReadPCIReg(ulPCIBaseAddress, 0, 0xFFFF);
+        VGAPCIInfo->usDeviceID = ReadPCIReg(ulPCIBaseAddress, 0, 0xFFFF0000) >> 16;
+        LinearAddressBase = ReadPCIReg (ulPCIBaseAddress, 0x10, 0xFFFFFFF0);
+        VGAPCIInfo->ulPhysicalBaseAddress = MapPhysicalToLinear (LinearAddressBase, 64 * 1024 * 1024 + 0x200000);
+        MMIOBaseAddress = ReadPCIReg (ulPCIBaseAddress, 0x14, 0xFFFF0000);
+        VGAPCIInfo->ulMMIOBaseAddress = MapPhysicalToLinear (MMIOBaseAddress, 64 * 1024 * 1024);
+        VGAPCIInfo->usRelocateIO = ReadPCIReg (ulPCIBaseAddress, 0x18, 0x0000FF80);
+        OUTDWPORT(0xcf8, ulPCIBaseAddress + 0x4);
+        data = INDWPORT(0xcfc);
+        OUTDWPORT(0xcfc, data | 0x3);
+        return    TRUE;
+    }
+    else {
+        return    FALSE;
+    }
+} // End ULONG  GetPCIInfo (DEVICE_PCI_INFO  *VGAPCIInfo)
+
+BOOLEAN  GetDevicePCIInfo (VIDEO_ENGINE_INFO *VideoEngineInfo)
+{
+    if (GetPCIInfo (&VideoEngineInfo->VGAPCIInfo) == TRUE) {
+        return    TRUE;
+    }
+    else {
+        printf("Can not find PCI device!\n");
+        exit(0);
+        return    FALSE;
+    }
+} // End
+#endif // End ifdef SPI_BUS
+#endif
