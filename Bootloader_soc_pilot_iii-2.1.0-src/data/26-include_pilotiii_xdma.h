--- u-boot-1.1.6/include/pilotiii/xdma.h	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/include/pilotiii/xdma.h	2010-04-05 16:47:25.000000000 -0400
@@ -0,0 +1,52 @@
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
+// ********************************************************************************
+
+
+#define XDMABASE        (SE_FGE_BASE + 0x380)
+
+#define XDMACTL         XDMABASE + 0x0
+#define XDMASTS         XDMABASE + 0x4
+#define XDMAQPTR        XDMABASE + 0x5
+#define XDMAAD0         XDMABASE + 0x10
+#define XDMAAD1         XDMABASE + 0x14
+#define XDMAAD2         XDMABASE + 0x18
+#define XDMAAD3         XDMABASE + 0x1C
+#define XDMAQBASE       (XDMABASE + 0x40)
+
+// Register bit definitions
+
+// XDMACTL
+#define GLBL_Q_PAUSE    (1<<1)
+#define ARM_MODE        (1<<2)
+
+// XDMASTS
+#define Q_SKIPPED       (1<<0)
+#define Q_PAUSED        (1<<1)
+#define Q_PAUSED_GLBL   (1<<2)
+#define Q_PAUSED_DPTR   (1<<3)
+#define Q_FULL          (1<<4)
+#define ARM_DPTR_DONE   (1<<5)
+
