--- linux-3.14.17/fs/jffs2/gc.c	2014-08-14 07:08:34.000000000 +0530
+++ linux-3.14.17.new/fs/jffs2/gc.c	2017-06-28 18:05:45.595546035 +0530
@@ -1296,14 +1296,18 @@
 		BUG_ON(start > orig_start);
 	}
 
-	/* First, use readpage() to read the appropriate page into the page cache */
-	/* Q: What happens if we actually try to GC the _same_ page for which commit_write()
-	 *    triggered garbage collection in the first place?
-	 * A: I _think_ it's OK. read_cache_page shouldn't deadlock, we'll write out the
-	 *    page OK. We'll actually write it out again in commit_write, which is a little
-	 *    suboptimal, but at least we're correct.
+	/* The rules state that we must obtain the page lock *before* f->sem, so
+	 * drop f->sem temporarily. Since we also hold c->alloc_sem, nothing's
+	 * actually going to *change* so we're safe; we only allow reading.
+	 *
+	 * It is important to note that jffs2_write_begin() will ensure that its
+	 * page is marked Uptodate before allocating space. That means that if we
+	 * end up here trying to GC the *same* page that jffs2_write_begin() is
+	 * trying to write out, read_cache_page() will not deadlock.
 	 */
+	mutex_unlock(&f->sem);
 	pg_ptr = jffs2_gc_fetch_page(c, f, start, &pg);
+	mutex_lock(&f->sem);
 
 	if (IS_ERR(pg_ptr)) {
 		pr_warn("read_cache_page() returned error: %ld\n",
