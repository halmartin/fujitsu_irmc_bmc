diff -Naur linux_old/include/linux/serial_8250.h linux/include/linux/serial_8250.h
--- linux_old/include/linux/serial_8250.h	2018-08-16 10:53:13.116892169 +0530
+++ linux/include/linux/serial_8250.h	2018-08-16 10:53:50.901194578 +0530
@@ -98,6 +98,10 @@
 	/* 8250 specific callbacks */
 	int			(*dl_read)(struct uart_8250_port *);
 	void			(*dl_write)(struct uart_8250_port *, int);
+#if defined CONFIG_SOC_SE_PILOT3 || defined CONFIG_SOC_SE_PILOT4
+	void (*console_log_callback_rx)(const char ch);
+	void (*console_log_callback_tx)(const char ch);
+#endif
 };
 
 int serial8250_register_8250_port(struct uart_8250_port *);
@@ -125,5 +129,11 @@
 extern void serial8250_set_isa_configurator(void (*v)
 					(int port, struct uart_port *up,
 						unsigned short *capabilities));
+#if defined CONFIG_SOC_SE_PILOT3 || defined CONFIG_SOC_SE_PILOT4
+/* console logging support */
+typedef void (*serial8250_console_log_callback_t)(const char ch);
+int serial8250_set_console_log_callback(const int tty_line, serial8250_console_log_callback_t callback);
+int serial8250_set_console_log_callback_tx(const int tty_line, serial8250_console_log_callback_t callback);
+#endif
 
 #endif
