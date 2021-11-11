--- u-boot-1.1.6/cpu/arm926ejs/PILOTIII/asttimer.c	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/cpu/arm926ejs/PILOTIII/asttimer.c	2012-12-06 16:50:00.000000000 +0800
@@ -0,0 +1,157 @@
+/*
+ * (C) Copyright 2006
+ * American Megatrends Inc.
+ *
+ * Timer functions for the Pilot-II SOC
+ *
+ * See file CREDITS for list of people who contributed to this
+ * project.
+ *
+ * This program is free software; you can redistribute it and/or
+ * modify it under the terms of the GNU General Public License as
+ * published by the Free Software Foundation; either version 2 of
+ * the License, or (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program; if not, write to the Free Software
+ * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
+ * MA 02111-1307 USA
+ */
+
+#include <common.h>
+#include <arm926ejs.h>
+#include <asm/proc-armv/ptrace.h>
+#include <pilotiii_hw.h>
+
+#define READ_TIMER (*(volatile ulong *)(TIMER1_COUNT_REG));
+#define TIMER_LOAD_VAL P_CLK
+
+#define ULONG_ROLLVER_CORRECTION (~(0UL)/(P_CLK/1000))
+
+static volatile ulong timestamp;
+static volatile ulong lastdec;
+
+int timer_init (void)
+{
+
+	/* Load the counter and start timer */
+	*(volatile ulong *)(TIMER1_LOAD_REG) = TIMER_LOAD_VAL;
+	*(volatile ulong *)(TIMER1_CONTROL_REG) = TIMER_ENABLE | TIMER_PERIODICAL;
+
+	/* init the timestamp and lastdec value */
+	reset_timer_masked();
+
+	return (0);
+}
+
+/*
+ * timer without interrupts
+ */
+
+void reset_timer (void)
+{
+	reset_timer_masked ();
+}
+
+ulong get_timer (ulong base)
+{
+	ulong time_mask = get_timer_masked();
+	if ((int)(time_mask - base) < 0)
+		return ((ULONG_ROLLVER_CORRECTION - base) + time_mask);
+
+	return (time_mask - base);
+}
+
+void set_timer (ulong t)
+{
+	timestamp = t;
+}
+
+/* delay x useconds AND perserve advance timstamp value */
+void udelay (unsigned long usec)
+{
+	udelay_masked(usec);
+}
+
+void reset_timer_masked (void)
+{
+	/* reset time */
+	lastdec = READ_TIMER;  /* capure current decrementer value time */
+	timestamp = 0;	       /* start "advancing" time stamp from 0 */
+}
+
+ulong get_timer_masked (void)
+{
+	volatile ulong now = READ_TIMER;		/* current tick value */
+
+	if (lastdec >= now) {		/* normal mode (non roll) */
+		/* normal mode */
+		timestamp += (lastdec - now); /* move stamp fordward with absoulte diff ticks */
+	} else {			/* we have overflow of the count down timer */
+		/* nts = ts + ld + (TLV - now)
+		 * ts=old stamp, ld=time that passed before passing through -1
+		 * (TLV-now) amount of time after passing though -1
+		 * nts = new "advancing time stamp"...it could also roll and cause problems.
+		 */
+		timestamp += (lastdec + TIMER_LOAD_VAL - now);
+	}
+	lastdec = now;
+
+	return (timestamp)/(P_CLK/1000);
+}
+
+/* waits specified delay value and resets timestamp */
+void udelay_masked (unsigned long usec)
+{
+	volatile ulong tmo;
+	volatile ulong endtime;
+	volatile signed long diff;
+	volatile ulong water_mark;
+
+	long small_granul = (usec*1000)/40;
+	unsigned long prev = READ_TIMER; 
+
+	while(small_granul > 0){
+
+	volatile ulong now = READ_TIMER;                /* current tick value */
+	if(prev == now){
+		continue;
+	}
+
+        if (prev >= now) {           /* normal mode (non roll) */
+                /* normal mode */
+                small_granul -= (prev - now); /* move stamp fordward with absoulte diff ticks */
+        } else {                        /* we have overflow of the count down timer */
+                small_granul -= (prev + TIMER_LOAD_VAL - now); /* move stamp fordward with absoulte diff ticks */
+        }
+        prev = now;
+	}
+}
+
+
+/*
+ * This function is derived from PowerPC code (read timebase as long long).
+ * On ARM it just returns the timer value.
+ */
+unsigned long long get_ticks(void)
+{
+	return get_timer(0);
+}
+
+/*
+ * This function is derived from PowerPC code (timebase clock frequency).
+ * On ARM it returns the number of timer ticks per second.
+ */
+ulong get_tbclk (void)
+{
+	ulong tbclk;
+
+	tbclk = CFG_HZ;
+	return tbclk;
+}
+
