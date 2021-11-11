diff -Naur uboot.org/vendors/ami/HardwareTest/lib/LIB.H uboot/vendors/ami/HardwareTest/lib/LIB.H
--- uboot.org/vendors/ami/HardwareTest/lib/LIB.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/LIB.H	2014-01-27 15:09:50.969236976 +0800
@@ -0,0 +1,29 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef LIB_H
+#define LIB_H
+
+#include "TYPEDEF.H"
+
+//
+// Macro
+//
+#define   INTFUNC              int386
+
+#define   OUTDWPORT         	outpd
+#define   INDWPORT          	inpd
+#define   OUTPUT            	outp
+#define   INPUT             	inp
+
+//
+// PCI
+//
+ULONG ReadPCIReg (ULONG ulPCIConfigAddress, BYTE jOffest, ULONG ulMask);
+ULONG FindPCIDevice (USHORT usVendorID, USHORT usDeviceID, USHORT usBusType);
+VOID  WritePCIReg (ULONG ulPCIConfigAddress, BYTE jOffest, ULONG ulMask, ULONG ulData);
+
+//
+// Map Resource
+//
+ULONG    MapPhysicalToLinear (ULONG ulBaseAddress, ULONG ulSize);
+#endif
+#endif
