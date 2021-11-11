diff -Naur uboot.org/vendors/ami/HardwareTest/dramtest/TRAPTEST.c uboot/vendors/ami/HardwareTest/dramtest/TRAPTEST.c
--- uboot.org/vendors/ami/HardwareTest/dramtest/TRAPTEST.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/dramtest/TRAPTEST.c	2014-01-27 15:09:50.957237126 +0800
@@ -0,0 +1,122 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#define PLLTEST_C
+static const char ThisFile[] = "PLLTEST.c";
+
+#include "SWFUNC.H"
+
+#include <COMMINF.H>
+#include <TYPEDEF.H>
+#include <IO.H>
+
+#define ASTCHIP_2400    0
+#define ASTCHIP_2300    1
+#define ASTCHIP_1400    2
+#define ASTCHIP_1300    3
+#define ASTCHIP_1050    4
+
+const UCHAR jVersion[] = "v.0.60.06";
+
+typedef struct _TRAPINFO {
+    USHORT  CPU_clk;
+    UCHAR   CPU_AHB_ratio;
+} _TrapInfo;
+
+const _TrapInfo AST_default_trap_setting[] = {
+    // CPU_clk, CPU_AHB_ratio
+    { 384,      2 },            // AST2400 or AST1250 ( ASTCHIP_2400 )
+    { 384,      2 },            // AST2300 ( ASTCHIP_2300 )
+    { 384,   0xFF },            // AST1400 ( ASTCHIP_1400 )
+    { 384,   0xFF },            // AST1300 ( ASTCHIP_1300 )
+    { 384,      2 }             // AST1050 ( ASTCHIP_1050 )
+};
+
+int trap_function(int argc, char *argv[])
+{
+    UCHAR   chiptype;   
+    ULONG   ulData, ulTemp; 
+    UCHAR   status = TRUE;
+    USHORT  val_trap;
+    
+    // Check chip type
+    switch ( ReadSOC_DD( 0x1e6e2000 + 0x7c ) ) {   
+        case 0x01010303 :
+        case 0x01000003 : 
+            printf("The chip is AST2300\n" ); 
+            chiptype = ASTCHIP_2300; 
+            break;
+            
+        case 0x02010303 :
+        case 0x02000303 : 
+            printf("The chip is AST2400 or AST1250\n" ); 
+            chiptype = ASTCHIP_2400; 
+            break;
+
+        default     : 
+            printf ("Error Silicon Revision ID(SCU7C) %08lx!!!\n", ReadSOC_DD( 0x1e6e2000 + 0x7c ) ); 
+            return(1);
+    }
+    
+    WriteSOC_DD(0x1e6e2000, 0x1688A8A8);
+    ulData = ReadSOC_DD(0x1e6e2070);
+    
+    // Check CPU clock
+    ulTemp  = ulData;
+    ulTemp &= 0x0300;
+    ulTemp >>= 8;
+    
+    switch (ulTemp)
+    {
+        case 0x00:
+            val_trap = 384;
+            break;
+        case 0x01:
+            val_trap = 360;
+            break;
+        case 0x02:
+            val_trap = 336;
+            break;
+        case 0x03:
+            val_trap = 408;
+            break;
+    }
+    
+    if (AST_default_trap_setting[chiptype].CPU_clk != val_trap)
+    {
+        printf("[ERROR] CPU CLK: Correct is %d; Real is %d \n", AST_default_trap_setting[chiptype].CPU_clk, val_trap);	
+    	status = FALSE;
+    }
+           	
+    // Check cpu_ahb_ratio
+    ulTemp  = ulData;
+    ulTemp &= 0x0c00;
+    ulTemp >>= 10;
+    
+    switch (ulTemp)
+    {
+        case 0x00:
+            val_trap = 1;
+            break;
+        case 0x01:
+            val_trap = 2;
+            break;
+        case 0x02:
+            val_trap = 4;
+            break;
+        case 0x03:
+            val_trap = 3;
+            break;
+    }
+    
+    if (AST_default_trap_setting[chiptype].CPU_AHB_ratio != val_trap)
+    {
+        printf("[ERROR] CPU:AHB: Correct is %x:1; Real is %x:1 \n", AST_default_trap_setting[chiptype].CPU_AHB_ratio, val_trap);	
+    	status = FALSE;
+    }
+    
+    if ( status == TRUE )
+        printf("[PASS] hardware trap for CPU clock and CPU\\AHB ratio.\n");
+    
+    return status;
+}
+
+#endif
