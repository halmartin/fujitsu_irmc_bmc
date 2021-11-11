--- uboot.old/board/HORNET/ddr_asic.c	2011-11-25 17:43:20.405484484 +0530
+++ uboot/board/HORNET/ddr_asic.c	2011-11-25 18:11:51.430666000 +0530
@@ -78,6 +78,7 @@
   *(tPVU32)(DDRBASE+0x10000000) = 0x0; // Write at 256MB+1.
   *(tPVU32)(DDRBASE+0x10000000) = 0xdeaddead; // Write at 256MB+1.
   *(tPVU32)(DDRBASE) = 0xcafecafe; // Write at 0.
+  temp = *(tPVU32)(DDRBASE);
   if (*(tPVU32)(DDRBASE+0x10000000) == 0xdeaddead) {
     // Type is 4 (512MB).
     // Already programmed as Type 4.
@@ -85,6 +86,7 @@
   else {
     *(tPVU32)(DDRBASE+0x08000000) = 0xdadadada; // Write at 128MB+1.
     *(tPVU32)(DDRBASE) = 0xcafecafe; // Write at 0.
+    temp = *(tPVU32)(DDRBASE);    
     if (*(tPVU32)(DDRBASE+0x08000000) == 0xdadadada) {
       // Type is 2 (256MB).
       temp1 = *(tPVU32)(MC_REG_ADDRESS04);
@@ -221,11 +223,14 @@
 
   *(tPVU32)(DDRINTFLAG) = 0xa55ab55b;
   *(tPVU32)(DDRINTFLAG + 0x04) = 0xc3d25aa5;
-  temp = gateon_training();
+  if(gateon_training())
+    return 1;
 #if defined (HW_ENV)
-  redeye_training();
+  if(redeye_training())
+    return 1;
 #endif
-  lsi_phy_fifo_training();
+  if(lsi_phy_fifo_training())
+    return 1;
   WRITE_REG32(HM_TRAINREQ, 0);
   
   // OPTION4
@@ -1269,15 +1274,16 @@
 	i = i + 1;
       }
     }
-#if defined (HW_ENV)
     else {
+#if defined (HW_ENV)
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
 	{
       nc_printf("!!! ERROR Should have not come here :%d !!!!!\n",i);
        }
 #endif
-    }
 #endif
+	return (1);
+    }
   }
   training_done = 0;
   i = 0;
@@ -1358,7 +1364,10 @@
   #endif
   #endif
  
