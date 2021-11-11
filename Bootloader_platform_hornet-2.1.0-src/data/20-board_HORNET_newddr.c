--- olduboot/board/HORNET/ddr_asic.c	2011-09-22 15:36:43.000000000 +0800
+++ uboot/board/HORNET/ddr_asic.c	2011-09-22 15:32:33.000000000 +0800
@@ -1,28 +1,32 @@
-//////////////////////////////////////////////////
-//#ServerEngines Public License
-//#This license governs use of the accompanying software. If you use the software, you
-//#accept this license. If you do not accept the license, do not use the software.
-//#
-//#1. Definitions
-//#The terms "reproduce," "reproduction," "derivative works," and "distribution" have the
-//#same meaning here as under U.S. copyright law.
-//#A "contribution" is the original software, or any additions or changes to the software.
-//#A "contributor" is any person that distributes its contribution under this license.
-//#"Licensed patents" are a contributor's patent claims that read directly on its contribution.
-//#
-//##2. Grant of Rights
-//###(A) Copyright Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free copyright license to reproduce its contribution, prepare derivative works of its contribution, and distribute its contribution or any derivative works that you create.
-//##(B) Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the contribution in the software.
-//##
-//##3. Conditions and Limitations
-//##(A) No Trademark License- This license does not grant you rights to use any contributors' name, logo, or trademarks.
-//##(B) If you bring a patent claim against any contributor over patents that you claim are infringed by the software, your patent license from such contributor to the software ends automatically.
-//#(C) If you distribute any portion of the software, you must retain all copyright, patent, trademark, and attribution notices that are present in the software.
-//#(D) If you distribute any portion of the software in source code form, you may do so only under this license by including a complete copy of this license with your distribution. If you distribute any portion of the software in compiled or object code form, you may only do so under a license that complies with this license.
-//#(E) The software is licensed "as-is." You bear the risk of using it. The contributors give no express warranties, guarantees or conditions. You may have additional consumer rights under your local laws which this license cannot change. To the extent permitted under your local laws, the contributors exclude the implied warranties of merchantability, fitness for a particular purpose and non-infringement.
-//##
-//###       Project  : Pilot
+/*
+Emulex Public License
+This license governs use of the accompanying software. If you use the software, you
+accept this license. If you do not accept the license, do not use the software.
+
+1. Definitions
+The terms "reproduce," "reproduction," "derivative works," and "distribution" have the
+same meaning here as under U.S. copyright law.
+A "contribution" is the original software, or any additions or changes to the software.
+A "contributor" is any person that distributes its contribution under this license.
+"Licensed patents" are a contributor's patent claims that read directly on its contribution.
+
+2. Grant of Rights
+(A) Copyright Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free copyright license to reproduce its contribution, prepare derivative works of its contribution, and distribute its contribution or any derivative works that you create.
+(B) Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the contribution in the software.
+
+3. Conditions and Limitations
+(A) No Trademark License- This license does not grant you rights to use any contributors' name, logo, or trademarks.
+(B) If you bring a patent claim against any contributor over patents that you claim are infringed by the software, your patent license from such contributor to the software ends automatically.
+(C) If you distribute any portion of the software, you must retain all copyright, patent, trademark, and attribution notices that are present in the software.
+(D) If you distribute any portion of the software in source code form, you may do so only under this license by including a complete copy of this license with your distribution. If you distribute any portion of the software in compiled or object code form, you may only do so under a license that complies with this license.
+(E) The software is licensed "as-is." You bear the risk of using it. The contributors give no express warranties, guarantees or conditions. You may have additional consumer rights under your local laws which this license cannot change. To the extent permitted under your local laws, the contributors exclude the implied warranties of merchantability, fitness for a particular purpose and non-infringement.
+*/
+
+
 // ********************************************************************************
+ // This routine has been modified to support 1 x16 device only so should be left 
+// untouched for TYPE's.
+// ddrsize_calc added to calculate the type automatically assuming only 1 x16 device.
 
 
 //#include "pilot_II.h"
@@ -36,14 +40,29 @@
 tU8 xdma_lsi_fifo_training (void); 
 
 
+//Out ddr init automatically takes care of all the MEMORY TYPES,so do not change any of the following defines
 #if defined TYPE1
 #elif defined TYPE2
 #elif defined TYPE3
 #elif defined TYPE4
 #elif defined TYPE5 
 #else
-#define TYPE0 1
+#define TYPE4 1
 #endif
