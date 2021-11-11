--- linux-3.14.17/fs/jffs2/file.c	2014-08-14 07:08:34.000000000 +0530
+++ linux-3.14.17.new/fs/jffs2/file.c	2017-06-28 18:21:02.367561515 +0530
@@ -139,39 +139,33 @@
 	struct page *pg;
 	struct inode *inode = mapping->host;
 	struct jffs2_inode_info *f = JFFS2_INODE_INFO(inode);
-	struct jffs2_sb_info *c = JFFS2_SB_INFO(inode->i_sb);
-	struct jffs2_raw_inode ri;
-	uint32_t alloc_len = 0;
 	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
 	uint32_t pageofs = index << PAGE_CACHE_SHIFT;
 	int ret = 0;
 
-	jffs2_dbg(1, "%s()\n", __func__);
-
-	if (pageofs > inode->i_size) {
-		ret = jffs2_reserve_space(c, sizeof(ri), &alloc_len,
-					  ALLOC_NORMAL, JFFS2_SUMMARY_INODE_SIZE);
-		if (ret)
-			return ret;
-	}
-
-	mutex_lock(&f->sem);
 	pg = grab_cache_page_write_begin(mapping, index, flags);
-	if (!pg) {
-		if (alloc_len)
-			jffs2_complete_reservation(c);
-		mutex_unlock(&f->sem);
+	if (!pg) 
 		return -ENOMEM;
-	}
 	*pagep = pg;
 
-	if (alloc_len) {
+	jffs2_dbg(1, "%s()\n", __func__);
+	if (pageofs > inode->i_size) {
 		/* Make new hole frag from old EOF to new page */
+		struct jffs2_sb_info *c = JFFS2_SB_INFO(inode->i_sb);
+		struct jffs2_raw_inode ri;
 		struct jffs2_full_dnode *fn;
+		uint32_t alloc_len;
 
 		jffs2_dbg(1, "Writing new hole frag 0x%x-0x%x between current EOF and new page\n",
 			  (unsigned int)inode->i_size, pageofs);
 
+		ret = jffs2_reserve_space(c, sizeof(ri), &alloc_len,
+					  ALLOC_NORMAL, JFFS2_SUMMARY_INODE_SIZE);
+
+		if (ret)
+			goto out_page;
+
+		mutex_lock(&f->sem);
 		memset(&ri, 0, sizeof(ri));
 
 		ri.magic = cpu_to_je16(JFFS2_MAGIC_BITMASK);
@@ -198,6 +192,7 @@
 		if (IS_ERR(fn)) {
 			ret = PTR_ERR(fn);
 			jffs2_complete_reservation(c);
+			mutex_unlock(&f->sem);
 			goto out_page;
 		}
 		ret = jffs2_add_full_dnode_to_inode(c, f, fn);
@@ -212,10 +207,12 @@
 			jffs2_mark_node_obsolete(c, fn->raw);
 			jffs2_free_full_dnode(fn);
 			jffs2_complete_reservation(c);
+			mutex_unlock(&f->sem);
 			goto out_page;
 		}
 		jffs2_complete_reservation(c);
 		inode->i_size = pageofs;
+		mutex_unlock(&f->sem);
 	}
 
 	/*
@@ -224,18 +221,18 @@
 	 * case of a short-copy.
 	 */
 	if (!PageUptodate(pg)) {
+		mutex_lock(&f->sem);
 		ret = jffs2_do_readpage_nolock(inode, pg);
+		mutex_unlock(&f->sem);
 		if (ret)
 			goto out_page;
 	}
-	mutex_unlock(&f->sem);
 	jffs2_dbg(1, "end write_begin(). pg->flags %lx\n", pg->flags);
 	return ret;
 
 out_page:
 	unlock_page(pg);
 	page_cache_release(pg);
-	mutex_unlock(&f->sem);
 	return ret;
 }
 
