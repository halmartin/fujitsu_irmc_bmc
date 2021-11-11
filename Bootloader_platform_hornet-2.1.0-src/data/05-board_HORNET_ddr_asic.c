--- uboot.original/board/HORNET/ddr_asic.c	1969-12-31 19:00:00.000000000 -0500
+++ uboot.new/board/HORNET/ddr_asic.c	2011-02-11 11:05:33.488528000 -0500
@@ -0,0 +1,1944 @@
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
+//#include "pilot_II.h"
+#define HW_ENV  1
+
+#include  "pilot_types.h"
+#include  "ddr.h"
+#include  "xdma.h"
+#include  "ddr_misc.h"
+
+tU8 xdma_lsi_fifo_training (void); 
+
+
+#if defined TYPE1
+#elif defined TYPE2
+#elif defined TYPE3
+#elif defined TYPE4
+#elif defined TYPE5 
+#else
+#define TYPE0 1
+#endif
+void DELAY(tU32 count)
+{
+  tU32 i=0;
+  for(i=0;i<count;i++)
+    ;
+}// DELAY
+
+tU32 ddrinit_asic(void){
+  tU32 temp/*, ddr*/;
+  //tU32 temp1;
+
+  *(tPVU32)(SYS_MCSS_CLK_CTRL) = 0x00; 
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  
+  // Remove the Reset from Phy clock synchronizer. 
+  SET_BIT32(MC_REG_ADDRESS10, 9);
+ 
+  // Slew rate of 4 for ADR, DQS, DQ.
+  WRITE_REG32(HM_IOBZ_CTRL, (READ_REG32(HM_IOBZ_CTRL) | 0x24800000));
+
+  if ((temp & 0x10000000) == 0x10000000)
+    *(tPVU32)(HM_RDPAT) = 0xc0000000;    // DDR2
+  else {
+    *(tPVU32)(HM_RDPAT) = 0xc00001c0;    // DDR3 WL5RL6 
+  }
+  *(tPVU32)(HM_MISC_CTRL) = *(tPVU32)(HM_MISC_CTRL) | 0x10;
+  *(tPVU32)(HM_MISC_CTRL) = *(tPVU32)(HM_MISC_CTRL) & 0xffffffef;
+
+  *(tPVU32)(HM_TRAINREQ) = 0x03;
+
+  while(*(tPVU32)(HM_ALLOWTRAIN) != 0x03);
+
+  // Check for First Update done.
+  *(tPVU32)(MC_REG_ADDRESS40) = (*(tPVU32)(MC_REG_ADDRESS40) & 0x7fffffff) | 0x80000000;  
+  while((READ_REG32(MC_REG_ADDRESS4C) & 0x10000) != 0x10000);
+  temp = READ_REG32(MC_REG_ADDRESS4C);
+
+  *(tPVU32)(MC_REG_ADDRESS18) = (*(tPVU32)(MC_REG_ADDRESS18) & 0xff9fffff) | 0x00200000;
+  while((READ_REG32(MC_REG_ADDRESS4C) & 0x00000001) != 0x00000001);
+  //temp1 = READ_REG32(MC_REG_ADDRESS4C);
+  READ_REG32(MC_REG_ADDRESS4C);
+#if defined (ENABLE_NCPRINTF)
+	{
+  nc_printf("*****************************************\n");
+  nc_printf("BZ first update done: ER %d, EP %d, EN %d\n", (temp & 0x03f80000) >> 19, (temp1 & 0x000001fc) >> 2, (temp1 & 0x0000fe00) >> 9);
+  nc_printf("BZ Ideal values: ER = 33d, EP = 37d, EN = 35d");
+  nc_printf("*****************************************\n");
+	}
+
+#endif
+
+#if defined BZ_OVERRIDE
+    // Force update of drv controller.
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xfffe0007) | 0x09518; // EN 35d, EP 37d
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xfffffffd) | 0x02;
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xfffffffb) | 0x04;
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xfffffffb);
+
+    // Force update of odt controller.
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xf80fffff) | 0x02100000; // ER 33d
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xfffbffff) | 0x00040000;
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xfff7ffff) | 0x00080000;
+    *(tPVU32)(MC_REG_ADDRESS0C) = (*(tPVU32)(MC_REG_ADDRESS0C) & 0xfff7ffff);
+#endif
+
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  if ((temp & 0x10000000) == 0x10000000){
+    ddrcfg_asic(2);
+    //ddr = 2;
+  }
+  else {
+    ddrcfg_asic(3);
+    //ddr = 3;
+  }
+  WRITE_REG32(HM_FIFO_DELAY_1, 0xff);
+  while(*(tPVU32)(HM_DPBASEDDELAY) != 0x03);
+
+  *(tPVU32)(DDRINTFLAG) = 0xa55ab55b;
+  *(tPVU32)(DDRINTFLAG + 0x04) = 0xc3d25aa5;
+  temp = gateon_training();
+#if defined (HW_ENV)
+  redeye_training();
+#endif
+  lsi_phy_fifo_training();
+  WRITE_REG32(HM_TRAINREQ, 0);
+  
+  // OPTION4
+  temp = *(tPVU32)(MC_REG_ADDRESS28);
+  temp = temp & ~0xfc000000;
+  temp = temp | 0xc4000000;
+  *(tPVU32)(MC_REG_ADDRESS28) = temp;
+    
+  temp = *(tPVU32)(MC_REG_ADDRESS34);
+  temp = temp & ~0x7fffffff;
+  temp = temp | 0x40948c14;
+  *(tPVU32)(MC_REG_ADDRESS34) = temp;
+
+  temp = *(tPVU32)(MC_REG_ADDRESS40);
+  temp = temp & ~0x4000003f;
+  temp = temp | 0x00000039;
+  *(tPVU32)(MC_REG_ADDRESS40) = temp;
+ 
+
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x20000000;
+  temp = temp | 0x20000000; // ahb2ddr_pads_reset_n = 1. To mask off the reset from the PHY going to I/Os.
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+
+  // ECC Mode.
+#if defined ECC
+  *(tPVU32)(MC_REG_ADDRESS58) = LOWER_ECC_ADDRESS;
+  *(tPVU32)(MC_REG_ADDRESS5C) = UPPER_ECC_ADDRESS;
+
+  temp = *(tPVU32)(MC_REG_ADDRESS08);
+  temp = temp & ~0x000000ff;
+  temp = temp | 0x00000067; // ECC bits
+  *(tPVU32)(MC_REG_ADDRESS08) = temp;
+
+  temp = *(tPVU32)(MC_REG_ADDRESS00);
+  temp = temp & ~0x00000008;
+  temp = temp | 0x00000008; // Initialization of memory with correct ECC bits.
+  *(tPVU32)(MC_REG_ADDRESS00) = temp;
+  while((*(tPVU32)(MC_REG_ADDRESS00) & 0x08) != 0);
+#endif
+
+  // Perform a write and read to see if the read passes. That will tell the training passed.
+  *(tPVU32)(DDRINTFLAG) = 0xbcbc5a5a;
+  if (*(tPVU32)(DDRINTFLAG) != 0xbcbc5a5a) {
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+	{
+    nc_printf("*****************************************\n");
+    nc_printf("FATAL ERROR : DDR PHY training failed!!!!\n");
+    nc_printf("*****************************************\n");
+	}
+#endif
+#endif
+  }
+  else {
+#if defined (HW_ENV)
+    #if defined (ENABLE_NCPRINTF)
+	{
+    nc_printf("**************************\n");
+    nc_printf("DDR%d Initialized\n", ddr);
+    nc_printf("**************************\n");
+	}
+  #endif
+#endif
+  }
+
+  // All interrupts cleared.
+  *(tPVU32)(MC_REG_ADDRESS44) = 0xffffffff;
+  *(tPVU32)(MC_REG_ADDRESS50) = 0xffffffff;
+  *(tPVU32)(MC_REG_ADDRESS08) = (*(tPVU32)(MC_REG_ADDRESS08) | 0x00000418);
+  *(tPVU32)(MC_REG_ADDRESS28) = (*(tPVU32)(MC_REG_ADDRESS28) | 0x01004900); 
+  *(tPVU32)(MC_REG_ADDRESS40) = (*(tPVU32)(MC_REG_ADDRESS40) | 0x00010000);
+ 
+  temp = *(tPVU32)(MC_REG_ADDRESS00);
+  temp = temp & ~0x00000200;
+  temp = temp | 0x00000200;
+  *(tPVU32)(MC_REG_ADDRESS00) = temp;
+
+  // Shutoff mcss register clk.
+  *(tPVU32)(SYS_MCSS_CLK_CTRL) = 0xA000;
+
+  return 0;
+}// ddrinit
+
+
+void ddrcfg_asic(tU8 ddr_type){
+  tU32 temp;
+
+  // Programming TYPE.
+#if defined TYPE0
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x0e000000;
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+#endif
+#if defined TYPE1
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x0e000000;
+  temp = temp | 0x02000000;
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+#endif
+#if defined TYPE2
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x0e000000;
+  temp = temp | 0x04000000;
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+#endif
+#if defined TYPE3
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x0e000000;
+  temp = temp | 0x06000000;
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+#endif
+#if defined TYPE4
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x0e000000;
+  temp = temp | 0x08000000;
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+#endif
+#if defined TYPE5
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x0e000000;
+  temp = temp | 0x0a000000;
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+#endif
+
+  // Programming tRFC, r_trfcplus10counter, r_ref2actcounter.
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x01fe00ff;
+  if (ddr_type == 2) {
+
+#if defined TYPE0
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0030001a; 
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00280017; 
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x00240014; 
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x001c0010; 
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x0016000c; 
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x00100009; 
+#else
+    temp = temp | 0x0030001a; 
+#endif
+#endif
+
+#if defined TYPE1
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0030001a; 
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00280017; 
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x00240014; 
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x001c0010; 
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x0016000c; 
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x00100009; 
+#else
+    temp = temp | 0x0030001a; 
+#endif
+#endif
+
+#if defined TYPE2
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x004a0027; 
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00420023; 
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0038001e; 
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x002e0019; 
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x00240013; 
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x001a000e; 
+#else
+    temp = temp | 0x004a0027; 
+#endif
+#endif
+
+#if defined TYPE3
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x004a0027; 
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00420023; 
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0038001e; 
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x002e0019; 
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x00240013; 
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x001a000e; 
+#else
+    temp = temp | 0x004a0027; 
+#endif
+#endif
+
+#if defined TYPE4
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x00800042; 
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0070003a; 
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x00600032; 
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x004e0029; 
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x003e0020; 
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x002e0018; 
+#else
+    temp = temp | 0x00800042; 
+#endif
+#endif
+
+#if defined TYPE5
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x00800042; 
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0070003a; 
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x00600032; 
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x004e0029; 
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x003e0020; 
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x002e0018; 
+#else
+    temp = temp | 0x00800042; 
+#endif
+#endif
+
+  }
+  else {
+    // DDR3
+#if defined TYPE0
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x00280016;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00240014;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x001e0011;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x0018000e;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x0012000a;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x000e0008;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x000a0006;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x00060004;
+#else
+    temp = temp | 0x00280016;
+#endif
+#endif
+
+#if defined TYPE1
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x00280016;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00240014;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x001e0011;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x0018000e;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x0012000a;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x000e0008;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x000a0006;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x00060004;
+#else
+    temp = temp | 0x00280016;
+#endif
+#endif
+
+#if defined TYPE2
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x003c0020;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0036001d;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x002e0019;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x00240014;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x001c000f;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x0016000c;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x00100009;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x000a0006;
+#else
+    temp = temp | 0x003c0020;
+#endif
+#endif
+
+#if defined TYPE3
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x003c0020;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0036001d;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x002e0019;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x00240014;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x001c000f;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x0016000c;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x00100009;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x000a0006;
+#else
+    temp = temp | 0x003c0020;
+#endif
+#endif
+
+#if defined TYPE4
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0074003c;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00660035;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0058002e;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x00480026;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x0038001d;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x002a0016;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x00220012;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x0014000b;
+#else 
+    temp = temp | 0x0074003c;
+#endif
+#endif
+
+#if defined TYPE5
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0074003c;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00660035;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0058002e;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x00480026;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x0038001d;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x002a0016;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x00220012;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x0014000b;
+#else 
+    temp = temp | 0x0074003c;
+#endif
+#endif
+  }
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+
+  // Programming tFAW, r_act2precounter.
+  temp = *(tPVU32)(MC_REG_ADDRESS10);
+  temp = temp & ~0x3ff00000;
+  if (ddr_type == 2) {
+
+#if defined TYPE0
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0e900000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0c800000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0a700000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x08600000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04400000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x04300000;
+#else
+    temp = temp | 0x0e900000;
+#endif
+#endif
+
+#if defined TYPE1
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0e700000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0c600000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0a500000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x08400000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04300000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x04200000;
+#else
+    temp = temp | 0x0e700000;
+#endif
+#endif
+
+#if defined TYPE2
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0e900000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0c800000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0a700000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x08600000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04400000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x04300000;
+#else
+    temp = temp | 0x0e900000;
+#endif
+#endif
+
+#if defined TYPE3
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0e700000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0c600000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0a500000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x08400000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04300000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x04200000;
+#else
+    temp = temp | 0x0e700000;
+#endif
+#endif
+
+#if defined TYPE4
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0e900000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0c800000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0a700000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x08600000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04400000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x04300000;
+#else
+    temp = temp | 0x0e900000;
+#endif
+#endif
+
+#if defined TYPE5
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0e700000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0c600000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x0a500000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x08400000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04300000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x04200000;
+#else
+    temp = temp | 0x0e700000;
+#endif
+#endif
+
+  }
+  else {
+
+#if defined TYPE0
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0c900000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0a800000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x08700000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x06600000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04400000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x02100000;
+#else
+    temp = temp | 0x0c900000;
+#endif
+#endif
+
+#if defined TYPE1
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0c700000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0a700000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x08600000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x06400000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04300000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x02200000;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x02100000;
+#else
+    temp = temp | 0x0c700000;
+#endif
+#endif
+
+#if defined TYPE2
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0c900000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0a800000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x08700000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x06600000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04400000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x02100000;
+#else
+    temp = temp | 0x0c900000;
+#endif
+#endif
+
+#if defined TYPE3
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0c700000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0a700000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x08600000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x06400000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04300000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x02200000;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x02100000;
+#else
+    temp = temp | 0x0c700000;
+#endif
+#endif
+
+#if defined TYPE4
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0c900000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0a800000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x08700000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x06600000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04400000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x02100000;
+#else
+    temp = temp | 0x0c900000;
+#endif
+#endif
+
+#if defined TYPE5
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0c700000;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0a700000;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x08600000;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x06400000;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04300000;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x02300000;
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x02200000;
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x02100000;
+#else
+    temp = temp | 0x0c700000;
+#endif
+#endif
+
+  }
+  *(tPVU32)(MC_REG_ADDRESS10) = temp;
+
+  temp = *(tPVU32)(MC_REG_ADDRESS14);
+  temp = temp & ~0x01f00000;
+  if (ddr_type == 2) {
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x00a00000; // r_act2actsbcounter=tRC
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00900000; // r_act2actsbcounter=tRC
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x00700000; // r_act2actsbcounter=tRC
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x00600000; // r_act2actsbcounter=tRC
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x00400000; // r_act2actsbcounter=tRC
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x00300000; // r_act2actsbcounter=tRC
+#else
+    temp = temp | 0x00a00000; // r_act2actsbcounter=tRC
+#endif
+  }
+  else {
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x00800000; // r_act2actsbcounter=tRC
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x00700000; // r_act2actsbcounter=tRC
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x00600000; // r_act2actsbcounter=tRC
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x00500000; // r_act2actsbcounter=tRC
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x00300000; // r_act2actsbcounter=tRC
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x00200000; // r_act2actsbcounter=tRC
+#elif defined DDR23_75_125MHZ
+    temp = temp | 0x00200000; // r_act2actsbcounter=tRC
+#elif defined DDR23_25_75MHZ
+    temp = temp | 0x00100000; // r_act2actsbcounter=tRC
+#else
+    temp = temp | 0x00800000; // r_act2actsbcounter=tRC
+#endif
+  }
+  *(tPVU32)(MC_REG_ADDRESS14) = temp;
+
+  // Programming r_zq2anycounter and ddr2 mode.
+  temp = *(tPVU32)(MC_REG_ADDRESS04);
+  temp = temp & ~0x0001ff00;
+  temp = temp | 0x0000fe00;
+  *(tPVU32)(MC_REG_ADDRESS04) = temp;
+
+  temp = *(tPVU32)(MC_REG_ADDRESS10);
+  temp = temp & ~0xc000ffff;
+  if (ddr_type == 2) {
+    temp = temp | 0x00002240; // WL4RL5
+  }
+  else {
+#if defined DLL_OFF
+    temp = temp | 0x00000244; // WL5RL6, Delay WR by a clock
+#else
+    temp = temp | 0x00000204; // WL5RL6
+#endif
+  }
+  *(tPVU32)(MC_REG_ADDRESS10) = temp;
+
+  if (ddr_type == 2) {
+    temp = temp & ~0xffffffff;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+  }
+  else {
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffff;
+#if defined DLL_OFF
+    temp = temp | 0x00000024; // DDR3 out of reset
+#else
+    temp = temp | 0x00000004; // DDR3 out of reset
+#endif
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    DELAY(100);
+  }
+
+  // AllowCke.
+  temp = *(tPVU32)(MC_REG_ADDRESS00);
+  temp = temp & ~0xffffffc7;
+  temp = temp | 0x00000400;
+  *(tPVU32)(MC_REG_ADDRESS00) = temp;
+
+  if (ddr_type == 2) {
+    // Precharge All.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffff0c00;
+    temp = temp | 0x00000c00;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x800) != 0);
+  
+    // EMRS2.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00005400;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // EMRS3.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00007400;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // EMRS1. OCD should be set to 0.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00043400;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // MRS. Reset DLL. CL5.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x0b521400;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // Precharge All.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffff0c00;
+    temp = temp | 0x00000c00;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x800) != 0);
+
+    // Refresh.
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002004;
+    DELAY(5);
+#if defined DDR23_350_400MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x000022AA; // Refresh of 3.9us at 350MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_300_350MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002249; // Refresh of 3.9us at 300MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_250_300MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x000021e7; // Refresh of 3.9us at 250MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_200_250MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002186; // Refresh of 3.9us at 200MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_150_200MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002124; // Refresh of 3.9us at 150MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_125_150MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x000020f4; // Refresh of 3.9us at 125MHz DDR clock at Tcase 85-95 C.
+#else
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x0000230c; // Refresh of 3.9us at 400MHz DDR clock at Tcase 85-95 C.
+#endif
+    DELAY(2);
+
+    // MRS.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+#if defined DDR23_350_400MHZ
+    temp = temp | 0x0a521400;
+#elif defined DDR23_300_350MHZ
+    temp = temp | 0x0a521400;
+#elif defined DDR23_250_300MHZ
+    temp = temp | 0x08521400;
+#elif defined DDR23_200_250MHZ
+    temp = temp | 0x06521400;
+#elif defined DDR23_150_200MHZ
+    temp = temp | 0x04521400;
+#elif defined DDR23_125_150MHZ
+    temp = temp | 0x04521400;
+#else
+    temp = temp | 0x0a521400;
+#endif
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // EMRS1. OCD default setting.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x03843400;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // EMRS1. OCD should be set to 0.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00043400;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+  }
+  else {
+    // DDR3 
+#if defined DLL_OFF
+    // WL6 EMRS2.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00085404;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+#else
+    // WL5 EMRS2.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00005404;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+#endif
+
+    // EMRS3.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00007404;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // EMRS1.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    //temp = temp | 0x00063404;
+    temp = temp | 0x00403404; //ODT = 120 ohm, output drive strength = 40 ohm
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // MRS0. DLL reset.
+#if defined DLL_OFF
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xff000040;
+    temp = temp | 0x13000000;           
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+#else
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xff000040;
+    temp = temp | 0x15000040;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+#endif
+
+    // MRS0.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0x00ffff87;
+    temp = temp | 0x00211404;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    // ZQ calib.
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00000584;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x80) != 0);
+
+    // EMRS1.
+#if defined DLL_OFF
+    temp = *(tPVU32)(MC_REG_ADDRESS00);
+    temp = temp & ~0xffffffc7;
+    temp = temp | 0x00033444;
+    *(tPVU32)(MC_REG_ADDRESS00) = temp;
+    while(((*(tPVU32)(MC_REG_ADDRESS00)) & 0x1000) != 0);
+
+    temp = *(tPVU32)(MC_REG_ADDRESS10);
+    temp = temp & ~0x0000001f;           
+    temp = temp | 0x00000001;
+    *(tPVU32)(MC_REG_ADDRESS10) = temp;
+#endif
+    
+    // Refresh. 
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002004;
+    DELAY(5);
+#if defined DDR23_350_400MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x000022AA; // Refresh of 3.9us at 350MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_300_350MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002249; // Refresh of 3.9us at 300MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_250_300MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x000021e7; // Refresh of 3.9us at 250MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_200_250MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002186; // Refresh of 3.9us at 200MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_150_200MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002124; // Refresh of 3.9us at 150MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_125_150MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x000020f4; // Refresh of 3.9us at 125MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_75_125MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002092; // Refresh of 3.9us at 75MHz DDR clock at Tcase 85-95 C.
+#elif defined DDR23_25_75MHZ
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x00002031; // Refresh of 3.9us at 25MHz DDR clock at Tcase 85-95 C.
+#else
+    *(tPVU32)(MC_REG_ADDRESS18) = 0x0000230c; // Refresh of 3.9us at 400MHz DDR clock at Tcase 85-95 C.
+#endif
+    DELAY(5);
+  }
+
+  // 
+  temp = *(tPVU32)(MC_REG_ADDRESS08);
+  temp = temp & ~0xffffff00;
+  temp = temp | 0xa8354300;
+  *(tPVU32)(MC_REG_ADDRESS08) = temp;
+
+  //
+  temp = *(tPVU32)(MC_REG_ADDRESS14);
+  temp = temp & ~0xfe0fffff;
+  temp = temp | 0x5205b030;
+  *(tPVU32)(MC_REG_ADDRESS14) = temp;
+   
+  // 
+  temp = *(tPVU32)(MC_REG_ADDRESS28);
+  temp = temp & ~0xfc00f000;
+  temp = temp | 0x90000000; 
+  *(tPVU32)(MC_REG_ADDRESS28) = temp;
+
+  // 
+  temp = *(tPVU32)(MC_REG_ADDRESS34);
+  temp = temp & ~0x7fffffff;
+  temp = temp | 0x42492248;
+  *(tPVU32)(MC_REG_ADDRESS34) = temp;
+
+  // MAGM value. 
+  temp = *(tPVU32)(MC_REG_ADDRESS38);
+  temp = temp | MAGM_VALUE;
+  *(tPVU32)(MC_REG_ADDRESS38) = temp;
+  // UAGM value.
+  temp = *(tPVU32)(MC_REG_ADDRESS3C);
+  temp = temp | UAGM_VALUE;
+  *(tPVU32)(MC_REG_ADDRESS3C) = temp;
+
+  // 
+  temp = *(tPVU32)(MC_REG_ADDRESS40);
+  temp = temp & ~0x7fffffff;
+  temp = temp | 0x40004de7;
+  *(tPVU32)(MC_REG_ADDRESS40) = temp;
+
+  //
+  temp = *(tPVU32)(MC_REG_ADDRESS48);
+  temp = temp | 0xffffffff;
+  *(tPVU32)(MC_REG_ADDRESS48) = temp;
+  temp = *(tPVU32)(MC_REG_ADDRESS54);
+  temp = temp | 0xefffffff;                  
+  *(tPVU32)(MC_REG_ADDRESS54) = temp;
+
+  temp = *(tPVU32)(MC_REG_ADDRESS10);
+  temp = temp & ~0x00010de0;
+  if (ddr_type == 2) {
+    temp = temp | 0x00010040;
+  }
+  else {
+#if defined DLL_OFF
+    temp = temp | 0x00000040;               
+#else
+    temp = temp | 0x00000000;               
+#endif
+  }
+  *(tPVU32)(MC_REG_ADDRESS10) = temp;
+
+  temp = *(tPVU32)(MC_REG_ADDRESS1C);
+  temp = temp & ~0xbfffc000;
+  temp = temp | 0xbc420000;
+  *(tPVU32)(MC_REG_ADDRESS1C) = temp;
+
+  DELAY(5);
+}
+
+int gateon_training(void) {
+  tVU32 temp, i, training_done, rData0, rData1;
+  tVU32 gateon, prev_val;
+
+  i = 0;
+  training_done = 0;
+#if defined (HW_ENV)
+  gateon = 0x20202020; // Starting value .. depend upon freq
+#else
+  gateon = 0x40404040; // Starting value .. depend upon freq
+#endif
+  prev_val = gateon;
+  while(training_done == 0) {
+    prg_gate(gateon);
+    rData0 = READ_REG32(DDRINTFLAG);
+    prg_gate(gateon);
+    rData1 = READ_REG32(DDRINTFLAG+0x04);
+#if defined (ENABLE_NCPRINTF)
+    {
+    nc_printf("Register00:%x\n", READ_REG32(MC_REG_ADDRESS00));
+    nc_printf("HMPROG:%x, i:%d\n", READ_REG32(HM_PROGGATEON), i);
+    nc_printf("DDRINTFLAG:%x %x\n", rData0, rData1);
+  }
+#endif
+    if((rData0 != 0xa55ab55b) || (rData1 != 0xc3d25aa5)) {
+      temp = *(tPVU32)(HM_PROGGATEON);
+      if((temp & 0xe0) == 0xe0) {
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+        {
+	nc_printf("!!! ERROR GATEON COULDN'T COMPLETE, MAX COARSE REACHED !!!!\n");
+       }
+    #endif
+#endif
+	return(1);
+      }
+      else {
+	gateon = gateon + 0x08080808;
+	i = i + 1;
+      }
+    }
+    else if((i != 0) && (rData0 == 0xa55ab55b) && (rData1 == 0xc3d25aa5)) {
+      training_done = 1;
+      prev_val = gateon;
+    }
+    else if((i == 0) && (rData0 == 0xa55ab55b) && (rData1 == 0xc3d25aa5)){
+      temp = *(tPVU32)(HM_PROGGATEON);
+      if(temp == 0x00){
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+	{
+	nc_printf("!!! ERROR GATEON COULDN'T COMPLETE, MIN REACHED !!!!\n");
+       }
+#endif
+#endif
+	return(1); // Could not complete training becoz min value reached
+      }
+      else {
+	gateon = gateon - 0x08080808;
+	i = i + 1;
+      }
+    }
+#if defined (HW_ENV)
+    else {
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+	{
+      nc_printf("!!! ERROR Should have not come here :%d !!!!!\n",i);
+       }
+#endif
+    }
+#endif
+  }
+  training_done = 0;
+  i = 0;
+  gateon = prev_val - 0x08080808 + 0x01010101;
+  
+  // Fine training.
+  while((training_done == 0)) {
+    prg_gate(gateon);
+    rData0 = READ_REG32(DDRINTFLAG);
+    prg_gate(gateon);
+    rData1 = READ_REG32(DDRINTFLAG+0x04);
+#if defined (ENABLE_NCPRINTF)
+	{
+    nc_printf("HMPROG:%x, i:%d\n", READ_REG32(HM_PROGGATEON), i);
+    nc_printf("DDRINTFLAG:%x %x:\n", rData0, rData1);
+	}
+#endif
+    //  Burst_Read_PHY_TRAINING(DDRINTFLAG);
+    if((rData0 != 0xa55ab55b) || (rData1 != 0xc3d25aa5)) {
+      temp = *(tPVU32)(HM_PROGGATEON);
+      if((temp & 0x07) == 0x07)	
+	training_done = 1;
+      else {
+	gateon = gateon + 0x01010101;
+	i = i + 1;
+      }
+    }
+    else {
+      training_done = 1;
+      prev_val = gateon;
+    }
+  }
+  // Final 1/2 clock. 
+  gateon = prev_val + 0x10101010;
+  prg_gate(gateon);
+  rData0 = READ_REG32(DDRINTFLAG);
+  rData1 = READ_REG32(DDRINTFLAG+0x4);
+  if((rData0 != 0xa55ab55b) || (rData1 != 0xc3d25aa5)) {
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+	{
+    nc_printf("!!! ERROR GATEON TRAINING COULD NOT COMPLETE !!!! rData0:%x rData1:%x\n", rData0, rData1);
+	}
+#endif
+#endif
+    return(1); // Training could not complete properly
+  }
+  else {
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+	{
+    nc_printf("GATEON TRAINING COMPLETE\n");
+	}
+#endif
+#endif
+    *(tPVU32)(HM_MISC_CTRL) =  (*(tPVU32)(HM_MISC_CTRL) & 0xfffffffc) | 0x02;
+    *(tPVU32)(MC_REG_ADDRESS00) = (*(tPVU32)(MC_REG_ADDRESS00) & 0xffffffef) | 0x03; //training complete
+    return(0);
+  }
+}
+
+void redeye_training(void){
+  tVU32 temp, slave_count_p, slave_count_n;
+  tVU8 lbpe, lbne, ubpe, ubne;
+  tVU8 par_done = 0;
+  tVU8 nar_done = 0;
+  tVU8 lock_p=0;
+  tVU8 lock_n=0;
+  tVU16 slave_delay_offset_uop = 0, slave_delay_offset_lop = 0, slave_delay_offset_uon = 0,slave_delay_offset_lon = 0;
+  tVU16 optimum_uop_0 = 0, optimum_lop_0 = 0, optimum_uon_0 = 0, optimum_lon_0 = 0;
+  tVU16 optimum_uop_1 = 0, optimum_lop_1 = 0, optimum_uon_1 = 0, optimum_lon_1 = 0;
+  tVU16 max_slave_delay_offset_uop = 0, max_slave_delay_offset_lop = 0, max_slave_delay_offset_uon = 0,max_slave_delay_offset_lon = 0;
+  tVU16 min_slave_delay_offset_uop = 0, min_slave_delay_offset_lop = 0, min_slave_delay_offset_uon = 0,min_slave_delay_offset_lon = 0;
+  tVU16 threshold_p,threshold_n;
+  #if defined (HW_ENV)
+  #if defined (ENABLE_NCPRINTF_FEW)
+  tVU16 DP0Prange, DP0Nrange, DP1Prange, DP1Nrange;
+  #endif
+  #endif
+ 
+  *(tPVU32)(DDRINTFLAG+0x100) = 0xdeadbeef;
+  
+#if defined (ENABLE_NCPRINTF)
+	{
+    nc_printf("**************************\n");
+    WRITE_REG32(HM_DPLOADDELAY, (0x0800 << 16));
+    nc_printf("UOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0200 << 16));
+    nc_printf("LOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0400 << 16));
+    nc_printf("UON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0100 << 16));
+    nc_printf("LON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0801 << 16));
+    nc_printf("UOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0201 << 16));
+    nc_printf("LOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0401 << 16));
+    nc_printf("UON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0101 << 16));
+    nc_printf("LON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    nc_printf("  START EYE TRAINING      \n");
+    nc_printf("**************************\n");
+}
+#endif
+
+  /**************** DP0 Start *******************/
+  while(!par_done) {
+    SET_BIT32(HM_MISC_CTRL, 0);
+    CLEAR_BIT32(HM_MISC_CTRL, 1);
+
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8800 << 16) | slave_delay_offset_uop));
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8200 << 16) | slave_delay_offset_lop));
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8400 << 16) | slave_delay_offset_uon));
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8100 << 16) | slave_delay_offset_lon));
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0800);
+#if defined (ENABLE_NCPRINTF)
+	{
+    nc_printf("DP0 PAR READ DELAY:%x\n", READ_REG32(HM_DPREADDELAY));
+	}
+#endif
+    CLEAR_BIT32(HM_MISC_CTRL, 0);
+    SET_BIT32(HM_MISC_CTRL, 1);
+
+    temp = READ_REG32(DDRINTFLAG+0x100);
+    lbpe = temp & 0xff;
+    lbne = (temp >> 16) & 0xff;
+    if((lbpe != 0xef) && (lock_p == 0)) {
+      max_slave_delay_offset_uop =  slave_delay_offset_uop;  
+      max_slave_delay_offset_lop =  slave_delay_offset_lop;
+      lock_p = 1;
+    }
+    if((lbne != 0xad) && (lock_n == 0)) {
+      max_slave_delay_offset_uon =  slave_delay_offset_uon;
+      max_slave_delay_offset_lon =  slave_delay_offset_lon;
+      lock_n = 1;
+    }
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0800);
+    threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY);
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0400);
+    threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY);
+    if(((lbne != 0xad) && (lbpe != 0xef)) || (threshold_p >= 0x140) || (threshold_n >= 0x140)) {
+      par_done = 1;
+      max_slave_delay_offset_uop = (threshold_p >= 0x140) ? (
+							     (slave_delay_offset_uop < 0x0f) ? threshold_p : slave_delay_offset_uop) : max_slave_delay_offset_uop;
+      max_slave_delay_offset_uon = (threshold_n >= 0x140) ? (
+							     (slave_delay_offset_uon < 0x0f) ? threshold_n : slave_delay_offset_uon) : max_slave_delay_offset_uon;
+      max_slave_delay_offset_lop = (threshold_p >= 0x140) ? (
+							     (slave_delay_offset_lop < 0x0f) ? threshold_p : slave_delay_offset_lop) : max_slave_delay_offset_lop;
+      max_slave_delay_offset_lon = (threshold_n >= 0x140) ? (
+							     (slave_delay_offset_lon < 0x0f) ? threshold_n : slave_delay_offset_lon) : max_slave_delay_offset_lon;
+    }
+    if ((lbpe == 0xef)){
+      slave_delay_offset_uop =  slave_delay_offset_uop + 1;  
+      slave_delay_offset_lop =  slave_delay_offset_lop + 1;
+    }
+    if((lbne == 0xad)) {
+      slave_delay_offset_uon =  slave_delay_offset_uon + 1;
+      slave_delay_offset_lon =  slave_delay_offset_lon + 1;
+    }
+#if defined (ENABLE_NCPRINTF)
+	{
+    nc_printf("PAR DP0 lbne :%x lbpe:%x slave_delay_offset_uop:%x slave_delay_offset_uon:%x\n", lbne, lbpe, slave_delay_offset_uop, slave_delay_offset_uon);
+}
+#endif
+  }
+#if defined (ENABLE_NCPRINTF)
+{
+  nc_printf("UOP MAX DP0 :%x lbne :%x lbpe:%x slave_delay_offset_uop:%x slave_delay_offset_uon:%x\n", max_slave_delay_offset_uop, lbne, lbpe, slave_delay_offset_uop, slave_delay_offset_uon);
+  nc_printf("LOP MAX DP0 :%x threshold_p:%x threshold_n:%x\n", max_slave_delay_offset_lop, threshold_p, threshold_n);
+  nc_printf("UON MAX DP0 :%x\n", max_slave_delay_offset_uon);
+  nc_printf("LON MAX DP0 :%x\n", max_slave_delay_offset_lon);
+}
+#endif
+
+  lock_p = 0;
+  lock_n = 0;
+  slave_count_p = 0;
+  slave_count_n = 0;
+  slave_delay_offset_uop = 0;
+  slave_delay_offset_lop = 0;
+  slave_delay_offset_uon = 0;
+  slave_delay_offset_lon = 0;
+
+  while(!nar_done) {
+    SET_BIT32(HM_MISC_CTRL, 0);
+    CLEAR_BIT32(HM_MISC_CTRL, 1);
+
+    WRITE_REG32(HM_DPLOADDELAY, (0x8800 << 16) | slave_delay_offset_uop);
+    WRITE_REG32(HM_DPLOADDELAY, (0x8200 << 16) | slave_delay_offset_lop);
+    WRITE_REG32(HM_DPLOADDELAY, (0x8400 << 16) | slave_delay_offset_uon);
+    WRITE_REG32(HM_DPLOADDELAY, (0x8100 << 16) | slave_delay_offset_lon);
+    WRITE_REG32(HM_DPLOADDELAY, (0x0800 << 16));
+#if defined (ENABLE_NCPRINTF)
+{
+    nc_printf("DP0 NAR READ DELAY:%x\n", READ_REG32(HM_DPREADDELAY));
+}
+#endif
+    CLEAR_BIT32(HM_MISC_CTRL, 0);
+    SET_BIT32(HM_MISC_CTRL, 1);
+    temp = READ_REG32(DDRINTFLAG+0x100);
+    lbpe = temp & 0xff;
+    lbne = (temp >> 16) & 0xff;
+    if((lbpe != 0xef) && (lock_p == 0)) {
+      min_slave_delay_offset_uop =  1024 - slave_delay_offset_uop;  
+      min_slave_delay_offset_lop =  1024 - slave_delay_offset_lop;
+      lock_p = 1;
+    }
+    if((lbne != 0xad)  && (lock_n == 0)) {
+      min_slave_delay_offset_uon =  1024 - slave_delay_offset_uon;
+      min_slave_delay_offset_lon =  1024 - slave_delay_offset_lon;
+      lock_n = 1;
+    }
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0800);
+    threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY);
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0400);
+    threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY);
+    if(((lbne != 0xad) && (lbpe != 0xef)) || (threshold_p <= 0x0f) || (threshold_n <= 0x0f)){
+      nar_done = 1;
+      min_slave_delay_offset_uop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_uop;  
+      min_slave_delay_offset_uon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_uon;  
+      min_slave_delay_offset_lop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_lop;  
+      min_slave_delay_offset_lon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_lon;  
+    }
+    if((lbpe == 0xef)) {
+      slave_count_p = slave_count_p + 1;
+      slave_delay_offset_uop =  1024 - slave_count_p;  
+      slave_delay_offset_lop =  1024 - slave_count_p;    
+    }
+    if((lbne == 0xad)) {
+      slave_count_n = slave_count_n + 1;
+      slave_delay_offset_uon =  1024 - slave_count_n;
+      slave_delay_offset_lon =  1024 - slave_count_n;
+    }
+#if defined (ENABLE_NCPRINTF)
+{
+    nc_printf("NAR DP0 lbne :%x lbpe:%x slave_count_n:%x, slave_count_p:%x\n", lbne, lbpe, slave_count_n, slave_count_p);
+}
+#endif
+  }
+
+#if defined (ENABLE_NCPRINTF)
+{
+  nc_printf("UOP MIN DP0 :%x lbne :%x lbpe:%x slave_count_n:%x, slave_count_p:%x\n", min_slave_delay_offset_uop, lbne, lbpe, slave_count_n, slave_count_p);
+  nc_printf("LOP MIN DP0 :%x threshold_p:%x threshold_n:%x\n", min_slave_delay_offset_lop, threshold_p, threshold_n);
+  nc_printf("UON MIN DP0 :%x\n", min_slave_delay_offset_uon);
+  nc_printf("LON MIN DP0 :%x\n", min_slave_delay_offset_lon);
+}
+#endif
+
+  optimum_uop_0 = (max_slave_delay_offset_uop - min_slave_delay_offset_uop)/2;
+  if (max_slave_delay_offset_uop < min_slave_delay_offset_uop) {
+    optimum_uop_0 = 1024 - (min_slave_delay_offset_uop - max_slave_delay_offset_uop)/2;
+  }
+  optimum_lop_0 = (max_slave_delay_offset_lop - min_slave_delay_offset_lop)/2;
+  if (max_slave_delay_offset_lop < min_slave_delay_offset_lop) {
+    optimum_lop_0 = 1024 - (min_slave_delay_offset_lop - max_slave_delay_offset_lop)/2;
+  }
+  optimum_uon_0 = (max_slave_delay_offset_uon - min_slave_delay_offset_uon)/2;
+  if (max_slave_delay_offset_uon < min_slave_delay_offset_uon) {
+    optimum_uon_0 = 1024 - (min_slave_delay_offset_uon - max_slave_delay_offset_uon)/2;
+  }
+  optimum_lon_0 = (max_slave_delay_offset_lon - min_slave_delay_offset_lon)/2;
+  if (max_slave_delay_offset_lon < min_slave_delay_offset_lon) {
+    optimum_lon_0 = 1024 - (min_slave_delay_offset_lon - max_slave_delay_offset_lon)/2;
+  } 
+  #if defined (HW_ENV)
+  #if defined (ENABLE_NCPRINTF_FEW)
+  DP0Prange = (max_slave_delay_offset_uop + min_slave_delay_offset_uop);
+  DP0Nrange = (max_slave_delay_offset_uon + min_slave_delay_offset_uon);
+  #endif
+  #endif
+  /**************** DP0 End   *******************/
+
+  /**************** DP1 Start *******************/
+  lock_p = 0;
+  lock_n = 0;
+  par_done = 0;
+  nar_done = 0;
+  slave_delay_offset_uop = 0;
+  slave_delay_offset_lop = 0;
+  slave_delay_offset_uon = 0;
+  slave_delay_offset_lon = 0;
+
+  while(!par_done) {
+    SET_BIT32(HM_MISC_CTRL, 0);
+    CLEAR_BIT32(HM_MISC_CTRL, 1);
+
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8801 << 16) | slave_delay_offset_uop));
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8201 << 16) | slave_delay_offset_lop));
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8401 << 16) | slave_delay_offset_uon));
+    WRITE_REG32(HM_DPLOADDELAY, ((0x8101 << 16) | slave_delay_offset_lon));
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0801);
+#if defined (ENABLE_NCPRINTF)
+{
+    nc_printf("DP1 PAR READ DELAY:%x\n", READ_REG32(HM_DPREADDELAY));
+}
+#endif
+    CLEAR_BIT32(HM_MISC_CTRL, 0);
+    SET_BIT32(HM_MISC_CTRL, 1);
+
+    temp = READ_REG32(DDRINTFLAG+0x100);
+    ubpe = (temp >> 8) & 0xff;
+    ubne = (temp >> 24) & 0xff;
+    if((ubpe != 0xbe) && (lock_p == 0)) {
+      max_slave_delay_offset_uop =  slave_delay_offset_uop;  
+      max_slave_delay_offset_lop =  slave_delay_offset_lop;
+      lock_p = 1;
+    }
+    if((ubne != 0xde) && (lock_n == 0)) {
+      max_slave_delay_offset_uon =  slave_delay_offset_uon;
+      max_slave_delay_offset_lon =  slave_delay_offset_lon;
+      lock_n = 1;
+    }
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0801);
+    threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0401);
+    threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
+    if(((ubne != 0xde) && (ubpe != 0xbe)) || (threshold_p >= 0x140) || (threshold_n >= 0x140)) {
+      par_done = 1;
+      max_slave_delay_offset_uop = (threshold_p >= 0x140) ? (
+							     (slave_delay_offset_uop < 0x0f) ? threshold_p : slave_delay_offset_uop) : max_slave_delay_offset_uop;
+      max_slave_delay_offset_uon = (threshold_n >= 0x140) ? (
+							     (slave_delay_offset_uon < 0x0f) ? threshold_n : slave_delay_offset_uon) : max_slave_delay_offset_uon;
+      max_slave_delay_offset_lop = (threshold_p >= 0x140) ? (
+							     (slave_delay_offset_lop < 0x0f) ? threshold_p : slave_delay_offset_lop) : max_slave_delay_offset_lop;
+      max_slave_delay_offset_lon = (threshold_n >= 0x140) ? (
+							     (slave_delay_offset_lon < 0x0f) ? threshold_n : slave_delay_offset_lon) : max_slave_delay_offset_lon;
+    }
+
+    if ((ubpe == 0xbe)){
+      slave_delay_offset_uop =  slave_delay_offset_uop + 1;  
+      slave_delay_offset_lop =  slave_delay_offset_lop + 1;
+    }
+    if((ubne == 0xde)) {
+      slave_delay_offset_uon =  slave_delay_offset_uon + 1;
+      slave_delay_offset_lon =  slave_delay_offset_lon + 1;
+    }
+#if defined (ENABLE_NCPRINTF)
+{
+    nc_printf("PAR DP1 PAR ubne :%x ubpe:%x slave_delay_offset_uop:%x slave_delay_offset_uon:%x\n", ubne, ubpe, slave_delay_offset_uop, slave_delay_offset_uon);
+}
+#endif
+  }
+
+#if defined (ENABLE_NCPRINTF)
+{
+  nc_printf("UOP MAX DP1 :%x ubne :%x ubpe:%x slave_delay_offset_uop:%x slave_delay_offset_uon:%x\n", max_slave_delay_offset_uop, ubne, ubpe, slave_delay_offset_uop, slave_delay_offset_uon);
+  nc_printf("LOP MAX DP1 :%x threshold_p:%x threshold_n:%x\n", max_slave_delay_offset_lop, threshold_p, threshold_n);
+  nc_printf("UON MAX DP1 :%x\n", max_slave_delay_offset_uon);
+  nc_printf("LON MAX DP1 :%x\n", max_slave_delay_offset_lon);
+}
+#endif
+
+  lock_p = 0;
+  lock_n = 0;
+  slave_count_n = 0;
+  slave_count_p = 0;
+  slave_delay_offset_uop = 0;
+  slave_delay_offset_lop = 0;
+  slave_delay_offset_uon = 0;
+  slave_delay_offset_lon = 0;
+
+  while(!nar_done) {
+    SET_BIT32(HM_MISC_CTRL, 0);
+    CLEAR_BIT32(HM_MISC_CTRL, 1);
+
+    WRITE_REG32(HM_DPLOADDELAY, (0x8801 << 16) | slave_delay_offset_uop);
+    WRITE_REG32(HM_DPLOADDELAY, (0x8201 << 16) | slave_delay_offset_lop);
+    WRITE_REG32(HM_DPLOADDELAY, (0x8401 << 16) | slave_delay_offset_uon);
+    WRITE_REG32(HM_DPLOADDELAY, (0x8101 << 16) | slave_delay_offset_lon);
+    WRITE_REG32(HM_DPLOADDELAY, (0x0801 << 16));
+#if defined (ENABLE_NCPRINTF)
+{
+    nc_printf("DP1 NAR READ DELAY:%x\n", READ_REG32(HM_DPREADDELAY));
+}
+#endif
+    CLEAR_BIT32(HM_MISC_CTRL, 0);
+    SET_BIT32(HM_MISC_CTRL, 1);
+    temp = READ_REG32(DDRINTFLAG+0x100);
+    ubpe = (temp >> 8) & 0xff;
+    ubne = (temp >> 24) & 0xff;
+    if((ubpe != 0xbe) && (lock_p == 0)) {
+      min_slave_delay_offset_uop =  1024 - slave_delay_offset_uop;  
+      min_slave_delay_offset_lop =  1024 - slave_delay_offset_lop;
+      lock_p = 1;
+    }
+    if((ubne != 0xde) && (lock_n == 0)) {
+      min_slave_delay_offset_uon =  1024 - slave_delay_offset_uon;
+      min_slave_delay_offset_lon =  1024 - slave_delay_offset_lon;
+      lock_n = 1;
+    }
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0801);
+    threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
+    WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0401);
+    threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
+    if(((ubne != 0xde) && (ubpe != 0xbe)) || (threshold_p <= 0x0f) || (threshold_n <= 0x0f)){
+      nar_done = 1;
+      min_slave_delay_offset_uop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_uop;  
+      min_slave_delay_offset_uon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_uon;  
+      min_slave_delay_offset_lop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_lop;  
+      min_slave_delay_offset_lon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_lon;  
+    }
+    if((ubpe == 0xbe)) {
+      slave_count_p = slave_count_p + 1;
+      slave_delay_offset_uop =  1024 - slave_count_p;  
+      slave_delay_offset_lop =  1024 - slave_count_p;
+    }
+    if((ubne == 0xde)) {
+      slave_count_n = slave_count_n + 1;
+      slave_delay_offset_uon =  1024 - slave_count_n;
+      slave_delay_offset_lon =  1024 - slave_count_n;
+    }
+#if defined (ENABLE_NCPRINTF)
+{
+    nc_printf("NAR DP1 NAR ubne :%x ubpe:%x slave_count_n:%x, slave_count_p:%x\n", ubne, ubpe, slave_count_n, slave_count_p);
+}
+#endif
+  }
+#if defined (ENABLE_NCPRINTF)
+{
+  nc_printf("UOP MIN DP1 :%x ubne :%x ubpe:%x slave_count_n:%x, slave_count_p:%x\n", min_slave_delay_offset_uop, ubne, ubpe, slave_count_n, slave_count_p);
+  nc_printf("LOP MIN DP1 :%x threshold_p:%x threshold_n:%x\n", min_slave_delay_offset_lop, threshold_p, threshold_n);
+  nc_printf("UON MIN DP1 :%x\n", min_slave_delay_offset_uon);
+  nc_printf("LON MIN DP1 :%x\n", min_slave_delay_offset_lon);
+}
+#endif
+
+  optimum_uop_1 = (max_slave_delay_offset_uop - min_slave_delay_offset_uop)/2;
+  if (max_slave_delay_offset_uop < min_slave_delay_offset_uop) {
+    optimum_uop_1 = 1024 - (min_slave_delay_offset_uop - max_slave_delay_offset_uop)/2;
+  }
+  optimum_lop_1 = (max_slave_delay_offset_lop - min_slave_delay_offset_lop)/2;
+  if (max_slave_delay_offset_lop < min_slave_delay_offset_lop) {
+    optimum_lop_1 = 1024 - (min_slave_delay_offset_lop - max_slave_delay_offset_lop)/2;
+  }
+  optimum_uon_1 = (max_slave_delay_offset_uon - min_slave_delay_offset_uon)/2;
+  if (max_slave_delay_offset_uon < min_slave_delay_offset_uon) {
+    optimum_uon_1 = 1024 - (min_slave_delay_offset_uon - max_slave_delay_offset_uon)/2;
+  }
+  optimum_lon_1 = (max_slave_delay_offset_lon - min_slave_delay_offset_lon)/2;
+  if (max_slave_delay_offset_lon < min_slave_delay_offset_lon) {
+    optimum_lon_1 = 1024 - (min_slave_delay_offset_lon - max_slave_delay_offset_lon)/2;
+  } 
+  #if defined (HW_ENV)
+  #if defined (ENABLE_NCPRINTF_FEW)
+  DP1Prange = (max_slave_delay_offset_uop + min_slave_delay_offset_uop);
+  DP1Nrange = (max_slave_delay_offset_uon + min_slave_delay_offset_uon);
+  #endif
+  #endif
+  /**************** DP1 End   *******************/
+
+  SET_BIT32(HM_MISC_CTRL, 0);
+  CLEAR_BIT32(HM_MISC_CTRL, 1);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8800 << 16) | optimum_uop_0);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8801 << 16) | optimum_uop_1);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8200 << 16) | optimum_lop_0);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8201 << 16) | optimum_lop_1);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8400 << 16) | optimum_uon_0);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8401 << 16) | optimum_uon_1);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8100 << 16) | optimum_lon_0);
+  WRITE_REG32(HM_DPLOADDELAY, (0x8101 << 16) | optimum_lon_1);
+  WRITE_REG32(HM_DPLOADDELAY, (0x0800 << 16));
+  CLEAR_BIT32(HM_MISC_CTRL, 0);
+  SET_BIT32(HM_MISC_CTRL, 1);
+  temp = READ_REG32(DDRINTFLAG+0x100);
+  if(temp != 0xdeadbeef) 
+    {
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+{
+    nc_printf("!!! ERROR EYE TRAINING FAILED !!!! :%x\n", temp);
+	return;
+}
+#endif
+#endif
+  }
+  else 
+    {
+#if defined (HW_ENV)
+    #if defined (ENABLE_NCPRINTF)
+{
+    nc_printf("**************************\n");
+    WRITE_REG32(HM_DPLOADDELAY, (0x0800 << 16));
+    nc_printf("UOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0200 << 16));
+    nc_printf("LOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0400 << 16));
+    nc_printf("UON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0100 << 16));
+    nc_printf("LON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0801 << 16));
+    nc_printf("UOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0201 << 16));
+    nc_printf("LOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0401 << 16));
+    nc_printf("UON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    WRITE_REG32(HM_DPLOADDELAY, (0x0101 << 16));
+    nc_printf("LON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+    nc_printf("**************************\n");
+}
+    #endif
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF_FEW)
+{
+    nc_printf("EYE TRAINING COMPLETED with DP0 - P range:%d, N range:%d, DP1 - P range:%d, N range:%d\n", DP0Prange, DP0Nrange, DP1Prange, DP1Nrange);
+}
+#endif
+#endif
+#endif
+  }
+return;
+}
+
+void clean_fifo(void){
+  WRITE_REG32(HM_MISC_CTRL, ((READ_REG32(HM_MISC_CTRL) & 0xcfff) | 0x01));
+  CLEAR_BIT32(HM_MISC_CTRL, 1);
+  SET_BIT32(HM_MISC_CTRL, 13);
+  CLEAR_BIT32(HM_MISC_CTRL, 13);
+  CLEAR_BIT32(HM_MISC_CTRL, 0);
+  SET_BIT32(HM_MISC_CTRL, 1);
+}
+void prg_gate(tU32 wdata) {
+  *(tPVU32)(MC_REG_ADDRESS00) |= 0x07; // For memcontroller
+  WRITE_REG32(HM_MISC_CTRL, ((READ_REG32(HM_MISC_CTRL) & 0xcfff) | 0x09));
+  CLEAR_BIT32(HM_MISC_CTRL, 1);
+  *(tPVU32)(HM_PROGGATEON)    = wdata;
+  SET_BIT32(HM_MISC_CTRL, 13);
+  CLEAR_BIT32(HM_MISC_CTRL, 13);
+  CLEAR_BIT32(HM_MISC_CTRL, 0);
+  SET_BIT32(HM_MISC_CTRL, 1);
+}
+
+void prg_eye(tU32 wdata) {
+  *(tPVU32)(MC_REG_ADDRESS00) |= 0x03; 
+  *(tPVU32)(HM_MISC_CTRL)     = (*(tPVU32)(HM_MISC_CTRL) & 0xfffc) | 0x01;
+}
+
+void lsi_phy_fifo_training(void) {
+  tU8 rd_fail, fifo_training_complete;
+  tU32 refresh_counter_val;
+  WRITE_REG32(HM_FIFO_ALIGN, 0x80000000);
+  SET_BIT32(HM_MISC_CTRL, 0);
+  CLEAR_BIT32(HM_MISC_CTRL, 0);
+  WRITE_REG32(HM_FIFO_DELAY_1, 0xff);
+
+  //Refresh Counter value, save the value
+  refresh_counter_val = READ_REG32(MC_REG_ADDRESS18);
+  WRITE_REG32(MC_REG_ADDRESS18, ((refresh_counter_val & 0xfffff000) | 0x10));
+  SET_BIT32(MC_REG_ADDRESS18, 12); //reset the refresh to take new value into account
+  CLEAR_BIT32(MC_REG_ADDRESS18, 12);
+
+  fifo_training_complete = 0;
+  while(fifo_training_complete == 0) 
+    {
+    rd_fail = xdma_lsi_fifo_training();
+    if(rd_fail != 0) {
+	if(READ_REG8(HM_FIFO_DELAY_1) == 0) 
+	  {
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+{
+	nc_printf("!!! ERROR FIFO TRAINING COULD NOT COMPLETE !!!!\n");
+	      return;
+}
+#endif
+#endif
+	return;
+      }
+      
+	if((rd_fail == 1) || (rd_fail == 2) || (rd_fail == 3) || (rd_fail == 4)) 
+	  {
+	if(((READ_REG8(LSI_FIFO_TRAINING) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0x55)) &&
+	   ((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0x55))){
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11)); // Both dps are wrong
+	}
+	else if((READ_REG8(LSI_FIFO_TRAINING) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0x55)) 
+	  {
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x02)); // DP0 is wrong
+	  SET_BIT32(HM_FIFO_ALIGN, 0);
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x10)); // DP1 is correct
+	  fifo_training_complete = 1;
+	}
+	else if((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0x55)) 
+	  {
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x20)); // DP1 is wrong
+	  SET_BIT32(HM_FIFO_ALIGN, 1);
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x01)); // DP0 is correct
+	  fifo_training_complete = 1;	
+	}
+      } 
+	else if((rd_fail == 5) || (rd_fail == 6) || (rd_fail == 7) || (rd_fail == 8))
+	  {
+	if(((READ_REG8(LSI_FIFO_TRAINING) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0xAA)) &&
+	   ((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0xAA))){
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11)); // Both dps are wrong
+	}
+	    else if((READ_REG8(LSI_FIFO_TRAINING) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0xAA)) 
+	      {
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x02)); // DP0 is wrong
+	  SET_BIT32(HM_FIFO_ALIGN, 0);
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x10)); // DP1 is correct
+	  fifo_training_complete = 1;
+	}
+	    else if((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0xAA)) 
+	      {
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x20)); // DP1 is wrong
+	  SET_BIT32(HM_FIFO_ALIGN, 1);
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x01)); // DP0 is correct
+	  fifo_training_complete = 1;
+	}
+      }
+	else
+	  {
+#if defined (HW_ENV)
+
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+{
+	nc_printf("!!! ERROR Fifo Read Failed .. Should not have come to this print.. check code !!!!\n");
+	      return;
+}
+#endif
+#endif
+      }
+    }
+      else
+	{
+      WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11));
+      fifo_training_complete = 1;
+    }
+  }
+#if defined (HW_ENV)
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+{
+      nc_printf("GATEON:%x\n",READ_REG32(0x40300290));
+      nc_printf("READ DELAY:%x\n",READ_REG32(0x40300364));
+      nc_printf("FIFO DELAY:%x\n",READ_REG32(0x40300274));
+}
+      #endif
+      if((READ_REG32(0x40100104) & 0x20000000) != 0x20000000)
+	{
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+{
+      nc_printf("!!! ERROR DDR PLL IS NOT LOCKED !!!!\n");
+	return;
+}
+     #endif 
+	}
+#endif
+#if defined (HW_ENV)
+      WRITE_REG32(MC_REG_ADDRESS18, refresh_counter_val);
+      WRITE_REG32(DDRINTFLAG, 0xabcdef01);
+      if(READ_REG32(DDRINTFLAG) != 0xabcdef01)
+	{
+#if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
+{
+	nc_printf("!!! ERROR FIFO TRAINING FAILED %x !!!!\n", READ_REG32(DDRINTFLAG));
+	return;
+}
+   #endif
+	}
+#if defined (HW_ENV)
+  else 
+    {
+    #if defined (ENABLE_NCPRINTF)
+{
+	nc_printf("LSI FIFO TRAINING COMPLETE\n");
+}
+    #endif
+      }
+#endif
+#endif
+  return;
+}
+
+void  ldma_enable_local(void)
+{
+  *(tPVU32)(0x40900300)|=1;
+}
+
+void ldma_disable_local (void)
+{
+  *(tPVU32)(0x40900300)&=(0xFFFFFFFE);
+}
+
+tU8 xdma_lsi_fifo_training (void) {
+
+  // Enable the LDMA Engine.
+  ldma_enable_local();
+
+  // Set the ARM mode.
+  *(tPVU32)(XDMACTL) |= ARM_MODE;        
+
+  // Program the data in lmem.
+  *(tPVU32)(LSI_FIFO_TRAINING + 0x0) = 0x55555555;
+  *(tPVU32)(LSI_FIFO_TRAINING + 0x4) = 0x55555555;
+  *(tPVU32)(LSI_FIFO_TRAINING + 0x8) = 0x55555555;
+  *(tPVU32)(LSI_FIFO_TRAINING + 0xc) = 0x55555555;
+  *(tPVU32)(LSI_FIFO_TRAINING + 0x10) = 0xAAAAAAAA;
+  *(tPVU32)(LSI_FIFO_TRAINING + 0x14) = 0xAAAAAAAA;
+  *(tPVU32)(LSI_FIFO_TRAINING + 0x18) = 0xAAAAAAAA;
+  *(tPVU32)(LSI_FIFO_TRAINING + 0x1c) = 0xAAAAAAAA;
+
+  // Program both the descriptors.
+  *(tPVU32)(XDMAAD0) = ((32-1)<< 8) | LDMA_DSTDDR | LDMA_SRCLMEM;        
+  *(tPVU32)(XDMAAD1) = LSI_FIFO_TRAINING;
+  *(tPVU32)(XDMAAD2) = DDRINTFLAG;        
+  *(tPVU32)(XDMAAD3) = 0;        
+
+  // Wait for first descriptor to finish.
+  while (!((*(tPVU32)(XDMASTS)) & ARM_DPTR_DONE));
+  *(tPVU32)(XDMASTS) |= ARM_DPTR_DONE;
+
+  CLEAR_BIT32(MC_REG_ADDRESS18, 13); //disable refresh
+ 
+  *(tPVU32)(XDMAAD0) = ((32-1)<< 8) | LDMA_DSTLMEM | LDMA_SRCDDR;        
+  *(tPVU32)(XDMAAD1) = DDRINTFLAG;        
+  *(tPVU32)(XDMAAD2) = LSI_FIFO_TRAINING + 0x100;
+  *(tPVU32)(XDMAAD3) = 0;        
+
+  // Wait for second descriptor to finish.
+  while (!((*(tPVU32)(XDMASTS)) & ARM_DPTR_DONE));
+  *(tPVU32)(XDMASTS) |= ARM_DPTR_DONE;
+
+  SET_BIT32(MC_REG_ADDRESS18, 13); //enable refresh
+
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x100) != *(tPVU32)(LSI_FIFO_TRAINING + 0x00)) {
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x100);
+    return 1;
+  }
+
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x104) != *(tPVU32)(LSI_FIFO_TRAINING + 0x04)) { 
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x104);
+    return 2;
+  }
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x108) != *(tPVU32)(LSI_FIFO_TRAINING + 0x08)) { 
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x108);
+    return 3;
+  }
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x10c) != *(tPVU32)(LSI_FIFO_TRAINING + 0x0c)) { 
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x10c);
+    return 4;
+  }
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x110) != *(tPVU32)(LSI_FIFO_TRAINING + 0x10)) { 
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x110);
+    return 5;
+  }
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x114) != *(tPVU32)(LSI_FIFO_TRAINING + 0x14)) { 
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x114);
+    return 6;
+  }
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x118) != *(tPVU32)(LSI_FIFO_TRAINING + 0x18)) { 
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x118);
+    return 7;
+  }
+  if (*(tPVU32)(LSI_FIFO_TRAINING + 0x11c) != *(tPVU32)(LSI_FIFO_TRAINING + 0x1c)) { 
+    *(tPVU32)(LSI_FIFO_TRAINING) = *(tPVU32)(LSI_FIFO_TRAINING + 0x11c);
+    return 8;
+  }
+
+  *(tPVU32)(XDMACTL) &= ~ARM_MODE;
+  ldma_disable_local ();
+
+  return 0;
+}
