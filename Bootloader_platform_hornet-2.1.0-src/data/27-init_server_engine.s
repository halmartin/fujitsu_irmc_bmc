*** uboot.old/board/HORNET/init.S	2011-11-25 17:43:20.405484484 +0530
--- uboot/board/HORNET/init.S	2011-11-25 18:06:46.708460000 +0530
***************
*** 56,63 ****
  
  @ UBoot disables interrupts before calling this
  @ Need IRQ enabled to handle clock switching interrupt
!         ldr     r2,= SYSTEMMODE        @User32Mode
!         MSR     CPSR, r2
  
  	ldr	r13,=LMEMSTART+0x5000	@temp stack in local memory yy
  
--- 56,74 ----
  
  @ UBoot disables interrupts before calling this
  @ Need IRQ enabled to handle clock switching interrupt
! @        ldr     r2,= SYSTEMMODE        @User32Mode
! @        MSR     CPSR, r2
! 
! @Enable  ARM Interrupts for Clk Switching
! 	mrs       r2,cpsr
!         bic       r2,r2,#0x80
!         msr        cpsr,r2
! 
! @Graphics Controller's P3 ID
!         ldr     r0,=0x40100152
!         mov     r1,#0x2
!         strb    r1,[r0]
! 
  
  	ldr	r13,=LMEMSTART+0x5000	@temp stack in local memory yy
  
***************
*** 103,108 ****
--- 114,124 ----
          ldr     r1, =0x78001F00
          str      r1, [r0]
  
+ 	ldr	r0,=0x40100150
+ 	ldrb	r1,[r0]
+ 	and	r1,r1,#0xff
+ 	cmp	r1,#0
+ 	bne	01f
  @G2PE	
  	ldr	r0,=0x40100130
  	ldr	r1,[r0]
***************
*** 115,120 ****
--- 131,137 ----
          orr     r1,r1,r2
          str     r1,[r0,#0x50]
  
+ 01:	
  @Program the Subsytem ID to be 0x0102
          ldr     r0,=0x40100100
          ldr     r1,[r0,#0x4c]
***************
*** 142,147 ****
--- 159,168 ----
  	and	r1,r1,r2
  	str	r1,[r0]
  
+ @Disable ARM Interrupts
+ 	mrs       r2,cpsr
+         orr       r2,r2,#0x80
+         msr        cpsr,r2
  
  
  @Program the Tx and Rx Delay Lines for MAC0 (Value = 2 for both)
***************
*** 156,166 ****
  	str	r1,[r0]
  
  @Do clk switch for mac0 and mac1
!         ldr     r0,=0x40100108
!         ldr     r1,[r0]
!         ldr     r2,=0x1E00000
!         orr     r2,r2,r1
!         str     r2,[r0]
  
  
  @enble option rom for graphics	
--- 177,187 ----
  	str	r1,[r0]
  
  @Do clk switch for mac0 and mac1
! @        ldr     r0,=0x40100108
! @        ldr     r1,[r0]
! @        ldr     r2,=0x1E00000
! @        orr     r2,r2,r1
! @        str     r2,[r0]
  
  
  @enble option rom for graphics	
***************
*** 179,184 ****
--- 200,215 ----
  
  	mov	lr, r11
  	mov	pc, lr
+ @Programme LPC pins control to disable Internal Pull ups ,these are supposed to be ON only after VDD power is ON
+ 	ldr	r0,=0x40100920
+ 	ldr	r1,=0xAAAAA
+ 	str	r1,[r0]
+ 	
+ @Enable Clk Gating for Battery Backed Registers
+ 	ldr	r0,=0x40426527
+ 	ldrb	r1,[r0]
+ 	orr	r1,r1,#0x80
+ 	strb	r1,[r0]
  	
  	b    .
  
