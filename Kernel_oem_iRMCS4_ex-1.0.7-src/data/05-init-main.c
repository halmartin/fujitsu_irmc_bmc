--- linux_old/init/main.c	2015-01-30 13:45:25.610149503 +0530
+++ linux/init/main.c	2015-01-28 19:28:22.000000000 +0530
@@ -475,6 +475,34 @@
 	pgtable_init();
 	vmalloc_init();
 }
+//FTSCHG_WU_BEGIN
+struct timer_list bmcBootTimer;
+void bmcBootTimerFunc(unsigned long ptr)
+{
+	unsigned char stat;
+	stat = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+0x37));
+	if( (stat & 0x03) == 0x03 ) {
+		*((unsigned char *)(SE_GPIO_VA_BASE + 0x0D)) &= 0xCF; //disable LED Blinking Rate Selection
+		*((unsigned char *)(SE_GPIO_VA_BASE + 0x06)) |= 0x20;  //Event Debounce Enable
+		*((unsigned char *)(SE_GPIO_VA_BASE + 0x08)) &= 0xBF;  //Pin6 GPIO LOW
+	}
+
+	if( (stat & 0x0F) == 0x00 ) { //restart timer utill: PWR is pressed or interrupt for PWR button is registred 
+		bmcBootTimer.expires = jiffies + 50;
+		add_timer(&bmcBootTimer);
+	}
+}
+
+void startBmcBootTimer(void)
+{
+	init_timer(&bmcBootTimer);
+	bmcBootTimer.expires = jiffies + 100;
+	bmcBootTimer.function = bmcBootTimerFunc;
+	bmcBootTimer.data = 0;
+	add_timer(&bmcBootTimer);	
+	printk(KERN_ERR "%s:%i\n",__FUNCTION__,__LINE__);
+}
+//FTSCHG_WU_END
 
 asmlinkage void __init start_kernel(void)
 {
@@ -563,6 +591,7 @@
 	softirq_init();
 	timekeeping_init();
 	time_init();
+	startBmcBootTimer();  //FTSCHG_WU	
 	sched_clock_postinit();
 	perf_event_init();
 	profile_init();
