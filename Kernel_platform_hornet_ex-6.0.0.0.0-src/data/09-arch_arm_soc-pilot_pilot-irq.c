--- linux-3.14.17/arch/arm/soc-pilot/pilot-irq.c	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/soc-pilot/pilot-irq.c	2014-09-17 19:39:44.284897659 +0530
@@ -0,0 +1,257 @@
+/*
+ *  linux/arch/arm/mach-xxx/xxx-irq.c
+ *
+ *  Copyright (C) 2005 American Megatrends Inc
+ *
+ *  SE PILOT-II SOC IRQ handling functions
+ *
+ * This program is free software; you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation; either version 2 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program; if not, write to the Free Software
+ * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
+ */
+
+#include <linux/init.h>
+#include <linux/interrupt.h>
+#include <linux/list.h>
+
+#include <asm/mach/irq.h>
+#include <asm/irq.h>
+#include <mach/hardware.h>
+
+#include <asm/mach-types.h>
+
+
+static unsigned long Combined2IrqMap[] =COMBINED2_IRQ_MAP;
+static int GetCombined2IrqSrc(unsigned int irq)
+{
+        int i,count;
+        unsigned long bitmask = (1 << irq);
+
+        count = sizeof(Combined2IrqMap)/sizeof(unsigned long);
+        for (i=0;i<count;i++)
+        {
+                if (Combined2IrqMap[i] & bitmask)
+                        break;
+        }
+
+        return i+8;     /* 8 to 15 */
+
+}
+
+static unsigned long CombinedIrqMap[] =COMBINED_IRQ_MAP;
+static
+int
+GetCombinedIrqSrc(unsigned int irq)
+{
+	int i,count;
+	unsigned long bitmask = (1 << irq);
+
+	count = sizeof(CombinedIrqMap)/sizeof(unsigned long);
+	for (i=0;i<count;i++)
+	{
+		if (CombinedIrqMap[i] & bitmask)
+			break;
+	}
+
+	return i+8;	/* 8 to 15 */
+	
+}
+
+
+/* Same as disabling Intr (mask_irq), but for combinined interrupts
+   we mask the source irq also */
+static void
+se_pilot2_ack_irq (struct irq_data *d)
+{
+	volatile unsigned long * mask_reg;
+  int irq=0;
+
+  irq = d->irq;
+
+	if (irq > NR_IRQS)
+		return;
+
+	if(irq >= COMBINED2_IRQ_START) 
+	{
+                irq -=COMBINED2_IRQ_START;
+                mask_reg = ((volatile unsigned long*) Pilot2_Irq_Ctrl_Hi_Reg);
+                *mask_reg &= ~(1 << irq);
+
+                /* Fall thru to do the combined  irq mask */
+                irq = GetCombined2IrqSrc(irq);
+	}
+
+	if ((irq >= COMBINED_IRQ_START) && (irq < COMBINED2_IRQ_START))
+	{
+		irq -=COMBINED_IRQ_START;
+		mask_reg = ((volatile unsigned long*) Pilot2_Combined_Irq_Ctrl_Reg);
+		*mask_reg &= ~(1 << irq);
+
+		/* Fall thru to do the source irq mask */
+		irq = GetCombinedIrqSrc(irq);
+	}
+
+	mask_reg = (volatile unsigned long *)(Pilot2_Irq_intmask_Reg);
+	*mask_reg |= (1 << irq);
+	return;
+}
+
+static void
+se_pilot2_mask_irq(struct irq_data *d)
+{
+	volatile unsigned long * mask_reg;
+  int irq=0;
+
+  irq=d->irq;
+
+	if (irq >= NR_IRQS)
+		return;
+
+  if(irq >= COMBINED2_IRQ_START)
+	{
+                irq -=COMBINED2_IRQ_START;
+                mask_reg = ((volatile unsigned long*) Pilot2_Irq_Ctrl_Hi_Reg);
+                *mask_reg &= ~(1 << irq);
+
+		/* We don't want to disable the source combined irq, because some 
+		   other irq in the group may be using that. So return */
+		return;
+        }
+
+	if ((irq >= COMBINED_IRQ_START) && (irq < COMBINED2_IRQ_START))
+	{
+		irq -=COMBINED_IRQ_START;
+		mask_reg = ((volatile unsigned long*) Pilot2_Combined_Irq_Ctrl_Reg);
+		*mask_reg &= ~(1 << irq);
+
+		/* We don't want to disable the source combined irq, because some 
+		   other irq in the group may be using that. So return */
+		return;
+	}
+
+	mask_reg = (volatile unsigned long *)(Pilot2_Irq_intmask_Reg);
+	*mask_reg |= (1 << irq);
+	return;
+}
+
+static void
+se_pilot2_unmask_irq(struct irq_data *d)
+{
+	volatile unsigned long * mask_reg;
+  int irq=0;
+
+  irq=d->irq;
+
+	if (irq >= NR_IRQS)
+		return;
+
+        if(irq >= COMBINED2_IRQ_START)
+	{
+                irq -=COMBINED2_IRQ_START;
+                mask_reg = ((volatile unsigned long*) Pilot2_Irq_Ctrl_Hi_Reg);
+                *mask_reg |= (1 << irq);
+
+		/* Though we enable all the combined irq's source irq, we are falling
+                   thru here as ack_irq disable the source and we have to enable it */
+                irq = GetCombined2IrqSrc(irq);
+        }
+
+	if ((irq >= COMBINED_IRQ_START) && (irq < COMBINED2_IRQ_START))
+	{
+		irq -=COMBINED_IRQ_START;
+		mask_reg = ((volatile unsigned long*) Pilot2_Combined_Irq_Ctrl_Reg);
+		*mask_reg |= (1 << irq);
+
+		/* Though we enable all the combined irq's source irq, we are falling
+                   thru here as ack_irq disable the source and we have to enable it */
+		irq = GetCombinedIrqSrc(irq);
+	}
+
+	mask_reg = (volatile unsigned long *)(Pilot2_Irq_intmask_Reg);
+	*mask_reg &= ~(1 << irq);
+	return;
+}
+ 
+
+static struct irq_chip se_pilot2_irq_chip = 
+{
+	.irq_ack		= se_pilot2_ack_irq,
+	.irq_mask		= se_pilot2_mask_irq,
+	.irq_unmask		= se_pilot2_unmask_irq,
+};
+
+static int InvalidMap[] = INVALID_IRQ_MAP;
+
+static
+int __init
+CheckIrqValidity(int irq)
+{
+	int i,count;
+
+	count = sizeof(InvalidMap)/sizeof(int);
+	for (i=0;i<count;i++)
+	{
+		if (irq == InvalidMap[i])
+			return 1; // Invalid
+	}
+	return 0; // Valid
+}
+
+
+
+void __init
+se_pilot2_init_irq(void)
+{
+	unsigned int i;
+
+	printk("Pilot-II Interrupt Controller Enabled\n");
+
+	/* Disable all interrupts */
+	*((volatile unsigned long*) Pilot2_Irq_intmask_Reg) = 0xFFFFFFFF;
+	*((volatile unsigned long*) Pilot2_Irq_inten_Reg)   = 0;
+	*((volatile unsigned long*) Pilot2_Combined_Irq_Ctrl_Reg) = 0x0;
+	*((volatile unsigned long*) Pilot2_Irq_Ctrl_Hi_Reg) = 0x0;
+
+	*((volatile unsigned long*) Pilot2_Fiq_intmask_Reg) = 0xFFFFFFFF;
+	*((volatile unsigned long*) Pilot2_Fiq_inten_Reg)   = 0;
+	*((volatile unsigned long*) Pilot2_Combined_Fiq_Ctrl_Reg) = 0x0;
+	*((volatile unsigned long*) Pilot2_Fiq_Ctrl_Hi_Reg) = 0x0;
+
+	/*Enable them so that we need not have to enable them explicitly */
+	*((volatile unsigned long*) Pilot2_Irq_inten_Reg) = 0xFFFFFFFF;
+
+	/* Enable the Combined Irqs */ /* 8,9,11,12,13,14,15 */
+	/* These will be controlled (masked and unmasked) by Combined Irq Control Reg */
+	*((volatile unsigned long*) Pilot2_Irq_intmask_Reg) = 0xFFFF00FF;
+	
+
+	for (i = 0; i < NR_IRQS; i++)
+	{	
+    irq_set_chip_and_handler(i,&se_pilot2_irq_chip, handle_level_irq);
+
+		if (CheckIrqValidity(i) == 0)
+			set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
+		else
+			set_irq_flags(i, 0);
+	}
+
+	/* Special Case of USB 2.0 : It is in the location of combibed interrupt
+           and is treated as a combined interrupt by pilot-ii(It has a extra bit 
+           to enable the interrupt). But the bit does not present in CIRQCTL. It 
+	   is present in IRQCTLHI. 
+   	   So what Iam doing this is enable this bit permanently and treat USB 2.0
+           interrupt as normal Interrupt.
+	*/
+	//*((volatile unsigned long*) Pilot2_Irq_Ctrl_Hi_Reg) |= 0x10; /* Bit 4 */
+	return;
+}
