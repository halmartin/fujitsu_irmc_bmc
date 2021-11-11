--- linux_oooo/arch/arm/soc-pilot/pilot-time.c	2016-04-11 10:09:07.400061832 +0530
+++ linux/arch/arm/soc-pilot/pilot-time.c	2016-04-11 10:08:12.900061953 +0530
@@ -3,7 +3,6 @@
  *
  *  Copyright (C) 2005 American Megatrends Inc
  *
- *  SE PILOT-II SOC timer functions
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
@@ -19,6 +18,13 @@
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */
+/*
+ * Copyright (c) 2010-2015, Emulex Corporation.
+ * Modifications made by Emulex Corporation under the terms of the
+ * GNU General Public License as published by the Free Software
+ * Foundation; either version 2 of the License, or (at your option)
+ * any later version.
+ */
 
 #include <linux/kernel.h>
 #include <linux/interrupt.h>
@@ -32,115 +38,167 @@
 #include <asm/uaccess.h>
 #include <asm/mach/irq.h>
 #include <asm/mach/time.h>
+#include <linux/clocksource.h>
+#include <linux/clockchips.h>
+#include <asm/smp_twd.h>
 
+//#define CLOCK_TICK_RATE1			(25*1000*1000*2)
+#define OUR_LATCH  ((CLOCK_TICK_RATE + HZ/2) / HZ)	/* For divider */
 
-#if 0 //Newer Kernel Doesn't use this
 static inline unsigned long get_elapsed(void)
 {
 	unsigned long counter;
 	counter = *(volatile unsigned long *)(SE_TIMER_VA_BASE + 0x4);  // Current Value of Counter
-	return LATCH-counter;
+	return OUR_LATCH-counter;
 }
 
-/* Returns number of us since last clock interrupt. Note the
-   interrupts will be disabled before calling this function */
-static unsigned long
-se_pilot2_gettimeoffset(void)
-{
-	unsigned long elapsed, usec;
-	u32 tisr1, tisr2;
-
-	/*
-	 * If an interrupt was pending before we read the timer,
-	 * we've already wrapped.  Factor this into the time.
-	 * If an interrupt was pending after we read the timer,
-	 * it may have wrapped between checking the interrupt
-	 * status and reading the timer.  Re-read the timer to
-	 * be sure its value is after the wrap.
-	 */
-
-	tisr1 =	*(volatile unsigned long*)(SE_TIMER_VA_BASE + 0x10);
-	elapsed = get_elapsed();
-	tisr2 =	*(volatile unsigned long*)(SE_TIMER_VA_BASE + 0x10);
-
-	if(tisr1 & 0x01)
-		elapsed += LATCH;
-	else if (tisr2 & 0x01)
-		elapsed = LATCH + get_elapsed();
-
-	/* Now convert them to usec */
-	usec = (unsigned long)(elapsed / (CLOCK_TICK_RATE/1000000));
-
-	return usec;
-}
+static irqreturn_t pilot_timer_interrupt(int irq, void *dev_id);
+static struct irqaction se_pilot3_timer_irq = {
+	.name		= "Pilot3 Timer Tick",
+	.handler	= pilot_timer_interrupt,
+	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
+};
 
-static long counter=0;
-#endif
+static struct clock_event_device pilot_clockevent;
 
-static irqreturn_t
-se_pilot2_timer_interrupt(int irq, void *dev_id)
+static irqreturn_t pilot_timer_interrupt(int irq, void *dev_id)
 {
 	volatile unsigned long dummy_read;
-
-//	write_seqlock(&xtime_lock);
-
+	struct clock_event_device *evt = &pilot_clockevent;
+//	printk("pilot_timer_interrupt irq %d\n", irq);
 	/* Clear Interrupt */
 	dummy_read = *((volatile unsigned long*) (SE_TIMER_VA_BASE+0x0c));
 
-	timer_tick();
+	evt->event_handler(evt);
 
-#if 0
-	if ((counter++)== HZ)
-	{
-		counter=0;
-		printk("X");
-	}
-#endif
+	return IRQ_HANDLED;
+}
 
-//	write_sequnlock(&xtime_lock);
+static int pilot_set_next_event(unsigned long evt,
+			struct clock_event_device *unused)
+{
+	unsigned long ctrl = 1;//Timer enable
+	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0;//Disable
+	*((volatile unsigned long *)(SE_TIMER_VA_BASE)) = evt;
+	/* Enable Interrupts and timer */
+	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = ctrl;
+	return 0;
+}
 
-	return IRQ_HANDLED;
+static void pilot_set_mode(enum clock_event_mode mode,
+			struct clock_event_device *clk)
+{
+	volatile unsigned long dummy_read;
+	unsigned long ctrl;
+	switch (mode) {
+	case CLOCK_EVT_MODE_PERIODIC:
+		printk("CLOCK_EVT_MODE_PERIODIC OUR_LATCH %x CLOCK_TICK_RATE %d\n",  OUR_LATCH, CLOCK_TICK_RATE);
+		/* timer load already set up */
+		/* Disable timer and interrups */
+		*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x0;
+		dummy_read = *((volatile unsigned long*) (SE_TIMER_VA_BASE+0x0c));
+		/* Load counter values */
+		*((volatile unsigned long *)(SE_TIMER_VA_BASE)) = (OUR_LATCH);
+		ctrl = 0x3;
+		break;
+	case CLOCK_EVT_MODE_ONESHOT:
+		printk("CLOCK_EVT_MODE_ONESHOT OUR_LATCH %x CLOCK_TICK_RATE %d\n",  OUR_LATCH, CLOCK_TICK_RATE);
+		dummy_read = *((volatile unsigned long*) (SE_TIMER_VA_BASE+0x0c));
+		/* Load counter values */
+		/* period set, and timer enabled in 'next_event' hook */
+		ctrl = 0x0;//One shot
+		break;
+	case CLOCK_EVT_MODE_UNUSED:
+	case CLOCK_EVT_MODE_SHUTDOWN:
+	default:
+		ctrl = 0;
+	}
+	/* Enable Interrupts and timer */
+	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = ctrl;
+	printk("0 %x 4 %x 8 %x c %x\n", *((volatile unsigned int*)(SE_TIMER_VA_BASE+0x0)), 
+		*((volatile unsigned int*)(SE_TIMER_VA_BASE+0x04)), *((volatile unsigned int*)(SE_TIMER_VA_BASE+0x08)),
+		*((volatile unsigned int*)(SE_TIMER_VA_BASE+0x0c)));
 }
 
