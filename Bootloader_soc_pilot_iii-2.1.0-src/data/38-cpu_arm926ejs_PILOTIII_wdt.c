--- uboot/cpu/arm926ejs/PILOTIII/wdt.c	2013-06-18 09:59:57.000000000 +0200
+++ uboot_new/cpu/arm926ejs/PILOTIII/wdt.c	2013-06-18 10:00:19.000000000 +0200
@@ -0,0 +1,77 @@
+#ifdef CONFIG_SPX_FEATURE_FAIL_SAFE_BOOTING
+
+#include <pilotiii_hw.h>
+#define WATCHDOG_WDCTL_RUN 0x01
+#define WATCHDOG_SYSRCR_ENABLE 0x01
+#define WATCHDOG_SYSWRER_VALUE 0x00
+
+#define WATCHDOG_RESET_ARM 0x01
+#define WATCHDOG_RESET_I2C 0xFF00
+
+#define WATCHDOG_TRIGGER 0x80
+
+/**
+ * @fn Enable_watchdog
+ * @brief Enables watchdog reset for modules ARM and I2C devices.
+ * @param[in] wdt_timeout_status - reset status.
+ * @retval  0 - on success.
+ */
+int Enable_watchdog(unsigned long *watchdog_reset_status)
+{
+       int wdt_count=300; // 5 minutes
+
+       *((volatile unsigned long *)(WDT_CONTROL_REG)) &= ~WATCHDOG_WDCTL_RUN; 
+       *((volatile unsigned long *)(SYS_RST_CTRL_REG)) |= WATCHDOG_SYSRCR_ENABLE;
+
+       /*pilot spec  recommends to reset only ARM processor */
+       *((volatile unsigned long *)(SYS_WDT_RST_EN)) = WATCHDOG_RESET_ARM; //Watchdod reset to ARM
+       *((volatile unsigned long *)(SYS_WDT_RST_EN)) |= WATCHDOG_RESET_I2C; //Watchdog reset to I2C
+
+       *((volatile unsigned long *)(SE_SYS_CLK_VA_BASE  + 0x00)) &= 0xFFFFFFFC;
+      
+       /* Check whether reset caused by watchdog, bit-0 indicates reset cause */
+       *watchdog_reset_status = *((volatile unsigned long *)(SYS_RST_STAT_REG)) & 0x01;
+       /* Clearing the Watdog Reset Status bit*/
+       *((volatile unsigned long *)(SYS_RST_STAT_REG)) |= 0x01;
+       /* Setting Timeout value to 5 minutes */
+
+
+       //To configure watchdog Timer count
+       /*  
+       char *wdt_test = 0;
+       wdt_test = getenv("wdtcnt");
+       if (wdt_test != 0)
+       {
+                wdt_count = (int)simple_strtoul(wdt_test,0,0);
+       }
+       */
+ 
+       *((volatile unsigned long *)(WDT_LOAD_REG0)) = wdt_count & 0x000000FF;
+       *((volatile unsigned long *)(WDT_LOAD_REG1)) = (wdt_count & 0x0000FF00)>>8;
+
+       // *((volatile unsigned long *)(WDT_LOAD_REG0)) = 0x2c;
+       // *((volatile unsigned long *)(WDT_LOAD_REG1)) = 0x1;
+
+       *((volatile unsigned long *)(WDT_CONTROL_REG)) |= WATCHDOG_WDCTL_RUN; 
+       /* Trigger Watchdog Timer */
+       *((volatile unsigned long *)(WDT_CONTROL_REG)) |= WATCHDOG_TRIGGER;
+       
+return 0;
+}
+
+/**
+ * @fn Disable_watchdog
+ * @brief Disables watchdog reset.
+ * @param[in] void.
+ * @retval  0 - on success.
+ */
+int Disable_watchdog(void)
+{
+
+       *((volatile unsigned long *)(WDT_CONTROL_REG)) &= ~WATCHDOG_WDCTL_RUN;
+
+return 0;
+}
+#endif
+
+
