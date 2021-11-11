--- u-boot-1.1.6/include/pilotiii/ddr_misc.h	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/include/pilotiii/ddr_misc.h	2010-04-05 16:47:25.000000000 -0400
@@ -0,0 +1,66 @@
+//////////////////////////////////////////////////
+//#ServerEngines Public License
+//#This license governs use of the accompanying software. If you use the software, you
+//#accept this license. If you do not accept the license, do not use the software.
+//#
+//#1. Definitions
+//#The terms "reproduce," "reproduction," "derivative works," and "distribution" have the
+//#same meaning here as under U.S. copyright law.
+//#A "contribution" is the original software, or any additions or changes to the software.
+//#A "contributor" is any person that distributes its contribution under this license.
+//#"Licensed patents" are a contributor's patent claims that read directly on its contribution.
+//#
+//##2. Grant of Rights
+//###(A) Copyright Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free copyright license to reproduce its contribution, prepare derivative works of its contribution, and distribute its contribution or any derivative works that you create.
+//##(B) Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the contribution in the software.
+//##
+//##3. Conditions and Limitations
+//##(A) No Trademark License- This license does not grant you rights to use any contributors' name, logo, or trademarks.
+//##(B) If you bring a patent claim against any contributor over patents that you claim are infringed by the software, your patent license from such contributor to the software ends automatically.
+//#(C) If you distribute any portion of the software, you must retain all copyright, patent, trademark, and attribution notices that are present in the software.
+//#(D) If you distribute any portion of the software in source code form, you may do so only under this license by including a complete copy of this license with your distribution. If you distribute any portion of the software in compiled or object code form, you may only do so under a license that complies with this license.
+//#(E) The software is licensed "as-is." You bear the risk of using it. The contributors give no express warranties, guarantees or conditions. You may have additional consumer rights under your local laws which this license cannot change. To the extent permitted under your local laws, the contributors exclude the implied warranties of merchantability, fitness for a particular purpose and non-infringement.
+//##
+//###       Project  : Pilot
+
+
+
+//#define DDRINTFLAG           0x807ffff8
+#define DDRINTFLAG           0x807ff100
+#define SYSCONTROL                      0x40100100
+#define SYS_MCSS_CLK_CTRL       (SYSCONTROL + 0x84)
+//LDMA Defines
+// Descriptor defines
+#define LDMA_INTERRUPT  (1<<0) // Generates interrupt for current descriptor
+#define LDMA_NOTLAST    (1<<1) // used for chaining. means current descriptor is not last
+#define LDMA_SRCDDR     (0<<2) // source of data is in DDR
+#define LDMA_SRCLMEM    (1<<2) // source of data is in LMEM
+#define LDMA_DSTDDR     (0<<3) // Destination for data is in DDR
+#define LDMA_DSTLMEM    (1<<3) // Destination for data is in LMEM
+#define LDMA_CHKSUMRST  (1<<5) // Resets the checksum in hardware b4 processing current descriptor
+#define LDMA_CHKSUMONLY (1<<6) // current descriptor to do checksumonly and no data transfer
+
+#define READ_REG8(a)      (*((tPVU8)  (a)))
+#define READ_REG16(a)     (*((tPVU16) (a)))
+#define READ_REG32(a)     (*((tPVU32) (a)))
+ 
+#define WRITE_REG8(a,d)   (*((tPVU8) (a)) = d)
+#define WRITE_REG16(a,d)  (*((tPVU16) (a)) = d)
+#define WRITE_REG32(a,d)  (*((tPVU32) (a)) = d)
+ 
+#define SET_BIT8(a,b)     (WRITE_REG8 (a, (READ_REG8(a)  | (tU8) (1<<b))))
+#define SET_BIT16(a,b)    (WRITE_REG16(a, (READ_REG16(a) | (tU16)(1<<b))))
+#define SET_BIT32(a,b)    (WRITE_REG32(a, (READ_REG32(a) | (tU32) (1<<b))))
+#define SET_BIT(a,b)      (SET_BIT32(a,b))
+ 
+#define CLEAR_BIT8(a,b)   (WRITE_REG8 (a, (READ_REG8(a) & (tU8)(~(1<<b)))))
+#define CLEAR_BIT16(a,b)  (WRITE_REG16(a, (READ_REG16(a) & (tU16)(~(1<<b)))))
+#define CLEAR_BIT32(a,b)  (WRITE_REG32(a, (READ_REG32(a) & (tU32)(~(1<<b)))))
+#define CLEAR_BIT(a,b)    (CLEAR_BIT32(a,b))
+ 
+#define TOGGLE_BIT8(a,b)  (WRITE_REG8 (a, (READ_REG8(a)  ^ (tU8) (1<<b))))
+#define TOGGLE_BIT16(a,b) (WRITE_REG16(a, (READ_REG16(a) ^ (tU16)(1<<b))))
+#define TOGGLE_BIT32(a,b) (WRITE_REG32(a, (READ_REG32(a) ^ (tU32)(1<<b))))
+#define TOGGLE_BIT(a,b)   (TOGGLE_BIT32(a,b))
+
+