-static struct irqaction se_pilot2_timer_irq = {
-	.name		= "Pilot2 Timer Tick",
-	.handler	= se_pilot2_timer_interrupt,
-	.flags		= IRQF_DISABLED|IRQF_TIMER|IRQF_IRQPOLL,
+static struct clock_event_device pilot_clockevent = {
+	.name		= "pilot_sys_timer1",
+	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
+	.set_next_event	= pilot_set_next_event,
+	.set_mode	= pilot_set_mode,
+	.rating		= 300,
 };
 
-void __init
-se_pilot2_timer_init(void)
+static cycle_t notrace pilot3_read_cycles(struct clocksource *cs)
 {
-	volatile unsigned long dummy_read;
-	int i;
+        return (cycle_t) (0xffffffff - *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14 +0x4)));
+}
 
-	/* Disable timer and interrups */
-	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x0;
-	dummy_read = *((volatile unsigned long*) (SE_TIMER_VA_BASE+0x0c));
+cycle_t notrace pilot3_read_raw_cycle(void)
+{
+        return (cycle_t) (0xffffffff - *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14 +0x4)));
+}
 
-	/* Setup Timer Interrupt routine */
-	setup_irq(IRQ_TIMER_0, &se_pilot2_timer_irq);
+unsigned int  notrace read_raw_cycle(void)
+{
+        return (u32)*((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14 +0x4));
+}
+EXPORT_SYMBOL(read_raw_cycle);
+static struct clocksource pilot_clk_src = {
+        .name           = "pilot3_timer2",
+        .rating         = 200,
+        .read           = pilot3_read_cycles,
+        .mask           = CLOCKSOURCE_MASK(32),
+        .flags          = CLOCK_SOURCE_IS_CONTINUOUS,
+};
 
-	/* Load counter values */
-    	*((volatile unsigned long *)(SE_TIMER_VA_BASE)) = LATCH;
 
-	/* Enable Interrupts and timer */
-	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x00000003;
+#ifdef CONFIG_HAVE_ARM_TWD
+static DEFINE_TWD_LOCAL_TIMER(twd_local_timer,
+			      0x40460600, 29);
+#endif
+static void __init clk_src_pilot_init(void)
+{
 
-	printk("Pilot-II Timer Initialized\n");
-	
-	/*Patch: Sometimes the timer does not start. So make sure the timer is 
-	 really running. Otherwise, the kernel will hang at calibrate_delay.
-         If the timer does not start, disable and renable agai, till it starts*/
-	while( (*((volatile unsigned long *)((SE_TIMER_VA_BASE+0x04)))) >(LATCH) )
-        {
-		printk("WARNING: Pilot-II Timer Not Started Yet\n");
-		*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x0;
-		*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08)) = 0x00000003;
-	 	for(i=0;i<10;i++);
+       /* timer load already set up */
+       /* Disable timer and interrups */
+       *((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08 +0x14)) = 0x0;
+
+       /* Load counter values */
+       *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14)) = (0xffffffff);
+        printk("Registering clock source timercrnt addr %x\n", (SE_TIMER_VA_BASE + 0x14 +0x4));
+
+       *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x8 + 0x14)) = 5;//no interrupt ,free running , start
+
+
+        while( (*((volatile unsigned long *)((SE_TIMER_VA_BASE+ 0x14+ 0x04)))) >(OUR_LATCH) )
+        {  int i;
+                printk("WARNING: Pilot-II Timer Not Started Yet\n");
+                *((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08 + 0x14)) = 0x0;
+                *((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08 + 0x14)) = 5 ;
+                for( i=0;i<10;i++);
         }
-	printk("Pilot-II Timer Started\n");
 
+        clocksource_register_hz(&pilot_clk_src, 25000000);
+}
+
+void __init
+se_pilot3_timer_init(void)
+{
+
+	//int  err;
+ pilot_clockevent.cpumask = cpumask_of(smp_processor_id());
+	clockevents_config_and_register(&pilot_clockevent, 25*1000*1000,
+					0xff, 0xffffffff);
+	/* Setup Timer Interrupt routine */
+	setup_irq(IRQ_TIMER_0, &se_pilot3_timer_irq);
+	printk("mult %x shift %x max_delta_ns %llx min_delta_ns %llx\n",pilot_clockevent.mult, pilot_clockevent.shift, pilot_clockevent.max_delta_ns, pilot_clockevent.min_delta_ns);
+	printk("Pilot-4 Timer configured\n");
+#ifdef CONFIG_HAVE_ARM_TWD
+	err = twd_local_timer_register(&twd_local_timer);
+	if (err)
+		pr_err("twd_local_timer_register failed %d\n", err);
+#endif
+	clk_src_pilot_init();
 	return;
 }
 
