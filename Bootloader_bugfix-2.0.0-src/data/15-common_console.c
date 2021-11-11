--- uboot/common/console.c	2006-11-02 09:15:01.000000000 -0500
+++ uboot/common/console.c	2012-03-23 10:31:47.424684356 -0400
@@ -98,7 +98,7 @@
 void serial_printf (const char *fmt, ...)
 {
 	va_list args;
-	uint i;
+	//uint i;
 	char printbuffer[CFG_PBSIZE];
 
 	va_start (args, fmt);
@@ -106,7 +106,7 @@
 	/* For this to work, printbuffer must be larger than
 	 * anything we ever want to print.
 	 */
-	i = vsprintf (printbuffer, fmt, args);
+	vsprintf (printbuffer, fmt, args);
 	va_end (args);
 
 	serial_puts (printbuffer);
@@ -143,7 +143,7 @@
 void fprintf (int file, const char *fmt, ...)
 {
 	va_list args;
-	uint i;
+	//uint i;
 	char printbuffer[CFG_PBSIZE];
 
 	va_start (args, fmt);
@@ -151,7 +151,7 @@
 	/* For this to work, printbuffer must be larger than
 	 * anything we ever want to print.
 	 */
-	i = vsprintf (printbuffer, fmt, args);
+	vsprintf (printbuffer, fmt, args);
 	va_end (args);
 
 	/* Send to desired file */
@@ -217,7 +217,7 @@
 void printf (const char *fmt, ...)
 {
 	va_list args;
-	uint i;
+	//uint i;
 	char printbuffer[CFG_PBSIZE];
 
 	va_start (args, fmt);
@@ -225,7 +225,7 @@
 	/* For this to work, printbuffer must be larger than
 	 * anything we ever want to print.
 	 */
-	i = vsprintf (printbuffer, fmt, args);
+	vsprintf (printbuffer, fmt, args);
 	va_end (args);
 
 	/* Print the string */
@@ -234,13 +234,13 @@
 
 void vprintf (const char *fmt, va_list args)
 {
-	uint i;
+	//uint i;
 	char printbuffer[CFG_PBSIZE];
 
 	/* For this to work, printbuffer must be larger than
 	 * anything we ever want to print.
 	 */
-	i = vsprintf (printbuffer, fmt, args);
+	vsprintf (printbuffer, fmt, args);
 
 	/* Print the string */
 	puts (printbuffer);
