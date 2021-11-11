--- linux-3.14.17/drivers/tty/serial/8250/8250_core.c	2014-08-14 07:08:34.000000000 +0530
+++ linux-3.14.17.new/drivers/tty/serial/8250/8250_core.c	2014-09-04 12:08:12.269829703 +0530
@@ -1318,6 +1318,15 @@
 		up->acr &= ~UART_ACR_TXDIS;
 		serial_icr_write(up, UART_ACR, up->acr);
 	}
+
+    // Pilot3 Amazon issue 8/3/2011 -- SOL chars are getting stuck in tx buffer
+    // If we aren't transmitting, force start regardless.
+    {
+        unsigned char lsr = serial_in(up, UART_LSR);
+        up->lsr_saved_flags |= lsr & LSR_SAVE_FLAGS;
+        if (lsr & UART_LSR_TEMT)
+            serial8250_tx_chars(up);
+    }
 }
 
 static void serial8250_stop_rx(struct uart_port *port)
@@ -2336,7 +2345,11 @@
 	 * have sufficient FIFO entries for the latency of the remote
 	 * UART to respond.  IOW, at least 32 bytes of FIFO.
 	 */
-	if (up->capabilities & UART_CAP_AFE && port->fifosize >= 32) {
+#if 1
+	if (up->capabilities & UART_CAP_AFE){
+#else
+	if (up->capabilities & UART_CAP_AFE && up->port.fifosize >= 32) {
+#endif
 		up->mcr &= ~UART_MCR_AFE;
 		if (termios->c_cflag & CRTSCTS)
 			up->mcr |= UART_MCR_AFE;
@@ -2814,8 +2827,15 @@
 		port->membase  = old_serial_port[i].iomem_base;
 		port->iotype   = old_serial_port[i].io_type;
 		port->regshift = old_serial_port[i].iomem_reg_shift;
+		//#ifdef CONFIG_SOC_SE_PILOT3 || CONFIG_SOC_SE_PILOT4
+		port->type 	  = PORT_16550A;
+		port->fifosize = uart_config[up->port.type].fifo_size;
+		up->capabilities = uart_config[up->port.type].flags;
+		up->tx_loadsz = uart_config[up->port.type].tx_loadsz;
+    //          #endif
 		set_io_from_upio(port);
 		port->irqflags |= irqflag;
+		
 		if (serial8250_isa_config != NULL)
 			serial8250_isa_config(i, &up->port, &up->capabilities);
 
