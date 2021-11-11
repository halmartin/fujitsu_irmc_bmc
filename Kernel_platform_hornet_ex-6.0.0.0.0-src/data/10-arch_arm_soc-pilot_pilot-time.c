--- linux-3.14.17/arch/arm/soc-pilot/pilot-time.c	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/arch/arm/soc-pilot/pilot-time.c	2014-09-04 12:36:38.525879016 +0530
@@ -0,0 +1,146 @@
+/*
+ *  linux/arch/arm/mach-xxx/xxx-time.c
+ *
+ *  Copyright (C) 2005 American Megatrends Inc
+ *
+ *  SE PILOT-II SOC timer functions
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
+#include <linux/kernel.h>
+#include <linux/interrupt.h>
+#include <linux/time.h>
+#include <linux/init.h>
+#include <linux/timex.h>
+
+#include <mach/hardware.h>
+#include <asm/io.h>
+#include <asm/irq.h>
+#include <asm/uaccess.h>
+#include <asm/mach/irq.h>
+#include <asm/mach/time.h>
+
+
+#if 0 //Newer Kernel Doesn't use this
+static inline unsigned long get_elapsed(void)
+{
+	unsigned long counter;
+	counter = *(volatile unsigned long *)(SE_TIMER_VA_BASE + 0x4);  // Current Value of Counter
+	return LATCH-counter;
+}
+
+/* Returns number of us since last clock interrupt. Note the
+   interrupts will be disabled before calling this function */
+static unsigned long
+se_pilot2_gettimeoffset(void)
+{
+	unsigned long elapsed, usec;
+	u32 tisr1, tisr2;
+
+	/*
+	 * If an interrupt was pending before we read the timer,
+	 * we've already wrapped.  Factor this into the time.
+	 * If an interrupt was pending after we read the timer,
+	 * it may have wrapped between checking the interrupt
+	 * status and reading the timer.  Re-read the timer to
+	 * be sure its value is after the wrap.
+	 */
+
+	tisr1 =	*(volatile unsigned long*)(SE_TIMER_VA_BASE + 0x10);
+	elapsed = get_elapsed();
+	tisr2 =	*(volatile unsigned long*)(SE_TIMER_VA_BASE + 0x10);
+
+	if(tisr1 & 0x01)
+		elapsed += LATCH;
+	else if (tisr2 & 0x01)
+		elapsed = LATCH + get_elapsed();
+
+	/* Now convert them to usec */
+	usec = (unsigned long)(elapsed / (CLOCK_TICK_RATE/1000000));
+
+	return usec;
+}
+
+static long counter=0;
+#endif
+
+static irqreturn_t
+se_pilot2_timer_interrupt(int irq, void *dev_id)
+{
+	volatile unsigned long dummy_read;
+
+//	write_seqlock(&xtime_lock);
+
+	/* Clear Interrupt */
+	dummy_read = *((volatile unsigned long*) (SE_TIMER_VA_BASE+0x0c));
+
+	timer_tick();
+
+#if 0
+	if ((counter++)== HZ)
+	{
+		counter=0;
+		printk("X");
+	}
+#endif
+
+//	write_sequnlock(&xtime_lock);
+
+	return IRQ_HANDLED;
+}
+
+static struct irqaction se_pilot2_timer_irq = {
+	.name		= "Pilot2 Timer Tick",
+	.handler	= se_pilot2_timer_interrupt,
+	.flags		= IRQF_DISABLED|IRQF_TIMER|IRQF_IRQPOLL,
+};
+
+void __init
+se_pilot2_timer_init(void)
+{
+	volatile unsigned long dummy_read;
+	int i;
+
+	/* Disable timer and interrups */
+	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x0;
+	dummy_read = *((volatile unsigned long*) (SE_TIMER_VA_BASE+0x0c));
+
+	/* Setup Timer Interrupt routine */
+	setup_irq(IRQ_TIMER_0, &se_pilot2_timer_irq);
+
+	/* Load counter values */
+    	*((volatile unsigned long *)(SE_TIMER_VA_BASE)) = LATCH;
+
+	/* Enable Interrupts and timer */
+	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x00000003;
+
+	printk("Pilot-II Timer Initialized\n");
+	
+	/*Patch: Sometimes the timer does not start. So make sure the timer is 
+	 really running. Otherwise, the kernel will hang at calibrate_delay.
+         If the timer does not start, disable and renable agai, till it starts*/
+	while( (*((volatile unsigned long *)((SE_TIMER_VA_BASE+0x04)))) >(LATCH) )
+        {
+		printk("WARNING: Pilot-II Timer Not Started Yet\n");
+		*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x0;
+		*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x00000003;
+	 	for(i=0;i<10;i++);
+        }
+	printk("Pilot-II Timer Started\n");
+
+	return;
+}
+
