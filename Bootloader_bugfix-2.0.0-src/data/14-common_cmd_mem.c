--- uboot/common/cmd_mem.c	2012-03-23 11:03:47.433438359 -0400
+++ uboot/common/cmd_mem.c	2012-03-23 10:30:15.063434369 -0400
@@ -638,7 +638,7 @@
 
 int do_mem_loop (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
 {
-	ulong	addr, length, i, junk;
+	ulong	addr, length, i, junk=junk;
 	int	size;
 	volatile uint	*longp;
 	volatile ushort *shortp;
