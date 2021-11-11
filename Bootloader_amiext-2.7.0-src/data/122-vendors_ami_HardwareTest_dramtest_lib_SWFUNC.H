diff -Naur uboot.org/vendors/ami/HardwareTest/lib/SWFUNC.H uboot/vendors/ami/HardwareTest/lib/SWFUNC.H
--- uboot.org/vendors/ami/HardwareTest/lib/SWFUNC.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/SWFUNC.H	2014-01-27 15:09:50.969236976 +0800
@@ -0,0 +1,93 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef SWFUNC_H
+#define SWFUNC_H
+
+
+//---------------------------------------------------------
+// Program information
+//---------------------------------------------------------
+#define    VER_NAME    "Ver 0.25 version @2013/08/13 0824"
+
+/* == Step 1: ====== Support OS system =================== */
+// LinuxAP
+// #define Windows
+#define SLT_UBOOT
+//#define SLT_DOS
+
+/* == Step 2:======== Support interface ================== */
+/* Choose One */
+//#define SPI_BUS
+//#define USE_LPC
+//#define USE_P2A     // PCI or PCIe bus to AHB bus
+
+/* == Step 3:==========  Support Chip   ================== */
+//#define AST1010_CHIP
+//#define AST3200_IOMAP
+//#define FPGA 
+
+/* ====================== Program ======================== */
+// The "PHY_NCSI" option is only for DOS compiler
+#if defined (PHY_NCSI)
+    #ifdef SLT_UBOOT
+        #error Wrong setting......
+    #endif
+#endif
+
+/* =================  Check setting  ===================== */
+#ifdef USE_P2A
+    #ifdef SLT_UBOOT
+        #error Can NOT be set PCI bus in Uboot
+    #endif
+#endif
+#ifdef USE_LPC
+    #ifdef SLT_UBOOT
+        #error Can NOT be set LPC bus in Uboot
+    #endif
+#endif
+#ifdef SPI_BUS
+    #ifdef SLT_UBOOT
+        #error Can NOT be set SPI bus in Uboot 
+    #endif
+#endif
+
+/* ======================== Program flow control ======================== */
+#define RUN_STEP                     5
+// 0: read_scu
+// 1: parameter setup
+// 2: init_scu1,
+// 3: init_scu_macrst
+// 4: Data Initial
+// 5: ALL
+
+/* ====================== Switch print debug message ====================== */
+#define   DbgPrn_Enable_Debug_delay  0
+//#define   DbgPrn_FuncHeader          0 //1
+#define   DbgPrn_ErrFlg              0
+#define   DbgPrn_BufAdr              0 //1
+#define   DbgPrn_Bufdat              0
+#define   DbgPrn_BufdatDetail        0
+#define   DbgPrn_PHYRW               0
+#define   DbgPrn_PHYInit             0
+#define   DbgPrn_PHYName             0
+#define   DbgPrn_DumpMACCnt          0
+#define   DbgPrn_Info                0 //1
+#define   DbgPrn_FRAME_LEN           0
+
+
+/* ============ Enable or Disable Check item of the descriptor ============ */
+#define   CheckRxOwn
+#define   CheckRxErr
+//#define   CheckOddNibble
+#define   CheckCRC
+#define   CheckRxFIFOFull
+#define   CheckRxLen
+//#define   CheckDataEveryTime
+
+//#define   CheckRxbufUNAVA
+#define   CheckRPktLost
+//#define   CheckNPTxbufUNAVA
+#define   CheckTPktLost
+#define   CheckRxBuf
+
+#endif // SWFUNC_H
+#endif
