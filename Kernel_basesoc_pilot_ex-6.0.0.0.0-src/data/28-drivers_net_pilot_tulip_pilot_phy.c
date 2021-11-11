--- linux-3.14.17/drivers/net/ethernet/pilot_tulip/pilot_phy.c	1970-01-01 05:30:00.000000000 +0530
+++ linux-3.14.17.new/drivers/net/ethernet/pilot_tulip/pilot_phy.c	2014-09-04 12:04:26.721823185 +0530
@@ -0,0 +1,230 @@
+#include "tulip.h"
+/* CSR9_MII_MANAGEMENT_REG */
+#define MII_MGMT_DATA_IN            (1 << 19) /* 0x00080000L */
+#define MII_MGMT_RD_WRBAR           (1 << 18) /* 0x00040000L */
+#define MII_MGMT_DATA_OUT           (1 << 17) /* 0x00020000L */
+#define MII_MGMT_CLK                (1 << 16) /* 0x00010000L */
+
+#define MDI     MII_MGMT_DATA_IN    /* Input */
+#define MDRE    MII_MGMT_RD_WRBAR   /* Input if set, Output if not set */
+#define MDO     MII_MGMT_DATA_OUT   /* Output */
+#define MDC     MII_MGMT_CLK        /* Clock */
+
+
+#define MII_PHY_CTRL_REGNO          0
+#define MII_PHY_STATUS_REGNO        1
+#define MII_PHY_PHYID_HIGH          2
+#define MII_PHY_PHYID_LOW           3
+
+
+#define LAN_BASE_ADDR		    	0x40500000
+#define MDIO_MASK                   ~(MDI | MDRE | MDO | MDC)
+#define MDIO_SET_BITS(mMdioBits)    { CSR9_MII_MANAGEMENT_REG = ((CSR9_MII_MANAGEMENT_REG & MDIO_MASK) | (mMdioBits)); }
+#define LAN_REG32(ofs)          	*(volatile unsigned int *)(LAN_BASE_ADDR + ofs)
+#define CSR9_MII_MANAGEMENT_REG     LAN_REG32(0x48)
+#define MDIO_RD 0                   /* BIT19 Clear */
+#define MDIO_WR MDRE                /* BIT19 Set   */
+
+/* The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
+   met by back-to-back PCI I/O cycles, but we insert a delay to avoid
+   "overclocking" issues or future 66Mhz PCI. */
+//#define mdio_delay() ioread32(mdio_addr)
+
+void pilot_mdio_delay(void);
+void pilot_mdio_delay(void)
+{
+    udelay(5);
+}
+
+
+unsigned int Lan_MII_MDIO_Read(
+        struct net_device *dev,             /* Lan port */
+        unsigned char DataBitLen        /* Number of bits to read in (maximum 32 bits) */
+)
+{
+    unsigned int Mask;
+    unsigned int Data = 0;
+	//struct tulip_private *tp = netdev_priv(dev);
+	//void __iomem *ioaddr = tp->base_addr;
+	void __iomem *ioaddr = (void *)IOADDR_INTERFACE_ETH_A;
+    void __iomem *mdio_addr = ioaddr + CSR9;
+    unsigned int csr9;
+    /* Bit bang in the data */
+    for (Mask=1<<(DataBitLen-1);Mask!=0;Mask>>=1)
+    {
+        /* Bit bang in one bit's worth of data */
+                   /* MDRE    | MDO | MDC */
+		pilot_mdio_delay();
+        iowrite32(MDIO_RD | 0 | 0  , mdio_addr);
+		pilot_mdio_delay();
+        iowrite32(MDIO_RD | 0 | MDC, mdio_addr);
+		pilot_mdio_delay();
+        csr9 = ioread32(mdio_addr);
+		pilot_mdio_delay();
+        if (csr9 & MDI)
+        {
+            Data |= Mask;
+        }
+        else
+        {
+            Data &= ~Mask;
+        }
+        iowrite32(MDIO_RD | 0 | 0  , mdio_addr);
+		pilot_mdio_delay();
+    }
+
+    return Data;
+}
+
+void Lan_MII_MDIO_Write(
+        struct net_device *dev,        /* Lan port */
+        unsigned int Data,        /* Data to be written */
+        unsigned char  DataBitLen   /* Number of relevant bits in Data */
+)
+{
+	//struct tulip_private *tp = netdev_priv(dev);
+    //void __iomem *ioaddr = tp->base_addr;
+    void __iomem *ioaddr = (void *)IOADDR_INTERFACE_ETH_A;
+    void __iomem *mdio_addr = ioaddr + CSR9;
+    unsigned int Mask;
+
+    for (Mask=1<<(DataBitLen-1);Mask!=0;Mask>>=1)
+    {
+        /* Bit bang out a 1 or a 0 */
+        if (Data & Mask)
+        {
+                       /* MDRE    | MDO | MDC */
+		    pilot_mdio_delay();
+            iowrite32(MDIO_WR | MDO | 0, mdio_addr);
+		    pilot_mdio_delay();
+            iowrite32(MDIO_WR | MDO | MDC, mdio_addr);
+		    pilot_mdio_delay();
+            iowrite32(MDIO_WR | MDO | 0, mdio_addr);
+		    pilot_mdio_delay();
+        }
+        else
+        {
+                       /* MDRE    | MDO | MDC */
+		    pilot_mdio_delay();
+            iowrite32(MDIO_WR | 0   | 0, mdio_addr);
+		    pilot_mdio_delay();
+            iowrite32(MDIO_WR | 0   | MDC, mdio_addr);
+		    pilot_mdio_delay();
+            iowrite32(MDIO_WR | 0   | 0, mdio_addr);
+		    pilot_mdio_delay();
+        }
+    }
+}
+
+void Lan_MII_MDIO_WriteZ(struct net_device *dev)
+{
+	//struct tulip_private *tp = netdev_priv(dev);
+	//void __iomem *ioaddr = tp->base_addr ;
+	void __iomem *ioaddr = (void *)IOADDR_INTERFACE_ETH_A;
+    void __iomem *mdio_addr = ioaddr + CSR9;
+               /* MDRE    | MDO | MDC */
+    pilot_mdio_delay();
+    iowrite32(MDIO_RD | 0 | 0  , mdio_addr);
+    pilot_mdio_delay();
+    iowrite32(MDIO_RD | 0 | MDC  , mdio_addr);
+    pilot_mdio_delay();
+    iowrite32(MDIO_RD | 0 | 0  , mdio_addr);
+    pilot_mdio_delay();
+}
+
+void Lan_MII_MDIO_Release(struct net_device *dev)
+{
+	//struct tulip_private *tp = netdev_priv(dev);
+	//void __iomem *ioaddr = tp->base_addr ;
+	void __iomem *ioaddr = (void *)IOADDR_INTERFACE_ETH_A;
+    void __iomem *mdio_addr = ioaddr + CSR9;
+
+               /* MDRE    | MDO | MDC */
+    iowrite32(MDIO_RD | 0 | 0  , mdio_addr);
+    pilot_mdio_delay();
+}
+
+void Lan_MII_MDIO_WriteReg16(
+        struct net_device *dev,        /* Lan port */
+        unsigned char  RegNum,      /* MII PHY register number */
+        unsigned char phy_id,
+        unsigned int Data         /* Data to be written */
+)
+{
+//    printk("%s: phy_id = 0x%x\n", __FUNCTION__, phy_id);
+    Lan_MII_MDIO_Write(dev, 0xFFFFFFFF, 32);  // Idle = <1111111111111111111111111111111111>
+    Lan_MII_MDIO_Write(dev, 0x00000001,  2);  // Start = <01>
+    Lan_MII_MDIO_Write(dev, 0x00000001,  2);  // Command = <Write=01>
+    Lan_MII_MDIO_Write(dev, phy_id,  5);  // Phy Address = <InternalPHY=00000>
+    Lan_MII_MDIO_Write(dev, RegNum,      5);  // PHY reg to write = <XXXXX>
+    Lan_MII_MDIO_Write(dev, 0x00000002,  2);  // TurnAround = <01>
+    Lan_MII_MDIO_Write(dev, Data,       16);  // Data = <XXXXXXXXXXXXXXXX>
+    Lan_MII_MDIO_Release(dev);
+
+}
+
+unsigned int Lan_MII_MDIO_ReadReg16(
+        struct net_device *dev,
+        unsigned char phy_id, 
+        unsigned char RegNum        /* MII PHY register number */
+//	unsigned int PhyAdd	 /* Keep passing different addresses for PHY*/
+)
+{
+    unsigned int RegData16;
+//    printk("%s: phy_id = 0x%x\n", __FUNCTION__, phy_id);
+
+    Lan_MII_MDIO_Write(dev, 0xFFFFFFFF, 32);    // Idle = <1111111111111111111111111111111111>
+    Lan_MII_MDIO_Write(dev, 0x00000001,  2);    // Start = <01>
+    Lan_MII_MDIO_Write(dev, 0x00000002,  2);    // Command = <Read=10>
+    //Lan_MII_MDIO_Write(dev, PhyAdd,  5);  // Phy Address = <InternalPHY=00000>
+    Lan_MII_MDIO_Write(dev, phy_id,  5);    // Phy Address = <InternalPHY=00000>
+    Lan_MII_MDIO_Write(dev, RegNum,      5);    // PHY reg to write = <XXXXX>
+    Lan_MII_MDIO_WriteZ(dev);                   // TurnAround = <Z1>
+    RegData16 = Lan_MII_MDIO_Read(dev, 16);     // Data = <XXXXXXXXXXXXXXXX>
+    Lan_MII_MDIO_WriteZ(dev);                   // TurnAround
+    Lan_MII_MDIO_Release(dev);
+
+
+    return RegData16;
+}
+#if 0
+void TEST (struct net_device *dev)
+{
+    unsigned short int PhyIdHigh, PhyIdLow, PhyStatus, PhyControl;
+//    unsigned short int PhyIdHigh, PhyIdLow, PhyAdd, PhyStatus, PhyControl;
+
+    int i;
+    /* Reset */
+    Lan_MII_MDIO_WriteReg16(dev, MII_PHY_CTRL_REGNO, 0, 0x8000);
+    for(i = 0; i < 1024; i++);
+
+    //for(PhyAdd = 0; PhyAdd <=0x1f; PhyAdd += 1){
+    PhyIdHigh = Lan_MII_MDIO_ReadReg16(dev, 0, MII_PHY_PHYID_HIGH);
+    printk("PHY ID HIGH : %x\n", PhyIdHigh);
+    /*	if(PhyIdHigh != 0xffff){
+        while(1);
+
+        }*/
+    //}
+
+
+
+    PhyIdLow  = Lan_MII_MDIO_ReadReg16(dev, 0, MII_PHY_PHYID_LOW);
+    printk("PHY ID LOW : %x\n", PhyIdLow);
+
+
+    PhyStatus = Lan_MII_MDIO_ReadReg16(dev, 0, MII_PHY_STATUS_REGNO);
+    printk("PHY Status : %x\n", PhyStatus);
+
+    PhyControl = Lan_MII_MDIO_ReadReg16(dev, 0, MII_PHY_CTRL_REGNO);
+    printk("PHY Control: %x\n", PhyControl);
+
+    PhyControl &= 0x1000;
+
+    Lan_MII_MDIO_WriteReg16(dev, MII_PHY_CTRL_REGNO, 0, PhyControl);
+
+    PhyControl = Lan_MII_MDIO_ReadReg16(dev, 0, MII_PHY_CTRL_REGNO);
+    printk("PHY Status : %x\n", PhyControl);
+
+}
+#endif
