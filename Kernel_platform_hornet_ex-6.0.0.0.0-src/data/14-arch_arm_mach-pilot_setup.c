diff -Naur linux_org/arch/arm/mach-pilot/setup.c linux/arch/arm/mach-pilot/setup.c
--- linux_org/arch/arm/mach-pilot/setup.c	1969-12-31 19:00:00.000000000 -0500
+++ linux/arch/arm/mach-pilot/setup.c	2016-10-07 12:56:04.415985294 -0400
@@ -0,0 +1,164 @@
+/*
+ *  linux/arch/arm/mach-se_pilot3/setup.c
+ *
+ *  Copyright (C) 2005 American Megatrends Inc
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
+ *
+ */
+#include <linux/mm.h>
+#include <linux/init.h>
+#include <linux/major.h>
+#include <linux/fs.h>
+#include <linux/device.h>
+#include <linux/serial.h>
+#include <linux/tty.h>
+#include <linux/serial_core.h>
+#include <linux/serial_8250.h>
+
+#include <asm/io.h>
+#include <asm/pgtable.h>
+#include <asm/page.h>
+#include <asm/mach/map.h>
+#include <asm/setup.h>
+#include <asm/system.h>
+#include <asm/memory.h>
+#include <mach/hardware.h>
+#include <asm/mach-types.h>
+#include <asm/mach/arch.h>
+#include <mach/system.h>
+#include "../../../drivers/mtd/spichips/spiflash.h"
+
+static struct map_desc aviator_std_desc[] __initdata =
+{
+	/* SE-PILOTIII Register mappings */
+	PILOT_MAP_DESC,
+ 	/* HORNET Flash Space */
+ 	{CPE_FLASH_VA_BASE, 	__phys_to_pfn(CPE_FLASH_BASE),		CPE_FLASH_SZ, 	MT_DEVICE },
+};
+
+int io_mapping_done = 0;
+
+#define flush()  do { } while(0)
+
+/*static struct uart_port early_serial_ports[] =
+{
+	{
+		.membase	= (char *)SE_UART_2_VA_BASE,
+		.mapbase	= SE_UART_2_BASE,
+		.irq		= IRQ_UART_2,
+		.flags		= UPF_SKIP_TEST,
+		.iotype		= UPIO_PORT,
+		.regshift	= 2,
+		.uartclk	= CONFIG_UART_CLK,
+		.line		= 0,
+		.type		= PORT_16550A,
+		.fifosize	= 16
+	}
+};
+
+extern void printascii(char *Str);
+void
+EarlyPrintk(char *Str)
+{
+	printascii(Str);
+}*/
+
+static inline void pilot3_arch_idle(void)
+{
+  /*    
+   * This should do all the clock switching
+   * and wait for interrupt tricks
+   */
+  cpu_do_idle();
+}
+
+static void pilot3_arch_reset(enum reboot_mode reboot_mode, const char *cmd)
+{
+//  void (*resetloc)(void);
+ // volatile unsigned long resetspi;
+ 
+  int spi_bank = 0;
+
+  local_irq_disable();
+  
+   
+   
+   // Restore the Boot SPI device to default state  (In uboot code, SPI bank (ractrends.c) starts with index 0)
+  restore_spidevice_to_default_state(spi_bank);
+
+  /* Disable all interrupts */
+  *((volatile unsigned long*) Pilot2_Irq_intmask_Reg) = 0xFFFFFFFF;
+  *((volatile unsigned long*) Pilot2_Irq_inten_Reg)   = 0;
+  *((volatile unsigned long*) Pilot2_Combined_Irq_Ctrl_Reg) = 0x0;
+
+  *((volatile unsigned long*) Pilot2_Fiq_intmask_Reg) = 0xFFFFFFFF;
+  *((volatile unsigned long*) Pilot2_Fiq_inten_Reg)   = 0;
+  *((volatile unsigned long*) Pilot2_Combined_Fiq_Ctrl_Reg) = 0x0;
+
+  *((volatile unsigned long*) Pilot2_Irq_Ctrl_Hi_Reg) = 0x0;
+  *((volatile unsigned long*) Pilot2_Fiq_Ctrl_Hi_Reg) = 0x0;
+
+  /* Bring down SPI clock */
+  *(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+0x20) &= ~SPI_CLK_MAX;
+  *(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+0x20) |= SPI_CLK_DEFAULT;
+/* Bring down SPI clock */
+  *(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+0x20) &= ~SPI_CLK_MAX;
+  *(volatile unsigned long*)(SE_SYS_CLK_VA_BASE+0x20) |= SPI_CLK_DEFAULT;
+
+  // clear the cold reset bit
+  *((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x00)) &= 0xFFFFFFFC;
+
+  //Reset using software reset control registers
+  *(volatile unsigned long*)(SE_RES_DEB_VA_BASE+0x04) &= 0xFFFFFFFD;
+  *(volatile unsigned long*)(SE_RES_DEB_VA_BASE+0x08) = 0;
+  *(volatile unsigned long*)(SE_RES_DEB_VA_BASE+0x0C) |= 0x00081; //Reset the SPI controller along with ARM CPU reset
+  *(volatile unsigned long*)(SE_RES_DEB_VA_BASE+0x00) = 0x00003002;
+ 
+  printk("arch_reset() failed \n");
+
+  while (1);
+}
+
+
+void __init
+se_pilot2_init(void)
+{
+	/* Do any initial hardware setup if needed */
+	*((volatile unsigned long *)(SEDID)) = 0x00020200;
+  /* Register idle and restart handler */
+  arm_pm_idle = pilot3_arch_idle;
+  arm_pm_restart = pilot3_arch_reset;
+}
+
+void __init
+se_pilot2_map_io(void)
+{
+	/* Do any initial hardware setup if needed */
+	iotable_init(aviator_std_desc, ARRAY_SIZE(aviator_std_desc));
+	io_mapping_done = 1;
+	//early_serial_setup(&early_serial_ports[0]);	
+}
+
+extern void se_pilot3_timer_init(void);
+extern void se_pilot2_init_irq(void);
+
+MACHINE_START(HORNET, "ServerEngines Hornet Board")
+	.map_io			= se_pilot2_map_io,
+	.init_time			= se_pilot3_timer_init,
+	.init_irq		= se_pilot2_init_irq,
+	.atag_offset    = 0x100,
+	.init_machine   = se_pilot2_init,
+MACHINE_END
