--- uboot/board/HORNET/init.S	1969-12-31 19:00:00.000000000 -0500
+++ uboot.ok/board/HORNET/init.S	2010-05-21 14:23:42.137302125 -0400
@@ -0,0 +1,325 @@
+/***************************************************************************
+#
+# CONFIDENTIAL
+#
+# Copyright (c) 2004 Server Engines 2004 
+# All rights reserved
+#
+#       Project  : Pilot
+#       Filename : init.s
+#
+# Modified for U-Boot by American Megatrends Inc.
+# Set TAB Spaces to 4 for proper view of the source
+#      
+#****************************************************************************
+*/
+
+	.include  "pilot3.equ"
+	.global    hornet_init
+
+remap_vectors:
+		B	.
+		B	.
+		B	.
+		B	.
+		B	.
+		B	.
+		ldr	 pc,_IRQHandler_LOCAL
+
+_IRQHandler_LOCAL:
+		.word (IRQHandler_LOCAL - TEXT_BASE)
+
+remap_vectors_offset:
+		.word (remap_vectors - TEXT_BASE)
+
+		.align
+RELOADLPCADDR	=	0x40100200		@ 128Bytes of Scratch Area
+	
+stbyint_sts	        = (1<<2)	
+stbyint_en	        = (1<<3)
+	
+        .align   
+
+/*
+#************************************************************************
+#              Called from lowlevelinit.s to initialize DDR
+#************************************************************************
+*/
+hornet_init:
+	mov		r11, lr			   	   @ save link register so we can return
+
+@Enable Boot SPI Prefetch
+	ldr	r0,=0x40200010
+	ldr	r1,[r0]
+	orr	r1,r1,#0x10
+	str	r1,[r0]
+
+@ UBoot disables interrupts before calling this
+@ Need IRQ enabled to handle clock switching interrupt
+        ldr     r2,= SYSTEMMODE        @User32Mode
+        MSR     CPSR, r2
+
+	ldr	r13,=LMEMSTART+0x5000	@temp stack in local memory yy
+
+	ldr	r0, =LMEMSTART		@ save r12 (ip) temporarily in local ram
+	str	r12, [r0]
+
+@default ddr pll to 800mhz,though ddr pll comes up with 800mhz,we still needs to write this,as the register at 0x40100110 does not
+@have correct reset values,so at later point if we are doing Read modify writes to change ddr frequency for low power we may end u
+@writting in-correct values to PLL,to avoid this scenario we are correcting default value by overwiting them.
+        ldr     r0, =0x40100110
+        ldr     r1, =0x78001F00
+        str     r1, [r0]
+
+/*
+# Move the vectors to scratchpad and remap
+# this allows us to have our own irq handler for clock switching.
+# unmap at the end 
+*/
+@@@@load handlers at remap address
+        ldr     r0,remap_vectors_offset
+        ldr     r1,=RELOADLPCADDR
+        ldmia   r0!,{r2-r9}
+        stmia   r1!,{r2-r9}
+
+        ldmia   r0!,{r2-r9}
+        stmia   r1!,{r2-r9}
+
+	ldr	r0,=SYSCONTROL
+	ldr	r1,[r0]
+	orr	r1,r1,#1
+	str	r1,[r0]               @ remap
+
+	
+@programing pll to switch clock to higher clock rate
+@to do that we put arm into stanby mode and we programme h/w such that it will
+@generate interrupt after clock is switched to bring ARM out of standby
+	@Reset the Phy clock synchronizer 
+        ldr     r0, =0x40300010 
+        mov     r1, #0
+	str     r1, [r0]
+
+        ldr     r0, =0x40100110
+        ldr     r1, =0x78001F00
+        str      r1, [r0]
+
+@G2PE	
+	ldr	r0,=0x40100130
+	ldr	r1,[r0]
+	orr	r1,r1,#0x80
+	str	r1,[r0]
+@Program the version id to be 0x3
+        ldr     r0,=0x40100100
+        ldr     r1,[r0,#0x50]
+        ldr     r2,=0x3
+        orr     r1,r1,r2
+        str     r1,[r0,#0x50]
+
+@Program the Subsytem ID to be 0x0102
+        ldr     r0,=0x40100100
+        ldr     r1,[r0,#0x4c]
+        ldr     r2,=0x01020000
+        ldr     r3,=0x0000ffff
+        and     r1,r1,r3
+        orr     r1,r1,r2
+        str     r1,[r0,#0x4c]
+
+	
+@enable stanby interrupt,hw will generate interrupt once clock is switched
+	bl	Enable_stby_intr_vic
+	bl	Enable_standbyfi_intr
+	bl	Wait_For_PLLock
+	bl	Switch_Clk_Pll
+	
+	bl	Disable_standbyfi_intr
+@	b	.
+@	bl	change_spi_100_nostack
+@	bl	set_spi_fastread_nostack
+
+	ldr	r0,=SYSCONTROL		@ restore remap irq vectors back to zero
+	ldr	r2,=0xfffffff6		@ and disable clock switch interrupt
+	ldr	r1,[r0]
+	and	r1,r1,r2
+	str	r1,[r0]
+
+
+
+@Program the Tx and Rx Delay Lines for MAC0 (Value = 2 for both)
+@This is required for the proper functioning of the gigabit port 
+@on silicon
+@MAC_RGMII_CTRL      =  0x40100188
+
+	ldr	r0,=0x40100188
+	ldr	r1,=0x293
+	ldr	r2,[r0]
+	orr	r1,r1,r2
+	str	r1,[r0]
+
+@Do clk switch for mac0 and mac1
+        ldr     r0,=0x40100108
+        ldr     r1,[r0]
+        ldr     r2,=0x1E00000
+        orr     r2,r2,r1
+        str     r2,[r0]
+
+
+@enble option rom for graphics	
+	 ldr     r6,=SYSCONTROL            @0x40100054
+         ldr     r4, [r6,#0x54]
+ 	 ldr     r5,=0x00000200
+	 orr     r4, r4,r5
+         str     r4, [r6,#PIDBG0]
+
+@ Do DDR Initializtaion
+	mov     r0,#0x0
+     bl      ddrinit_asic
+
+	ldr	r0, =LMEMSTART		@ restore r12 (ip) from local ram
+	ldr	r12, [r0]
+
+	mov	lr, r11
+	mov	pc, lr
+	
+	b    .
+
+@--------------------------------------------------------------------        
+@ clk switching intr handler
+@--------------------------------------------------------------------        
+IRQHandler_LOCAL:
+	ldr		r2,=stbyint_sts
+	ldr		r0,=SYSCONTROL
+	ldr		r1,[r0]
+	orr		r1,r1,r2
+	str		r1,[r0]
+   	subs   	r15, r14, #4           	
+
+@--------------------------------------------------------------------        
+@enable stby interrupt at VIC level
+@--------------------------------------------------------------------
+	.global Enable_stby_intr_vic
+Enable_stby_intr_vic:	
+	ldr    	r0,=STANDBYINT
+	ldr		r1,=VIC
+	ldr    	r2,[r1]
+	orr    	r0,r0,r2
+	str		r0,[r1]
+	mov    	pc,r14
+
+	.global Enable_standbyfi_intr
+Enable_standbyfi_intr:	
+
+	ldr	r0,=SYSCONTROL
+
+@clear any of previous standby interrupt status	
+	ldr	r2,=stbyint_sts
+	
+03:		
+	ldr	r1,[r0]	
+	orr	r1,r1,r2
+	str	r1,[r0]
+
+	ldr	r1,[r0]
+	tst     r1,#stbyint_sts
+	bne	03b
+
+@enable stby intr	
+	ldr	r2,=stbyint_en
+	ldr	r1,[r0]
+	orr	r1,r1,r2
+	str	r1,[r0]
+
+	mov    	pc,r14
+
+@Waits till PLL is stable
+	.global Wait_For_PLLock
+Wait_For_PLLock:	
+	ldr     r0,=SCLKSTS
+	ldr     r2,=PLLLOCK
+01:
+	ldr     r1,[r0]
+	and     r1,r1,r2
+	cmp     r1,r2
+	bne     01b
+	mov    	pc,r14
+
+
+@make sure that Enable_standbyfi_intr is called before this subroutine and appropriate handler is available
+@This subroutine switches mux to PLL(500/400Mhz) mode
+	.global Switch_Clk_Pll
+Switch_Clk_Pll:	
+	ldr   r1,=SCLKCTL
+	ldrb   r0,[r1]
+	ldr    r2,=0xffffffff
+	orr   r0,r0,r2
+	str   r0,[r1]
+	
+	MCR    15, 0, r0, c7, c0, 4
+	mov    	pc,r14
+
+	.global Disable_standbyfi_intr
+Disable_standbyfi_intr:
+	ldr	r0,=SYSCONTROL
+	ldr	r2,=stbyint_en
+	ldr	r1,[r0]
+	bic	r1,r1,r2
+	str	r1,[r0]
+
+@clear any of previous standby interrupt status
+	ldr	r0,=SYSCONTROL
+	ldr	r2,=stbyint_sts
+	
+03:		
+	ldr	r1,[r0]	
+	orr	r1,r1,r2
+	str	r1,[r0]
+
+	ldr	r1,[r0]
+	tst     r1,#stbyint_sts
+	bne	03b
+	mov    	pc,r14	
+	
+
+	
+
+	
+	.global change_spi_100_nostack
+change_spi_100_nostack:	
+	ldr	r0, =0x40100120
+	ldrb	r1, [r0]
+	and	r1, r1, #0xf0
+	orr	r1, r1, #0x01
+	strb	r1, [r0]
+	mov	pc, r14
+	
+	.global	change_spi_40_nostack
+change_spi_40_nostack:	
+	ldr	r0, =0x40100120
+	ldrb	r1, [r0]
+	and	r1, r1, #0xf0
+	orr	r1, r1, #0x04
+	strb	r1, [r0]
+	mov	pc, r14
+	.global set_spi_fastread_nostack
+set_spi_fastread_nostack:			
+	ldr	r0, =0x4020000B       
+	ldrb	r1, [r0]
+	and	r1, r1, #0x7f
+	orr	r1, r1, #0x04
+	strb	r1, [r0]
+	mov	pc, r14
+
+    	.ltorg 
+	.global clr_spi_fastread_nostack
+clr_spi_fastread_nostack:			
+	ldr	r0, =0x4020000b
+	ldrb	r1, [r0]
+	and	r1, r1, #0x7f
+	and	r1, r1, #0xfb
+	strb	r1, [r0]
+	mov	pc, r14	
+	
+	
+	
+    .ltorg 
+	.end     
+
