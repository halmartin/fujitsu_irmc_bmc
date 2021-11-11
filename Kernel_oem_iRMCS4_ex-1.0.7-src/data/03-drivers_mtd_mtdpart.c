--- linux_old/drivers/mtd/mtdpart.c	2015-01-30 13:43:23.270146560 +0530
+++ linux/drivers/mtd/mtdpart.c	2015-01-28 20:03:46.000000000 +0530
@@ -475,6 +475,7 @@
 			part->name, master->name, (unsigned long long)slave->mtd.size);
 	}
 	if (master->numeraseregions > 1) {
+		u_int32_t min_erasesize, curr_erasesize;
 		/* Deal with variable erase size stuff */
 		int i, max = master->numeraseregions;
 		u64 end = slave->offset + slave->mtd.size;
@@ -488,12 +489,17 @@
 		if (i > 0)
 			i--;
 
+		min_erasesize = curr_erasesize = regions[0].erasesize;
 		/* Pick biggest erasesize */
 		for (; i < max && regions[i].offset < end; i++) {
-			if (slave->mtd.erasesize < regions[i].erasesize) {
-				slave->mtd.erasesize = regions[i].erasesize;
+			curr_erasesize = regions[i].erasesize;
+//			if (slave->mtd.erasesize > regions[i].erasesize) {
+			if (curr_erasesize < min_erasesize) {
+//				slave->mtd.erasesize = regions[i].erasesize;
+				min_erasesize = curr_erasesize;
 			}
 		}
+		slave->mtd.erasesize = min_erasesize;
 		BUG_ON(slave->mtd.erasesize == 0);
 	} else {
 		/* Single erase size */
