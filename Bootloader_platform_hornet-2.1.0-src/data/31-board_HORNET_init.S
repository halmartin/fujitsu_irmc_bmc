--- uboot.org/board/HORNET/init.S Tue Dec 24 17:15:57 2013
+++ uboot/board/HORNET/init.S Tue Dec 24 18:16:31 2013
@@ -1,17 +1,25 @@
-/***************************************************************************
-#
-# CONFIDENTIAL
-#
-# Copyright (c) 2004 Server Engines 2004 
-# All rights reserved
-#
-#       Project  : Pilot
-#       Filename : init.s
-#
-# Modified for U-Boot by American Megatrends Inc.
-# Set TAB Spaces to 4 for proper view of the source
-#      
-#****************************************************************************
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
 */
 
 	.include  "pilot3.equ"
@@ -46,8 +54,8 @@
 #************************************************************************
 */
 hornet_init:
-	mov		r11, lr			   	   @ save link register so we can return
-
+
+	stmfd  r13!,{r0-r12,r14}
 @Enable Boot SPI Prefetch
 	ldr	r0,=0x40200010
 	ldr	r1,[r0]
@@ -68,12 +76,6 @@
         ldr     r0,=0x40100152
         mov     r1,#0x2
         strb    r1,[r0]
-
-
-	ldr	r13,=LMEMSTART+0x5000	@temp stack in local memory yy
-
-	ldr	r0, =LMEMSTART		@ save r12 (ip) temporarily in local ram
-	str	r12, [r0]
 
 @default ddr pll to 800mhz,though ddr pll comes up with 800mhz,we still needs to write this,as the register at 0x40100110 does not
 @have correct reset values,so at later point if we are doing Read modify writes to change ddr frequency for low power we may end u
@@ -93,9 +95,6 @@
         ldmia   r0!,{r2-r9}
         stmia   r1!,{r2-r9}
 
-        ldmia   r0!,{r2-r9}
-        stmia   r1!,{r2-r9}
-
 	ldr	r0,=SYSCONTROL
 	ldr	r1,[r0]
 	orr	r1,r1,#1
@@ -114,11 +113,13 @@
         ldr     r1, =0x78001F00
         str      r1, [r0]
 
+
 	ldr	r0,=0x40100150
 	ldrb	r1,[r0]
 	and	r1,r1,#0xff
 	cmp	r1,#0
 	bne	01f
+	
 @G2PE	
 	ldr	r0,=0x40100130
 	ldr	r1,[r0]
@@ -130,8 +131,9 @@
         ldr     r2,=0x3
         orr     r1,r1,r2
         str     r1,[r0,#0x50]
-
+	
 01:	
+	
 @Program the Subsytem ID to be 0x0102
         ldr     r0,=0x40100100
         ldr     r1,[r0,#0x4c]
@@ -165,16 +167,18 @@
         msr        cpsr,r2
 
 
-@Program the Tx and Rx Delay Lines for MAC0 (Value = 2 for both)
-@This is required for the proper functioning of the gigabit port 
-@on silicon
+	
+@Enable Tx delay and Program Tx Delay 6  for MAC0 (1gig)
+@keep default Rx delay 
 @MAC_RGMII_CTRL      =  0x40100188
-
 	ldr	r0,=0x40100188
-	ldr	r1,=0x293
-	ldr	r2,[r0]
-	orr	r1,r1,r2
-	str	r1,[r0]
+	ldr	r1,=0x3ff
+	ldrh	r2,[r0]
+	bic	r2,r2,r1
+	ldr	r1,=0x231  
+@	ldr	r1,=0x293  
+	orr	r2,r2,r1
+	strh	r2,[r0]
 
 @Do clk switch for mac0 and mac1
 @        ldr     r0,=0x40100108
@@ -193,13 +197,9 @@
 
 @ Do DDR Initializtaion
 	mov     r0,#0x0
-     bl      ddrinit_asic
-
-	ldr	r0, =LMEMSTART		@ restore r12 (ip) from local ram
-	ldr	r12, [r0]
-
-	mov	lr, r11
-	mov	pc, lr
+        bl      ddrinit_asic
+
+
 @Programme LPC pins control to disable Internal Pull ups ,these are supposed to be ON only after VDD power is ON
 	ldr	r0,=0x40100920
 	ldr	r1,=0xAAAAA
@@ -210,6 +210,47 @@
 	ldrb	r1,[r0]
 	orr	r1,r1,#0x80
 	strb	r1,[r0]
+	
+#ifdef USB1_NOT_USED	
+	ldr	r0,=0x40100130
+	ldr	r1,=0x80000
+	ldr	r2,[r0]
+	orr	r1,r1,r2
+	str	r1,[r0]
+#endif
+
+
+#ifdef USB2_NOT_USED	
+	ldr	r0,=0x408001a7
+	mov	r1,#1
+	strb	r1,[r0]
+#endif
+
+#ifdef PCIE_PINS_ISOLATE
+	ldr	r0,=0x4010091C
+	ldr	r1,[r0]
+	ldr	r2,=0x20000
+	orr	r1,r1,r2
+	str	r1,[r0]
+
+	ldr	r0,=0x40441600
+	ldr	r1,=0xCCC00000
+	str	r1,[r0]
+
+	ldr	r0,=0x40441604
+	ldr	r1,=0xCCC00000
+	str	r1,[r0]
+
+	ldr	r0,=0x4010091C
+	ldr	r1,[r0]
+	ldr	r2,=0x20000
+	bic	r1,r1,r2
+	str	r1,[r0]
+#endif				
+
+	
+	ldmfd  r13!,{r0-r12,r15}
+
 	
 	b    .
 
@@ -349,7 +390,7 @@
 	strb	r1, [r0]
 	mov	pc, r14	
 	
-	
+
 	
     .ltorg 
 	.end     
