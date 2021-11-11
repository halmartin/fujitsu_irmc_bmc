diff -Naur uboot.org/vendors/ami/HardwareTest/dramtest/PLLTESTU.H uboot/vendors/ami/HardwareTest/dramtest/PLLTESTU.H
--- uboot.org/vendors/ami/HardwareTest/dramtest/PLLTESTU.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/dramtest/PLLTESTU.H	2014-01-27 15:09:50.957237126 +0800
@@ -0,0 +1,42 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef PLLTESTU_H
+#define PLLTESTU_H
+
+//PLL Mode Definition
+#define	NAND_PLLMODE		    0x00
+#define	DELAY_PLLMODE		    0x04
+#define	PCI_PLLMODE	            0x08
+#define	DPLL_PLLMODE		    0x2c
+#define	MPLL_PLLMODE		    0x10
+#define	HPLL_PLLMODE		    0x14
+#define	LPC_PLLMODE	            0x18
+#define	VIDEOA_PLLMODE		    0x1c
+#define	D2PLL_PLLMODE		    0x0c
+#define	VIDEOB_PLLMODE		    0x3c
+
+#define	PCI_PLLMODE_AST1160     0x10
+#define	MPLL_PLLMODE_AST1160	0x14
+#define	HPLL_PLLMODE_AST1160	0x14
+#define	DPLL_PLLMODE_AST1160	0x1c
+
+#define	PCI_PLLMODE_AST2300     0x2c
+#define	MPLL_PLLMODE_AST2300	0x10
+#define	HPLL_PLLMODE_AST2300	0x30
+#define	DPLL_PLLMODE_AST2300	0x08
+#define	DEL0_PLLMODE_AST2300	0x00
+
+#define ERR_FATAL		0x00000001
+
+typedef struct _VGAINFO {
+    USHORT usDeviceID;
+    UCHAR  jRevision;           
+    
+    ULONG  ulMCLK;
+    ULONG  ulDRAMBusWidth;    
+    
+    ULONG  ulCPUCLK;
+    ULONG  ulAHBCLK;    
+} _VGAInfo;
+
+#endif // End PLLTESTU_H
+#endif
