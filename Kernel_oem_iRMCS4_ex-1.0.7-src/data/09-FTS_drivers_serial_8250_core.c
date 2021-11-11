diff -Naur linux_old/drivers/tty/serial/8250/8250_core.c linux/drivers/tty/serial/8250/8250_core.c
--- linux_old/drivers/tty/serial/8250/8250_core.c	2018-08-16 10:56:44.508900400 +0530
+++ linux/drivers/tty/serial/8250/8250_core.c	2018-08-16 11:00:31.663859591 +0530
@@ -1421,6 +1421,10 @@
 
 		uart_insert_char(port, lsr, UART_LSR_OE, ch, flag);
 
+#if defined CONFIG_SOC_SE_PILOT3 || defined CONFIG_SOC_SE_PILOT4
+		if (up->console_log_callback_rx)
+			up->console_log_callback_rx(ch);
+#endif
 ignore_char:
 		lsr = serial_in(up, UART_LSR);
 	} while ((lsr & (UART_LSR_DR | UART_LSR_BI)) && (max_count-- > 0));
@@ -1455,6 +1459,10 @@
 	count = up->tx_loadsz;
 	do {
 		serial_out(up, UART_TX, xmit->buf[xmit->tail]);
+#if defined CONFIG_SOC_SE_PILOT3 || defined CONFIG_SOC_SE_PILOT4
+		if (up->console_log_callback_tx)
+			up->console_log_callback_tx(xmit->buf[xmit->tail]);
+#endif
 		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
 		port->icount.tx++;
 		if (uart_circ_empty(xmit))
@@ -3445,6 +3453,44 @@
 EXPORT_SYMBOL(serial8250_suspend_port);
 EXPORT_SYMBOL(serial8250_resume_port);
 
+#if defined CONFIG_SOC_SE_PILOT3 || defined CONFIG_SOC_SE_PILOT4
+
+int serial8250_set_console_log_callback(const int tty_line,
+	serial8250_console_log_callback_t callback)
+{
+	printk(KERN_DEBUG "%s() tty_line = %d, callback = 0x%p\n",
+		__FUNCTION__, tty_line, callback);
+
+	if ( tty_line < 0 || tty_line >= UART_NR ) {
+		printk( KERN_ERR "%s() tty_line %d is not valid\n",
+			__FUNCTION__, tty_line);
+		return -EINVAL;
+	}
+	serial8250_ports[tty_line].console_log_callback_rx = callback;
+	return 0;
+}
+
+int serial8250_set_console_log_callback_tx(const int tty_line,
+	serial8250_console_log_callback_t callback)
+{
+	printk(KERN_DEBUG "%s() tty_line = %d, callback = 0x%p\n",
+		__FUNCTION__, tty_line, callback);
+
+	if ( tty_line < 0 || tty_line >= UART_NR ) {
+		printk( KERN_ERR "%s() tty_line %d is not valid\n",
+			__FUNCTION__, tty_line);
+		return -EINVAL;
+	}
+
+	serial8250_ports[tty_line].console_log_callback_tx = callback;
+
+	return 0;
+}
+
+EXPORT_SYMBOL(serial8250_set_console_log_callback);
+EXPORT_SYMBOL(serial8250_set_console_log_callback_tx);
+
+#endif // CONFIG_SOC_SE_PILOT3/4
 MODULE_LICENSE("GPL");
 MODULE_DESCRIPTION("Generic 8250/16x50 serial driver");
 
