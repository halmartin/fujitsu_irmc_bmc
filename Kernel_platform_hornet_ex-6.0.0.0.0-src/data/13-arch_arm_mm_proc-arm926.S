--- linux-3.14.17/arch/arm/mm/proc-arm926.S	2014-08-14 07:08:34.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mm/proc-arm926.S	2014-09-04 11:53:31.741804255 +0530
@@ -86,8 +86,9 @@
 	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
 #endif
 	mrc	p15, 0, ip, c1, c0, 0		@ ctrl register
-	bic	ip, ip, #0x000f			@ ............wcam
-	bic	ip, ip, #0x1100			@ ...i...s........
+  bic ip, ip, #0xf      @ ............wcam
+  bic ip, ip, #0x1100     @ ............i
+  bic ip, ip, #0x2000     @ ............VBIt
 	mcr	p15, 0, ip, c1, c0, 0		@ ctrl register
 	mov	pc, r0
 ENDPROC(cpu_arm926_reset)
