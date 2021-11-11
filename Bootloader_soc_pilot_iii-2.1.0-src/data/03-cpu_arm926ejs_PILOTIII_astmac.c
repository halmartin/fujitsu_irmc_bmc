--- uboot/cpu/arm926ejs/PILOTIII/astmac.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot.new/cpu/arm926ejs/PILOTIII/astmac.c	2013-06-26 16:12:40.108408829 +0800
@@ -0,0 +1,1123 @@
+/* Tulip based Ethernet MAC 10/100 Controller for U-boot (1.1.4)*/
+
+#include <config.h>
+#include <common.h>
+#if (CONFIG_COMMANDS & CFG_CMD_NET)
+#include <malloc.h>
+#include <command.h>
+#include <miiphy.h>
+#include "soc_hw.h"
+#include "astmac.h"
+#include "net.h"
+
+#define DEFAULT_CSR11  (CSR11_TT_VAL(0)| CSR11_NTP_VAL(0) | CSR11_RT_VAL(0) | CSR11_NRP_VAL(0))
+#define DEFAULT_CSR6   ((CSR6_TR_VAL(0))|(CSR6_SF))
+#define DEFAULT_CSR0   (CSR0_BAR_RXH | CSR0_DSL_VAL(0) | CSR0_PBL_VAL(0) | CSR0_TAP_VAL(0))
+
+extern int ast_miiphy_write (char *devname, unsigned char addr, unsigned char reg, unsigned short value);
+extern int ast_miiphy_read (char *devname, unsigned char addr, unsigned char reg,  unsigned short *value);
+
+extern int GetPhySpeed(char *devname);
+extern int GetPhyDuplex(char *devname);
+extern int GetDevNum(char *devname);
+
+#define inl(addr) 			(*((volatile u32 *)(addr)))
+#define inw(addr)			(*((volatile u16 *)(addr)))
+#define outl(value, addr)  	(*((volatile u32 *)(addr)) = value)
+#define outb(value, addr)	(*((volatile u8 *)(addr)) = value)
+
+#define virt2phys(x)   (x)
+
+//#define ASTMAC_DEBUG
+
+#define ETH_ZLEN 60
+
+extern int eth_init(bd_t *bd);
+extern void eth_halt(void);
+extern int eth_rx(void);
+extern int eth_send(volatile void *packet, int length);
+
+
+/* Notes:
+      Functions beginning with ast_mac100_ interfaces with hardware
+	  Functions beginning with ast_eth_ interfaces with U-boot
+*/
+
+static  int  ast_mac100_open(struct eth_device *dev);
+static void  ast_mac100_close(struct eth_device *dev);
+static void  ast_mac100_reset(struct eth_device *dev);
+static int 	 ast_mac100_alloc_buffers(AST_ETH_PRIV_DATA *lp);
+static void  ast_mac100_enable( struct eth_device *dev );
+static int   ast_mac100_recv(struct eth_device *dev);
+static  int  ast_mac100_send_packet(struct eth_device *dev, void *packet, int length);
+
+static int 	 ast_eth_init(struct eth_device *dev,bd_t *bd);
+static int   ast_eth_rx(struct eth_device *dev);
+static void  ast_eth_halt(struct eth_device *dev);
+static int   ast_eth_send(struct eth_device *dev, volatile void *packet, int length);
+
+#ifdef  ASTMAC_DEBUG
+void
+ast_mac100_dump(char *Mesg,struct eth_device *dev)
+{
+	unsigned int ioaddr;
+	AST_ETH_PRIV_DATA *lp;
+	int i;
+
+	lp = (AST_ETH_PRIV_DATA *)dev->priv;
+	ioaddr = lp->hw_addr;
+
+	printf("-----------------------------------------------------\n");
+	printf("         AST MAC Dump : %s\n",Mesg);
+	printf("-----------------------------------------------------\n");
+
+	printf("CSR0 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR0)& 0xFF);
+	printf("CSR1 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR1));
+	printf("CSR2 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR2));
+	printf("CSR3 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR3));
+	printf("CSR4 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR4));
+	printf("CSR5 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR5)& 0x7FFFFF);
+	printf("CSR6 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR6));
+	printf("CSR7 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR7)& 0x1FFFF);
+	printf("CSR8 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR8));
+	printf("CSR11 = 0x%08X\n",inl(ioaddr+AST_MAC_CSR11));
+
+	for(i=0;i<RXDES_NUM;i++)
+	{
+		printf("RX Desc[%d].Status  = 0x%08x\n",i, lp->rx_descs[i].Status);
+		printf("RX Desc[%d].Length  = 0x%08x\n",i, lp->rx_descs[i].Length);
+		printf("RX Desc[%d].Buffer1 = 0x%08x\n",i, lp->rx_descs[i].Buffer1);
+		printf("RX Desc[%d].Buffer2 = 0x%08x\n",i, lp->rx_descs[i].Buffer2);
+	}
+
+	for(i=0;i<TXDES_NUM;i++)
+	{
+		printf("TX Desc[%d].Status  = 0x%08x\n",i, lp->tx_descs[i].Status);
+		printf("TX Desc[%d].Length  = 0x%08x\n",i, lp->tx_descs[i].Length);
+		printf("TX Desc[%d].Buffer1 = 0x%08x\n",i, lp->tx_descs[i].Buffer1);
+		printf("TX Desc[%d].Buffer2 = 0x%08x\n",i, lp->tx_descs[i].Buffer2);
+	}
+	return;
+}
+#else
+#define ast_mac100_dump(dev)
+#endif
+
+static
+int
+ast_mac100_open(struct eth_device *dev)
+{
+//	unsigned int ioaddr;
+//	AST_ETH_PRIV_DATA *hw;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Open\n",dev->name);
+#endif
+
+//	hw = (AST_ETH_PRIV_DATA *)dev->priv;
+//	ioaddr = hw->hw_addr;
+
+	/* Rset the hardware */
+	ast_mac100_reset( dev );
+//	ast_mac100_dump("After Reset",dev);
+
+	/* Enable device for communication */
+	ast_mac100_enable( dev );
+
+//	ast_mac100_dump("After Open",dev);
+	return 0;
+}
+
+
+static
+void
+ast_mac100_reset_rings(struct eth_device *dev)
+{
+	AST_ETH_PRIV_DATA *lp;
+	int i;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Reset Rings\n",dev->name);
+#endif
+
+	lp = (AST_ETH_PRIV_DATA *)dev->priv;
+
+	lp->rx_idx = 0;
+	lp->tx_idx = 0;
+
+	for(i=0;i<RXDES_NUM;i++)
+	{
+		lp->rx_descs[i].Status = RDESC0_OWN;
+		lp->rx_descs[i].Buffer1 = (u32)&(lp->rx_buf_dma[i*RX_BUF_SIZE]);
+		lp->rx_descs[i].Length = RX_BUF_SIZE | RDESC1_RCH ;
+//		lp->rx_descs[i].Length = RX_BUF_SIZE ;
+		if ((i+1) != RXDES_NUM)
+			lp->rx_descs[i].Buffer2 = (u32)&(lp->rx_descs_dma[i+1]);
+	//		lp->rx_descs[i].Buffer2 = 0;
+		else
+		{
+			lp->rx_descs[i].Buffer2 = (u32)&(lp->rx_descs_dma[0]);
+	//		lp->rx_descs[i].Buffer2 = 0 ;
+			lp->rx_descs[i].Length |= RDESC1_RER ;
+		}
+	}
+
+	for(i=0;i<TXDES_NUM;i++)
+	{
+		lp->tx_descs[i].Status = 0;
+		lp->tx_descs[i].Length = TDESC1_TCH ;
+	//	lp->tx_descs[i].Length = 0 ;
+		lp->tx_descs[i].Buffer1 = (u32)&(lp->tx_buf_dma[i*TX_BUF_SIZE]);
+		if ((i+1) != TXDES_NUM)
+			lp->tx_descs[i].Buffer2 = (u32)&(lp->tx_descs_dma[i+1]);
+		//	lp->tx_descs[i].Buffer2 = 0 ;
+		else
+		{
+			lp->tx_descs[i].Buffer2 = (u32)&(lp->tx_descs_dma[0]);
+		//	lp->tx_descs[i].Buffer2 =  0 ;
+			lp->tx_descs[i].Length |= TDESC1_TER | TDESC1_TCH ;
+		//	lp->tx_descs[i].Length |= TDESC1_TER ;
+		}
+	}
+
+
+	return;
+}
+
+static
+void
+ast_mac100_close(struct eth_device *dev)
+{
+	unsigned int ioaddr;
+	AST_ETH_PRIV_DATA *hw;
+	u32 val;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Close\n",dev->name);
+#endif
+
+	hw = (AST_ETH_PRIV_DATA *)dev->priv;
+	ioaddr = hw->hw_addr;
+
+	/*Clear interrupts */
+	outl(0, ioaddr + AST_MAC_CSR7 );
+
+	/* Disable Transmission and Receive if it is started */
+	val = inl(ioaddr + AST_MAC_CSR6);
+	if (val & (CSR6_ST | CSR6_SR))
+	{
+		val &= (~(CSR6_ST | CSR6_SR));
+		outl(val,ioaddr + AST_MAC_CSR6);
+
+		/* Wait until the process state becomes stopped */
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Wating for xmit and recv process to stop ...\n",dev->name);
+#endif
+		do
+		{
+			val = inl(ioaddr+AST_MAC_CSR5);
+			/* Before the MAC stops receive processes, recv descriptor will
+			   become full and it enters to suspend mode. In this condition,
+    	       we cannot stop recv process. So clear all descriptors */
+			if (val & CSR5_RU)
+			{
+				ast_mac100_recv(dev);
+				outl(CSR5_RU,ioaddr+AST_MAC_CSR5);
+				outl(0,ioaddr + AST_MAC_CSR2);
+			}
+#ifdef  ASTMAC_DEBUG
+			printf(".");
+#endif
+			val &= (CSR5_TS_MSK | CSR5_RS_MSK);
+
+			/* Patch : if Xmit is stopped and Receive buffer is available. exit */
+			if (((val & CSR5_TS_MSK) == CSR5_TS_ST) && (!(val & CSR5_RU)))
+				break;
+		}
+		while (val != (CSR5_TS_ST| CSR5_RS_ST));
+#ifdef  ASTMAC_DEBUG
+		printf("Stopped\n");
+#endif
+	}
+
+	return;
+}
+
+
+static
+void
+ast_mac100_reset(struct eth_device *dev)
+{
+
+//	unsigned int ioaddr;
+	u32 val;
+	AST_ETH_PRIV_DATA *hw;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Reset\n",dev->name);
+#endif
+
+	hw = (AST_ETH_PRIV_DATA *)dev->priv;
+//	ioaddr = hw->hw_addr;
+
+	/* Send a Software Reset */ /* Errata for PIlot-II */
+//	outl(CSR0_SWR, ioaddr + AST_MAC_CSR0 );
+//	udelay(100);	/* Wait for reset to get effect */
+//	outl(0, ioaddr + AST_MAC_CSR0);
+//	udelay(100);	/* Wait for stabilize */
+
+	/* Till Pilot-II errata for soft-reset is fixed, 
+           Simulate soft-reset . atleast make sure the 
+           interrupts are disabled  and Tx/Rx is stopped  */
+	outl(0,hw->hw_addr+AST_MAC_CSR6);
+	val = inl(hw->hw_addr+AST_MAC_CSR5);
+	val |= 0x1FFFF;
+	outl(val,hw->hw_addr+AST_MAC_CSR5);
+
+	return;
+}
+
+
+static
+int
+ast_mac100_alloc_buffers(AST_ETH_PRIV_DATA *lp)
+{
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:Alloc Buffers\n");
+#endif
+	/*---------------------------------- Receive Descriptors ----------------------------------*/
+	/* Allocate Memory for Descriptors */
+	lp->rx_descs = malloc( sizeof(RECV_DESC)*(RXDES_NUM+1));
+	if (lp->rx_descs == NULL)
+	{
+		printf("ERROR: Receive Ring Descriptor allocation error\n");
+		return -1;
+	}
+
+	/* Align to the Dword Boundary */
+	lp->rx_descs =  (RECV_DESC *)((int)(((char *)lp->rx_descs)+sizeof(RECV_DESC)-1)&0xfffffff0);
+
+	/* Get Physical Address for DMA */
+	lp->rx_descs_dma = virt2phys(lp->rx_descs);
+
+	/* Clear out Descriptors */
+	memset((void *)lp->rx_descs, 0, sizeof(RECV_DESC)*RXDES_NUM);
+
+
+	/* Allocate Memory for Buffers */
+	lp->rx_buf = malloc(RX_BUF_SIZE*(RXDES_NUM+1));
+	if (lp->rx_buf == NULL)
+	{
+		printf("ERROR:Receive Ring Buffer allocation error\n");
+		return -1;
+	}
+
+	/* Align to the Dword Boundary */
+	lp->rx_buf =  (char *)((int)((((char *)lp->rx_buf)+3))&0xfffffff0);
+
+	/* Get Physical Address for DMA */
+	lp->rx_buf_dma = virt2phys(lp->rx_buf);
+
+	/*---------------------------------- Xmit Descriptors ----------------------------------*/
+
+	/* Allocate Memory for Descriptors */
+	lp->tx_descs = malloc( sizeof(XMIT_DESC)*(TXDES_NUM+1));
+	if (lp->tx_descs == NULL)
+	{
+		printf("ERROR:Transmit Ring Descriptors allocation error\n");
+		return -1;
+	}
+
+	/* Align to the Dword Boundary */
+	lp->tx_descs =  (XMIT_DESC *)((int)(((char *)lp->tx_descs)+sizeof(XMIT_DESC)-1)&0xfffffff0);
+
+	/* Get Physical Address for DMA */
+	lp->tx_descs_dma = virt2phys(lp->tx_descs);
+
+	/* Clear out Descriptors */
+	memset((void *)lp->tx_descs, 0, sizeof(XMIT_DESC)*TXDES_NUM);
+
+	/* Allocate Memory for Buffers */
+	lp->tx_buf = malloc( TX_BUF_SIZE*(TXDES_NUM+1));
+	if (lp->tx_buf == NULL)
+	{
+		printf("ERROR:Transmit Ring Buffer allocation error\n");
+		return -1;
+	}
+
+	/* Align to the Dword Boundary */
+	lp->tx_buf =  (char *)((int)((((char *)lp->tx_buf)+3))&0xfffffff0);
+
+	/* Get Physical Address for DMA */
+	lp->tx_buf_dma = virt2phys(lp->tx_buf);
+
+	/*---------------------------------- Setup Frame ----------------------------------*/
+	/* Allocate Memory for Descriptors */
+	lp->setup_descs = malloc(sizeof(XMIT_DESC) *2 );
+	if (lp->setup_descs == NULL)
+	{
+		printf("ERROR:Setup Descriptors allocation error\n");
+		return -1;
+	}
+
+	/* Align to the Dword Boundary */
+	lp->setup_descs =  (XMIT_DESC *)((int)(((char *)lp->setup_descs)+sizeof(XMIT_DESC)-1)&0xfffffff0);
+
+	/* Get Physical Address for DMA */
+	lp->setup_descs_dma = virt2phys(lp->setup_descs);
+
+	/* Clear out Descriptors */
+	memset((void *)lp->setup_descs, 0, sizeof(XMIT_DESC));
+
+	/* Allocate Memory for Buffers */
+	lp->setup_frame = malloc(SETUP_FRAME_SIZE+3);
+	if (lp->setup_frame == NULL)
+	{
+		printf("ERROR:Setup Ring  Buffer allocation error\n");
+		return -1;
+	}
+
+	/* Align to the Dword Boundary */
+	lp->setup_frame =  (char *)((int)((((char *)lp->setup_frame)+3))&0xfffffff0);
+
+	/* Get Physical Address for DMA */
+	lp->setup_frame_dma = virt2phys(lp->setup_frame);
+
+
+
+#ifdef  ASTMAC_DEBUG
+	printf("lp->rx_descs    = %x, lp->rx_descs_dma    = %x\n", lp->rx_descs, lp->rx_descs_dma);
+	printf("lp->rx_buf      = %x, lp->rx_buf_dma      = %x\n", lp->rx_buf, lp->rx_buf_dma);
+	printf("lp->tx_descs    = %x, lp->tx_descs_dma    = %x\n", lp->tx_descs, lp->tx_descs_dma);
+	printf("lp->tx_buf      = %x, lp->tx_buf_dma      = %x\n", lp->tx_buf, lp->tx_buf_dma);
+	printf("lp->setup_descs = %x, lp->setup_descs_dma = %x\n", lp->setup_descs, lp->setup_descs_dma);
+	printf("lp->setup_frame = %x, lp->setup_frame_dma = %x\n", lp->setup_frame,lp->setup_frame_dma);
+#endif
+
+	return 0;
+}
+
+static
+void
+ast_send_setupframe( struct eth_device *dev, char *mac_addr)
+{
+	unsigned int ioaddr;
+	AST_ETH_PRIV_DATA *lp;
+	u32 val;
+	//int i;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Send Setup Frame\n",dev->name);
+#endif
+
+	lp = (AST_ETH_PRIV_DATA *)dev->priv;
+	ioaddr = lp->hw_addr;
+
+#if 1
+	/* Set all to broadcast address */
+	memset((char *)(&lp->setup_frame[0]),0xFF,192);
+
+	/* Set our MAC Address */
+	lp->setup_frame[0] = mac_addr[0];
+	lp->setup_frame[1] = mac_addr[1];
+	lp->setup_frame[4] = mac_addr[2];
+	lp->setup_frame[5] = mac_addr[3];
+	lp->setup_frame[8] = mac_addr[4];
+	lp->setup_frame[9] = mac_addr[5];
+
+/*
+	for(i=0;i<SETUP_FRAME_SIZE;i++)
+	{
+		printf("%02X ",(unsigned char)lp->setup_frame[i]);
+		if (!((i+1) % 16 ))
+			printf("\n");
+	}
+	printf("\n");
+*/
+
+
+	/* Create the setup descriptor */
+	lp->setup_descs[0].Length = SETUP_FRAME_SIZE |TDESC1_TER | TDESC1_PERFECT | TDESC1_SET;
+	lp->setup_descs[0].Buffer1 = (u32)lp->setup_frame_dma;
+	lp->setup_descs[0].Buffer2 = 0;
+	lp->setup_descs[0].Status = TDESC0_OWN;
+
+	/* Load setup descriptor address */
+	outl((u32)lp->setup_descs_dma,ioaddr+ AST_MAC_CSR4);
+
+	/* Enable xmit start */
+	val = inl(ioaddr+AST_MAC_CSR6);
+	val |= (CSR6_ST);
+	outl(val,ioaddr + AST_MAC_CSR6);
+
+	/* Tell the mac to do a immediate TX of  setup frame */
+	outl(0,ioaddr + AST_MAC_CSR1);
+
+	udelay(1000);
+	/* Set Xmit stop */
+	val = inl(ioaddr+AST_MAC_CSR6);
+	val &= (~CSR6_ST);
+	outl(val,ioaddr + AST_MAC_CSR6);
+
+#else
+
+	val = inl(ioaddr+AST_MAC_CSR6);
+	val |= (CSR6_PM | CSR6_PR);			/* Set Promiscuous Mode */
+	outl(val,ioaddr + AST_MAC_CSR6);
+#endif
+	return;
+}
+
+
+static
+void
+ast_mac100_enable( struct eth_device *dev )
+{
+	unsigned int ioaddr;
+	AST_ETH_PRIV_DATA *lp;
+	unsigned long csr6;
+	u32 val;
+	u32 lnkspeed;
+	u32 lnkmode;
+
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Enable\n",dev->name);
+#endif
+
+	lp = (AST_ETH_PRIV_DATA *)dev->priv;
+	ioaddr = lp->hw_addr;
+
+	/* Setup Default settings of registers */
+	outl(DEFAULT_CSR0,ioaddr + AST_MAC_CSR0);
+	outl(DEFAULT_CSR11,ioaddr + AST_MAC_CSR11);
+	
+	/* Setup CSR6 for 10/100 Mbps Half/Full Duplex depending upon PHY info*/
+	csr6=0;
+
+	lnkspeed=GetPhySpeed(dev->name);
+
+	if (lnkspeed == _1000BASET)
+	{
+		printf("Setting 1000 BASE-X\n");
+	    csr6 |= ((0x1 << 16) | (0x1 << 17));
+
+	}
+	else 
+	{
+		if(lnkspeed == _100BASET)
+		{
+	  		printf("Setting 100 BASE-X\n");
+	  		csr6 &= ~((0x1 << 16) | (0x1 << 17));
+		}
+		else
+	  	{
+			printf("Setting 10 BASE-T\n");
+			csr6 |= (0x1 << 17);
+	    		csr6 &= ~(0x1 << 16);
+	  	}
+	}
+	
+	lnkmode=GetPhyDuplex(dev->name);
+	if(lnkmode==FULL)
+	{
+	    printf("Setting full duplex mode.\n");
+	    csr6 |= (0x1 << 9);
+	}
+	else
+	{
+	    csr6 &=( ~(0x1 << 9) );
+	    printf("Setting half duplex mode\n");
+	}
+	
+	
+
+
+	outl(DEFAULT_CSR6|csr6,ioaddr + AST_MAC_CSR6);
+
+	/* Init Ring */
+	ast_mac100_reset_rings(dev);
+
+	/* Mask interrupts */
+	outl(0,ioaddr + AST_MAC_CSR7);
+
+	/* Clear interrupts */
+	val = inl(ioaddr+AST_MAC_CSR5);
+	val |= 0x1FFFF;
+	outl(val,ioaddr + AST_MAC_CSR5);
+
+	/*Send Setup Frame to setup MAC address*/
+	ast_send_setupframe(dev,(char *)dev->enetaddr);
+
+	/* Load recv and send descriptors */
+	outl((u32)lp->rx_descs_dma,ioaddr+ AST_MAC_CSR3);
+	outl((u32)lp->tx_descs_dma,ioaddr+ AST_MAC_CSR4);
+
+	/* Set xmit and recv to running state */
+	val = inl(ioaddr+AST_MAC_CSR6);
+	val |= (CSR6_ST | CSR6_SR);
+	outl(val,ioaddr + AST_MAC_CSR6);
+
+	/* Tell the MAC to acquire the receive descriptors */
+	outl(0,ioaddr + AST_MAC_CSR2);
+
+	return;
+}
+
+
+static
+void
+ast_prog_phy_get_speed( struct eth_device *dev )
+{
+    unsigned int ioaddr;
+    AST_ETH_PRIV_DATA *lp;
+    unsigned long csr6;
+    u32 val;
+    u32 lnkspeed;
+    u32 lnkmode;
+    unsigned char addr;
+    int devnum;
+    int data;
+    devnum = GetDevNum(dev->name);
+    u16 phyid=0;
+    int to=0x100;
+    u32 link_chk_iterations=40;
+
+    switch (devnum)
+    {
+        case 0:
+            addr = CONFIG_PHY_ADDR0;
+            break;
+        case 1:
+            addr = CONFIG_PHY_ADDR1;
+            break;
+        default:
+            return 0;
+    }
+
+#ifdef  ASTMAC_DEBUG
+    printf("ASTMAC:(%s):prog_phy\n",dev->name);
+#endif
+
+    ast_miiphy_read(dev->name, addr, MII_PHY_ID1,&phyid);
+    if(((phyid&0x3f0)>>0x4)==0x11)
+    {
+#ifdef  ASTMAC_DEBUG
+        printf("Found Real Tek Phy,Programme 125Mhz clk source\n");
+#endif
+        //prog realteck phy to provide 125 mhz clk
+        ast_miiphy_write(dev->name, addr, 0x1F, 0x5);
+        ast_miiphy_write(dev->name, addr, 0xc, 0x0);
+        ast_miiphy_read(dev->name, addr, 1,&data);
+        data|=0x80;
+        ast_miiphy_write(dev->name, addr, 0x1, data);
+        ast_miiphy_write(dev->name, addr, 0x1F, 0x0);
+        //As per H/w set MACRGMICTRL value
+        *((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x88)) = 0x293;
+    }
+    else if(((phyid&0x3f0)>>0x4)==0x21)
+    {
+#ifdef  ASTMAC_DEBUG
+        printf("Found Mircel phy %x %x\n",devnum,phyid);
+#endif
+
+        //Micrel Tx clk skew programming
+        data=0x104;
+        ast_miiphy_write(dev->name, addr,0xb,data);
+        ast_miiphy_read(dev->name, addr, 0xd, &data);
+#ifdef  ASTMAC_DEBUG
+        printf("RGMII CLK and Control Pad Skew Default values %x\n",data);
+#endif
+        data=( (data)&(0xff0f) );
+        data=( (data)|(0xf<<4));
+
+        ast_miiphy_write(dev->name, addr, 0xc, data);
+        data=0x8104;
+        ast_miiphy_write(dev->name, addr,0xb,data);
+
+        //read back and make sure that our bits are programmed correctly
+        data=0x104;
+        ast_miiphy_write(dev->name, addr,0xb,data);
+        ast_miiphy_read(dev->name, addr, 0xd, &data);
+#ifdef  ASTMAC_DEBUG
+        printf("RGMII CLK and Control Pad Skew Modified values %x\n",data);
+#endif
+
+        //enable micrel clk jitter work around
+        ast_miiphy_read(dev->name, addr,0x9,&data);
+        data|=(0x3<<11);
+        ast_miiphy_write(dev->name, addr,0x9, data);
+
+        //restart auto negotiation
+        ast_miiphy_read(dev->name, addr,0x0,&data);
+        data|=(0x1<<9);
+        ast_miiphy_write(dev->name, addr, 0x0, data);
+        //As Per Micrel's Errata ,some times it may take 10secs for linkup
+
+        while( (link_chk_iterations)&&(GetLinkStatus(dev->name)==0x0) )
+        {
+            link_chk_iterations--;
+            udelay(1000);
+        }
+
+#ifdef  ASTMAC_DEBUG
+        if(link_chk_iterations==0x0)
+        {
+            printf("Link Is Down For Device%x\n",devnum);
+        }
+#endif
+    }
+    else if( (((phyid&0x3f0)>>0x4)==0x0f)|| (((phyid&0x3f0)>>0x4)==0x0c) )
+    {
+#ifdef  ASTMAC_DEBUG
+        printf("Found Smsc phy %x PhyId %x\n",devnum,phyid);
+#endif
+        phyid=0x8000;
+        ast_miiphy_write(dev->name, addr, 0x0, phyid);
+        while(phyid==0x8000)
+        {
+            ast_miiphy_read(dev->name, addr, 0x0,&phyid);
+        }
+#ifdef  ASTMAC_DEBUG
+        printf("phy reset is done and autoneg started\n");
+#endif
+
+        ast_miiphy_write(dev->name, addr, 0x0, 0x1200);
+    }
+    else
+    {
+#ifdef  ASTMAC_DEBUG
+        printf("Phy Id %x  not matching dev_num is %x\n",phyid,devnum);
+#endif
+    }
+
+    return;
+}
+
+
+
+
+static
+int
+ast_mac100_send_packet(struct eth_device *dev,  void *packet, int length)
+{
+	volatile XMIT_DESC *cur_desc;
+	unsigned int ioaddr;
+	AST_ETH_PRIV_DATA *lp;
+    unsigned long count = 0;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Send\n",dev->name);
+#endif
+
+	lp = (AST_ETH_PRIV_DATA *)dev->priv;
+	ioaddr =lp->hw_addr;
+
+	cur_desc = &lp->tx_descs[lp->tx_idx];
+	while (cur_desc->Status & TDESC0_OWN)
+	{
+#ifdef  ASTMAC_DEBUG
+		printf("ASTMAC:(%s):Send : Transmit Busy\n",dev->name);
+#endif
+		udelay(10);
+
+        	if (++count >= 2000000/10)  // 2 sec
+        	{	
+	            printf("ASTMAC:(%s):Send Timeout\n",dev->name);
+        	    return -1;
+	        }
+   	}
+
+	/* Compute length to be transfered */
+	length = ETH_ZLEN < length ? length : ETH_ZLEN;
+	length = length > TX_BUF_SIZE ? TX_BUF_SIZE : length;
+
+	/* Copy to Xmit buffer */
+	memcpy((char *)cur_desc->Buffer1, packet, length);
+
+	/* Set necessary bits and Length of data */
+	cur_desc->Length = TDESC1_TCH | TDESC1_FS | TDESC1_LS |length;
+//	cur_desc->Length =  TDESC1_FS | TDESC1_LS |length;
+
+	/* If last descriptor, set end of chain */
+	if (lp->tx_idx  == (TXDES_NUM -1))
+		cur_desc->Length |= TDESC1_TER;
+
+	/* Hand it over to MAC ownership */
+	cur_desc->Status = TDESC0_OWN;
+
+	/* Move curdesc ptr to next desc */
+	lp->tx_idx = (lp->tx_idx+1)%TXDES_NUM;
+
+	/* Start Xmit */
+	outl(0,ioaddr + AST_MAC_CSR1);			/* Issue Xmit Poll Demand */
+	return length;
+}
+
+
+static
+int
+ast_mac100_recv(struct eth_device *dev)
+{
+	unsigned int ioaddr;
+	int     packet_length, tmplength = 0, count = 0;
+	volatile RECV_DESC *cur_desc;
+	int 	cpy_length;
+	int		start_idx,org_start_idx;
+	int		seg_length;
+	AST_ETH_PRIV_DATA *lp;
+	int 	FrameFS = 0;
+	uchar *data;
+	//int CSR4;
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Recv\n",dev->name);
+#endif
+
+	lp= (AST_ETH_PRIV_DATA *)dev->priv;
+	ioaddr = lp->hw_addr;
+
+	packet_length = 0;
+	start_idx = lp->rx_idx;
+
+	while (1)
+	{
+		/* Get the current descriptor*/
+		cur_desc = &lp->rx_descs[lp->rx_idx];
+
+		/* Still owned by MAC. No data yet */
+		if (cur_desc->Status & RDESC0_OWN)
+		{
+			/* If we already got partial frame, wait for the full frame*/
+			if (FrameFS)
+				continue;
+			goto ReleaseDesc;
+		}
+
+		/* Got a descriptor. Increment our pointer with MAC's pointer */
+		lp->rx_idx = (lp->rx_idx+1)%RXDES_NUM;
+
+        // Check if the FS & LS bits are set. Otherwise it means that this
+        // frame spans across multiple descriptors. Hence read the length
+        // and shift the descriptor index to the last frame.
+        if ((cur_desc->Status & 0x38008300) != 0x0300)
+        {
+            if ((cur_desc->Status & 0x38000300) != 0x0300)
+            {
+                printf("ASTMAC:(%s) : Oversized ethernet frame: 0x%x\n",dev->name,cur_desc->Status);
+                tmplength = RDESC0_FL_GET(cur_desc->Status);
+                count = tmplength / RX_BUF_SIZE;
+                count = tmplength % RX_BUF_SIZE ? count + 1 : count;
+
+                // Increment the descriptor index by the no. of frames the current packet occupies.
+                // (doing count -1 since the index was already incremented before entering this if block
+                lp->rx_idx = (lp->rx_idx + count - 1) % RXDES_NUM;
+                goto ReleaseDesc;
+            }
+        }
+
+		/* Check if error is set */
+		if (cur_desc->Status & RDESC0_ES)
+		{
+			// Software fix for PILOT-II Mac Bug 
+			// Ignore Collisions on Full Duplex
+			if ((cur_desc->Status & RDESC0_CS) && 
+				(GetPhyDuplex(dev->name) ==1))
+				cur_desc->Status &= (~RDESC0_CS);	
+				
+			if (cur_desc->Status & (RDESC0_CE | RDESC0_TL | RDESC0_DE | RDESC0_RF))
+			{
+				printf("ASTMAC:(%s) : Recv Error : 0x%x\n",dev->name,cur_desc->Status);
+				goto ReleaseDesc;
+			}
+		}
+
+
+
+		/* Check if we got a first descriptor */
+		if (FrameFS == 0)
+		{
+			if (!(cur_desc->Status & RDESC0_FD))
+			{
+				printf("ASRMAC(%s): Recv Error : First Descriptor of Frame not got\n",dev->name);
+				goto ReleaseDesc;
+			}
+			else
+				FrameFS = 1;
+		}
+
+		/* If we get a last descriptor of the frame, get the frame length */
+		if (cur_desc->Status & RDESC0_LS)
+		{
+			packet_length = RDESC0_FL_GET(cur_desc->Status);
+#ifdef  ASTMAC_DEBUG
+			printf("ASTMAC:(%s):Recevied Frame of size 0x%x\n",dev->name,packet_length);
+#endif
+//			printf("R\n");
+			break;
+		}
+#ifdef  ASTMAC_DEBUG
+//		printf("ASTMAC:(%s):Waiting for full frame \n",dev->name,packet_length);
+#endif
+	}
+
+	/* Check if valid packet */
+	if (packet_length == 0)
+		goto ReleaseDesc;
+
+	/* Copy the data to U-Boot buffer */
+	data = (uchar *)NetRxPackets[0];
+	cpy_length = 0;
+	org_start_idx = start_idx;
+	for (; start_idx!=lp->rx_idx; start_idx=(start_idx+1)%RXDES_NUM)
+	{
+		seg_length = min(packet_length - cpy_length, RX_BUF_SIZE);
+		memcpy(data+cpy_length, (char *)lp->rx_descs[start_idx].Buffer1,seg_length);
+#ifdef  ASTMAC_DEBUG
+//		printf("ASTMAC:(%s):Copying Partial Frame of size 0x%x\n",dev->name,seg_length);
+#endif
+		cpy_length += seg_length;
+	}
+	start_idx = org_start_idx;
+/*
+	int z ;
+	for(z=0;z<12;z++) {
+		printf("%x ",data[z]);
+	}
+	printf("\n");
+*/
+	NetReceive(NetRxPackets[0], packet_length);		/* Send to U-Boot layer */
+	
+
+
+ReleaseDesc:
+	/* Release the descriptors back to MAC */
+	for (; start_idx!=lp->rx_idx; start_idx=(start_idx+1)%RXDES_NUM)
+			lp->rx_descs[start_idx].Status = RDESC0_OWN;
+
+	/* Check if Recv is suspended : Due to unavailable desc */
+	if (inl(ioaddr +AST_MAC_CSR5) & CSR5_RU)
+	{
+#ifdef  ASTMAC_DEBUG
+//		printf("ASTMAC:(%s):Recv process is in Suspended State 0x%08x\n",dev->name,inl(ioaddr+AST_MAC_CSR5));
+#endif
+		outl(CSR5_RU,ioaddr+AST_MAC_CSR5);		/* Clear RU Bit */
+		outl(0,ioaddr + AST_MAC_CSR2);			/* Issue Recv Poll Demand */
+	}
+#ifdef  ASTMAC_DEBUG
+//		printf("ASTMAC:(%s): CSR5 Status = 0x%x\n",dev->name,inl(ioaddr+AST_MAC_CSR5));
+#endif
+
+	return packet_length;
+}
+
+/*----------------------------------------------------------------------*/
+/*    				 U-Boot Interface Functions							*/
+/*----------------------------------------------------------------------*/
+static int eth_init_called[MAX_SE_ETH];
+static
+int
+ast_eth_init(struct eth_device *dev,bd_t *bd)
+{
+	AST_ETH_PRIV_DATA *hw;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC:(%s):Init \n",dev->name);
+#endif
+	hw = (AST_ETH_PRIV_DATA *)dev->priv;
+
+	/* Allocate the required decsriptors and buffers */
+	if (!(eth_init_called[hw->devnum]))
+	{
+		if (ast_mac100_alloc_buffers(hw) == -1)
+			return -1;
+	}
+
+	ast_mac100_open(dev);
+	eth_init_called[hw->devnum] = 1;
+	return 1;
+}
+
+
+static
+void
+ast_eth_halt(struct eth_device *dev)
+{
+	ast_mac100_close(dev);
+}
+
+static
+int
+ast_eth_rx(struct eth_device *dev)
+{
+	return ast_mac100_recv(dev);
+}
+
+static
+int
+ast_eth_send(struct eth_device *dev, volatile void *packet, int length)
+{
+	return ast_mac100_send_packet(dev,(void *)packet,length);
+}
+
+
+static int eth_initialized = 0;
+int
+ast_eth_initialize(bd_t *bis)
+{
+	struct eth_device *dev;
+	int eth_num;
+	AST_ETH_PRIV_DATA *hw;
+	u32 val;
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC: Initialize Called\n");
+#endif
+	if (eth_initialized != 0)
+	{
+		printf("ast_eth_initialize: WARNING: Already Initialized\n");
+		return -1;
+	}
+
+	for(eth_num=0;eth_num < MAX_SE_ETH;eth_num++)
+	{
+
+		/* Allocate Device Structure */
+		dev = (struct eth_device *) malloc(sizeof(struct eth_device));
+		if (dev == NULL)
+		{
+			printf("ast_eth_initialize: Cannot allocate eth_device %d\n",eth_num);
+			return 1;
+		}
+		memset(dev,0,sizeof(*dev));
+
+		/* Allocate Our Private Strcuture */
+		hw = (AST_ETH_PRIV_DATA *) malloc(sizeof (AST_ETH_PRIV_DATA));
+		if (hw == NULL)
+		{
+			printf("ast_eth_initialize: Cannot allocate private data %d\n",eth_num);
+			return -1;
+		}
+		memset(hw,0,sizeof(*hw));
+
+
+		switch (eth_num)
+		{
+#if defined (CONFIG_NET_MULTI)
+#ifdef CONFIG_HAS_ETH1
+			case 1:
+				hw->hw_addr = SE_MAC_B_BASE;
+				memcpy(dev->enetaddr,bis->bi_enet1addr,6);
+				break;
+#endif
+#endif
+			case 0:
+				hw->hw_addr = SE_MAC_A_BASE;
+				memcpy(dev->enetaddr,bis->bi_enetaddr,6);
+				break;
+			default:
+				printf("Invalid MAC %d\n",eth_num);
+				return -1 ;
+				break;
+		}
+
+		hw->devnum = eth_num;
+		eth_init_called[hw->devnum] = 0;
+
+		sprintf (dev->name, "pilot_eth%d", eth_num);
+      	dev->priv = (void *) hw;
+      	dev->init = ast_eth_init;
+        dev->halt = ast_eth_halt;
+        dev->send = ast_eth_send;
+        dev->recv = ast_eth_rx;
+
+		/* Install IRQ Handlers here if needed */
+
+		/* Stop Rx/Tx and Clear Pending Interrupts. 
+		   This is needed because when linux reboots using 
+		   warm reset, the mac is still active and interrupts 
+		   will be pending. In this state if the linux network
+		   driver is loaded, it crashes*/
+		outl(0,hw->hw_addr+AST_MAC_CSR6);
+		val = inl(hw->hw_addr+AST_MAC_CSR5);
+		val |= 0x1FFFF;
+		outl(val,hw->hw_addr+AST_MAC_CSR5);
+
+#ifdef  ASTMAC_DEBUG
+	printf("ASTMAC: Registering %s\n",dev->name);
+#endif
+		eth_register(dev);
+
+#if defined (CONFIG_MII) || (CONFIG_COMMANDS & CFG_CMD_MII)
+		/* register mii command access routiast_send_setupframenes */
+		miiphy_register(dev->name,ast_miiphy_read,ast_miiphy_write);
+#endif
+
+        ast_prog_phy_get_speed(dev );
+
+	}		/* for ethnum */
+
+	eth_initialized = 1;
+	return 1;
+}
+
+
+/* --------------------------------------------------------------------------------
+ *  If CONFIG_NET_MULTI is not defined => Only one ethernet device, define
+ *  eth_xxx functions here (eth.c will compile its eth_xxx functions)
+ *---------------------------------------------------------------------------------
+*/
+#if !defined(CONFIG_NET_MULTI)
+
+struct eth_device *eth_dev = NULL;
+
+struct
+eth_device *
+eth_get_dev(void)
+{
+	return eth_dev;
+}
+
+int
+eth_register(struct eth_device *dev)
+{
+	eth_dev = dev;
+	return 0;
+}
+
+void
+eth_halt(void)
+{
+	if (eth_dev)
+		ast_eth_halt(eth_dev);
+	return;
+}
+
+int
+eth_init(bd_t *bis)
+{
+		if (!eth_initialized)
+		{
+			if (ast_eth_initialize(bis) == -1)
+				return -1;
+		}
+		if (eth_dev)
+			return ast_eth_init(eth_dev,bis);
+		else
+			return -1;
+}
+
+int
+eth_send(volatile void *packet,int length)
+{
+		return ast_eth_send(eth_dev,packet,length);
+}
+
+int
+eth_rx(void)
+{
+		return  ast_eth_rx(eth_dev);
+}
+
+#endif    /* !CONFIG_NET_MULTI */
+
+#endif