+
+//#define ENABLE_NCPRINTF 1
+//#define ENABLE_NCPRINTF_FEW  1
+
+//#define DDR23_350_400MHZ 0
+//#define DDR23_300_350MHZ 1
+//#define DDR23_250_300MHZ 0
+//#define DDR23_200_250MHZ 0
+//#define DDR23_150_200MHZ 0
+//#define DDR23_125_150MHZ 0
+//#define DDR23_75_125MHZ 0
+//#define DDR23_25_75MHZ  0
+
+
 void DELAY(tU32 count)
 {
   tU32 i=0;
@@ -51,6 +70,85 @@
     ;
 }// DELAY
 
+void ddrsize_calc(void){
+  tU32 temp, temp1;
+
+  *(tPVU32)(DDRBASE) = 0x0; // Write at 0.
+  *(tPVU32)(DDRBASE+0x08000000) = 0x0; // Write at 128MB+1.
+  *(tPVU32)(DDRBASE+0x10000000) = 0x0; // Write at 256MB+1.
+  *(tPVU32)(DDRBASE+0x10000000) = 0xdeaddead; // Write at 256MB+1.
+  *(tPVU32)(DDRBASE) = 0xcafecafe; // Write at 0.
+  if (*(tPVU32)(DDRBASE+0x10000000) == 0xdeaddead) {
+    // Type is 4 (512MB).
+    // Already programmed as Type 4.
+  } 
+  else {
+    *(tPVU32)(DDRBASE+0x08000000) = 0xdadadada; // Write at 128MB+1.
+    *(tPVU32)(DDRBASE) = 0xcafecafe; // Write at 0.
+    if (*(tPVU32)(DDRBASE+0x08000000) == 0xdadadada) {
+      // Type is 2 (256MB).
+      temp1 = *(tPVU32)(MC_REG_ADDRESS04);
+      temp1 = temp1 & ~0x0e000000;
+      temp1 = temp1 | 0x04000000;
+      *(tPVU32)(MC_REG_ADDRESS04) = temp1;
+      temp = *(tPVU32)(MC_REG_ADDRESS04);
+      temp = temp & ~0x01fe00ff;
+      // Programming tRFC, r_trfcplus10counter, r_ref2actcounter.
+#if defined DDR23_350_400MHZ
+      temp = temp | 0x003c0020;
+#elif defined DDR23_300_350MHZ
+      temp = temp | 0x0036001d;
+#elif defined DDR23_250_300MHZ
+      temp = temp | 0x002e0019;
+#elif defined DDR23_200_250MHZ
+      temp = temp | 0x00240014;
+#elif defined DDR23_150_200MHZ
+      temp = temp | 0x001c000f;
+#elif defined DDR23_125_150MHZ
+      temp = temp | 0x0016000c;
+#elif defined DDR23_75_125MHZ
+      temp = temp | 0x00100009;
+#elif defined DDR23_25_75MHZ
+      temp = temp | 0x000a0006;
+#else
+      temp = temp | 0x003c0020;
+#endif
+    }
+    else {
+      // Type is 0 (128MB).
+      temp1 = *(tPVU32)(MC_REG_ADDRESS04);
+      temp1 = temp1 & ~0x0e000000;
+      *(tPVU32)(MC_REG_ADDRESS04) = temp1;
+      temp = *(tPVU32)(MC_REG_ADDRESS04);
+      temp = temp & ~0x01fe00ff;
+      // Programming tRFC, r_trfcplus10counter, r_ref2actcounter.
+#if defined DDR23_350_400MHZ
+      temp = temp | 0x00280016;
+#elif defined DDR23_300_350MHZ
+      temp = temp | 0x00240014;
+#elif defined DDR23_250_300MHZ
+      temp = temp | 0x001e0011;
+#elif defined DDR23_200_250MHZ
+      temp = temp | 0x0018000e;
+#elif defined DDR23_150_200MHZ
+      temp = temp | 0x0012000a;
+#elif defined DDR23_125_150MHZ
+      temp = temp | 0x000e0008;
+#elif defined DDR23_75_125MHZ
+      temp = temp | 0x000a0006;
+#elif defined DDR23_25_75MHZ
+      temp = temp | 0x00060004;
+#else
+      temp = temp | 0x00280016;
+#endif
+    }
+    *(tPVU32)(MC_REG_ADDRESS04) = temp;
+  }
+}
+
+
+
+
 tU32 ddrinit_asic(void){
   tU32 temp/*, ddr*/;
   //tU32 temp1;
@@ -152,6 +250,8 @@
   temp = temp | 0x20000000; // ahb2ddr_pads_reset_n = 1. To mask off the reset from the PHY going to I/Os.
   *(tPVU32)(MC_REG_ADDRESS04) = temp;
 
+  ddrsize_calc();
+
   // ECC Mode.
 #if defined ECC
   *(tPVU32)(MC_REG_ADDRESS58) = LOWER_ECC_ADDRESS;
@@ -829,13 +929,14 @@
     temp = temp | 0x00000004; // DDR3 out of reset
 #endif
     *(tPVU32)(MC_REG_ADDRESS00) = temp;
-    DELAY(100);
+    //DELAY(100);
+    DELAY(100*100);
   }
 
   // AllowCke.
   temp = *(tPVU32)(MC_REG_ADDRESS00);
   temp = temp & ~0xffffffc7;
