--- u-boot-1.1.6/cpu/arm926ejs/PILOTIII/astmac.h	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/cpu/arm926ejs/PILOTIII/astmac.h	2010-04-05 16:47:24.000000000 -0400
@@ -0,0 +1,54 @@
+#ifndef _ASTMAC_H_
+#define _ASTMAC_H
+
+#define RXDES_NUM   		16
+#define TXDES_NUM			8
+#define RX_BUF_SIZE			1536
+#define TX_BUF_SIZE			1536
+
+typedef struct
+{
+	u32 Status;
+	u32 Length;
+	u32 Buffer1;
+	u32 Buffer2;
+} RECV_DESC;
+
+typedef struct
+{
+	u32 Status;
+	u32 Length;
+	u32 Buffer1;
+	u32 Buffer2;
+} XMIT_DESC;
+
+
+
+/*---------------------------------------------------------------------------------
+ * 							Private Information of driver
+ *---------------------------------------------------------------------------------*/
+typedef struct mac100_local
+{
+	unsigned long 	hw_addr;				// IO Base Addr
+	int 			devnum;					// Instance Number
+
+	volatile RECV_DESC 	*rx_descs;			// receive ring base address
+	volatile RECV_DESC	*rx_descs_dma;		// receive ring base physical address
+	volatile char		*rx_buf;			// receive buffer cpu address
+	volatile char		*rx_buf_dma;		// receive buffer physical address
+	int					rx_idx;				// receive descriptor
+
+	volatile XMIT_DESC 	*tx_descs;
+	volatile XMIT_DESC	*tx_descs_dma;
+	volatile char		*tx_buf;
+	volatile char		*tx_buf_dma;
+	int					tx_idx;
+
+	volatile XMIT_DESC 	*setup_descs;
+	volatile XMIT_DESC	*setup_descs_dma;
+	volatile char		*setup_frame;
+	volatile char		*setup_frame_dma;
+
+} AST_ETH_PRIV_DATA;
+
+#endif
