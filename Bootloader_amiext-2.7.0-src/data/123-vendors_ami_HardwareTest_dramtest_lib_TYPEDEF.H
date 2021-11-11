diff -Naur uboot.org/vendors/ami/HardwareTest/lib/TYPEDEF.H uboot/vendors/ami/HardwareTest/lib/TYPEDEF.H
--- uboot.org/vendors/ami/HardwareTest/lib/TYPEDEF.H	1970-01-01 08:00:00.000000000 +0800
+++ uboot/vendors/ami/HardwareTest/lib/TYPEDEF.H	2014-01-27 15:09:50.973236926 +0800
@@ -0,0 +1,66 @@
+#ifdef CONFIG_SPX_FEATURE_LAN_AND_DRAM_TEST_CMD
+#ifndef TYPEDEF_H
+#define TYPEDEF_H
+
+#include "SWFUNC.H"
+
+//
+// Define
+//
+#define   PCI                   1
+#define   PCIE                  2
+#define   AGP                   3
+#define   ACTIVE                4
+
+#if defined(LinuxAP)
+    #ifndef FLONG
+        #define FLONG   unsigned long
+    #endif
+    #ifndef ULONG
+        #define ULONG   unsigned long
+    #endif
+    #ifndef LONG
+        #define LONG    long
+    #endif
+    #ifndef USHORT
+        #define USHORT  unsigned short
+    #endif
+    #ifndef SHORT
+        #define SHORT   short
+    #endif
+    #ifndef UCHAR
+        #define UCHAR   unsigned char
+    #endif
+    #ifndef CHAR
+        #define CHAR    char
+    #endif
+    #ifndef BYTE
+        #define BYTE    unsigned char
+    #endif
+    #ifndef VOID
+        #define VOID    void
+    #endif
+    #ifndef SCHAR
+        #define SCHAR   signed char
+    #endif
+#else	
+/* DOS Program */
+    #define     VOID	  void
+    #define     FLONG     unsigned long
+    #define     ULONG     unsigned long
+    #define     USHORT    unsigned short
+    #define     UCHAR     unsigned char
+    #define     LONG      long
+    #define     SHORT     short
+    #define     CHAR      char
+    #define     BYTE	  UCHAR
+    #define     BOOL	  SHORT
+    #define     BOOLEAN   unsigned short
+    #define     PULONG    ULONG *
+    #define     SCHAR     signed char
+#endif    
+    #define	    TRUE      1
+    #define	    FALSE     0
+    
+#endif // TYPEDEF_H
+#endif
