--- linux-3.14.17/include/linux/mm.h	2014-08-13 21:38:34.000000000 -0400
+++ linux/include/linux/mm.h	2016-10-26 16:41:18.053114618 -0400
@@ -1040,6 +1040,7 @@
 extern void show_free_areas(unsigned int flags);
 extern bool skip_free_areas_node(unsigned int flags, int nid);
 
+void shmem_set_file(struct vm_area_struct *vma, struct file *file);
 int shmem_zero_setup(struct vm_area_struct *);
 
 extern int can_do_mlock(void);
@@ -1944,6 +1945,7 @@
 #define FOLL_HWPOISON	0x100	/* check page is hwpoisoned */
 #define FOLL_NUMA	0x200	/* force NUMA hinting page fault */
 #define FOLL_MIGRATION	0x400	/* wait for page to replace migration entry */
+#define FOLL_COW	0x4000  /* internal GUP flag */
 
 typedef int (*pte_fn_t)(pte_t *pte, pgtable_t token, unsigned long addr,
 			void *data);
