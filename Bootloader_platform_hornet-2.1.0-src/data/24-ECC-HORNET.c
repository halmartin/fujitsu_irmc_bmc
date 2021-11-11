--- /usr/src/dev/killme/build/hornet/Build/bootloader/uboot/board/HORNET/HORNET.c	2011-10-25 14:38:23.000000000 +0800
+++ uboot/board/HORNET/HORNET.c	2011-10-21 12:01:32.000000000 +0800
@@ -22,10 +22,20 @@
 int dram_init (void)
 {
 	bd_t *bd = gd->bd;
+	int remainder;
+	int mem_size = CFG_SDRAM_LEN;
 
 	/* Only one Bank*/
 	bd->bi_dram[0].start = CFG_SDRAM_BASE;
-	bd->bi_dram[0].size = CFG_SDRAM_LEN;
+
+	/* Check available memory and fill into bdinfo*/
+	remainder = mem_size % (1024*1024);
+	if(remainder != 0){
+	    //ex : 117.1MB is not accept by Linux kernel
+	    bd->bi_dram[0].size = mem_size - remainder;
+	}else{
+	    bd->bi_dram[0].size = mem_size;
+	}
 
 
 	return (0);
