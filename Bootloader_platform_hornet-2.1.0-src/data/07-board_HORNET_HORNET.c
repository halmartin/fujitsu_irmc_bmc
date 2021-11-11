--- uboot/board/HORNET/HORNET.c	1969-12-31 19:00:00.000000000 -0500
+++ uboot.ok/board/HORNET/HORNET.c	2010-05-21 14:23:42.117302374 -0400
@@ -0,0 +1,40 @@
+#include <common.h>
+#include <config.h>
+#include <asm/processor.h>
+
+DECLARE_GLOBAL_DATA_PTR;
+
+extern void soc_init(void);
+
+int board_init (void)
+{
+	/*Should match with linux mach type for this board */
+	gd->bd->bi_arch_number = 5555;
+	/* Should match with linux Makefile.boot entry for params-phys-y */
+	gd->bd->bi_boot_params = CFG_SDRAM_BASE+0x0100;
+
+	/* Initialize SOC related */
+	soc_init();
+	return 0;
+}
+
+
+int dram_init (void)
+{
+	bd_t *bd = gd->bd;
+
+	/* Only one Bank*/
+	bd->bi_dram[0].start = CFG_SDRAM_BASE;
+	bd->bi_dram[0].size = CFG_SDRAM_LEN;
+
+
+	return (0);
+}
+
+short 
+ReadJumperConfig(void)
+{
+	/* Return Path ID */
+	return -1;
+}
+
