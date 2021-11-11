diff -Naur uboot.org/vendors/ami/HardwareTest/lib/DRAM_SPI.c uboot/vendors/ami/HardwareTest/lib/DRAM_SPI.c
--- uboot.org/vendors/ami/HardwareTest/lib/DRAM_SPI.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/DRAM_SPI.c	2014-01-27 15:09:50.969236976 +0800
@@ -0,0 +1,70 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#define DRAM_SPI_C
+static const char ThisFile[] = "DRAM_SPI.c";
+
+#include "SWFUNC.H"
+
+#ifdef SPI_BUS
+#include <stdio.h>
+#include "DEF_SPI.H"
+#include "LIB_SPI.H"
+
+VOID Set_MMIO_Base(ULONG PCI_BASE, ULONG addr)
+{
+  static ULONG MMIO_BASE = -1;
+
+  if(MMIO_BASE != (addr & 0xffff0000)){
+    if(MMIO_BASE == -1){
+      *(ULONG *)(PCI_BASE + 0xF000) = 1;
+    }
+    *(ULONG *)(PCI_BASE + 0xF004) = addr;
+    MMIO_BASE = addr & 0xffff0000;
+  }
+}
+
+VOID  MOutbm(ULONG PCI_BASE, ULONG Offset, BYTE Data)
+{
+  Set_MMIO_Base(PCI_BASE, Offset);
+  *(BYTE *)(PCI_BASE + 0x10000 + (Offset & 0xffff)) = Data;
+}
+
+VOID  MOutwm(ULONG PCI_BASE, ULONG Offset, USHORT Data)
+{
+  Set_MMIO_Base(PCI_BASE, Offset);
+  *(USHORT *)(PCI_BASE + 0x10000 + (Offset & 0xffff)) = Data;
+}
+
+VOID  MOutdwm(ULONG PCI_BASE, ULONG Offset, ULONG Data)
+{
+  Set_MMIO_Base(PCI_BASE, Offset);
+  *(ULONG *)(PCI_BASE + 0x10000 + (Offset & 0xffff)) = Data;
+}
+
+BYTE  MInbm(ULONG PCI_BASE, ULONG Offset)
+{
+  BYTE jData;
+
+  Set_MMIO_Base(PCI_BASE, Offset);
+  jData = *(BYTE *)(PCI_BASE + 0x10000 + (Offset & 0xffff));
+  return(jData);
+}
+
+USHORT  MInwm(ULONG PCI_BASE, ULONG Offset)
+{
+  USHORT usData;
+
+  Set_MMIO_Base(PCI_BASE, Offset);
+  usData = *(USHORT *)(PCI_BASE + 0x10000 + (Offset & 0xffff));
+  return(usData);
+}
+
+ULONG  MIndwm(ULONG PCI_BASE, ULONG Offset)
+{
+  ULONG ulData;
+
+  Set_MMIO_Base(PCI_BASE, Offset);
+  ulData = *(ULONG *)(PCI_BASE + 0x10000 + (Offset & 0xffff));
+  return(ulData);
+}
+#endif // End SPI_BUS
+#endif
