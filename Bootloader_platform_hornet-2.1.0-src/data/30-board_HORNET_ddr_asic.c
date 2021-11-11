--- uboot.org/board/HORNET/ddr_asic.c Tue Dec 24 18:10:03 2013
+++ uboot/board/HORNET/ddr_asic.c Tue Dec 24 18:14:29 2013
@@ -23,12 +23,16 @@
 */
 
 
-// ********************************************************************************
- // This routine has been modified to support 1 x16 device only so should be left 
+// Variables : - LOWER_ECC_ADDRESS, UPPER_ECC_ADDRESS, MAGM_VALUE, UAGM_VALUE.
+// Compilation options :- TYPE0, TYPE1, TYPE2, TYPE3, TYPE4, TYPE5, ECC, DLL_OFF, BZ_OVERRIDE.
+//                      - DDR23_350_400MHZ, DDR23_300_350MHZ, DDR23_250_300MHZ,
+//                        DDR23_200_250MHZ, DDR23_150_200MHZ, DDR23_125_150MHZ, 
+//                        DDR23_75_125MHZ, DDR23_25_75MHZ.
+// This routine has been modified to support 1 x16 device only so should be left 
 // untouched for TYPE's.
 // ddrsize_calc added to calculate the type automatically assuming only 1 x16 device.
 
-
+// ********************************************************************************
 //#include "pilot_II.h"
 #define HW_ENV  1
 
@@ -153,7 +157,7 @@
 
 tU32 ddrinit_asic(void){
   tU32 temp/*, ddr*/;
-  //tU32 temp1;
+  tU32 temp1;
 
   *(tPVU32)(SYS_MCSS_CLK_CTRL) = 0x00; 
   temp = *(tPVU32)(MC_REG_ADDRESS04);
@@ -183,8 +187,7 @@
 
   *(tPVU32)(MC_REG_ADDRESS18) = (*(tPVU32)(MC_REG_ADDRESS18) & 0xff9fffff) | 0x00200000;
   while((READ_REG32(MC_REG_ADDRESS4C) & 0x00000001) != 0x00000001);
-  //temp1 = READ_REG32(MC_REG_ADDRESS4C);
-  READ_REG32(MC_REG_ADDRESS4C);
+  temp1 = READ_REG32(MC_REG_ADDRESS4C);
 #if defined (ENABLE_NCPRINTF)
 	{
   nc_printf("*****************************************\n");
@@ -231,6 +234,7 @@
 #endif
   if(lsi_phy_fifo_training())
     return 1;
+
   WRITE_REG32(HM_TRAINREQ, 0);
   
   // OPTION4
@@ -255,6 +259,7 @@
   temp = temp | 0x20000000; // ahb2ddr_pads_reset_n = 1. To mask off the reset from the PHY going to I/Os.
   *(tPVU32)(MC_REG_ADDRESS04) = temp;
 
+  // Calling the automatic type detection routine.
   ddrsize_calc();
 
   // ECC Mode.
@@ -1358,17 +1363,13 @@
   tVU16 max_slave_delay_offset_uop = 0, max_slave_delay_offset_lop = 0, max_slave_delay_offset_uon = 0,max_slave_delay_offset_lon = 0;
   tVU16 min_slave_delay_offset_uop = 0, min_slave_delay_offset_lop = 0, min_slave_delay_offset_uon = 0,min_slave_delay_offset_lon = 0;
   tVU16 threshold_p,threshold_n;
-  #if defined (HW_ENV)
-  #if defined (ENABLE_NCPRINTF_FEW)
   tVU16 DP0Prange, DP0Nrange, DP1Prange, DP1Nrange;
-  #endif
-  #endif
  
   *(tPVU32)(DDRINTFLAG+0x100) = 0xa5a55a5a;
   *(tPVU32)(DDRINTFLAG+0x104) = 0xa5a55a5a;
   temp = READ_REG32(DDRINTFLAG+0x100);
   temp = READ_REG32(DDRINTFLAG+0x104);
-  
+
 #if defined (ENABLE_NCPRINTF)
 	{
     nc_printf("**************************\n");
@@ -1561,12 +1562,8 @@
   if (max_slave_delay_offset_lon < min_slave_delay_offset_lon) {
     optimum_lon_0 = 1024 - (min_slave_delay_offset_lon - max_slave_delay_offset_lon)/2;
   } 
-  #if defined (HW_ENV)
-  #if defined (ENABLE_NCPRINTF_FEW)
   DP0Prange = (max_slave_delay_offset_uop + min_slave_delay_offset_uop);
   DP0Nrange = (max_slave_delay_offset_uon + min_slave_delay_offset_uon);
-  #endif
-  #endif
   /**************** DP0 End   *******************/
 
   /**************** DP1 Start *******************/
@@ -1626,7 +1623,6 @@
     if((lock_p == 1) && (lock_n == 1)){
       par_done = 1;
     }
-
     if ((ubpe == 0x5a)){
       slave_delay_offset_uop =  slave_delay_offset_uop + 1;  
       slave_delay_offset_lop =  slave_delay_offset_lop + 1;
@@ -1747,12 +1743,8 @@
   if (max_slave_delay_offset_lon < min_slave_delay_offset_lon) {
     optimum_lon_1 = 1024 - (min_slave_delay_offset_lon - max_slave_delay_offset_lon)/2;
   } 
-  #if defined (HW_ENV)
-  #if defined (ENABLE_NCPRINTF_FEW)
   DP1Prange = (max_slave_delay_offset_uop + min_slave_delay_offset_uop);
   DP1Nrange = (max_slave_delay_offset_uon + min_slave_delay_offset_uon);
-  #endif
-  #endif
   /**************** DP1 End   *******************/
 
   SET_BIT32(HM_MISC_CTRL, 0);
@@ -1918,7 +1910,6 @@
 	else
 	  {
 #if defined (HW_ENV)
-	    
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
 	    {
 	      nc_printf("!!! ERROR Fifo Read Failed .. Should not have come to this print.. check code !!!!\n");
