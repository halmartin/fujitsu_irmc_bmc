--- linux-3.14.17/mm/memory.c	2014-08-13 21:38:34.000000000 -0400
+++ linux/mm/memory.c	2016-10-26 16:22:02.177106393 -0400
@@ -1441,6 +1441,16 @@
 }
 EXPORT_SYMBOL_GPL(zap_vma_ptes);
 
+/*
+ * FOLL_FORCE can write to even unwritable pte's, but only
+ * after we've gone through a COW cycle and they are dirty.
+ */
+static inline bool can_follow_write_pte(pte_t pte, unsigned int flags)
+{
+	return pte_write(pte) ||
+		((flags & FOLL_FORCE) && (flags & FOLL_COW) && pte_dirty(pte));
+}
+
 /**
  * follow_page_mask - look up a page descriptor from a user-virtual address
  * @vma: vm_area_struct mapping @address
@@ -1561,7 +1571,7 @@
 	}
 	if ((flags & FOLL_NUMA) && pte_numa(pte))
 		goto no_page;
-	if ((flags & FOLL_WRITE) && !pte_write(pte))
+	if ((flags & FOLL_WRITE) && !can_follow_write_pte(pte,flags))
 		goto unlock;
 
 	page = vm_normal_page(vma, address, pte);
@@ -1868,7 +1878,7 @@
 				 */
 				if ((ret & VM_FAULT_WRITE) &&
 				    !(vma->vm_flags & VM_WRITE))
-					foll_flags &= ~FOLL_WRITE;
+					foll_flags |= FOLL_COW;
 
 				cond_resched();
 			}
