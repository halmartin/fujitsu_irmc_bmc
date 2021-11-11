diff -Naur uboot.org/vendors/ami/HardwareTest/lib/SPIM.c uboot/vendors/ami/HardwareTest/lib/SPIM.c
--- uboot.org/vendors/ami/HardwareTest/lib/SPIM.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/SPIM.c	2014-01-27 15:09:50.973236926 +0800
@@ -0,0 +1,55 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#define SPIM_C
+static const char ThisFile[] = "SPIM.c";
+
+#include "SWFUNC.H"
+
+#ifdef SPI_BUS
+
+#include <stdio.h>
+#include <stdlib.h>
+#include <conio.h>
+#include <string.h>
+#include "TYPEDEF.H"
+#include "LIB_SPI.H"
+
+#define SPIM_CMD_WHA     0x01
+#define SPIM_CMD_RD      0x0B
+#define SPIM_CMD_DRD     0xBB
+#define SPIM_CMD_WR      0x02
+#define SPIM_CMD_DWR     0xD2
+#define SPIM_CMD_STA     0x05
+#define SPIM_CMD_ENBYTE  0x06
+#define SPIM_CMD_DISBYTE 0x04
+
+ULONG spim_cs;
+ULONG spim_base;
+ULONG spim_hadr;
+
+void spim_end()
+{
+  ULONG data;
+
+  data = MIndwm((ULONG)mmiobase, 0x1E620010 + (spim_cs << 2));
+  MOutdwm( (ULONG)mmiobase, 0x1E620010 + (spim_cs << 2), data | 0x4);
+  MOutdwm( (ULONG)mmiobase, 0x1E620010 + (spim_cs << 2), data);
+}
+
+//------------------------------------------------------------
+void spim_init(int cs)
+{
+  ULONG data;
+
+  spim_cs = cs;
+  MOutdwm( (ULONG)mmiobase, 0x1E620000, (0x2 << (cs << 1)) | (0x10000 << cs));
+  MOutdwm( (ULONG)mmiobase, 0x1E620010 + (cs << 2), 0x00000007);
+  MOutdwm( (ULONG)mmiobase, 0x1E620010 + (cs << 2), 0x00002003);
+  MOutdwm( (ULONG)mmiobase, 0x1E620004, 0x100 << cs);
+  data = MIndwm((ULONG)mmiobase, 0x1E620030 + (cs << 2));
+  spim_base = 0x20000000 | ((data & 0x007f0000) << 7);
+  MOutwm ( (ULONG)mmiobase, spim_base, SPIM_CMD_WHA);
+  spim_end();
+  spim_hadr = 0;
+}
+#endif // End SPI_BUS
+#endif