-  *(tPVU32)(DDRINTFLAG+0x100) = 0xdeadbeef;
+  *(tPVU32)(DDRINTFLAG+0x100) = 0xa5a55a5a;
+  *(tPVU32)(DDRINTFLAG+0x104) = 0xa5a55a5a;
+  temp = READ_REG32(DDRINTFLAG+0x100);
+  temp = READ_REG32(DDRINTFLAG+0x104);
   
 #if defined (ENABLE_NCPRINTF)
 	{
@@ -1405,12 +1414,12 @@
     temp = READ_REG32(DDRINTFLAG+0x100);
     lbpe = temp & 0xff;
     lbne = (temp >> 16) & 0xff;
-    if((lbpe != 0xef) && (lock_p == 0)) {
+    if((lbpe != 0x5a) && (lock_p == 0)) {
       max_slave_delay_offset_uop =  slave_delay_offset_uop;  
       max_slave_delay_offset_lop =  slave_delay_offset_lop;
       lock_p = 1;
     }
-    if((lbne != 0xad) && (lock_n == 0)) {
+    if((lbne != 0xa5) && (lock_n == 0)) {
       max_slave_delay_offset_uon =  slave_delay_offset_uon;
       max_slave_delay_offset_lon =  slave_delay_offset_lon;
       lock_n = 1;
@@ -1419,22 +1428,24 @@
     threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY);
     WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0400);
     threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY);
-    if(((lbne != 0xad) && (lbpe != 0xef)) || (threshold_p >= 0x140) || (threshold_n >= 0x140)) {
+    if (threshold_p >= 0x140) {
+	max_slave_delay_offset_uop = (slave_delay_offset_uop < 0x0f) ? threshold_p : slave_delay_offset_uop;
+	max_slave_delay_offset_lop = (slave_delay_offset_lop < 0x0f) ? threshold_p : slave_delay_offset_lop;
+	lock_p = 1;
+    }
+    if (threshold_n >= 0x140) {
+	max_slave_delay_offset_uon = (slave_delay_offset_uon < 0x0f) ? threshold_n : slave_delay_offset_uon;
+	max_slave_delay_offset_lon = (slave_delay_offset_lon < 0x0f) ? threshold_n : slave_delay_offset_lon; 
+	lock_n = 1;
+    }
+    if((lock_p == 1) && (lock_n == 1)){
       par_done = 1;
-      max_slave_delay_offset_uop = (threshold_p >= 0x140) ? (
-							     (slave_delay_offset_uop < 0x0f) ? threshold_p : slave_delay_offset_uop) : max_slave_delay_offset_uop;
-      max_slave_delay_offset_uon = (threshold_n >= 0x140) ? (
-							     (slave_delay_offset_uon < 0x0f) ? threshold_n : slave_delay_offset_uon) : max_slave_delay_offset_uon;
-      max_slave_delay_offset_lop = (threshold_p >= 0x140) ? (
-							     (slave_delay_offset_lop < 0x0f) ? threshold_p : slave_delay_offset_lop) : max_slave_delay_offset_lop;
-      max_slave_delay_offset_lon = (threshold_n >= 0x140) ? (
-							     (slave_delay_offset_lon < 0x0f) ? threshold_n : slave_delay_offset_lon) : max_slave_delay_offset_lon;
     }
-    if ((lbpe == 0xef)){
+    if ((lbpe == 0x5a)){
       slave_delay_offset_uop =  slave_delay_offset_uop + 1;  
       slave_delay_offset_lop =  slave_delay_offset_lop + 1;
     }
-    if((lbne == 0xad)) {
+    if((lbne == 0xa5)) {
       slave_delay_offset_uon =  slave_delay_offset_uon + 1;
       slave_delay_offset_lon =  slave_delay_offset_lon + 1;
     }
@@ -1481,12 +1492,12 @@
     temp = READ_REG32(DDRINTFLAG+0x100);
     lbpe = temp & 0xff;
     lbne = (temp >> 16) & 0xff;
-    if((lbpe != 0xef) && (lock_p == 0)) {
+    if((lbpe != 0x5a) && (lock_p == 0)) {
       min_slave_delay_offset_uop =  1024 - slave_delay_offset_uop;  
       min_slave_delay_offset_lop =  1024 - slave_delay_offset_lop;
       lock_p = 1;
     }
-    if((lbne != 0xad)  && (lock_n == 0)) {
+    if((lbne != 0xa5)  && (lock_n == 0)) {
       min_slave_delay_offset_uon =  1024 - slave_delay_offset_uon;
       min_slave_delay_offset_lon =  1024 - slave_delay_offset_lon;
       lock_n = 1;
@@ -1495,19 +1506,25 @@
     threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY);
     WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0400);
     threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY);
-    if(((lbne != 0xad) && (lbpe != 0xef)) || (threshold_p <= 0x0f) || (threshold_n <= 0x0f)){
+    if (threshold_p <= 0x0f) {
+	min_slave_delay_offset_uop = slave_count_p;
+	min_slave_delay_offset_lop = slave_count_p;
+	lock_p = 1;
+    }
+    if (threshold_n <= 0x0f) {
+	min_slave_delay_offset_uon = slave_count_n;
+	min_slave_delay_offset_lon = slave_count_n; 
+	lock_n = 1;
+    }
+    if((lock_p == 1) && (lock_n == 1)){
       nar_done = 1;
-      min_slave_delay_offset_uop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_uop;  
-      min_slave_delay_offset_uon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_uon;  
-      min_slave_delay_offset_lop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_lop;  
-      min_slave_delay_offset_lon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_lon;  
     }
-    if((lbpe == 0xef)) {
+    if((lbpe == 0x5a)) {
       slave_count_p = slave_count_p + 1;
       slave_delay_offset_uop =  1024 - slave_count_p;  
       slave_delay_offset_lop =  1024 - slave_count_p;    
     }
-    if((lbne == 0xad)) {
+    if((lbne == 0xa5)) {
       slave_count_n = slave_count_n + 1;
       slave_delay_offset_uon =  1024 - slave_count_n;
       slave_delay_offset_lon =  1024 - slave_count_n;
@@ -1582,12 +1599,12 @@
     temp = READ_REG32(DDRINTFLAG+0x100);
     ubpe = (temp >> 8) & 0xff;
     ubne = (temp >> 24) & 0xff;
-    if((ubpe != 0xbe) && (lock_p == 0)) {
+    if((ubpe != 0x5a) && (lock_p == 0)) {
       max_slave_delay_offset_uop =  slave_delay_offset_uop;  
       max_slave_delay_offset_lop =  slave_delay_offset_lop;
       lock_p = 1;
     }
-    if((ubne != 0xde) && (lock_n == 0)) {
+    if((ubne != 0xa5) && (lock_n == 0)) {
       max_slave_delay_offset_uon =  slave_delay_offset_uon;
       max_slave_delay_offset_lon =  slave_delay_offset_lon;
       lock_n = 1;
@@ -1596,23 +1613,25 @@
     threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
     WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0401);
     threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
-    if(((ubne != 0xde) && (ubpe != 0xbe)) || (threshold_p >= 0x140) || (threshold_n >= 0x140)) {
+    if (threshold_p >= 0x140) {
+	max_slave_delay_offset_uop = (slave_delay_offset_uop < 0x0f) ? threshold_p : slave_delay_offset_uop;
+	max_slave_delay_offset_lop = (slave_delay_offset_lop < 0x0f) ? threshold_p : slave_delay_offset_lop;
+	lock_p = 1;
+    }
+    if (threshold_n >= 0x140) {
+	max_slave_delay_offset_uon = (slave_delay_offset_uon < 0x0f) ? threshold_n : slave_delay_offset_uon;
+	max_slave_delay_offset_lon = (slave_delay_offset_lon < 0x0f) ? threshold_n : slave_delay_offset_lon; 
+	lock_n = 1;
+    }
+    if((lock_p == 1) && (lock_n == 1)){
       par_done = 1;
-      max_slave_delay_offset_uop = (threshold_p >= 0x140) ? (
-							     (slave_delay_offset_uop < 0x0f) ? threshold_p : slave_delay_offset_uop) : max_slave_delay_offset_uop;
-      max_slave_delay_offset_uon = (threshold_n >= 0x140) ? (
-							     (slave_delay_offset_uon < 0x0f) ? threshold_n : slave_delay_offset_uon) : max_slave_delay_offset_uon;
-      max_slave_delay_offset_lop = (threshold_p >= 0x140) ? (
-							     (slave_delay_offset_lop < 0x0f) ? threshold_p : slave_delay_offset_lop) : max_slave_delay_offset_lop;
-      max_slave_delay_offset_lon = (threshold_n >= 0x140) ? (
-							     (slave_delay_offset_lon < 0x0f) ? threshold_n : slave_delay_offset_lon) : max_slave_delay_offset_lon;
     }
 
-    if ((ubpe == 0xbe)){
+    if ((ubpe == 0x5a)){
       slave_delay_offset_uop =  slave_delay_offset_uop + 1;  
       slave_delay_offset_lop =  slave_delay_offset_lop + 1;
     }
-    if((ubne == 0xde)) {
+    if((ubne == 0xa5)) {
       slave_delay_offset_uon =  slave_delay_offset_uon + 1;
       slave_delay_offset_lon =  slave_delay_offset_lon + 1;
     }
@@ -1660,12 +1679,12 @@
     temp = READ_REG32(DDRINTFLAG+0x100);
     ubpe = (temp >> 8) & 0xff;
     ubne = (temp >> 24) & 0xff;
-    if((ubpe != 0xbe) && (lock_p == 0)) {
+    if((ubpe != 0x5a) && (lock_p == 0)) {
       min_slave_delay_offset_uop =  1024 - slave_delay_offset_uop;  
       min_slave_delay_offset_lop =  1024 - slave_delay_offset_lop;
       lock_p = 1;
     }
-    if((ubne != 0xde) && (lock_n == 0)) {
+    if((ubne != 0xa5) && (lock_n == 0)) {
       min_slave_delay_offset_uon =  1024 - slave_delay_offset_uon;
       min_slave_delay_offset_lon =  1024 - slave_delay_offset_lon;
       lock_n = 1;
@@ -1674,19 +1693,25 @@
     threshold_p = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
     WRITE_REG16(HM_DPLOADDELAY+0x02, 0x0401);
     threshold_n = 0x3ff & READ_REG16(HM_DPREADDELAY+0x02);
-    if(((ubne != 0xde) && (ubpe != 0xbe)) || (threshold_p <= 0x0f) || (threshold_n <= 0x0f)){
+    if (threshold_p <= 0x0f) {
+	min_slave_delay_offset_uop = slave_count_p;
+	min_slave_delay_offset_lop = slave_count_p;
+	lock_p = 1;
+    }
+    if (threshold_n <= 0x0f) {
+	min_slave_delay_offset_uon = slave_count_n;
+	min_slave_delay_offset_lon = slave_count_n; 
+	lock_n = 1;
+    }
+    if((lock_p == 1) && (lock_n == 1)){
       nar_done = 1;
-      min_slave_delay_offset_uop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_uop;  
-      min_slave_delay_offset_uon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_uon;  
-      min_slave_delay_offset_lop = (threshold_p <= 0x0f) ? slave_count_p : min_slave_delay_offset_lop;  
-      min_slave_delay_offset_lon = (threshold_n <= 0x0f) ? slave_count_n : min_slave_delay_offset_lon;  
     }
-    if((ubpe == 0xbe)) {
+    if((ubpe == 0x5a)) {
       slave_count_p = slave_count_p + 1;
       slave_delay_offset_uop =  1024 - slave_count_p;  
       slave_delay_offset_lop =  1024 - slave_count_p;
     }
-    if((ubne == 0xde)) {
+    if((ubne == 0xa5)) {
       slave_count_n = slave_count_n + 1;
       slave_delay_offset_uon =  1024 - slave_count_n;
       slave_delay_offset_lon =  1024 - slave_count_n;
@@ -1744,16 +1769,16 @@
   CLEAR_BIT32(HM_MISC_CTRL, 0);
   SET_BIT32(HM_MISC_CTRL, 1);
   temp = READ_REG32(DDRINTFLAG+0x100);
-  if(temp != 0xdeadbeef) 
+  if(temp != 0xa5a55a5a) 
     {
 #if defined (HW_ENV)
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
       {
 	nc_printf("!!! ERROR EYE TRAINING FAILED !!!! :%x\n", temp);
-	return 1;
       }
 #endif
 #endif
+	return 1;
     }
   else 
     {
@@ -1842,7 +1867,6 @@
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
 	    {
 	      nc_printf("!!! ERROR FIFO TRAINING COULD NOT COMPLETE !!!!\n");
-	      return 1;
 	    }
 #endif
 #endif
@@ -1898,10 +1922,10 @@
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
 	    {
 	      nc_printf("!!! ERROR Fifo Read Failed .. Should not have come to this print.. check code !!!!\n");
-	      return 1;
 	    }
 #endif
 #endif
+	      return 1;
 	  }
       }
       else
@@ -1923,9 +1947,9 @@
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
       {
 	nc_printf("!!! ERROR DDR PLL IS NOT LOCKED !!!!\n");
-	return 1;
       }
 #endif 
+	return 1;
     }
 #endif
 #if defined (HW_ENV)
@@ -1936,9 +1960,9 @@
 #if defined (ENABLE_NCPRINTF)||(ENABLE_NCPRINTF_FEW)
       {
 	nc_printf("!!! ERROR FIFO TRAINING FAILED %x !!!!\n", READ_REG32(DDRINTFLAG));
-	return 1;
       }
 #endif
+	return 1;
     }
 #if defined (HW_ENV)
   else 
