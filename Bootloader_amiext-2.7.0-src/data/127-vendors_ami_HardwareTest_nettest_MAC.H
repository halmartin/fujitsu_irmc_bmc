diff -Naur uboot.org/vendors/ami/HardwareTest/nettest/MAC.H uboot/vendors/ami/HardwareTest/nettest/MAC.H
--- uboot.org/vendors/ami/HardwareTest/nettest/MAC.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/nettest/MAC.H	2014-01-27 15:09:50.949237227 +0800
@@ -0,0 +1,140 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef MAC_H
+#define MAC_H
+
+#ifdef SPI_BUS
+  #include <stdio.h>
+  #include <stdlib.h>
+  #include <time.h>
+  #define SPI_CS            1
+#endif
+// ( USE_P2A | USE_LPC )
+
+#if defined(LinuxAP)
+  #include <stdio.h>
+  #include <stdlib.h>
+  #include <string.h>
+  #include <stdarg.h>
+  #include <unistd.h>
+  #include <string.h>
+  #include <fcntl.h>
+  #include <pthread.h>
+  #include <sys/mman.h>
+  #include <sys/io.h>
+#endif
+
+#include <common.h>
+#include <command.h>
+
+#include "NCSI.H"
+#include "IO.H"
+
+// --------------------------------------------------------------
+// Define                                                        
+// --------------------------------------------------------------
+
+//#define Force_Enable_MAC34             //[ON][SLT:off] (Force enable mac34)
+//#define Force_Enable_NewMDIO           //[off][SLT:off] (Force enable new MDC/MDIO)
+//#define Enable_Fast_SCU                //[off]
+//#define Enable_Old_Style               //[off]
+#define ENABLE_DASA                    //[ON]
+//#define Enable_AST2300_Int125MHz       //[off]
+//#define ENABLE_ARP_2_WOL               //[off]
+//#define Enable_MAC_SWRst               //[off]
+
+#define Enable_Runt
+//#define Enable_Jumbo
+//#define Enable_BufMerge
+//#define Disable_VGA
+
+//#define SelectSimpleBoundary                        //[off] Using in debug
+//#define SelectSimpleData                            //[off] Using in debug
+//#define SelectSimpleLength         1512             //[off] 60(0x3c) ~ 1514(0x5ea); 1512(0x5e8)
+//#define SelectDesNumber            8                //[off] 1 ~
+//#define SelectSimpleDA                              //[off] Using in debug
+//#define SelectSimpleDes                             //[off]
+//#define SelectLengthInc                             //[off] Using in debug
+
+#define   SimpleData_Fix                                                //[ON] Using in debug
+#define   SimpleData_FixNum          12
+#define   SimpleData_FixVal00        0x00000000     //[0]no SelectSimpleDA: (60: 0412 8908)(1512: e20d e9da)
+#define   SimpleData_FixVal01        0xffffffff     //[0]no SelectSimpleDA: (60: f48c f14d)(1512: af05 260c)
+#define   SimpleData_FixVal02        0x55555555     //[0]no SelectSimpleDA: (60: 5467 5ecb)(1512: d90a 5368)
+#define   SimpleData_FixVal03        0xaaaaaaaa     //[0]no SelectSimpleDA: (60: a4f9 268e)(1512: 9402 9cbe)
+#define   SimpleData_FixVal04        0x5a5a5a5a     //[1]no SelectSimpleDA: (60: 7f01 e22d)(1512: 4fd3 8012)
+#define   SimpleData_FixVal05        0xc3c3c3c3     //[1]no SelectSimpleDA: (60: 5916 02d5)(1512: 99f1 6127)
+#define   SimpleData_FixVal06        0x96969696     //[1]no SelectSimpleDA: (60: 0963 d516)(1512: a2f6 db95)
+#define   SimpleData_FixVal07        0xf0f0f0f0     //[1]no SelectSimpleDA: (60: dfea 4dab)(1512: 39dc f576)
+#define   SimpleData_FixVal08        0x5555aaaa     //[2]no SelectSimpleDA: (60: b61b 5777)(1512: 4652 ddb0)
+#define   SimpleData_FixVal09        0xffff0000     //[2]no SelectSimpleDA: (60: 16f0 f8f1)(1512: 305d a8d4)
+#define   SimpleData_FixVal10        0x5a5aa5a5     //[2]no SelectSimpleDA: (60: 9d7d eb91)(1512: d08b 0eca)
+#define   SimpleData_FixVal11        0xc3c33c3c     //[2]no SelectSimpleDA: (60: bb6a 0b69)(1512: 06a9 efff)
+
+#define   SelectSimpleDA_Dat0        0x67052301
+#define   SelectSimpleDA_Dat1        0xe0cda089
+#define   SelectSimpleDA_Dat2        0x98badcfe
+
+#define   SelectWOLDA_DatH           0x206a
+#define   SelectWOLDA_DatL           0x8a374d9b
+
+#define   MOVE_DATA_MB_SEC           800            // MByte per second to move data
+
+//---------------------------------------------------------
+// Frame size
+//---------------------------------------------------------
+#define  ENABLE_RAND_SIZE             0
+#define  Rand_Sed                     0xffccd
+#define  FRAME_Rand_Simple            0
+#define  MIN_FRAME_RAND_SIZE               60
+#define  MAX_FRAME_RAND_SIZE               1514
+
+#define FRAME_SELH_PERD               7
+#ifdef Enable_Jumbo
+//    #define FRAME_LENH                9212           //max:9212
+//    #define FRAME_LENL                9211           //max:9212
+    #define FRAME_LENH                9212           //max:9212
+    #define FRAME_LENL                9212           //max:9212
+//    #define FRAME_LENH                8120
+//    #define FRAME_LENL                8119
+//    #define FRAME_LENH                7000
+//    #define FRAME_LENL                6999
+//    #define FRAME_LENH                4095
+//    #define FRAME_LENL                4094
+//    #define FRAME_LENH                2040
+//    #define FRAME_LENL                2039
+#else
+  #ifdef SelectSimpleLength
+//    #define FRAME_LENH                  ( SelectSimpleLength + 1 )
+//    #define FRAME_LENL                  ( SelectSimpleLength     )
+    #define FRAME_LENH                  SelectSimpleLength
+    #define FRAME_LENL                  SelectSimpleLength
+  #else
+//    #define FRAME_LENH                  1514           //max:1514
+//    #define FRAME_LENL                  1513           //max:1514
+    #define FRAME_LENH                  1514           //max:1514
+    #define FRAME_LENL                  1514           //max:1514
+  #endif
+#endif
+
+const  ULONG   ARP_org_data[16] = {
+        0xffffffff,
+        0x0000ffff, // SA:00 00
+        0x12345678, // SA:12 34 56 78
+        0x01000608, // ARP(0x0806)
+        0x04060008,    
+        0x00000100, // sender MAC Address: 00 00
+        0x12345678, // sender MAC Address: 12 34 56 78
+        0xeb00a8c0, // sender IP Address:  192.168.0.235
+        0x00000000, // target MAC Address: 00 00 00 00
+        0xa8c00000, // target MAC Address: 00 00, sender IP Address:192.168
+        0x00000100, // sender IP Address:  0.1
+//      0x0000de00, // sender IP Address:  0.222
+        0x00000000,
+        0x00000000,
+        0x00000000,
+        0x00000000,
+        0xc68e2bd5
+};
+
+#endif // MAC_H
+#endif
