--- linux-3.14.17/arch/arm/mach-pilot/include/mach/entry-macro.S	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/mach-pilot/include/mach/entry-macro.S	2014-09-04 12:01:01.033817240 +0530
@@ -0,0 +1,387 @@
+/*
+ * arch/arm/include/asm/arch-xxx/entry-macro.S
+ *
+ * Low-level IRQ helper macros for SE PILOT2  platforms
+ *
+ * This file is licensed under  the terms of the GNU General Public
+ * License version 2. This program is licensed "as is" without any
+ * warranty of any kind, whether express or implied.
+ */
+#include <mach/platform.h>
+		.macro	disable_fiq
+		.endm
+
+		.macro  get_irqnr_preamble, base,tmp
+		.endm
+
+		.macro  arch_ret_to_user, tmp1,tmp2
+		.endm
+
+		.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp
+		ldr     \irqstat, =Pilot2_Irq_finalstatus_Reg
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x80000000
+		movne   \irqnr, #0x1f
+		bne     2f
+		
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x40000000
+		movne   \irqnr, #0x1e
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x20000000
+		movne   \irqnr, #0x1d
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x10000000
+		movne   \irqnr, #0x1c
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x08000000
+		movne   \irqnr, #0x1b
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x04000000
+		movne   \irqnr, #0x1a
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x02000000
+		movne   \irqnr, #0x19
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x01000000
+		movne   \irqnr, #0x18
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00800000
+		movne   \irqnr, #0x17
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00400000
+		movne   \irqnr, #0x16
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00200000
+		movne   \irqnr, #0x15
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00100000
+		movne   \irqnr, #0x14
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00080000
+		movne   \irqnr, #0x13
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00040000
+		movne   \irqnr, #0x12
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00020000
+		movne   \irqnr, #0x11
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00010000
+		movne   \irqnr, #0x10
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00008000
+//		movne   \irqnr, #0x0f
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00004000
+//		movne   \irqnr, #0x0e
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00002000
+//		movne   \irqnr, #0x0d
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00001000
+//		movne   \irqnr, #0x0c
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000800
+//		movne   \irqnr, #0x0b
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000400
+//		movne   \irqnr, #0x0a			// USB 2.0 
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000200
+//		movne   \irqnr, #0x09
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000100
+//		movne   \irqnr, #0x08
+		bne     3f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000080
+		movne   \irqnr, #0x07
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000040
+		movne   \irqnr, #0x06
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000020
+		movne   \irqnr, #0x05
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000010
+		movne   \irqnr, #0x04
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000008
+		movne   \irqnr, #0x03
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000004
+		movne   \irqnr, #0x02
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000002
+		movne   \irqnr, #0x01
+		bne     2f
+
+		ldr	\tmp,[\irqstat]
+		ands    \tmp, \tmp,#0x00000001
+		movne   \irqnr, #0x00
+		bne     2f
+2:
+		b	6f
+
+3: /* Combined Interrupts*/
+ 
+	mov 	\irqnr,#0x20
+	ldr	\irqstat,=Pilot2_Combined_Irq_Status_Reg
+	ldr	\tmp, [\irqstat]
+4:
+	cmp	\tmp, #0
+	beq	7f				   /* Exhausted :Check Hi Combined Interrupts */
+	and	\base, \tmp, #1
+	cmp	\base, #0
+	bne	6f				   /*  Match, return */
+	mov	\tmp, \tmp, lsr #1
+	add	\irqnr, \irqnr, #1
+	b	4b				   /* Next Bit */
+
+7: /* Hi Combined Interrupts */
+
+	mov 	\irqnr,#0x40
+	ldr		\irqstat,=Pilot2_Irq_Status_Hi_Reg
+	ldr		\tmp, [\irqstat]
+
+8:
+	cmp	\tmp, #0
+	beq	5f				/* Exhausted, return Invalid*/			       
+	and	\base, \tmp, #1
+	cmp	\base, #0
+	bne	6f				/* Match Return		*/
+	mov	\tmp, \tmp, lsr #1
+	add	\irqnr, \irqnr, #1
+	b	8b				/* Next Bit */
+	
+5:
+	mov  	\irqnr,#0x5F		/* Reserved = Invalid Irq */
+	
+	
+6:
+/*---------------------------------------------------------------------
+#if 0
+		ldr     \irqstat, =Pilot2_Combined_Irq_Status_Reg
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000001
+		movne	\irqnr, #0x20
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000002
+		movne	\irqnr, #0x21
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000004
+		movne	\irqnr, #0x22
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000008
+		movne	\irqnr, #0x23
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000010
+		movne	\irqnr, #0x24
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000020
+		movne	\irqnr, #0x25
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000040
+		movne	\irqnr, #0x26
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000080
+		movne	\irqnr, #0x27
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000100
+		movne	\irqnr, #0x28
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000200
+		movne	\irqnr, #0x29
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000400
+		movne	\irqnr, #0x2A
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00000800
+		movne	\irqnr, #0x2B
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00001000
+		movne	\irqnr, #0x2C
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00002000
+		movne	\irqnr, #0x2D
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00004000
+		movne	\irqnr, #0x2E
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00008000
+		movne	\irqnr, #0x2F
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00010000
+		movne	\irqnr, #0x30
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00020000
+		movne	\irqnr, #0x31
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00040000
+		movne	\irqnr, #0x32
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00080000
+		movne	\irqnr, #0x33
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00100000
+		movne	\irqnr, #0x34
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00200000
+		movne	\irqnr, #0x35
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00400000
+		movne	\irqnr, #0x36
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x00800000
+		movne	\irqnr, #0x37
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x01000000
+		movne	\irqnr, #0x38
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x02000000
+		movne	\irqnr, #0x39
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x04000000
+		movne	\irqnr, #0x3A
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x08000000
+		movne	\irqnr, #0x3B
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x10000000
+		movne	\irqnr, #0x3C
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x20000000
+		movne	\irqnr, #0x3D
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x40000000
+		movne	\irqnr, #0x3E
+		bne	4f
+
+		ldr	\tmp,[\irqstat]
+		ands	\tmp,\tmp,#0x80000000
+		movne	\irqnr, #0x3F
+		bne	4f
+4:
+#endif
+------------------------------------------------------------------------------*/
+		.endm
+
