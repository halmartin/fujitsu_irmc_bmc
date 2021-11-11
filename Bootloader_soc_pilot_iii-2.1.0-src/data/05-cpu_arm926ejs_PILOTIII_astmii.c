--- uboot/cpu/arm926ejs/PILOTIII/astmii.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot.new/cpu/arm926ejs/PILOTIII/astmii.c	2013-06-26 14:37:41.613620300 +0800
@@ -0,0 +1,266 @@
+#include <common.h>
+#if (CONFIG_COMMANDS & CFG_CMD_NET)
+#include <asm/processor.h>
+#include <miiphy.h>
+#include "soc_hw.h"
+#include "astmac.h"
+
+#define inl(addr) 		(*((volatile u32 *)(addr)))
+#define outl(addr,val) 	(*((volatile u32 *)(addr)) = (val))
+
+#define PILOT_RT_PHYSR	0x11 
+//Reg 0x11 defines - PHY specific Status register
+#define REG0x11_FULL_DUPLX_MODE         0x00002000
+#define REG0x11_10Mbps                  0x00000000
+#define REG0x11_100Mbps                 0x00004000
+#define REG0x11_1000Mbps                0x00008000
+#define Mircel_Phy_Control              0x1F
+
+/***************************** Tulip  MII read/write access macros *************************/
+#define	AST_MII_BIT_READ(Addr, pBData)                           		\
+   	{                                                           			\
+    	outl((Addr)+AST_MAC_CSR9,CSR9_MII_RD);								\
+        udelay(2);                                            				\
+        outl((Addr)+AST_MAC_CSR9,CSR9_MII_RD | CSR9_MDC);					\
+        udelay(2);                                                     		\
+        *(pBData) |= CSR9_MII_DBIT_RD (inl ((Addr)+ AST_MAC_CSR9));  		\
+   	}
+
+#define	AST_MII_BIT_WRITE(Addr, data)                                 		\
+    {                                                               		\
+    	outl((Addr)+AST_MAC_CSR9, CSR9_MII_DBIT_WR(data) | CSR9_MII_WR);	\
+        udelay(2);                                                     		\
+    	outl((Addr)+AST_MAC_CSR9, CSR9_MII_DBIT_WR(data) | CSR9_MII_WR | CSR9_MDC);				\
+        udelay(2);                                                     		\
+    }
+
+#define	AST_MII_RTRISTATE(Addr)                                       		\
+	{                                                               		\
+        int retBData;                                                     	\
+        AST_MII_BIT_READ ((Addr), &retBData);                          		\
+    }
+
+#define	AST_MII_WTRISTATE(Addr)                                       		\
+	{                                                               		\
+        AST_MII_BIT_WRITE((Addr), 0x1);                               		\
+        AST_MII_BIT_WRITE((Addr), 0x0);                               		\
+	}
+
+#define AST_MII_WRITE(Addr, data, bitCount)                           		\
+   	{                                                               		\
+        int i=(bitCount);                                               	\
+                                                                        	\
+        while (i--)                                                    	 	\
+            AST_MII_BIT_WRITE ((Addr), ((data) >> i) & 0x1);          		\
+    }
+
+#define	AST_MII_READ(Addr, pData, bitCount)                           		\
+   	{                                                               		\
+        int i=(bitCount);                                               	\
+                                                                        	\
+        while (i--)                                                     	\
+        {                                                           		\
+            *(pData) <<= 1;                                             	\
+	        AST_MII_BIT_READ ((Addr), (pData));                      		\
+        }                                                           		\
+    }
+
+int
+GetDevNum(char *devname)
+{
+	int num = -1;
+
+	/* Assumption: single digits 0 to 9 only and occurs at last char */
+	while (*devname)
+	{
+		num= *devname;
+		devname++;
+	}
+	if ((num < '0') || ( num  > '9'))
+		return -1;
+
+	/* Convert char to integer */
+	num-='0';
+
+	return num;
+}
+
+
+static
+u32
+getmacaddr(char *devname)
+{
+	int num;
+
+	if (devname == NULL)
+		return 0;
+	num = GetDevNum(devname);
+	if (num == -1)
+		return 0;
+	switch (num)
+	{
+		case 1:
+	// Pilot-II does not have MDIO/MDC signals for the second MAC
+	// So normally, we set the two PHY to different address and connect 
+	// both the PHY to the same MDIO/MDC signals from the first MAC
+	//		return SE_MAC_B_BASE;
+		case 0:
+			return SE_MAC_A_BASE;
+		default:
+			break;
+	}
+	return 0;
+}
+
+
+int
+ast_miiphy_read (char *devname, unsigned char addr, unsigned char reg,  unsigned short *value)
+{
+	u32 macaddr;
+
+    	macaddr = getmacaddr(devname);
+    	if (macaddr == 0)
+		return -1;
+
+    /* Write 34-bit preamble */
+    AST_MII_WRITE (macaddr, MII_PREAMBLE, 32);
+
+    /* start of frame + op-code nibble */
+    AST_MII_WRITE (macaddr, MII_SOF | MII_RD, 4);
+
+    /* device address */
+    AST_MII_WRITE (macaddr, addr, 5);
+    AST_MII_WRITE (macaddr, reg, 5);
+
+    /* turn around */
+    AST_MII_RTRISTATE (macaddr);
+
+#ifdef CONFIG_PHY_CMD_DELAY
+	udelay (CONFIG_PHY_CMD_DELAY);		/* Intel LXT971A needs this */
+#endif
+
+    /* read data */
+    AST_MII_READ (macaddr, value, 16);
+
+	/* turn around */
+    AST_MII_RTRISTATE (macaddr);
+
+//	printf("Phy Read Value = %x Phy Address = %x Reg = %x\n",*value,addr,reg);
+	return 0;
+}
+
+int
+ast_miiphy_write (char *devname, unsigned char addr, unsigned char reg, unsigned short value)
+{
+	u32 macaddr;
+
+	macaddr = getmacaddr(devname);
+	if (macaddr == 0)
+		return -1;
+
+    /* write 34-bit preamble */
+    AST_MII_WRITE (macaddr, MII_PREAMBLE, 32);
+
+    /* start of frame + op-code nibble */
+    AST_MII_WRITE (macaddr, MII_SOF | MII_WR, 4);
+
+    /* device address */
+    AST_MII_WRITE (macaddr, addr, 5);
+    AST_MII_WRITE (macaddr, reg, 5);
+
+    /* turn around */
+    AST_MII_WTRISTATE (macaddr);
+
+#ifdef CONFIG_PHY_CMD_DELAY
+	udelay (CONFIG_PHY_CMD_DELAY);		/* Intel LXT971A needs this */
+#endif
+    /* write data */
+    AST_MII_WRITE (macaddr,value, 16);
+
+	return 0;
+
+}
+
+int
+GetLinkStatus(char *devname)
+{
+	u16 status;
+
+	unsigned char addr;
+	int devnum;
+	u16 phyid=0;
+	
+	devnum = GetDevNum(devname);
+	switch (devnum)
+	{
+		case 0:
+			addr = CONFIG_PHY_ADDR0;
+			break;
+		case 1:
+			addr = CONFIG_PHY_ADDR1;
+			break;
+		default:
+			return 0;
+	}
+	
+	/* Check if PHY is preset. If not treat as NC-SI */
+     	ast_miiphy_read(devname, addr, MII_PHY_ID1,&phyid);
+//	printf("Get Link Status : PHYID = %x\n",phyid);
+	if (phyid == 0xffff)
+	{
+		printf("(%s): No PHY Found. Setting Link Up\n",devname);
+		return 1;			/* Link is up */
+	}
+
+	/* Needs two reads of Status reg to get correct link status */
+	ast_miiphy_read (devname,addr,MII_PHY_SR,&status);
+	ast_miiphy_read (devname,addr,MII_PHY_SR,&status);
+	if (!(status & MII_PHY_SR_LNK))	/* Link Status */
+		return 0;
+	if (!(status & MII_PHY_SR_AN))		/* Auto Nego Status */
+		return 0;
+	return 1;
+}
+
+int
+GetPhySpeed(char *devname)
+{
+	unsigned char addr;
+	int devnum;
+
+	devnum = GetDevNum(devname);
+	switch (devnum)
+	{
+		case 0:
+			addr = CONFIG_PHY_ADDR0;
+			break;
+		case 1:
+			addr = CONFIG_PHY_ADDR1;
+			break;
+		default:
+			return _100BASET;
+	}
+	return miiphy_speed (devname,addr);
+}
+
+int
+GetPhyDuplex(char *devname)
+{
+	unsigned char addr;
+	int devnum;
+
+	devnum = GetDevNum(devname);
+	switch (devnum)
+	{
+		case 0:
+			addr = CONFIG_PHY_ADDR0;
+			break;
+		case 1:
+			addr = CONFIG_PHY_ADDR1;
+			break;
+		default:
+			return FULL;
+	}
+	return miiphy_duplex(devname,addr);
+}
+#endif
