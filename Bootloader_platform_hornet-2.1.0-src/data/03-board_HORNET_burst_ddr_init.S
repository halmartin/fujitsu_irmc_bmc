--- u-boot-1.1.6/board/HORNET/burst_ddr_init.S	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/board/HORNET/burst_ddr_init.S	2010-04-05 16:47:24.000000000 -0400
@@ -0,0 +1,98 @@
+
+	
+	       .global  Burst_Read_LSI_FIFO
+	       .global  Burst_Read_PHY_TRAINING
+
+LSI_FIFO_TRAINING =0x10007000
+
+
+Burst_Read_PHY_TRAINING:
+	stmfd  r13!,{r0-r12,r14}	
+	ldr    r1, =LSI_FIFO_TRAINING
+	ldmfd  r0!,{r3-r6}
+	str    r3, [r1]
+	str    r4, [r1, #0x04]
+	ldmfd  r13!,{r0-r12,r15}
+
+
+Burst_Read_LSI_FIFO:
+	stmfd  r13!,{r1-r12,r14}
+	ldr    r2, =0x55555555
+	ldr    r3, =0x55555555
+	ldr    r4, =0x55555555
+	ldr    r5, =0x55555555
+	ldr    r6, =0xAAAAAAAA
+	ldr    r7, =0xAAAAAAAA
+	ldr    r8, =0xAAAAAAAA
+	ldr    r9, =0xAAAAAAAA
+	ldr    r11,=0xAAAAAAAA 
+	stmfd  r0!, {r2-r9}	   @@ to make sure reads are combined
+	ldmfd  r0!, {r3-r10}
+	ldr    r1, =LSI_FIFO_TRAINING
+	cmp    r2, r3
+	bne    ret_fail1
+	cmp    r2, r4	
+	bne    ret_fail2
+	cmp    r2, r5	
+	bne    ret_fail3
+	cmp    r2, r6	
+	bne    ret_fail4
+	cmp    r11, r7	
+	bne    ret_fail5
+	cmp    r11, r8	
+	bne    ret_fail6
+	cmp    r11, r9	
+	bne    ret_fail7
+	cmp    r11, r10	
+	bne    ret_fail8
+	
+	mov    r0, #0x00	
+	ldmfd  r13!,{r1-r12,r15}
+
+
+	
+	
+
+ret_fail1:	
+	mov    r0,#0x01
+	str    r3, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+ret_fail2:	
+	mov    r0,#0x02
+	str    r4, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+ret_fail3:	
+	mov    r0,#0x03
+	str    r5, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+ret_fail4:	
+	mov    r0,#0x04
+	str    r6, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+ret_fail5:	
+	mov    r0,#0x05
+	str    r7, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+ret_fail6:	
+	mov    r0,#0x06
+	str    r8, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+ret_fail7:	
+	mov    r0,#0x07
+	str    r9, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+ret_fail8:	
+	mov    r0,#0x08
+	str    r10, [r1]
+        ldmfd  r13!,{r1-r12,r15}
+
+
+
+	