-  temp = temp | 0x00000400;
+  temp = temp | 0x00000404;
   *(tPVU32)(MC_REG_ADDRESS00) = temp;
 
   if (ddr_type == 2) {
@@ -1238,7 +1339,7 @@
   }
 }
 
-void redeye_training(void){
+int redeye_training(void){
   tVU32 temp, slave_count_p, slave_count_n;
   tVU8 lbpe, lbne, ubpe, ubne;
   tVU8 par_done = 0;
@@ -1647,48 +1748,48 @@
     {
 #if defined (HW_ENV)
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
-{
-    nc_printf("!!! ERROR EYE TRAINING FAILED !!!! :%x\n", temp);
-	return;
-}
+      {
+	nc_printf("!!! ERROR EYE TRAINING FAILED !!!! :%x\n", temp);
+	return 1;
+      }
 #endif
 #endif
-  }
+    }
   else 
     {
 #if defined (HW_ENV)
-    #if defined (ENABLE_NCPRINTF)
-{
-    nc_printf("**************************\n");
-    WRITE_REG32(HM_DPLOADDELAY, (0x0800 << 16));
-    nc_printf("UOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
-    WRITE_REG32(HM_DPLOADDELAY, (0x0200 << 16));
-    nc_printf("LOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
-    WRITE_REG32(HM_DPLOADDELAY, (0x0400 << 16));
-    nc_printf("UON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
-    WRITE_REG32(HM_DPLOADDELAY, (0x0100 << 16));
-    nc_printf("LON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
-    WRITE_REG32(HM_DPLOADDELAY, (0x0801 << 16));
-    nc_printf("UOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
-    WRITE_REG32(HM_DPLOADDELAY, (0x0201 << 16));
-    nc_printf("LOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
-    WRITE_REG32(HM_DPLOADDELAY, (0x0401 << 16));
-    nc_printf("UON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
-    WRITE_REG32(HM_DPLOADDELAY, (0x0101 << 16));
-    nc_printf("LON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
-    nc_printf("**************************\n");
-}
+#if defined (ENABLE_NCPRINTF)
+      {
+	nc_printf("**************************\n");
+	WRITE_REG32(HM_DPLOADDELAY, (0x0800 << 16));
+	nc_printf("UOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+	WRITE_REG32(HM_DPLOADDELAY, (0x0200 << 16));
+	nc_printf("LOP OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+	WRITE_REG32(HM_DPLOADDELAY, (0x0400 << 16));
+	nc_printf("UON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+	WRITE_REG32(HM_DPLOADDELAY, (0x0100 << 16));
+	nc_printf("LON OPTIMUM DP0 :%x\n", READ_REG16(HM_DPREADDELAY));
+	WRITE_REG32(HM_DPLOADDELAY, (0x0801 << 16));
+	nc_printf("UOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+	WRITE_REG32(HM_DPLOADDELAY, (0x0201 << 16));
+	nc_printf("LOP OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+	WRITE_REG32(HM_DPLOADDELAY, (0x0401 << 16));
+	nc_printf("UON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+	WRITE_REG32(HM_DPLOADDELAY, (0x0101 << 16));
+	nc_printf("LON OPTIMUM DP1 :%x\n", READ_REG16(HM_DPREADDELAY+0x02));
+	nc_printf("**************************\n");
+      }
     #endif
 #if defined (HW_ENV)
 #if defined (ENABLE_NCPRINTF_FEW)
-{
-    nc_printf("EYE TRAINING COMPLETED with DP0 - P range:%d, N range:%d, DP1 - P range:%d, N range:%d\n", DP0Prange, DP0Nrange, DP1Prange, DP1Nrange);
-}
+      {
+	nc_printf("EYE TRAINING COMPLETED with DP0 - P range:%d, N range:%d, DP1 - P range:%d, N range:%d\n", DP0Prange, DP0Nrange, DP1Prange, DP1Nrange);
+      }
 #endif
 #endif
 #endif
-  }
-return;
+    }
+return 0;
 }
 
 void clean_fifo(void){
@@ -1715,143 +1816,145 @@
   *(tPVU32)(HM_MISC_CTRL)     = (*(tPVU32)(HM_MISC_CTRL) & 0xfffc) | 0x01;
 }
 
-void lsi_phy_fifo_training(void) {
+int lsi_phy_fifo_training(void) 
+{
   tU8 rd_fail, fifo_training_complete;
-  tU32 refresh_counter_val;
+  tU32  refresh_counter_val;
   WRITE_REG32(HM_FIFO_ALIGN, 0x80000000);
   SET_BIT32(HM_MISC_CTRL, 0);
   CLEAR_BIT32(HM_MISC_CTRL, 0);
   WRITE_REG32(HM_FIFO_DELAY_1, 0xff);
-
+  
   //Refresh Counter value, save the value
   refresh_counter_val = READ_REG32(MC_REG_ADDRESS18);
   WRITE_REG32(MC_REG_ADDRESS18, ((refresh_counter_val & 0xfffff000) | 0x10));
   SET_BIT32(MC_REG_ADDRESS18, 12); //reset the refresh to take new value into account
   CLEAR_BIT32(MC_REG_ADDRESS18, 12);
-
+  
   fifo_training_complete = 0;
   while(fifo_training_complete == 0) 
     {
-    rd_fail = xdma_lsi_fifo_training();
-    if(rd_fail != 0) {
+      rd_fail = xdma_lsi_fifo_training();
+      if(rd_fail != 0) {
 	if(READ_REG8(HM_FIFO_DELAY_1) == 0) 
 	  {
 #if defined (HW_ENV)
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
-{
-	nc_printf("!!! ERROR FIFO TRAINING COULD NOT COMPLETE !!!!\n");
-	      return;
-}
+	    {
+	      nc_printf("!!! ERROR FIFO TRAINING COULD NOT COMPLETE !!!!\n");
+	      return 1;
+	    }
 #endif
 #endif
-	return;
-      }
-      
+	    return 1;
+	  }
+	
 	if((rd_fail == 1) || (rd_fail == 2) || (rd_fail == 3) || (rd_fail == 4)) 
 	  {
-	if(((READ_REG8(LSI_FIFO_TRAINING) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0x55)) &&
-	   ((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0x55))){
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11)); // Both dps are wrong
-	}
+	    if(((READ_REG8(LSI_FIFO_TRAINING) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0x55)) &&
+	       ((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0x55))){
+	      WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11)); // Both dps are wrong
+	    }
 	else if((READ_REG8(LSI_FIFO_TRAINING) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0x55)) 
 	  {
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x02)); // DP0 is wrong
-	  SET_BIT32(HM_FIFO_ALIGN, 0);
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x10)); // DP1 is correct
-	  fifo_training_complete = 1;
-	}
+	    WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x02)); // DP0 is wrong
+	    SET_BIT32(HM_FIFO_ALIGN, 0);
+	    WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x10)); // DP1 is correct
+	    fifo_training_complete = 1;
+	  }
 	else if((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0x55) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0x55)) 
 	  {
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x20)); // DP1 is wrong
-	  SET_BIT32(HM_FIFO_ALIGN, 1);
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x01)); // DP0 is correct
-	  fifo_training_complete = 1;	
-	}
-      } 
+	    WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x20)); // DP1 is wrong
+	    SET_BIT32(HM_FIFO_ALIGN, 1);
+	    WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x01)); // DP0 is correct
+	    fifo_training_complete = 1;	
+	  }
+	  } 
 	else if((rd_fail == 5) || (rd_fail == 6) || (rd_fail == 7) || (rd_fail == 8))
 	  {
-	if(((READ_REG8(LSI_FIFO_TRAINING) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0xAA)) &&
-	   ((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0xAA))){
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11)); // Both dps are wrong
-	}
+	    if(((READ_REG8(LSI_FIFO_TRAINING) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0xAA)) &&
+	       ((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0xAA))){
+	      WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11)); // Both dps are wrong
+	    }
 	    else if((READ_REG8(LSI_FIFO_TRAINING) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x02) != 0xAA)) 
 	      {
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x02)); // DP0 is wrong
-	  SET_BIT32(HM_FIFO_ALIGN, 0);
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x10)); // DP1 is correct
-	  fifo_training_complete = 1;
-	}
+		WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x02)); // DP0 is wrong
+		SET_BIT32(HM_FIFO_ALIGN, 0);
+		WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x10)); // DP1 is correct
+		fifo_training_complete = 1;
+	      }
 	    else if((READ_REG8(LSI_FIFO_TRAINING+0x01) != 0xAA) || (READ_REG8(LSI_FIFO_TRAINING+0x03) != 0xAA)) 
 	      {
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x20)); // DP1 is wrong
-	  SET_BIT32(HM_FIFO_ALIGN, 1);
-	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x01)); // DP0 is correct
-	  fifo_training_complete = 1;
-	}
-      }
+		WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x20)); // DP1 is wrong
+		SET_BIT32(HM_FIFO_ALIGN, 1);
+		WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x01)); // DP0 is correct
+		fifo_training_complete = 1;
+	      }
+	  }
 	else
 	  {
 #if defined (HW_ENV)
-
+	    
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
-{
-	nc_printf("!!! ERROR Fifo Read Failed .. Should not have come to this print.. check code !!!!\n");
-	      return;
-}
+	    {
+	      nc_printf("!!! ERROR Fifo Read Failed .. Should not have come to this print.. check code !!!!\n");
+	      return 1;
+	    }
 #endif
 #endif
+	  }
       }
