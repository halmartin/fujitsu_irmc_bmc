--- linux_old/drivers/mtd/mtdconcat.c	2015-01-30 13:43:23.594146568 +0530
+++ linux/drivers/mtd/mtdconcat.c	2015-01-28 19:55:43.000000000 +0530
@@ -681,7 +681,7 @@
 	int i;
 	size_t size;
 	struct mtd_concat *concat;
-	uint32_t max_erasesize, curr_erasesize;
+	u_int32_t min_erasesize, curr_erasesize;
 	int num_erase_region;
 	int max_writebufsize = 0;
 
@@ -807,7 +807,7 @@
 	 * first, walk the map of the new device and see how
 	 * many changes in erase size we have
 	 */
-	max_erasesize = curr_erasesize = subdev[0]->erasesize;
+	min_erasesize = curr_erasesize = subdev[0]->erasesize;
 	num_erase_region = 1;
 	for (i = 0; i < num_devs; i++) {
 		if (subdev[i]->numeraseregions == 0) {
@@ -816,8 +816,8 @@
 				/* if it differs from the last subdevice's erase size, count it */
 				++num_erase_region;
 				curr_erasesize = subdev[i]->erasesize;
-				if (curr_erasesize > max_erasesize)
-					max_erasesize = curr_erasesize;
+				if (curr_erasesize < min_erasesize)
+					min_erasesize = curr_erasesize;
 			}
 		} else {
 			/* current subdevice has variable erase size */
@@ -831,8 +831,8 @@
 					curr_erasesize =
 					    subdev[i]->eraseregions[j].
 					    erasesize;
-					if (curr_erasesize > max_erasesize)
-						max_erasesize = curr_erasesize;
+					if (curr_erasesize < min_erasesize)
+						min_erasesize = curr_erasesize;
 				}
 			}
 		}
@@ -855,7 +855,7 @@
 		struct mtd_erase_region_info *erase_region_p;
 		uint64_t begin, position;
 
-		concat->mtd.erasesize = max_erasesize;
+		concat->mtd.erasesize = min_erasesize;
 		concat->mtd.numeraseregions = num_erase_region;
 		concat->mtd.eraseregions = erase_region_p =
 		    kmalloc(num_erase_region *