-    }
       else
 	{
-      WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11));
-      fifo_training_complete = 1;
+	  WRITE_REG32(HM_FIFO_DELAY_1, (READ_REG32(HM_FIFO_DELAY_1) - 0x11));
+	  fifo_training_complete = 1;
+	}
     }
-  }
 #if defined (HW_ENV)
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
-{
-      nc_printf("GATEON:%x\n",READ_REG32(0x40300290));
-      nc_printf("READ DELAY:%x\n",READ_REG32(0x40300364));
-      nc_printf("FIFO DELAY:%x\n",READ_REG32(0x40300274));
-}
-      #endif
-      if((READ_REG32(0x40100104) & 0x20000000) != 0x20000000)
-	{
+  {
+    nc_printf("GATEON:%x\n",READ_REG32(0x40300290));
+    nc_printf("READ DELAY:%x\n",READ_REG32(0x40300364));
+    nc_printf("FIFO DELAY:%x\n",READ_REG32(0x40300274));
+  }
+#endif
+  if((READ_REG32(0x40100104) & 0x20000000) != 0x20000000)
+    {
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
-{
-      nc_printf("!!! ERROR DDR PLL IS NOT LOCKED !!!!\n");
-	return;
-}
-     #endif 
-	}
+      {
+	nc_printf("!!! ERROR DDR PLL IS NOT LOCKED !!!!\n");
+	return 1;
+      }
+#endif 
+    }
 #endif
 #if defined (HW_ENV)
-      WRITE_REG32(MC_REG_ADDRESS18, refresh_counter_val);
-      WRITE_REG32(DDRINTFLAG, 0xabcdef01);
-      if(READ_REG32(DDRINTFLAG) != 0xabcdef01)
-	{
+  WRITE_REG32(MC_REG_ADDRESS18, refresh_counter_val);
+  WRITE_REG32(DDRINTFLAG, 0xabcdef01);
+  if(READ_REG32(DDRINTFLAG) != 0xabcdef01)
+    {
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
-{
+      {
 	nc_printf("!!! ERROR FIFO TRAINING FAILED %x !!!!\n", READ_REG32(DDRINTFLAG));
-	return;
-}
-   #endif
-	}
+	return 1;
+      }
+#endif
+    }
 #if defined (HW_ENV)
   else 
     {
-    #if defined (ENABLE_NCPRINTF)
-{
+#if defined (ENABLE_NCPRINTF)
+      {
 	nc_printf("LSI FIFO TRAINING COMPLETE\n");
-}
-    #endif
       }
 #endif
+    }
 #endif
-  return;
+#endif
+  return 0;
 }
 
+
 void  ldma_enable_local(void)
 {
   *(tPVU32)(0x40900300)|=1;
