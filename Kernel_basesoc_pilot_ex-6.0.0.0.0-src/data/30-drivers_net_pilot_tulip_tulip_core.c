--- linux-3.14.17/drivers/net/ethernet/pilot_tulip/tulip_core.c	2016-08-16 17:51:30.171712131 +0800
+++ linux-3.14.17.new/drivers/net/ethernet/pilot_tulip/tulip_core.c	2016-08-16 17:44:35.975712131 +0800
@@ -0,0 +1,2483 @@
+/*	tulip_core.c: A DEC 21x4x-family ethernet driver for Linux.
+
+	Copyright 2000,2001  The Linux Kernel Team
+	Written/copyright 1994-2001 by Donald Becker.
+
+	This software may be used and distributed according to the terms
+	of the GNU General Public License, incorporated herein by reference.
+
+	Please refer to Documentation/DocBook/tulip-user.{pdf,ps,html}
+	for more information on this driver.
+
+	Please submit bugs to http://bugzilla.kernel.org/ .
+*/
+
+
+#define DRV_NAME	"tulip"
+#ifdef CONFIG_TULIP_NAPI
+#define DRV_VERSION    "1.1.15-NAPI" /* Keep at least for test */
+#else
+#define DRV_VERSION	"1.1.15"
+#endif
+#define DRV_RELDATE	"Feb 27, 2007"
+
+
+#include <linux/interrupt.h>
+#include <linux/module.h>
+#include <linux/pci.h>
+#include "tulip.h"
+#include <linux/init.h>
+#include <linux/etherdevice.h>
+#include <linux/delay.h>
+#include <linux/mii.h>
+#include <linux/ethtool.h>
+#include <linux/crc32.h>
+#include <asm/unaligned.h>
+#include <asm/uaccess.h>
+#include <linux/ethtool.h>
+#include <linux/mii.h>
+#include <linux/eth_over.h>
+
+#ifdef CONFIG_SPARC
+#include <asm/prom.h>
+#endif
+
+#include "pilot_phy.h"
+
+static char version[] =
+	"Linux PilotIII Tulip driver version " DRV_VERSION " (" DRV_RELDATE ")\n";
+
+
+/* A few user-configurable values. */
+
+/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
+static unsigned int max_interrupt_work = 25;
+
+
+struct net_device *pilot3_devices[2];
+#define MAX_UNITS 8
+/* Used to pass the full-duplex flag, etc. */
+static int full_duplex[MAX_UNITS];
+static int options[MAX_UNITS];
+static int mtu[MAX_UNITS];			/* Jumbo MTU for interfaces. */
+
+/*  The possible media types that can be set in options[] are: */
+const char * const medianame[32] = {
+	"10baseT", "10base2", "AUI", "100baseTx",
+	"10baseT-FDX", "100baseTx-FDX", "100baseT4", "100baseFx",
+	"100baseFx-FDX", "MII 10baseT", "MII 10baseT-FDX", "MII",
+	"10baseT(forced)", "MII 100baseTx", "MII 100baseTx-FDX", "MII 100baseT4",
+	"MII 100baseFx-HDX", "MII 100baseFx-FDX", "Home-PNA 1Mbps", "Invalid-19",
+	"","","","", "","","","",  "","","","Transceiver reset",
+};
+
+/* Set the copy breakpoint for the copy-only-tiny-buffer Rx structure. */
+#if defined(__alpha__) || defined(__arm__) || defined(__hppa__) \
+	|| defined(CONFIG_SPARC) || defined(__ia64__) \
+	|| defined(__sh__) || defined(__mips__)
+static int rx_copybreak = 1518;
+#else
+static int rx_copybreak = 100;
+#endif
+
+/*
+  Set the bus performance register.
+	Typical: Set 16 longword cache alignment, no burst limit.
+	Cache alignment bits 15:14	     Burst length 13:8
+		0000	No alignment  0x00000000 unlimited		0800 8 longwords
+		4000	8  longwords		0100 1 longword		1000 16 longwords
+		8000	16 longwords		0200 2 longwords	2000 32 longwords
+		C000	32  longwords		0400 4 longwords
+	Warning: many older 486 systems are broken and require setting 0x00A04800
+	   8 longword cache alignment, 8 longword burst.
+	ToDo: Non-Intel setting could be better.
+*/
+
+#if defined(__alpha__) || defined(__ia64__)
+static int csr0 = 0x01A00000 | 0xE000;
+#elif defined(__i386__) || defined(__powerpc__) || defined(__x86_64__)
+static int csr0 = 0x01A00000 | 0x8000;
+#elif defined(CONFIG_SPARC) || defined(__hppa__)
+/* The UltraSparc PCI controllers will disconnect at every 64-byte
+ * crossing anyways so it makes no sense to tell Tulip to burst
+ * any more than that.
+ */
+static int csr0 = 0x01A00000 | 0x9000;
+#elif defined(__arm__) || defined(__sh__)
+static int csr0 = 0x01A00000 | 0x4800;
+#elif defined(__mips__)
+static int csr0 = 0x00200000 | 0x4000;
+#else
+#warning Processor architecture undefined!
+static int csr0 = 0x00A00000 | 0x2000;
+#endif
+
+/* Operational parameters that usually are not changed. */
+/* Time in jiffies before concluding the transmitter is hung. */
+#define TX_TIMEOUT  (4*HZ)
+
+
+MODULE_AUTHOR("The Linux Kernel Team");
+MODULE_DESCRIPTION("Digital 21*4* Tulip ethernet driver");
+MODULE_LICENSE("GPL");
+MODULE_VERSION(DRV_VERSION);
+module_param(tulip_debug, int, 0);
+module_param(max_interrupt_work, int, 0);
+module_param(rx_copybreak, int, 0);
+module_param(csr0, int, 0);
+module_param_array(options, int, NULL, 0);
+module_param_array(full_duplex, int, NULL, 0);
+
+#define PFX DRV_NAME ": "
+
+#ifdef TULIP_DEBUG
+int tulip_debug = TULIP_DEBUG;
+#else
+int tulip_debug = 1;
+#endif
+
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_SHARED_MDIO_MDC_LINES
+//to be used in media.c
+spinlock_t miilock;
+#endif
+
+static void tulip_timer(unsigned long data)
+{
+	struct net_device *dev = (struct net_device *)data;
+	struct tulip_private *tp = netdev_priv(dev);
+
+	if (netif_running(dev))
+		schedule_work(&tp->media_work);
+}
+
+/*
+ * This table use during operation for capabilities and media timer.
+ *
+ * It is indexed via the values in 'enum chips'
+ */
+
+struct tulip_chip_table tulip_tbl[] = {
+#if 0
+  { }, /* placeholder for array, slot unused currently */
+  { }, /* placeholder for array, slot unused currently */
+
+  /* DC21140 */
+  { "Digital DS21140 Tulip", 128, 0x0001ebef,
+	HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM | HAS_PCI_MWI, tulip_timer,
+	tulip_media_task },
+
+  /* DC21142, DC21143 */
+  { "Digital DS21142/43 Tulip", 128, 0x0801fbff,
+	HAS_MII | HAS_MEDIA_TABLE | ALWAYS_CHECK_MII | HAS_ACPI | HAS_NWAY
+	| HAS_INTR_MITIGATION | HAS_PCI_MWI, tulip_timer, t21142_media_task },
+
+  /* LC82C168 */
+  { "Lite-On 82c168 PNIC", 256, 0x0001fbef,
+	HAS_MII | HAS_PNICNWAY, pnic_timer, },
+
+  /* MX98713 */
+  { "Macronix 98713 PMAC", 128, 0x0001ebef,
+	HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM, mxic_timer, },
+
+  /* MX98715 */
+  { "Macronix 98715 PMAC", 256, 0x0001ebef,
+	HAS_MEDIA_TABLE, mxic_timer, },
+
+  /* MX98725 */
+  { "Macronix 98725 PMAC", 256, 0x0001ebef,
+	HAS_MEDIA_TABLE, mxic_timer, },
+
+  /* AX88140 */
+  { "ASIX AX88140", 128, 0x0001fbff,
+	HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM | MC_HASH_ONLY
+	| IS_ASIX, tulip_timer, tulip_media_task },
+
+  /* PNIC2 */
+  { "Lite-On PNIC-II", 256, 0x0801fbff,
+	HAS_MII | HAS_NWAY | HAS_8023X | HAS_PCI_MWI, pnic2_timer, },
+
+  /* COMET */
+  { "ADMtek Comet", 256, 0x0001abef,
+	HAS_MII | MC_HASH_ONLY | COMET_MAC_ADDR, comet_timer, },
+
+  /* COMPEX9881 */
+  { "Compex 9881 PMAC", 128, 0x0001ebef,
+	HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM, mxic_timer, },
+
+  /* I21145 */
+  { "Intel DS21145 Tulip", 128, 0x0801fbff,
+	HAS_MII | HAS_MEDIA_TABLE | ALWAYS_CHECK_MII | HAS_ACPI
+	| HAS_NWAY | HAS_PCI_MWI, tulip_timer, tulip_media_task },
+
+  /* DM910X */
+  { "Davicom DM9102/DM9102A", 128, 0x0001ebef,
+	HAS_MII | HAS_MEDIA_TABLE | CSR12_IN_SROM | HAS_ACPI,
+	tulip_timer, tulip_media_task },
+
+  /* RS7112 */
+  { "Conexant LANfinity", 256, 0x0001ebef,
+	HAS_MII | HAS_ACPI, tulip_timer, tulip_media_task },
+#endif
+  /* Pilot3 */
+	{"Pilot3 Tulip", 128, 0x00018942, 
+    HAS_MII, 
+    tulip_timer,tulip_media_task }, 
+};
+
+#if 0
+static struct pci_device_id tulip_pci_tbl[] = {
+	{ 0x1011, 0x0009, PCI_ANY_ID, PCI_ANY_ID, 0, 0, DC21140 },
+	{ 0x1011, 0x0019, PCI_ANY_ID, PCI_ANY_ID, 0, 0, DC21143 },
+	{ 0x11AD, 0x0002, PCI_ANY_ID, PCI_ANY_ID, 0, 0, LC82C168 },
+	{ 0x10d9, 0x0512, PCI_ANY_ID, PCI_ANY_ID, 0, 0, MX98713 },
+	{ 0x10d9, 0x0531, PCI_ANY_ID, PCI_ANY_ID, 0, 0, MX98715 },
+/*	{ 0x10d9, 0x0531, PCI_ANY_ID, PCI_ANY_ID, 0, 0, MX98725 },*/
+	{ 0x125B, 0x1400, PCI_ANY_ID, PCI_ANY_ID, 0, 0, AX88140 },
+	{ 0x11AD, 0xc115, PCI_ANY_ID, PCI_ANY_ID, 0, 0, PNIC2 },
+	{ 0x1317, 0x0981, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1317, 0x0985, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1317, 0x1985, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1317, 0x9511, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x13D1, 0xAB02, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x13D1, 0xAB03, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x13D1, 0xAB08, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x104A, 0x0981, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x104A, 0x2774, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1259, 0xa120, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x11F6, 0x9881, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMPEX9881 },
+	{ 0x8086, 0x0039, PCI_ANY_ID, PCI_ANY_ID, 0, 0, I21145 },
+	{ 0x1282, 0x9100, PCI_ANY_ID, PCI_ANY_ID, 0, 0, DM910X },
+	{ 0x1282, 0x9102, PCI_ANY_ID, PCI_ANY_ID, 0, 0, DM910X },
+	{ 0x1113, 0x1216, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1113, 0x1217, PCI_ANY_ID, PCI_ANY_ID, 0, 0, MX98715 },
+	{ 0x1113, 0x9511, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1186, 0x1541, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1186, 0x1561, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1186, 0x1591, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x14f1, 0x1803, PCI_ANY_ID, PCI_ANY_ID, 0, 0, CONEXANT },
+	{ 0x1626, 0x8410, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1737, 0xAB09, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x1737, 0xAB08, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x17B3, 0xAB08, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ 0x10b7, 0x9300, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET }, /* 3Com 3CSOHO100B-TX */
+	{ 0x14ea, 0xab08, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET }, /* Planex FNW-3602-TX */
+	{ 0x1414, 0x0002, PCI_ANY_ID, PCI_ANY_ID, 0, 0, COMET },
+	{ } /* terminate list */
+};
+MODULE_DEVICE_TABLE(pci, tulip_pci_tbl);
+#endif
+
+/* A full-duplex map for media types. */
+const char tulip_media_cap[32] =
+{0,0,0,16,  3,19,16,24,  27,4,7,5, 0,20,23,20,  28,31,0,0, };
+
+static void tulip_tx_timeout(struct net_device *dev);
+static void tulip_init_ring(struct net_device *dev);
+static int tulip_start_xmit(struct sk_buff *skb, struct net_device *dev);
+static int tulip_open(struct net_device *dev);
+static int tulip_close(struct net_device *dev);
+static void tulip_up(struct net_device *dev);
+static void tulip_down(struct net_device *dev);
+static struct net_device_stats *tulip_get_stats(struct net_device *dev);
+static int private_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
+static void set_rx_mode(struct net_device *dev);
+#ifdef CONFIG_NET_POLL_CONTROLLER
+static void poll_tulip(struct net_device *dev);
+#endif
+
+static void tulip_set_power_state (struct tulip_private *tp,
+				   int sleep, int snooze)
+{
+#if 0
+	if (tp->flags & HAS_ACPI) {
+		u32 tmp, newtmp;
+		pci_read_config_dword (tp->pdev, CFDD, &tmp);
+		newtmp = tmp & ~(CFDD_Sleep | CFDD_Snooze);
+		if (sleep)
+			newtmp |= CFDD_Sleep;
+		else if (snooze)
+			newtmp |= CFDD_Snooze;
+		if (tmp != newtmp)
+			pci_write_config_dword (tp->pdev, CFDD, newtmp);
+	}
+#endif
+}
+
+/****************************************
+	pilot_up() returns 
+		-2 : No PHY
+		-1 : No Link
+		 0 : PHY and Link UP 
+******************************************/
+int pilot_up(struct net_device *dev, int verbose)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	struct ethtool_cmd ecmd;
+	unsigned short phyid;
+
+	/* Start with 100Mbps Full Duplex */
+	tp->csr6 = ((0x1<<1) | (0x1<<30) | (0x1<<21) | (0x1 << 9));
+	tp->full_duplex = 1;
+
+	/* Check if we have valid PHY */
+	if (tp->mii.phy_id == INVALID_PHYID)	/* Possible NCSI */
+	{	
+		return -2;
+	}
+	
+	/* Confirm if we really have a MII PHY by reading Manufacturer ID  */
+#ifdef CONFIG_SOC_SE_PILOT3
+    	phyid = tulip_mdio_read(dev, tp->mii.phy_id, MII_PHYSID1);
+#else
+    	phyid = tulip_mdio_read(dev, tp->mii.phy_id, MII_PHYSID2);
+#endif
+	if ((phyid == 0) || (phyid == 0xFFFF))	/* Possible NCSI */
+	{
+		tp->phys[tp->phy_index] = INVALID_PHYID;
+		tp->mii.phy_id = INVALID_PHYID;
+		return -2;
+	}
+
+  	if (tulip_debug > 4)
+		printk("%s : PHY ID  = 0x%x\n",dev->name,tp->phys[tp->phy_index]-1);
+	
+	if (!mii_link_ok (&tp->mii))
+	{
+		if (verbose)
+			printk("%s : Link is Down\n",dev->name);
+		return -1;
+	}
+
+	/* Use ethtool commands to get speed and duplex */
+	mii_ethtool_gset(&(tp->mii),&ecmd);
+	if (ecmd.duplex == DUPLEX_FULL)
+	{
+  		if (tulip_debug > 4)
+	       		printk("%s : Auto Negotiated Full Duplex\n",dev->name);
+        	tp->csr6 |= (0x1 << 9);
+        	tp->full_duplex = 1;
+	}
+	else
+	{
+  		if (tulip_debug > 4)
+       			printk("%s : Auto Negotiated Half Duplex\n",dev->name);
+        	tp->csr6 &= (~(0x1 << 9));	
+        	tp->full_duplex = 0;
+	}
+
+	switch (ecmd.speed)
+	{	
+		case SPEED_1000:
+			if (verbose)
+	       			printk("%s : Auto Negotiated 1000-Base-TX %s\n",dev->name,(ecmd.duplex == DUPLEX_FULL)?"Full Duplex":"Half Duplex");
+			tp->csr6 |= (0x1 << 17);
+			tp->csr6 |= (0x1 << 16);
+			tp->csr6 &= ~TxThreshold;
+			tp->speed=1000;
+			break;
+
+		case SPEED_10:
+			if (verbose)
+       				printk("%s : Auto Negotiated 10-Base-TX %s\n",dev->name,(ecmd.duplex == DUPLEX_FULL)?"Full Duplex":"Half Duplex");
+ 			tp->csr6 |= (0x1 << 17);
+           		tp->csr6 &= ~(0x1 << 16);
+			tp->csr6 |= TxThreshold;
+			tp->speed=10;
+			break;
+
+		case SPEED_100:
+		default:
+			if (verbose)
+       				printk("%s : Auto Negotiated 100-Base-TX %s\n",dev->name,(ecmd.duplex == DUPLEX_FULL)?"Full Duplex":"Half Duplex");
+			tp->csr6 &= ~(0x1 << 16);
+			tp->csr6 &= ~(0x1 << 17);
+			tp->csr6 &= ~TxThreshold;
+			tp->speed=100;
+			break;
+	}
+
+	if (verbose)
+		printk("%s : Link is Up\n",dev->name);
+	return 0;
+}
+	
+#if 0
+int pilot_up(struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	unsigned short status,anlpr,cntrl,phyid;
+	int retval = 0;
+	struct ethtool_cmd ecmd;
+
+	/* Start at full duplex 100MBps  */ 
+   	tp->csr6 = ((0x1<<1) | (0x1<<30) | (0x1<<21) | (0x1 << 9));
+	tp->full_duplex = 1;
+
+	/* Check if the MII Registers are present */
+    phyid = tulip_mdio_read(dev, tp->phys[tp->phy_index], 0x03);
+	if ((phyid == 0) || (phyid == 0xFFFF))
+	{
+		printk("%s : No PHY Detected\n",dev->name);
+		/* Assume it is NC-SI or UMP */
+		return 0;
+	}
+
+	/* Read MII Status. Need two reads to get Link properly*/
+    status = tulip_mdio_read(dev, tp->phys[tp->phy_index], 0x01);
+    status = tulip_mdio_read(dev, tp->phys[tp->phy_index], 0x01);
+	if (status & 0x0004)
+		printk("%s : Link is Up\n",dev->name);
+	else
+	{
+		printk("%s : Link is Down\n",dev->name);
+		retval = -1;
+	}
+
+	/* Read Cntrl */
+    cntrl = tulip_mdio_read(dev, tp->phys[tp->phy_index], 0x00);
+
+	/* Read ANLPR */
+	anlpr = tulip_mdio_read(dev, tp->phys[tp->phy_index], 0x05);
+	
+
+	/* Check if Auto Negotiation completed */
+	if ((cntrl & 0x1000) && (status & 0x0020))
+	{
+		/* Check for 100-Base TX */
+		if (anlpr & 0x0180)
+		{
+   			if (tulip_debug > 4)
+       			 printk("%s : Auto Negotiated 100-Base-TX\n",dev->name);
+		}
+		else
+		{
+    		if (tulip_debug > 4)
+       			 printk("%s : Auto Negotiated 10-Base-TX\n",dev->name);
+        	tp->csr6 |= (0x1 << 17);
+		}
+
+		/* Check for Full Duplex */
+		if (anlpr & 0x0140)
+		{
+    		if (tulip_debug > 4)
+       			 printk("%s : Auto Negotiated Full Duplex\n",dev->name);
+        	tp->csr6 |= (0x1 << 9);
+        	tp->full_duplex = 1;
+        	tulip_mdio_write(dev, tp->phys[tp->phy_index], 0, cntrl | 0x100);
+		}
+		else
+		{
+  			if (tulip_debug > 4)
+       			 printk("%s : Auto Negotiated Half Duplex\n",dev->name);
+        	tp->csr6 &= (~(0x1 << 9));	
+        	tp->full_duplex = 0;
+        	tulip_mdio_write(dev, tp->phys[tp->phy_index], 0, cntrl & ~0x100);
+		}
+		return retval;
+	}
+	
+	/* Auto Negotiation is not done. Read the default settings */
+	/* Check for 100-Base TX */
+	if (cntrl &  0x2000)
+	{
+  		if (tulip_debug > 4)
+       		 printk("%s : Default Setting 100-Base-TX\n",dev->name);
+	}
+	else
+	{
+  		if (tulip_debug > 4)
+   			 printk("%s : Default Setting 10-Base-TX\n",dev->name);
+       	tp->csr6 |= (0x1 << 17);
+	}
+	/* Check for Full Duplex */
+	if (cntrl  & 0x0100)
+	{
+		if (tulip_debug > 4)
+   			 printk("%s : Default Setting Full Duplex\n",dev->name);
+       	tp->csr6 |= (0x1 << 9);
+       	tp->full_duplex = 1;
+	}
+	else
+	{
+ 		if (tulip_debug > 4)
+   			 printk("%s : Default Setting Half Duplex\n",dev->name);
+       	tp->csr6 &= (~(0x1 << 9));	
+       	tp->full_duplex = 0;
+	}
+
+    return retval;
+}
+#endif
+
+static void tulip_up(struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	void __iomem *ioaddr = tp->base_addr;
+	int next_tick = 3*HZ;
+//	u32 reg;
+	int i;
+
+#ifdef CONFIG_TULIP_NAPI
+	napi_enable(&tp->napi);
+#endif
+
+	/* Wake the chip from sleep/snooze mode. */
+	tulip_set_power_state (tp, 0, 0);
+	iowrite32(0x00000000, ioaddr + CSR6);     // Pilot-II/III Addition
+
+	/* On some chip revs we must set the MII/SYM port before the reset!? */
+	if (tp->mii_cnt  ||  (tp->mtable  &&  tp->mtable->has_mii))
+		iowrite32(0x00040000, ioaddr + CSR6);
+
+	/* Reset Pilot-III Chip */
+	iowrite32((ioread32(ioaddr + CSR0)|0x1), ioaddr + CSR0);
+        printk("Pilot-III: Waiting for MAC soft reset to be completed ...\n");
+	 while(ioread32(ioaddr + CSR0)&0x1 );
+	 printk("Pilot-III: MAC soft reset completed\n");
+	 
+ 	/* Reset the chip, holding bit 0 set at least 50 PCI cycles. */
+//	iowrite32(0x00000001, ioaddr + CSR0);
+//	pci_read_config_dword(tp->pdev, PCI_COMMAND, &reg);  /* flush write */
+//	udelay(100);
+
+	/* Deassert reset.
+	   Wait the specified 50 PCI cycles after a reset by initializing
+	   Tx and Rx queues and the address filter list. */
+	iowrite32(tp->csr0, ioaddr + CSR0);
+//	pci_read_config_dword(tp->pdev, PCI_COMMAND, &reg);  /* flush write */
+//	udelay(100);
+/* Pilot-III Addition Begin*/
+#if P3_A0
+	iowrite32((ioread32(ioaddr + CSR0)|0x2), ioaddr + CSR0);
+#endif
+	iowrite32( (CSR11_TIM), (ioaddr + CSR11)); 
+/* Pilot-III Addition End */
+	if (tulip_debug > 1)
+		printk(KERN_DEBUG "%s: tulip_up(), irq==%d.\n", dev->name, dev->irq);
+
+	iowrite32(tp->rx_ring_dma, ioaddr + CSR3);
+	iowrite32(tp->tx_ring_dma, ioaddr + CSR4);
+	tp->cur_rx = tp->cur_tx = 0;
+	tp->dirty_rx = tp->dirty_tx = 0;
+
+	if (tp->flags & MC_HASH_ONLY) {
+#if 0 
+		u32 addr_low = get_unaligned_le32(dev->dev_addr);
+		u32 addr_high = get_unaligned_le16(dev->dev_addr + 4);
+		if (tp->chip_id == AX88140) {
+			iowrite32(0, ioaddr + CSR13);
+			iowrite32(addr_low,  ioaddr + CSR14);
+			iowrite32(1, ioaddr + CSR13);
+			iowrite32(addr_high, ioaddr + CSR14);
+		} else if (tp->flags & COMET_MAC_ADDR) {
+			iowrite32(addr_low,  ioaddr + 0xA4);
+			iowrite32(addr_high, ioaddr + 0xA8);
+			iowrite32(0, ioaddr + 0xAC);
+			iowrite32(0, ioaddr + 0xB0);
+		}
+#endif
+	} else {
+		/* This is set_rx_mode(), but without starting the transmitter. */
+		u16 *eaddrs = (u16 *)dev->dev_addr;
+		u16 *setup_frm = &tp->setup_frame[15*6];
+		dma_addr_t mapping;
+
+		/* 21140 bug: you must add the broadcast address. */
+		memset(tp->setup_frame, 0xff, sizeof(tp->setup_frame));
+		/* Fill the final entry of the table with our physical address. */
+		*setup_frm++ = eaddrs[0]; *setup_frm++ = eaddrs[0];
+		*setup_frm++ = eaddrs[1]; *setup_frm++ = eaddrs[1];
+		*setup_frm++ = eaddrs[2]; *setup_frm++ = eaddrs[2];
+
+		mapping = pci_map_single(tp->pdev, tp->setup_frame,
+					 sizeof(tp->setup_frame),
+					 PCI_DMA_TODEVICE);
+		tp->tx_buffers[tp->cur_tx].skb = NULL;
+		tp->tx_buffers[tp->cur_tx].mapping = mapping;
+
+		/* Put the setup frame on the Tx list. */
+		tp->tx_ring[tp->cur_tx].length = cpu_to_le32(0x08000000 | 192);
+		tp->tx_ring[tp->cur_tx].buffer1 = cpu_to_le32(mapping);
+		tp->tx_ring[tp->cur_tx].status = cpu_to_le32(DescOwned);
+
+		tp->cur_tx++;
+	}
+
+	tp->saved_if_port = dev->if_port;
+	if (dev->if_port == 0)
+		dev->if_port = tp->default_port;
+
+	/* Allow selecting a default media. */
+	i = 0;
+	if (tp->mtable == NULL)
+		goto media_picked;
+	if (dev->if_port) {
+		int looking_for = tulip_media_cap[dev->if_port] & MediaIsMII ? 11 :
+			(dev->if_port == 12 ? 0 : dev->if_port);
+		for (i = 0; i < tp->mtable->leafcount; i++)
+			if (tp->mtable->mleaf[i].media == looking_for) {
+				printk(KERN_INFO "%s: Using user-specified media %s.\n",
+					   dev->name, medianame[dev->if_port]);
+				goto media_picked;
+			}
+	}
+	if ((tp->mtable->defaultmedia & 0x0800) == 0) {
+		int looking_for = tp->mtable->defaultmedia & MEDIA_MASK;
+		for (i = 0; i < tp->mtable->leafcount; i++)
+			if (tp->mtable->mleaf[i].media == looking_for) {
+				printk(KERN_INFO "%s: Using EEPROM-set media %s.\n",
+					   dev->name, medianame[looking_for]);
+				goto media_picked;
+			}
+	}
+	/* Start sensing first non-full-duplex media. */
+	for (i = tp->mtable->leafcount - 1;
+		 (tulip_media_cap[tp->mtable->mleaf[i].media] & MediaAlwaysFD) && i > 0; i--)
+		;
+media_picked:
+
+	tp->csr6 = 0;
+	tp->cur_index = i;
+	tp->nwayset = 0;
+#if 0
+	if (dev->if_port) {
+		if (tp->chip_id == DC21143  &&
+		    (tulip_media_cap[dev->if_port] & MediaIsMII)) {
+			/* We must reset the media CSRs when we force-select MII mode. */
+			iowrite32(0x0000, ioaddr + CSR13);
+			iowrite32(0x0000, ioaddr + CSR14);
+			iowrite32(0x0008, ioaddr + CSR15);
+		}
+		tulip_select_media(dev, 1);
+	} else if (tp->chip_id == DC21142) {
+		if (tp->mii_cnt) {
+			tulip_select_media(dev, 1);
+			if (tulip_debug > 1)
+				printk(KERN_INFO "%s: Using MII transceiver %d, status "
+					   "%4.4x.\n",
+					   dev->name, tp->phys[0], tulip_mdio_read(dev, tp->phys[0], 1));
+			iowrite32(csr6_mask_defstate, ioaddr + CSR6);
+			tp->csr6 = csr6_mask_hdcap;
+			dev->if_port = 11;
+			iowrite32(0x0000, ioaddr + CSR13);
+			iowrite32(0x0000, ioaddr + CSR14);
+		} else
+			t21142_start_nway(dev);
+	} else if (tp->chip_id == PNIC2) {
+	        /* for initial startup advertise 10/100 Full and Half */
+	        tp->sym_advertise = 0x01E0;
+                /* enable autonegotiate end interrupt */
+	        iowrite32(ioread32(ioaddr+CSR5)| 0x00008010, ioaddr + CSR5);
+	        iowrite32(ioread32(ioaddr+CSR7)| 0x00008010, ioaddr + CSR7);
+		pnic2_start_nway(dev);
+	} else if (tp->chip_id == LC82C168  &&  ! tp->medialock) {
+		if (tp->mii_cnt) {
+			dev->if_port = 11;
+			tp->csr6 = 0x814C0000 | (tp->full_duplex ? 0x0200 : 0);
+			iowrite32(0x0001, ioaddr + CSR15);
+		} else if (ioread32(ioaddr + CSR5) & TPLnkPass)
+			pnic_do_nway(dev);
+		else {
+			/* Start with 10mbps to do autonegotiation. */
+			iowrite32(0x32, ioaddr + CSR12);
+			tp->csr6 = 0x00420000;
+			iowrite32(0x0001B078, ioaddr + 0xB8);
+			iowrite32(0x0201B078, ioaddr + 0xB8);
+			next_tick = 1*HZ;
+		}
+	} else if ((tp->chip_id == MX98713 || tp->chip_id == COMPEX9881)
+			   && ! tp->medialock) {
+		dev->if_port = 0;
+		tp->csr6 = 0x01880000 | (tp->full_duplex ? 0x0200 : 0);
+		iowrite32(0x0f370000 | ioread16(ioaddr + 0x80), ioaddr + 0x80);
+	} else if (tp->chip_id == MX98715 || tp->chip_id == MX98725) {
+		/* Provided by BOLO, Macronix - 12/10/1998. */
+		dev->if_port = 0;
+		tp->csr6 = 0x01a80200;
+		iowrite32(0x0f370000 | ioread16(ioaddr + 0x80), ioaddr + 0x80);
+		iowrite32(0x11000 | ioread16(ioaddr + 0xa0), ioaddr + 0xa0);
+	} else if (tp->chip_id == COMET || tp->chip_id == CONEXANT) {
+		/* Enable automatic Tx underrun recovery. */
+		iowrite32(ioread32(ioaddr + 0x88) | 1, ioaddr + 0x88);
+		dev->if_port = tp->mii_cnt ? 11 : 0;
+		tp->csr6 = 0x00040000;
+	} else if (tp->chip_id == AX88140) {
+		tp->csr6 = tp->mii_cnt ? 0x00040100 : 0x00000100;
+	} else 
+#endif
+	if (tp->chip_id == PILOT_TULIP) 
+	{
+		tulip_select_media(dev, 1);
+	        pilot_up(dev,1);
+	} 
+
+
+	/* Start the chip's Tx to process setup frame. */
+	tulip_stop_rxtx(tp);
+	barrier();
+	udelay(5);
+	iowrite32(tp->csr6 | TxOn, ioaddr + CSR6);
+
+	/* Enable interrupts by setting the interrupt mask. */
+	iowrite32(tulip_tbl[tp->chip_id].valid_intrs, ioaddr + CSR5);
+	iowrite32(tulip_tbl[tp->chip_id].valid_intrs, ioaddr + CSR7);
+	tulip_start_rxtx(tp);
+	iowrite32(0, ioaddr + CSR2);		/* Rx poll demand */
+
+	if (tulip_debug > 2) {
+		printk(KERN_DEBUG "%s: Done tulip_up(), CSR0 %8.8x, CSR5 %8.8x CSR6 %8.8x.\n",
+			   dev->name, ioread32(ioaddr + CSR0), ioread32(ioaddr + CSR5),
+			   ioread32(ioaddr + CSR6));
+	}
+
+	/* Set the timer to switch to check for link beat and perhaps switch
+	   to an alternate media type. */
+	tp->timer.expires = RUN_AT(next_tick);
+	add_timer(&tp->timer);
+#ifdef CONFIG_TULIP_NAPI
+	init_timer(&tp->oom_timer);
+        tp->oom_timer.data = (unsigned long)dev;
+        tp->oom_timer.function = oom_timer;
+#endif
+}
+
+static int
+tulip_open(struct net_device *dev)
+{
+	int retval;
+
+	if ((retval = request_irq(dev->irq, &tulip_interrupt, IRQF_SHARED, dev->name, dev)))
+		return retval;
+
+	tulip_init_ring (dev);
+
+	tulip_up (dev);
+
+	netif_start_queue (dev);
+
+	return 0;
+}
+
+
+static void tulip_tx_timeout(struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	void __iomem *ioaddr = tp->base_addr;
+	unsigned long flags;
+
+	spin_lock_irqsave (&tp->lock, flags);
+
+	if (tulip_media_cap[dev->if_port] & MediaIsMII) {
+		/* Do nothing -- the media monitor should handle this. */
+		if (tulip_debug > 1)
+			printk(KERN_WARNING "%s: Transmit timeout using MII device.\n",
+				   dev->name);
+#if 0
+	} else if (tp->chip_id == DC21140 || tp->chip_id == DC21142
+			   || tp->chip_id == MX98713 || tp->chip_id == COMPEX9881
+			   || tp->chip_id == DM910X) {
+		printk(KERN_WARNING "%s: 21140 transmit timed out, status %8.8x, "
+			   "SIA %8.8x %8.8x %8.8x %8.8x, resetting...\n",
+			   dev->name, ioread32(ioaddr + CSR5), ioread32(ioaddr + CSR12),
+			   ioread32(ioaddr + CSR13), ioread32(ioaddr + CSR14), ioread32(ioaddr + CSR15));
+		tp->timeout_recovery = 1;
+		schedule_work(&tp->media_work);
+		goto out_unlock;
+	} else if (tp->chip_id == PNIC2) {
+		printk(KERN_WARNING "%s: PNIC2 transmit timed out, status %8.8x, "
+		       "CSR6/7 %8.8x / %8.8x CSR12 %8.8x, resetting...\n",
+		       dev->name, (int)ioread32(ioaddr + CSR5), (int)ioread32(ioaddr + CSR6),
+		       (int)ioread32(ioaddr + CSR7), (int)ioread32(ioaddr + CSR12));
+#endif
+	} else {
+		printk(KERN_WARNING "%s: Transmit timed out, status %8.8x, CSR12 "
+			   "%8.8x, resetting...\n",
+			   dev->name, ioread32(ioaddr + CSR5), ioread32(ioaddr + CSR12));
+		dev->if_port = 0;
+	}
+
+#if defined(way_too_many_messages)
+	if (tulip_debug > 3) {
+		int i;
+		for (i = 0; i < RX_RING_SIZE; i++) {
+			u8 *buf = (u8 *)(tp->rx_ring[i].buffer1);
+			int j;
+			printk(KERN_DEBUG "%2d: %8.8x %8.8x %8.8x %8.8x  "
+				   "%2.2x %2.2x %2.2x.\n",
+				   i, (unsigned int)tp->rx_ring[i].status,
+				   (unsigned int)tp->rx_ring[i].length,
+				   (unsigned int)tp->rx_ring[i].buffer1,
+				   (unsigned int)tp->rx_ring[i].buffer2,
+				   buf[0], buf[1], buf[2]);
+			for (j = 0; buf[j] != 0xee && j < 1600; j++)
+				if (j < 100) printk(" %2.2x", buf[j]);
+			printk(" j=%d.\n", j);
+		}
+		printk(KERN_DEBUG "  Rx ring %8.8x: ", (int)tp->rx_ring);
+		for (i = 0; i < RX_RING_SIZE; i++)
+			printk(" %8.8x", (unsigned int)tp->rx_ring[i].status);
+		printk("\n" KERN_DEBUG "  Tx ring %8.8x: ", (int)tp->tx_ring);
+		for (i = 0; i < TX_RING_SIZE; i++)
+			printk(" %8.8x", (unsigned int)tp->tx_ring[i].status);
+		printk("\n");
+	}
+#endif
+
+	tulip_tx_timeout_complete(tp, ioaddr);
+
+//out_unlock:
+	spin_unlock_irqrestore (&tp->lock, flags);
+	dev->trans_start = jiffies;
+	netif_wake_queue (dev);
+}
+
+
+/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
+static void tulip_init_ring(struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	int i;
+
+	tp->susp_rx = 0;
+	tp->ttimer = 0;
+	tp->nir = 0;
+
+	for (i = 0; i < RX_RING_SIZE; i++) {
+		tp->rx_ring[i].status = 0x00000000;
+		tp->rx_ring[i].length = cpu_to_le32(PKT_BUF_SZ);
+		tp->rx_ring[i].buffer2 = cpu_to_le32(tp->rx_ring_dma + sizeof(struct tulip_rx_desc) * (i + 1));
+		tp->rx_buffers[i].skb = NULL;
+		tp->rx_buffers[i].mapping = 0;
+	}
+	/* Mark the last entry as wrapping the ring. */
+	tp->rx_ring[i-1].length = cpu_to_le32(PKT_BUF_SZ | DESC_RING_WRAP);
+	tp->rx_ring[i-1].buffer2 = cpu_to_le32(tp->rx_ring_dma);
+
+	for (i = 0; i < RX_RING_SIZE; i++) {
+		dma_addr_t mapping;
+
+		/* Note the receive buffer must be longword aligned.
+		   dev_alloc_skb() provides 16 byte alignment.  But do *not*
+		   use skb_reserve() to align the IP header! */
+		struct sk_buff *skb = dev_alloc_skb(PKT_BUF_SZ);
+		tp->rx_buffers[i].skb = skb;
+		if (skb == NULL)
+			break;
+		mapping = pci_map_single(tp->pdev, skb->data,
+					 PKT_BUF_SZ, PCI_DMA_FROMDEVICE);
+		tp->rx_buffers[i].mapping = mapping;
+		skb->dev = dev;			/* Mark as being used by this device. */
+		tp->rx_ring[i].status = cpu_to_le32(DescOwned);	/* Owned by Tulip chip */
+		tp->rx_ring[i].buffer1 = cpu_to_le32(mapping);
+	}
+	tp->dirty_rx = (unsigned int)(i - RX_RING_SIZE);
+
+	/* The Tx buffer descriptor is filled in as needed, but we
+	   do need to clear the ownership bit. */
+	for (i = 0; i < TX_RING_SIZE; i++) {
+		tp->tx_buffers[i].skb = NULL;
+		tp->tx_buffers[i].mapping = 0;
+		tp->tx_ring[i].status = 0x00000000;
+		tp->tx_ring[i].buffer2 = cpu_to_le32(tp->tx_ring_dma + sizeof(struct tulip_tx_desc) * (i + 1));
+	}
+	tp->tx_ring[i-1].buffer2 = cpu_to_le32(tp->tx_ring_dma);
+}
+
+static int
+tulip_start_xmit(struct sk_buff *skb, struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	int entry;
+	u32 flag;
+	dma_addr_t mapping;
+    void __iomem *ioaddr = tp->base_addr;
+
+#ifndef CONFIG_PILOT_SG
+	spin_lock_irq(&tp->lock);
+#endif
+	/* Calculate the next Tx descriptor entry. */
+	entry = tp->cur_tx % TX_RING_SIZE;
+
+	tp->tx_buffers[entry].skb = skb;
+	mapping = pci_map_single(tp->pdev, skb->data,
+				 skb->len, PCI_DMA_TODEVICE);
+	tp->tx_buffers[entry].mapping = mapping;
+	tp->tx_ring[entry].buffer1 = cpu_to_le32(mapping);
+
+	if (tp->cur_tx - tp->dirty_tx < TX_RING_SIZE/2) {/* Typical path */
+		flag = 0x60000000; /* No interrupt */
+	} else if (tp->cur_tx - tp->dirty_tx == TX_RING_SIZE/2) {
+		flag = 0xe0000000; /* Tx-done intr. */
+	} else if (tp->cur_tx - tp->dirty_tx < TX_RING_SIZE - 2) {
+		flag = 0x60000000; /* No Tx-done intr. */
+	} else {		/* Leave room for set_rx_mode() to fill entries. */
+		flag = 0xe0000000; /* Tx-done intr. */
+     	/* enable tx interrupt,when the ring is full */
+      	tp->p_tx_coalising_en=1;
+      	iowrite32( (ioread32(ioaddr + CSR7)|TxIntr),(ioaddr + CSR7) ); 
+      	iowrite32( (CSR11_NTP), (ioaddr + CSR11) ); 
+		netif_stop_queue(dev);
+	}
+	if (entry == TX_RING_SIZE-1)
+		flag = 0xe0000000 | DESC_RING_WRAP;
+
+	tp->tx_ring[entry].length = cpu_to_le32(skb->len | flag);
+	/* if we were using Transmit Automatic Polling, we would need a
+	 * wmb() here. */
+	tp->tx_ring[entry].status = cpu_to_le32(DescOwned);
+	wmb();
+
+	tp->cur_tx++;
+
+	/* Trigger an immediate transmit demand. */
+	iowrite32(0, tp->base_addr + CSR1);
+
+#ifndef CONFIG_PILOT_SG
+	spin_unlock_irq(&tp->lock);
+#endif
+	dev->trans_start = jiffies;
+
+	return 0;
+}
+
+#ifdef CONFIG_PILOT_SG
+static int
+tulip_sg_start_xmit(struct sk_buff *skb, struct net_device *dev)
+{
+        unsigned int nr_frags;
+        unsigned int f, len, offset = 0;
+        int tot_len;
+        struct tulip_private *tp = netdev_priv(dev);
+        //  dma_addr_t mapping;
+        void __iomem *ioaddr = tp->base_addr;
+        int entry, last_entry, first_entry, enable_intr = 0;
+
+        spin_lock_irq(&tp->lock);
+        if(!skb_is_nonlinear(skb)) {
+                tot_len = tulip_start_xmit(skb, dev);
+                spin_unlock_irq(&tp->lock);
+                return tot_len;
+        }
+
+        len = skb_headlen(skb);
+        first_entry = last_entry = entry = tp->cur_tx % TX_RING_SIZE;
+        tp->tx_buffers[first_entry].skb = skb;
+        tp->tx_buffers[first_entry].mapping = pci_map_single(tp->pdev, skb->data, len, PCI_DMA_TODEVICE);
+        tp->tx_ring[first_entry].buffer1 = cpu_to_le32(tp->tx_buffers[first_entry].mapping);
+	tp->tx_buffers[entry].maplen = len;
+
+        if((tp->cur_tx - tp->dirty_tx) > (TX_RING_SIZE - 10)) {
+                tp->tx_ring[last_entry].length = ((1 << 24) | len | (1 << 29) | (1 << 31));//Chain first desc, no EOR TX done intr
+                tp->p_tx_coalising_en=1;
+                iowrite32( (ioread32(ioaddr + CSR7)|TxIntr),(ioaddr + CSR7) );
+                netif_stop_queue(dev);
+                enable_intr = 1;
+        } else {
+                tp->tx_ring[last_entry].length = ((1 << 24) | len | (1 << 29));//Chain first desc, no EOR
+        }
+
+        tp->cur_tx++;
+        nr_frags = skb_shinfo(skb)->nr_frags;
+
+        for(f = 0; f < nr_frags;f++) {
+                const struct skb_frag_struct *frag;
+                int frag_len = 0;
+                frag = &skb_shinfo(skb)->frags[f];
+                frag_len = skb_frag_size(frag);
+                (void)skb_get(skb);
+
+                entry = tp->cur_tx % TX_RING_SIZE;
+                tp->tx_buffers[entry].skb = skb;
+                tp->tx_buffers[entry].mapping = pci_map_single(tp->pdev, skb_frag_address(frag), frag_len, PCI_DMA_TODEVICE);
+		tp->tx_buffers[entry].maplen = frag_len;
+                tp->tx_ring[entry].buffer1 = cpu_to_le32(tp->tx_buffers[entry].mapping);
+                offset = (u8*)(tp->tx_ring + entry) - (u8 *)tp->tx_ring;
+                tp->tx_ring[last_entry].buffer2 = tp->tx_ring_dma + offset;
+                tp->tx_ring[entry].length = ((1 << 24) | frag_len);//Chain no EOR
+                tp->tx_ring[entry].status = cpu_to_le32(DescOwned);
+                last_entry = entry;
+                tp->cur_tx++;
+        }
+
+        if(enable_intr)
+                tp->tx_ring[entry].length |= (1 << 31);
+
+	tp->tx_ring[entry].length &= ~(1 << 24);
+
+        if(entry == TX_RING_SIZE-1) {
+                tp->tx_ring[entry].length |= (DESC_RING_WRAP | (1 << 30));//last
+        } else {
+                tp->tx_ring[entry].length |= (1 << 30);//last desc
+        }
+
+        tp->tx_ring[first_entry].status = cpu_to_le32(DescOwned);
+        iowrite32(0, tp->base_addr + CSR1);
+#ifndef CONFIG_TULIP_NAPI  //Ashok 
+	tulip_process_rx(dev);  //DJ
+#endif  
+        dev->trans_start = jiffies;
+
+        spin_unlock_irq(&tp->lock);
+
+        return NETDEV_TX_OK;
+}
+#endif
+
+static void tulip_clean_tx_ring(struct tulip_private *tp)
+{
+	unsigned int dirty_tx;
+
+	for (dirty_tx = tp->dirty_tx ; tp->cur_tx - dirty_tx > 0;
+		dirty_tx++) {
+		int entry = dirty_tx % TX_RING_SIZE;
+		int status = le32_to_cpu(tp->tx_ring[entry].status);
+
+		if (status < 0) {
+			tp->stats.tx_errors++;	/* It wasn't Txed */
+			tp->tx_ring[entry].status = 0;
+		}
+
+		/* Check for Tx filter setup frames. */
+		if (tp->tx_buffers[entry].skb == NULL) {
+			/* test because dummy frames not mapped */
+			if (tp->tx_buffers[entry].mapping)
+				pci_unmap_single(tp->pdev,
+					tp->tx_buffers[entry].mapping,
+					sizeof(tp->setup_frame),
+					PCI_DMA_TODEVICE);
+			continue;
+		}
+
+		pci_unmap_single(tp->pdev, tp->tx_buffers[entry].mapping,
+				tp->tx_buffers[entry].skb->len,
+				PCI_DMA_TODEVICE);
+
+		/* Free the original skb. */
+		dev_kfree_skb_irq(tp->tx_buffers[entry].skb);
+		tp->tx_buffers[entry].skb = NULL;
+		tp->tx_buffers[entry].mapping = 0;
+	}
+}
+
+static void tulip_down (struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	void __iomem *ioaddr = tp->base_addr;
+	unsigned long flags;
+
+	cancel_work_sync(&tp->media_work);
+
+#ifdef CONFIG_TULIP_NAPI
+	napi_disable(&tp->napi);
+#endif
+
+	del_timer_sync (&tp->timer);
+#ifdef CONFIG_TULIP_NAPI
+	del_timer_sync (&tp->oom_timer);
+#endif
+	spin_lock_irqsave (&tp->lock, flags);
+
+	/* Disable interrupts by clearing the interrupt mask. */
+	iowrite32 (0x00000000, ioaddr + CSR7);
+
+	/* Stop the Tx and Rx processes. */
+	tulip_stop_rxtx(tp);
+
+	/* prepare receive buffers */
+	tulip_refill_rx(dev);
+
+	/* release any unconsumed transmit buffers */
+	tulip_clean_tx_ring(tp);
+
+	if (ioread32 (ioaddr + CSR6) != 0xffffffff)
+		tp->stats.rx_missed_errors += ioread32 (ioaddr + CSR8) & 0xffff;
+
+	spin_unlock_irqrestore (&tp->lock, flags);
+
+	init_timer(&tp->timer);
+	tp->timer.data = (unsigned long)dev;
+	tp->timer.function = tulip_tbl[tp->chip_id].media_timer;
+
+	dev->if_port = tp->saved_if_port;
+
+	/* Leave the driver in snooze, not sleep, mode. */
+	tulip_set_power_state (tp, 0, 1);
+}
+
+
+static int tulip_close (struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	void __iomem *ioaddr = tp->base_addr;
+	int i;
+
+	netif_stop_queue (dev);
+
+	tulip_down (dev);
+
+	if (tulip_debug > 1)
+		printk (KERN_DEBUG "%s: Shutting down ethercard, status was %2.2x.\n",
+			dev->name, ioread32 (ioaddr + CSR5));
+
+	free_irq (dev->irq, dev);
+
+	/* Free all the skbuffs in the Rx queue. */
+	for (i = 0; i < RX_RING_SIZE; i++) {
+		struct sk_buff *skb = tp->rx_buffers[i].skb;
+		dma_addr_t mapping = tp->rx_buffers[i].mapping;
+
+		tp->rx_buffers[i].skb = NULL;
+		tp->rx_buffers[i].mapping = 0;
+
+		tp->rx_ring[i].status = 0;	/* Not owned by Tulip chip. */
+		tp->rx_ring[i].length = 0;
+		/* An invalid address. */
+		tp->rx_ring[i].buffer1 = cpu_to_le32(0xBADF00D0);
+		if (skb) {
+			pci_unmap_single(tp->pdev, mapping, PKT_BUF_SZ,
+					 PCI_DMA_FROMDEVICE);
+			dev_kfree_skb (skb);
+		}
+	}
+	for (i = 0; i < TX_RING_SIZE; i++) {
+		struct sk_buff *skb = tp->tx_buffers[i].skb;
+
+		if (skb != NULL) {
+			pci_unmap_single(tp->pdev, tp->tx_buffers[i].mapping,
+					 skb->len, PCI_DMA_TODEVICE);
+			dev_kfree_skb (skb);
+		}
+		tp->tx_buffers[i].skb = NULL;
+		tp->tx_buffers[i].mapping = 0;
+	}
+
+	return 0;
+}
+
+static struct net_device_stats *tulip_get_stats(struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	void __iomem *ioaddr = tp->base_addr;
+
+	if (netif_running(dev)) {
+		unsigned long flags;
+
+		spin_lock_irqsave (&tp->lock, flags);
+
+		tp->stats.rx_missed_errors += ioread32(ioaddr + CSR8) & 0xffff;
+
+		spin_unlock_irqrestore(&tp->lock, flags);
+	}
+
+	return &tp->stats;
+}
+
+
+static void tulip_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
+{
+	struct tulip_private *np = netdev_priv(dev);
+	
+	ethtool_override_get_drvinfo(np->phy_index,dev,info);	
+	strcpy(info->driver, DRV_NAME);
+	strcpy(info->version, DRV_VERSION);
+	strcpy(info->fw_version, "N/A");
+//	strcpy(info->bus_info, pci_name(np->pdev));
+	return;
+}
+
+static int tulip_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
+{
+        struct tulip_private *nic = netdev_priv(netdev);
+		
+		ethtool_override_get_settings(nic->phy_index,netdev,cmd);
+        return mii_ethtool_gset(&nic->mii, cmd);
+}
+
+static int tulip_set_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
+{
+        struct tulip_private *nic = netdev_priv(netdev);
+        int err;
+
+	    ethtool_override_set_settings(nic->phy_index,netdev,cmd);
+        tulip_mdio_write(netdev, nic->mii.phy_id, MII_BMCR, BMCR_RESET);
+        err = mii_ethtool_sset(&nic->mii, cmd);
+        return err;
+}
+
+static int tulip_nway_reset(struct net_device *netdev)
+{
+        struct tulip_private *nic = netdev_priv(netdev);
+		
+		ethtool_override_nway_reset(nic->phy_index,netdev);
+        return mii_nway_restart(&nic->mii);
+}
+
+static u32 tulip_get_link(struct net_device *netdev)
+{
+        struct tulip_private *nic = netdev_priv(netdev);
+		
+		ethtool_override_get_link(nic->phy_index,netdev);
+        return mii_link_ok(&nic->mii);
+}
+
+
+static const struct ethtool_ops ops = {
+	.get_drvinfo = tulip_get_drvinfo,
+	.get_link	 = tulip_get_link,
+	.nway_reset	 = tulip_nway_reset,
+	.get_settings = tulip_get_settings,
+	.set_settings = tulip_set_settings,
+};
+
+
+#if 1
+static int private_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
+{
+        struct tulip_private *tp = netdev_priv(netdev);
+
+#ifndef CONFIG_PILOT_AUTO_PHY
+#ifndef CONFIG_PILOT_MAC0_PHY_PRESENT
+	if (tp->phy_index == 0)
+		return -ENODEV;
+#endif
+#ifndef CONFIG_PILOT_MAC1_PHY_PRESENT
+	if (tp->phy_index == 1)
+		return -ENODEV;
+#endif
+#endif
+        return generic_mii_ioctl(&tp->mii, if_mii(ifr), cmd, NULL);
+}
+
+#else
+/* Provide ioctl() calls to examine the MII xcvr state. */
+static int private_ioctl (struct net_device *dev, struct ifreq *rq, int cmd)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	void __iomem *ioaddr = tp->base_addr;
+	struct mii_ioctl_data *data = if_mii(rq);
+	const unsigned int phy_idx = 0;
+	int phy = tp->phys[tp->phy_index] & 0x1f;
+	unsigned int regnum = data->reg_num;
+
+#ifndef CONFIG_PILOT_AUTO_PHY
+#ifndef CONFIG_PILOT_MAC0_PHY_PRESENT
+	if (tp->phy_index == 0)
+		return -ENODEV;
+#endif
+#ifndef CONFIG_PILOT_MAC1_PHY_PRESENT
+	if (tp->phy_index == 1)
+		return -ENODEV;
+#endif
+#endif
+
+	switch (cmd) {
+	case SIOCGMIIPHY:		/* Get address of MII PHY in use. */
+		if (tp->mii_cnt)
+			data->phy_id = phy;
+		else if (tp->flags & HAS_NWAY)
+			data->phy_id = 32;
+#if 0
+		else if (tp->chip_id == COMET)
+			data->phy_id = 1;
+#endif
+		else
+			return -ENODEV;
+
+	case SIOCGMIIREG:		/* Read MII PHY register. */
+		if (data->phy_id == 32 && (tp->flags & HAS_NWAY)) {
+			int csr12 = ioread32 (ioaddr + CSR12);
+			int csr14 = ioread32 (ioaddr + CSR14);
+			switch (regnum) {
+			case 0:
+                                if (((csr14<<5) & 0x1000) ||
+                                        (dev->if_port == 5 && tp->nwayset))
+                                        data->val_out = 0x1000;
+                                else
+                                        data->val_out = (tulip_media_cap[dev->if_port]&MediaIs100 ? 0x2000 : 0)
+                                                | (tulip_media_cap[dev->if_port]&MediaIsFD ? 0x0100 : 0);
+				break;
+			case 1:
+                                data->val_out =
+					0x1848 +
+					((csr12&0x7000) == 0x5000 ? 0x20 : 0) +
+					((csr12&0x06) == 6 ? 0 : 4);
+                                data->val_out |= 0x6048;
+				break;
+			case 4:
+                                /* Advertised value, bogus 10baseTx-FD value from CSR6. */
+                                data->val_out =
+					((ioread32(ioaddr + CSR6) >> 3) & 0x0040) +
+					((csr14 >> 1) & 0x20) + 1;
+                                data->val_out |= ((csr14 >> 9) & 0x03C0);
+				break;
+			case 5: data->val_out = tp->lpar; break;
+			default: data->val_out = 0; break;
+			}
+		} else {
+			data->val_out = tulip_mdio_read (dev, data->phy_id & 0x1f, regnum);
+		}
+		return 0;
+
+	case SIOCSMIIREG:		/* Write MII PHY register. */
+		if (!capable (CAP_NET_ADMIN))
+			return -EPERM;
+		if (regnum & ~0x1f)
+			return -EINVAL;
+		if (data->phy_id == phy) {
+			u16 value = data->val_in;
+			switch (regnum) {
+			case 0:	/* Check for autonegotiation on or reset. */
+				tp->full_duplex_lock = (value & 0x9000) ? 0 : 1;
+				if (tp->full_duplex_lock)
+					tp->full_duplex = (value & 0x0100) ? 1 : 0;
+				break;
+			case 4:
+				tp->advertising[phy_idx] =
+				tp->mii_advertise = data->val_in;
+				break;
+			}
+		}
+		if (data->phy_id == 32 && (tp->flags & HAS_NWAY)) {
+			u16 value = data->val_in;
+			if (regnum == 0) {
+			  if ((value & 0x1200) == 0x1200) {
+#if 0
+			    if (tp->chip_id == PNIC2) {
+                                   pnic2_start_nway (dev);
+                            } else {
+				   t21142_start_nway (dev);
+                            }
+#endif
+			  }
+			} else if (regnum == 4)
+				tp->sym_advertise = value;
+		} else {
+			tulip_mdio_write (dev, data->phy_id & 0x1f, regnum, data->val_in);
+		}
+		return 0;
+	default:
+		return -EOPNOTSUPP;
+	}
+
+	return -EOPNOTSUPP;
+}
+
+#endif
+
+
+/* Set or clear the multicast filter for this adaptor.
+   Note that we only use exclusion around actually queueing the
+   new frame, not around filling tp->setup_frame.  This is non-deterministic
+   when re-entered but still correct. */
+
+#undef set_bit_le
+#define set_bit_le(i,p) do { ((char *)(p))[(i)/8] |= (1<<((i)%8)); } while(0)
+
+static void build_setup_frame_hash(u16 *setup_frm, struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	u16 hash_table[32];
+	struct netdev_hw_addr *ha;
+	int i = 0;
+	u16 *eaddrs;
+
+	memset(hash_table, 0, sizeof(hash_table));
+	set_bit_le(255, hash_table); 			/* Broadcast entry */
+	/* This should work on big-endian machines as well. */
+	netdev_for_each_mc_addr(ha, dev) {
+		int index = ether_crc_le(ETH_ALEN, ha->addr) & 0x1ff;
+
+		set_bit_le(index, hash_table);
+	}
+	for (i = 0; i < 32; i++) {
+		*setup_frm++ = hash_table[i];
+		*setup_frm++ = hash_table[i];
+	}
+	setup_frm = &tp->setup_frame[13*6];
+
+	/* Fill the final entry with our physical address. */
+	eaddrs = (u16 *)dev->dev_addr;
+	*setup_frm++ = eaddrs[0]; *setup_frm++ = eaddrs[0];
+	*setup_frm++ = eaddrs[1]; *setup_frm++ = eaddrs[1];
+	*setup_frm++ = eaddrs[2]; *setup_frm++ = eaddrs[2];
+}
+
+static void build_setup_frame_perfect(u16 *setup_frm, struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	struct netdev_hw_addr *ha;
+	u16 *eaddrs;
+
+	/* We have <= 14 addresses so we can use the wonderful
+	   16 address perfect filtering of the Tulip. */
+	netdev_for_each_mc_addr(ha, dev) {
+		eaddrs = (u16 *) ha->addr;
+		*setup_frm++ = *eaddrs; *setup_frm++ = *eaddrs++;
+		*setup_frm++ = *eaddrs; *setup_frm++ = *eaddrs++;
+		*setup_frm++ = *eaddrs; *setup_frm++ = *eaddrs++;
+	}
+	/* Fill the unused entries with the broadcast address. */
+	memset(setup_frm, 0xff, (15 - netdev_mc_count(dev)) * 12);
+	setup_frm = &tp->setup_frame[15*6];
+
+	/* Fill the final entry with our physical address. */
+	eaddrs = (u16 *)dev->dev_addr;
+	*setup_frm++ = eaddrs[0]; *setup_frm++ = eaddrs[0];
+	*setup_frm++ = eaddrs[1]; *setup_frm++ = eaddrs[1];
+	*setup_frm++ = eaddrs[2]; *setup_frm++ = eaddrs[2];
+}
+
+
+static void set_rx_mode(struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	void __iomem *ioaddr = tp->base_addr;
+	int csr6;
+
+	csr6 = ioread32(ioaddr + CSR6) & ~0x00D5;
+
+	tp->csr6 &= ~0x00D5;
+	if (dev->flags & IFF_PROMISC) {			/* Set promiscuous. */
+		tp->csr6 |= AcceptAllMulticast | AcceptAllPhys;
+		csr6 |= AcceptAllMulticast | AcceptAllPhys;
+	} else if ((netdev_mc_count(dev) > 1000)  ||  (dev->flags & IFF_ALLMULTI)) {
+		/* Too many to filter well -- accept all multicasts. */
+		tp->csr6 |= AcceptAllMulticast;
+		csr6 |= AcceptAllMulticast;
+	} else	if (tp->flags & MC_HASH_ONLY) {
+		/* Some work-alikes have only a 64-entry hash filter table. */
+		/* Should verify correctness on big-endian/__powerpc__ */
+		struct netdev_hw_addr *ha;
+		if (netdev_mc_count(dev) > 64) {		/* Arbitrary non-effective limit. */
+			tp->csr6 |= AcceptAllMulticast;
+			csr6 |= AcceptAllMulticast;
+		} else {
+			u32 mc_filter[2] = {0, 0};		 /* Multicast hash filter */
+			int filterbit;
+			netdev_for_each_mc_addr(ha, dev) {
+				if (tp->flags & COMET_MAC_ADDR)
+					filterbit = ether_crc_le(ETH_ALEN,
+								 ha->addr);
+				else
+					filterbit = ether_crc(ETH_ALEN,
+							      ha->addr) >> 26;
+				filterbit &= 0x3f;
+				mc_filter[filterbit >> 5] |= 1 << (filterbit & 31);
+				if (tulip_debug > 2)
+					dev_info(&dev->dev,
+						 "Added filter for %pM  %08x bit %d\n",
+						 ha->addr,
+						 ether_crc(ETH_ALEN, ha->addr),
+						 filterbit);
+			}
+			if (mc_filter[0] == tp->mc_filter[0]  &&
+				mc_filter[1] == tp->mc_filter[1])
+				;				/* No change. */
+			else if (tp->flags & IS_ASIX) {
+				iowrite32(2, ioaddr + CSR13);
+				iowrite32(mc_filter[0], ioaddr + CSR14);
+				iowrite32(3, ioaddr + CSR13);
+				iowrite32(mc_filter[1], ioaddr + CSR14);
+			} else if (tp->flags & COMET_MAC_ADDR) {
+				iowrite32(mc_filter[0], ioaddr + 0xAC);
+				iowrite32(mc_filter[1], ioaddr + 0xB0);
+			}
+			tp->mc_filter[0] = mc_filter[0];
+			tp->mc_filter[1] = mc_filter[1];
+		}
+	} else {
+		unsigned long flags;
+		u32 tx_flags = 0x08000000 | 192;
+
+		/* Note that only the low-address shortword of setup_frame is valid!
+		   The values are doubled for big-endian architectures. */
+		if (netdev_mc_count(dev) > 14) { /* Must use a multicast hash table. */
+			build_setup_frame_hash(tp->setup_frame, dev);
+			tx_flags = 0x08400000 | 192;
+		} else {
+			build_setup_frame_perfect(tp->setup_frame, dev);
+		}
+
+		spin_lock_irqsave(&tp->lock, flags);
+
+		if (tp->cur_tx - tp->dirty_tx > TX_RING_SIZE - 2) {
+			/* Same setup recently queued, we need not add it. */
+		} else {
+			unsigned int entry;
+			int dummy = -1;
+
+			/* Now add this frame to the Tx list. */
+
+			entry = tp->cur_tx++ % TX_RING_SIZE;
+
+			if (entry != 0) {
+				/* Avoid a chip errata by prefixing a dummy entry. */
+				tp->tx_buffers[entry].skb = NULL;
+				tp->tx_buffers[entry].mapping = 0;
+				tp->tx_ring[entry].length =
+					(entry == TX_RING_SIZE-1) ? cpu_to_le32(DESC_RING_WRAP) : 0;
+				tp->tx_ring[entry].buffer1 = 0;
+				/* Must set DescOwned later to avoid race with chip */
+				dummy = entry;
+				entry = tp->cur_tx++ % TX_RING_SIZE;
+
+			}
+
+			tp->tx_buffers[entry].skb = NULL;
+			tp->tx_buffers[entry].mapping =
+				pci_map_single(tp->pdev, tp->setup_frame,
+					       sizeof(tp->setup_frame),
+					       PCI_DMA_TODEVICE);
+			/* Put the setup frame on the Tx list. */
+			if (entry == TX_RING_SIZE-1)
+				tx_flags |= DESC_RING_WRAP;		/* Wrap ring. */
+			tp->tx_ring[entry].length = cpu_to_le32(tx_flags);
+			tp->tx_ring[entry].buffer1 =
+				cpu_to_le32(tp->tx_buffers[entry].mapping);
+			tp->tx_ring[entry].status = cpu_to_le32(DescOwned);
+			if (dummy >= 0)
+				tp->tx_ring[dummy].status = cpu_to_le32(DescOwned);
+			if (tp->cur_tx - tp->dirty_tx >= TX_RING_SIZE - 2)
+				netif_stop_queue(dev);
+
+			/* Trigger an immediate transmit demand. */
+			iowrite32(0, ioaddr + CSR1);
+		}
+
+		spin_unlock_irqrestore(&tp->lock, flags);
+	}
+
+	iowrite32(csr6, ioaddr + CSR6);
+}
+
+#ifdef CONFIG_TULIP_MWI
+static void __devinit tulip_mwi_config (struct pci_dev *pdev,
+					struct net_device *dev)
+{
+	struct tulip_private *tp = netdev_priv(dev);
+	u8 cache;
+	u16 pci_command;
+	u32 csr0;
+
+	if (tulip_debug > 3)
+		printk(KERN_DEBUG "%s: tulip_mwi_config()\n", pci_name(pdev));
+
+	tp->csr0 = csr0 = 0;
+
+	/* if we have any cache line size at all, we can do MRM and MWI */
+	csr0 |= MRM | MWI;
+
+	/* Enable MWI in the standard PCI command bit.
+	 * Check for the case where MWI is desired but not available
+	 */
+	pci_try_set_mwi(pdev);
+
+	/* read result from hardware (in case bit refused to enable) */
+	pci_read_config_word(pdev, PCI_COMMAND, &pci_command);
+	if ((csr0 & MWI) && (!(pci_command & PCI_COMMAND_INVALIDATE)))
+		csr0 &= ~MWI;
+
+	/* if cache line size hardwired to zero, no MWI */
+	pci_read_config_byte(pdev, PCI_CACHE_LINE_SIZE, &cache);
+	if ((csr0 & MWI) && (cache == 0)) {
+		csr0 &= ~MWI;
+		pci_clear_mwi(pdev);
+	}
+
+	/* assign per-cacheline-size cache alignment and
+	 * burst length values
+	 */
+	switch (cache) {
+	case 8:
+		csr0 |= MRL | (1 << CALShift) | (16 << BurstLenShift);
+		break;
+	case 16:
+		csr0 |= MRL | (2 << CALShift) | (16 << BurstLenShift);
+		break;
+	case 32:
+		csr0 |= MRL | (3 << CALShift) | (32 << BurstLenShift);
+		break;
+	default:
+		cache = 0;
+		break;
+	}
+
+	/* if we have a good cache line size, we by now have a good
+	 * csr0, so save it and exit
+	 */
+	if (cache)
+		goto out;
+
+	/* we don't have a good csr0 or cache line size, disable MWI */
+	if (csr0 & MWI) {
+		pci_clear_mwi(pdev);
+		csr0 &= ~MWI;
+	}
+
+	/* sane defaults for burst length and cache alignment
+	 * originally from de4x5 driver
+	 */
+	csr0 |= (8 << BurstLenShift) | (1 << CALShift);
+
+out:
+	tp->csr0 = csr0;
+	if (tulip_debug > 2)
+		printk(KERN_DEBUG "%s: MWI config cacheline=%d, csr0=%08x\n",
+		       pci_name(pdev), cache, csr0);
+}
+#endif
+
+/*
+ *	Chips that have the MRM/reserved bit quirk and the burst quirk. That
+ *	is the DM910X and the on chip ULi devices
+ */
+ #if 0
+static int tulip_uli_dm_quirk(struct pci_dev *pdev)
+{
+	if (pdev->vendor == 0x1282 && pdev->device == 0x9102)
+		return 1;
+	return 0;
+}
+#endif
+
+static const struct net_device_ops tulip_pilot3_netdev_ops = {
+	.ndo_open		= tulip_open,
+#ifdef CONFIG_PILOT_SG
+        .ndo_start_xmit         = tulip_sg_start_xmit,
+#else
+	.ndo_start_xmit		= tulip_start_xmit,
+#endif
+	.ndo_tx_timeout		= tulip_tx_timeout,
+	.ndo_stop		= tulip_close,
+	.ndo_get_stats		= tulip_get_stats,
+	.ndo_do_ioctl 		= private_ioctl,
+	.ndo_set_rx_mode	= set_rx_mode,
+	.ndo_change_mtu		= eth_change_mtu,
+	.ndo_set_mac_address	= eth_mac_addr,
+	.ndo_validate_addr	= eth_validate_addr,
+#ifdef CONFIG_NET_POLL_CONTROLLER
+	.ndo_poll_controller	 = poll_tulip,
+#endif
+};
+
+
+//static int __devinit tulip_init_one (struct pci_dev *pdev,
+//				     const struct pci_device_id *ent)  amrut
+static int tulip_init_one (int interface,unsigned char *ethaddr)
+{
+	struct tulip_private *tp;
+	/* See note below on the multiport cards. */
+#if 0
+	static unsigned char last_phys_addr[6] = {0x00, 'L', 'i', 'n', 'u', 'x'};
+	static struct pci_device_id early_486_chipsets[] = {
+		{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82424) },
+		{ PCI_DEVICE(PCI_VENDOR_ID_SI, PCI_DEVICE_ID_SI_496) },
+		{ },
+	};
+#endif
+	static int last_irq;
+//	static int multiport_cnt;	/* For four-port boards w/one EEPROM */
+//	u8 chip_rev;
+	u8 chip_rev = 0;
+	int i, irq;
+	unsigned short sum;
+	unsigned char *ee_data;
+	struct net_device *dev;
+	void __iomem *ioaddr;
+	static int board_idx = -1;
+//	int chip_idx = ent->driver_data;
+	int chip_idx = PILOT_TULIP;
+	const char *chip_name = tulip_tbl[chip_idx].chip_name;
+	unsigned int eeprom_missing = 0;
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_SHARED_MDIO_MDC_LINES
+	static int miilock_init = 0;
+#endif
+	//unsigned int force_csr0 = 0;
+//	DECLARE_MAC_BUF(mac);
+
+#ifndef MODULE
+	static int did_version;		/* Already printed version info. */
+	if (tulip_debug > 0  &&  did_version++ == 0)
+		printk (KERN_INFO "%s", version);
+#endif
+
+	board_idx++;
+
+	/*
+	 *	Lan media wire a tulip chip to a wan interface. Needs a very
+	 *	different driver (lmc driver)
+	 */
+#if 0
+        if (pdev->subsystem_vendor == PCI_VENDOR_ID_LMC) {
+		printk (KERN_ERR PFX "skipping LMC card.\n");
+		return -ENODEV;
+	}
+
+
+	/*
+	 *	Early DM9100's need software CRC and the DMFE driver
+	 */
+
+	if (pdev->vendor == 0x1282 && pdev->device == 0x9100)
+	{
+		/* Read Chip revision */
+		if (pdev->revision < 0x30)
+		{
+			printk(KERN_ERR PFX "skipping early DM9100 with Crc bug (use dmfe)\n");
+			return -ENODEV;
+		}
+	}
+
+	/*
+	 *	Looks for early PCI chipsets where people report hangs
+	 *	without the workarounds being on.
+	 */
+
+	/* 1. Intel Saturn. Switch to 8 long words burst, 8 long word cache
+	      aligned.  Aries might need this too. The Saturn errata are not
+	      pretty reading but thankfully it's an old 486 chipset.
+
+	   2. The dreaded SiS496 486 chipset. Same workaround as Intel
+	      Saturn.
+	*/
+
+	if (pci_dev_present(early_486_chipsets)) {
+		csr0 = MRL | MRM | (8 << BurstLenShift) | (1 << CALShift);
+		force_csr0 = 1;
+	}
+
+	/* bugfix: the ASIX must have a burst limit or horrible things happen. */
+	if (chip_idx == AX88140) {
+		if ((csr0 & 0x3f00) == 0)
+			csr0 |= 0x2000;
+	}
+
+	/* PNIC doesn't have MWI/MRL/MRM... */
+	if (chip_idx == LC82C168)
+		csr0 &= ~0xfff10000; /* zero reserved bits 31:20, 16 */
+
+	/* DM9102A has troubles with MRM & clear reserved bits 24:22, 20, 16, 7:1 */
+	if (tulip_uli_dm_quirk(pdev)) {
+		csr0 &= ~0x01f100ff;
+#if defined(CONFIG_SPARC)
+                csr0 = (csr0 & ~0xff00) | 0xe000;
+#endif
+	}
+	/*
+	 *	And back to business
+	 */
+
+	i = pci_enable_device(pdev);
+	if (i) {
+		printk (KERN_ERR PFX
+			"Cannot enable tulip board #%d, aborting\n",
+			board_idx);
+		return i;
+	}
+#endif
+
+//	irq = pdev->irq;
+	irq = SE_INTERFACE_ETH_A_IRQ;
+    	if(interface == INTERFACE_ETH_B)
+    		irq = SE_INTERFACE_ETH_B_IRQ;
+
+	/* alloc_etherdev ensures aligned and zeroed private structures */
+	dev = alloc_etherdev (sizeof (*tp));
+	if (!dev) {
+		printk (KERN_ERR PFX "ether device alloc failed, aborting\n");
+		return -ENOMEM;
+	}
+
+#if 0
+	SET_NETDEV_DEV(dev, &pdev->dev);
+	if (pci_resource_len (pdev, 0) < tulip_tbl[chip_idx].io_size) {
+		printk (KERN_ERR PFX "%s: I/O region (0x%lx@0x%lx) too small, "
+			"aborting\n", pci_name(pdev),
+			pci_resource_len (pdev, 0),
+			pci_resource_start (pdev, 0));
+		goto err_out_free_netdev;
+	}
+
+	/* grab all resources from both PIO and MMIO regions, as we
+	 * don't want anyone else messing around with our hardware */
+	if (pci_request_regions (pdev, "tulip"))
+		goto err_out_free_netdev;
+#endif
+
+
+//	ioaddr =  pci_iomap(pdev, TULIP_BAR, tulip_tbl[chip_idx].io_size);
+	ioaddr = (void *)IOADDR_INTERFACE_ETH_A;
+    	if(interface == INTERFACE_ETH_B)
+        	ioaddr = (void *)IOADDR_INTERFACE_ETH_B;
+		
+	if (!ioaddr)
+		goto err_out_free_res;
+	if(request_region((unsigned long)ioaddr, tulip_tbl[chip_idx].io_size, "pilot3_tulip"))
+	{
+		goto err_out_free_netdev;
+    	}
+
+	/*
+	 * initialize private data structure 'tp'
+	 * it is zeroed and aligned in alloc_etherdev
+	 */
+	tp = netdev_priv(dev);
+	tp->dev = dev;
+
+
+//	tp->rx_ring = pci_alloc_consistent(pdev,
+//					   sizeof(struct tulip_rx_desc) * RX_RING_SIZE +
+//					   sizeof(struct tulip_tx_desc) * TX_RING_SIZE,
+//					   &tp->rx_ring_dma);
+    	tp->rx_ring = dma_alloc_coherent(NULL, 
+                     sizeof(struct tulip_rx_desc) * RX_RING_SIZE +
+                     sizeof(struct tulip_tx_desc) * TX_RING_SIZE,
+                     &tp->rx_ring_dma, GFP_ATOMIC);
+	if (!tp->rx_ring)
+		goto err_out_mtable;
+	tp->tx_ring = (struct tulip_tx_desc *)(tp->rx_ring + RX_RING_SIZE);
+	tp->tx_ring_dma = tp->rx_ring_dma + sizeof(struct tulip_rx_desc) * RX_RING_SIZE;
+
+	tp->chip_id = chip_idx;
+	tp->flags = tulip_tbl[chip_idx].flags;
+//	tp->pdev = pdev;
+	tp->pdev = NULL;
+	tp->base_addr = ioaddr;
+//	tp->revision = chip_rev;
+//	tp->csr0 = csr0 ;
+	tp->csr0 = (0x20<<8);
+	spin_lock_init(&tp->lock);
+	spin_lock_init(&tp->mii_lock);
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_SHARED_MDIO_MDC_LINES
+	/* Use a global MII lock for reading/writing PHY MDIO/MDC, 
+	   because in Orion both PHYs (A and B) share the same MDIO serial line 
+	*/
+        if (miilock_init == 0)
+        {
+                spin_lock_init(&miilock);
+                miilock_init = 1;
+        }
+#endif
+	init_timer(&tp->timer);
+	tp->timer.data = (unsigned long)dev;
+	tp->timer.function = tulip_tbl[tp->chip_id].media_timer;
+
+	INIT_WORK(&tp->media_work, tulip_tbl[tp->chip_id].media_task);
+
+	dev->base_addr = (unsigned long)ioaddr;
+#if 0
+#ifdef CONFIG_TULIP_MWI
+	if (!force_csr0 && (tp->flags & HAS_PCI_MWI))
+		tulip_mwi_config (pdev, dev);
+#endif
+#endif
+
+	/* Stop the chip's Tx and Rx processes. */
+	tulip_stop_rxtx(tp);
+#if 0
+	pci_set_master(pdev);
+
+#ifdef CONFIG_GSC
+	if (pdev->subsystem_vendor == PCI_VENDOR_ID_HP) {
+		switch (pdev->subsystem_device) {
+		default:
+			break;
+		case 0x1061:
+		case 0x1062:
+		case 0x1063:
+		case 0x1098:
+		case 0x1099:
+		case 0x10EE:
+			tp->flags |= HAS_SWAPPED_SEEPROM | NEEDS_FAKE_MEDIA_TABLE;
+			chip_name = "GSC DS21140 Tulip";
+		}
+	}
+#endif
+#endif
+
+	/* Clear the missed-packet counter. */
+	ioread32(ioaddr + CSR8);
+
+	/* The station address ROM is read byte serially.  The register must
+	   be polled, waiting for the value to be read bit serially from the
+	   EEPROM.
+	   */
+	ee_data = tp->eeprom;
+	memset(ee_data, 0, sizeof(tp->eeprom));
+	sum = 0;
+#if 0	
+	if (chip_idx == LC82C168) {
+		for (i = 0; i < 3; i++) {
+			int value, boguscnt = 100000;
+			iowrite32(0x600 | i, ioaddr + 0x98);
+			do
+				value = ioread32(ioaddr + CSR9);
+			while (value < 0  && --boguscnt > 0);
+			put_unaligned_le16(value, ((__le16 *)dev->dev_addr) + i);
+			sum += value & 0xffff;
+		}
+	} else if (chip_idx == COMET) {
+		/* No need to read the EEPROM. */
+		put_unaligned_le32(ioread32(ioaddr + 0xA4), dev->dev_addr);
+		put_unaligned_le16(ioread32(ioaddr + 0xA8), dev->dev_addr + 4);
+		for (i = 0; i < 6; i ++)
+			sum += dev->dev_addr[i];
+	} else {
+		/* A serial EEPROM interface, we read now and sort it out later. */
+		int sa_offset = 0;
+		int ee_addr_size = tulip_read_eeprom(dev, 0xff, 8) & 0x40000 ? 8 : 6;
+		int ee_max_addr = ((1 << ee_addr_size) - 1) * sizeof(u16);
+
+		if (ee_max_addr > sizeof(tp->eeprom))
+			ee_max_addr = sizeof(tp->eeprom);
+
+		for (i = 0; i < ee_max_addr ; i += sizeof(u16)) {
+			u16 data = tulip_read_eeprom(dev, i/2, ee_addr_size);
+			ee_data[i] = data & 0xff;
+			ee_data[i + 1] = data >> 8;
+		}
+
+		/* DEC now has a specification (see Notes) but early board makers
+		   just put the address in the first EEPROM locations. */
+		/* This does  memcmp(ee_data, ee_data+16, 8) */
+		for (i = 0; i < 8; i ++)
+			if (ee_data[i] != ee_data[16+i])
+				sa_offset = 20;
+		if (chip_idx == CONEXANT) {
+			/* Check that the tuple type and length is correct. */
+			if (ee_data[0x198] == 0x04  &&  ee_data[0x199] == 6)
+				sa_offset = 0x19A;
+		} else if (ee_data[0] == 0xff  &&  ee_data[1] == 0xff &&
+				   ee_data[2] == 0) {
+			sa_offset = 2;		/* Grrr, damn Matrox boards. */
+			multiport_cnt = 4;
+		}
+#ifdef CONFIG_MIPS_COBALT
+               if ((pdev->bus->number == 0) &&
+                   ((PCI_SLOT(pdev->devfn) == 7) ||
+                    (PCI_SLOT(pdev->devfn) == 12))) {
+                       /* Cobalt MAC address in first EEPROM locations. */
+                       sa_offset = 0;
+		       /* Ensure our media table fixup get's applied */
+		       memcpy(ee_data + 16, ee_data, 8);
+               }
+#endif
+#ifdef CONFIG_GSC
+		/* Check to see if we have a broken srom */
+		if (ee_data[0] == 0x61 && ee_data[1] == 0x10) {
+			/* pci_vendor_id and subsystem_id are swapped */
+			ee_data[0] = ee_data[2];
+			ee_data[1] = ee_data[3];
+			ee_data[2] = 0x61;
+			ee_data[3] = 0x10;
+
+			/* HSC-PCI boards need to be byte-swaped and shifted
+			 * up 1 word.  This shift needs to happen at the end
+			 * of the MAC first because of the 2 byte overlap.
+			 */
+			for (i = 4; i >= 0; i -= 2) {
+				ee_data[17 + i + 3] = ee_data[17 + i];
+				ee_data[16 + i + 5] = ee_data[16 + i];
+			}
+		}
+#endif
+
+		for (i = 0; i < 6; i ++) {
+			dev->dev_addr[i] = ee_data[i + sa_offset];
+			sum += ee_data[i + sa_offset];
+		}
+	}
+	/* Lite-On boards have the address byte-swapped. */
+	if ((dev->dev_addr[0] == 0xA0  ||  dev->dev_addr[0] == 0xC0 || dev->dev_addr[0] == 0x02)
+		&&  dev->dev_addr[1] == 0x00)
+		for (i = 0; i < 6; i+=2) {
+			char tmp = dev->dev_addr[i];
+			dev->dev_addr[i] = dev->dev_addr[i+1];
+			dev->dev_addr[i+1] = tmp;
+		}
+	/* On the Zynx 315 Etherarray and other multiport boards only the
+	   first Tulip has an EEPROM.
+	   On Sparc systems the mac address is held in the OBP property
+	   "local-mac-address".
+	   The addresses of the subsequent ports are derived from the first.
+	   Many PCI BIOSes also incorrectly report the IRQ line, so we correct
+	   that here as well. */
+	if (sum == 0  || sum == 6*0xff) {
+#if defined(CONFIG_SPARC)
+		struct device_node *dp = pci_device_to_OF_node(pdev);
+		const unsigned char *addr;
+		int len;
+#endif
+		eeprom_missing = 1;
+		for (i = 0; i < 5; i++)
+			dev->dev_addr[i] = last_phys_addr[i];
+		dev->dev_addr[i] = last_phys_addr[i] + 1;
+#if defined(CONFIG_SPARC)
+		addr = of_get_property(dp, "local-mac-address", &len);
+		if (addr && len == 6)
+			memcpy(dev->dev_addr, addr, 6);
+#endif
+#if defined(__i386__) || defined(__x86_64__)	/* Patch up x86 BIOS bug. */
+		if (last_irq)
+			irq = last_irq;
+#endif
+	}
+
+	for (i = 0; i < 6; i++)
+		last_phys_addr[i] = dev->dev_addr[i];
+#endif
+
+	eeprom_missing = 1;
+	for (i = 0; i <=5 ; i++)
+		dev->dev_addr[i] = ethaddr[i];
+
+	last_irq = irq;
+	dev->irq = irq;
+
+	/* The lower four bits are the media type. */
+	if (board_idx >= 0  &&  board_idx < MAX_UNITS) {
+		if (options[board_idx] & MEDIA_MASK)
+			tp->default_port = options[board_idx] & MEDIA_MASK;
+		if ((options[board_idx] & FullDuplex) || full_duplex[board_idx] > 0)
+			tp->full_duplex = 1;
+		if (mtu[board_idx] > 0)
+			dev->mtu = mtu[board_idx];
+	}
+	if (dev->mem_start & MEDIA_MASK)
+		tp->default_port = dev->mem_start & MEDIA_MASK;
+	if (tp->default_port) {
+		printk(KERN_INFO "tulip%d: Transceiver selection forced to %s.\n",
+		       board_idx, medianame[tp->default_port & MEDIA_MASK]);
+		tp->medialock = 1;
+		if (tulip_media_cap[tp->default_port] & MediaAlwaysFD)
+			tp->full_duplex = 1;
+	}
+	if (tp->full_duplex)
+		tp->full_duplex_lock = 1;
+
+#if 0
+	if (tulip_media_cap[tp->default_port] & MediaIsMII) {
+		u16 media2advert[] = { 0x20, 0x40, 0x03e0, 0x60, 0x80, 0x100, 0x200 };
+		tp->mii_advertise = media2advert[tp->default_port - 9];
+		tp->mii_advertise |= (tp->flags & HAS_8023X); /* Matching bits! */
+	}
+#endif
+#if 0
+	if (tp->flags & HAS_MEDIA_TABLE) {
+		sprintf(dev->name, "tulip%d", board_idx);	/* hack */
+		tulip_parse_eeprom(dev);
+		strcpy(dev->name, "eth%d");			/* un-hack */
+	}
+#endif
+	if ((tp->flags & ALWAYS_CHECK_MII) ||
+		(tp->mtable  &&  tp->mtable->has_mii) ||
+		( ! tp->mtable  &&  (tp->flags & HAS_MII))) {
+#if 0
+		if (tp->mtable  &&  tp->mtable->has_mii) {
+			for (i = 0; i < tp->mtable->leafcount; i++)
+				if (tp->mtable->mleaf[i].media == 11) {
+					tp->cur_index = i;
+					tp->saved_if_port = dev->if_port;
+					tulip_select_media(dev, 2);
+					dev->if_port = tp->saved_if_port;
+					break;
+				}
+		}
+#endif
+		/* Find the connected MII xcvrs.
+		   Doing this in open() would allow detecting external xcvrs
+		   later, but takes much time. */
+	tp->phys[board_idx]=INVALID_PHYID;
+#ifdef CONFIG_PILOT_AUTO_PHY
+	tulip_find_mii (dev, board_idx);
+	if (board_idx == 0)
+		printk("eth0: Auto detected Phy ID = %d\n",tp->phys[0]-1);
+	#if (CONFIG_PILOT_NET_COUNT == 2)
+		if (board_idx == 1)
+			printk("eth1: Auto detected Phy ID = %d\n",tp->phys[1]-1);
+	#endif
+#else
+	if (board_idx == 0)
+	{
+	#ifdef CONFIG_PILOT_MAC0_PHY_PRESENT
+		#ifdef CONFIG_SPX_FEATURE_GLOBAL_MAC0_PHYID
+			tp->phys[0] = CONFIG_SPX_FEATURE_GLOBAL_MAC0_PHYID + 1;
+		#else
+			tp->phys[0] = CONFIG_PILOT_MAC0_PHYID + 1;
+		#endif
+		tulip_init_mii (dev, board_idx,board_idx,tp->phys[0]);
+		if (tp->phys[board_idx]!= INVALID_PHYID)
+			printk("eth0: User Supplied Phy ID = %d\n",tp->phys[0]-1);
+	#else
+		tp->phys[0] = INVALID_PHYID;
+		printk("eth0: No PHY Present\n");
+	#endif
+	}
+
+	#if (CONFIG_PILOT_NET_COUNT == 2)
+		if (board_idx == 1)
+		{
+		#ifdef CONFIG_PILOT_MAC1_PHY_PRESENT
+			#ifdef CONFIG_SPX_FEATURE_GLOBAL_MAC1_PHYID
+				tp->phys[1] = CONFIG_SPX_FEATURE_GLOBAL_MAC1_PHYID + 1;
+			#else
+				tp->phys[1] = CONFIG_PILOT_MAC1_PHYID + 1;
+			#endif
+			tulip_init_mii (dev, board_idx,board_idx,tp->phys[1]);
+			if (tp->phys[board_idx]!= INVALID_PHYID)
+				printk("eth1: User Supplied Phy ID = %d\n",tp->phys[1]-1);
+		#else
+			tp->phys[1] = INVALID_PHYID; 
+			printk("eth1: No PHY Present\n");
+		#endif
+		}
+	#endif
+
+#endif
+	}
+	if (tp->phys[board_idx]==INVALID_PHYID)
+	{
+		/* Set to 100Mbps Full Duplex for NCSI to work */
+		tp->csr6 = ((0x1<<1) | (0x1<<30) | (0x1<<21) | (0x1 << 9));
+		tp->full_duplex = 1;
+	}
+
+	/* The Tulip-specific entries in the device structure. */
+	dev->netdev_ops = &tulip_pilot3_netdev_ops;
+	dev->watchdog_timeo = TX_TIMEOUT;
+#ifdef CONFIG_TULIP_NAPI
+	netif_napi_add(dev, &tp->napi, tulip_poll, 16);
+#endif
+	
+	SET_ETHTOOL_OPS(dev, &ops);
+#ifdef CONFIG_PILOT_SG
+        dev->features   |= NETIF_F_GSO;
+        dev->features   |= NETIF_F_SG;
+#endif
+
+	if (register_netdev(dev))
+		goto err_out_free_ring;
+#if 0
+	printk(KERN_INFO "%s: %s rev %d at "
+#ifdef CONFIG_TULIP_MMIO
+		"MMIO"
+#else
+		"Port"
+#endif
+		" %#llx,", dev->name, chip_name, pdev->revision,
+		(unsigned long long) pci_resource_start(pdev, TULIP_BAR));
+#else
+	//printk(KERN_INFO "%s: %s rev %d at %p,",
+	    //   dev->name, chip_name, chip_rev, ioaddr);
+
+        dev_info(&dev->dev,"%s rev %d %s %pM, IRQ %d\n",
+	 chip_name, chip_rev,
+	 eeprom_missing ? " EEPROM not present," : "",
+	 dev->dev_addr, irq);
+#endif
+
+       //pci_set_drvdata(pdev, dev);
+       pilot3_devices[interface] = dev;
+	
+	/* Initialize for generic mii functions */
+	tp->mii.phy_id_mask =0x1F;
+	tp->mii.reg_num_mask=0x1F;
+	tp->mii.phy_id = tp->phys[board_idx];
+	tp->mii.dev = dev;
+	tp->mii.mdio_read = tulip_mdio_read;
+	tp->mii.mdio_write = tulip_mdio_write;
+	if (tp->mii.phy_id != INVALID_PHYID) 
+		tp->mii.supports_gmii = mii_check_gmii_support(&tp->mii);
+	else
+		tp->mii.supports_gmii = 0;
+
+	/* tp->phys[X] =  ethX * where X is the board/instance number (board_idx) */
+	tp->phy_index = board_idx;
+	
+
+#if 0
+        if (tp->chip_id == PNIC2)
+		tp->link_change = pnic2_lnk_change;
+	else if (tp->flags & HAS_NWAY)
+		tp->link_change = t21142_lnk_change;
+	else if (tp->flags & HAS_PNICNWAY)
+		tp->link_change = pnic_lnk_change;
+#endif
+	/* Reset the xcvr interface and turn on heartbeat. */
+#if 0
+	switch (chip_idx) {
+	case DC21140:
+	case DM910X:
+	default:
+#endif
+		if (tp->mtable)
+			iowrite32(tp->mtable->csr12dir | 0x100, ioaddr + CSR12);
+#if 0			
+		break;
+	case DC21142:
+		if (tp->mii_cnt  ||  tulip_media_cap[dev->if_port] & MediaIsMII) {
+			iowrite32(csr6_mask_defstate, ioaddr + CSR6);
+			iowrite32(0x0000, ioaddr + CSR13);
+			iowrite32(0x0000, ioaddr + CSR14);
+			iowrite32(csr6_mask_hdcap, ioaddr + CSR6);
+		} else
+			t21142_start_nway(dev);
+		break;
+	case PNIC2:
+	        /* just do a reset for sanity sake */
+		iowrite32(0x0000, ioaddr + CSR13);
+		iowrite32(0x0000, ioaddr + CSR14);
+		break;
+	case LC82C168:
+		if ( ! tp->mii_cnt) {
+			tp->nway = 1;
+			tp->nwayset = 0;
+			iowrite32(csr6_ttm | csr6_ca, ioaddr + CSR6);
+			iowrite32(0x30, ioaddr + CSR12);
+			iowrite32(0x0001F078, ioaddr + CSR6);
+			iowrite32(0x0201F078, ioaddr + CSR6); /* Turn on autonegotiation. */
+		}
+		break;
+	case MX98713:
+	case COMPEX9881:
+		iowrite32(0x00000000, ioaddr + CSR6);
+		iowrite32(0x000711C0, ioaddr + CSR14); /* Turn on NWay. */
+		iowrite32(0x00000001, ioaddr + CSR13);
+		break;
+	case MX98715:
+	case MX98725:
+		iowrite32(0x01a80000, ioaddr + CSR6);
+		iowrite32(0xFFFFFFFF, ioaddr + CSR14);
+		iowrite32(0x00001000, ioaddr + CSR12);
+		break;
+	case COMET:
+		/* No initialization necessary. */
+		break;
+	}
+#endif
+	/* put the chip in snooze mode until opened */
+	tulip_set_power_state (tp, 0, 1);
+
+	return 0;
+
+err_out_free_ring:
+//	pci_free_consistent (pdev,
+//			     sizeof (struct tulip_rx_desc) * RX_RING_SIZE +
+//			     sizeof (struct tulip_tx_desc) * TX_RING_SIZE,
+//			     tp->rx_ring, tp->rx_ring_dma);
+	dma_free_coherent (NULL,
+			     sizeof (struct tulip_rx_desc) * RX_RING_SIZE +
+			     sizeof (struct tulip_tx_desc) * TX_RING_SIZE,
+			     tp->rx_ring, tp->rx_ring_dma);
+
+err_out_mtable:
+	kfree (tp->mtable);
+//	pci_iounmap(pdev, ioaddr);
+
+err_out_free_res:
+//	pci_release_regions (pdev);
+    release_region((unsigned long)ioaddr, tulip_tbl[chip_idx].io_size);
+
+err_out_free_netdev:
+	free_netdev (dev);
+    printk("Pilot3 Tulip initialization failed\n");
+	return -ENODEV;
+}
+
+
+#ifdef CONFIG_PM
+#if 0
+static int tulip_suspend (struct pci_dev *pdev, pm_message_t state)
+{
+	struct net_device *dev = pci_get_drvdata(pdev);
+
+	if (!dev)
+		return -EINVAL;
+
+	if (!netif_running(dev))
+		goto save_state;
+
+	tulip_down(dev);
+
+	netif_device_detach(dev);
+	free_irq(dev->irq, dev);
+
+save_state:
+	pci_save_state(pdev);
+	pci_disable_device(pdev);
+	pci_set_power_state(pdev, pci_choose_state(pdev, state));
+
+	return 0;
+}
+
+
+static int tulip_resume(struct pci_dev *pdev)
+{
+	struct net_device *dev = pci_get_drvdata(pdev);
+	int retval;
+
+	if (!dev)
+		return -EINVAL;
+
+	pci_set_power_state(pdev, PCI_D0);
+	pci_restore_state(pdev);
+
+	if (!netif_running(dev))
+		return 0;
+
+	if ((retval = pci_enable_device(pdev))) {
+		printk (KERN_ERR "tulip: pci_enable_device failed in resume\n");
+		return retval;
+	}
+
+	if ((retval = request_irq(dev->irq, &tulip_interrupt, IRQF_SHARED, dev->name, dev))) {
+		printk (KERN_ERR "tulip: request_irq failed in resume\n");
+		return retval;
+	}
+
+	netif_device_attach(dev);
+
+	if (netif_running(dev))
+		tulip_up(dev);
+
+	return 0;
+}
+#endif
+#endif /* CONFIG_PM */
+
+
+//static void __devexit tulip_remove_one (struct pci_dev *pdev)
+static void  tulip_remove_one (int interface)
+{
+//	struct net_device *dev = pci_get_drvdata (pdev);
+	struct net_device *dev = pilot3_devices[interface];
+	struct tulip_private *tp;
+	void __iomem *ioaddr;
+
+	if (!dev)
+		return;
+
+	tp = netdev_priv(dev);
+    ioaddr = tp->base_addr;
+	unregister_netdev(dev);
+//	pci_free_consistent (pdev,
+//			     sizeof (struct tulip_rx_desc) * RX_RING_SIZE +
+//			     sizeof (struct tulip_tx_desc) * TX_RING_SIZE,
+//			     tp->rx_ring, tp->rx_ring_dma);
+	dma_free_coherent (NULL,
+			     sizeof (struct tulip_rx_desc) * RX_RING_SIZE +
+			     sizeof (struct tulip_tx_desc) * TX_RING_SIZE,
+			     tp->rx_ring, tp->rx_ring_dma);
+	kfree (tp->mtable);
+//	pci_iounmap(pdev, tp->base_addr);
+	free_netdev (dev);
+//	pci_release_regions (pdev);
+    release_region((unsigned long)ioaddr, tulip_tbl[tp->chip_id].io_size);
+//	pci_set_drvdata (pdev, NULL);
+    pilot3_devices[interface] = NULL;
+	/* pci_power_off (pdev, -1); */
+}
+
+#ifdef CONFIG_NET_POLL_CONTROLLER
+/*
+ * Polling 'interrupt' - used by things like netconsole to send skbs
+ * without having to re-enable interrupts. It's not called while
+ * the interrupt routine is executing.
+ */
+
+static void poll_tulip (struct net_device *dev)
+{
+	/* disable_irq here is not very nice, but with the lockless
+	   interrupt handler we have no other choice. */
+	disable_irq(dev->irq);
+	tulip_interrupt (dev->irq, dev);
+	enable_irq(dev->irq);
+}
+#endif
+#if 0
+static struct pci_driver tulip_driver = {
+	.name		= DRV_NAME,
+	.id_table	= tulip_pci_tbl,
+	.probe		= tulip_init_one,
+	.remove		= __devexit_p(tulip_remove_one),
+#ifdef CONFIG_PM
+	.suspend	= tulip_suspend,
+	.resume		= tulip_resume,
+#endif /* CONFIG_PM */
+};
+#endif
+
+extern unsigned long enetaddr[4][6];
+
+static int __init tulip_init (void)
+{
+    int status;
+#ifdef MODULE
+	printk (KERN_INFO "%s", version);
+#endif
+
+	/* copy module parms into globals */
+	tulip_rx_copybreak = rx_copybreak;
+	tulip_max_interrupt_work = max_interrupt_work;
+
+	/* probe for and init boards */
+//	return pci_register_driver(&tulip_driver);	
+
+#ifdef CONFIG_PILOT_SWAP_MAC
+    	status = tulip_init_one(INTERFACE_ETH_B,(unsigned char *)enetaddr[INTERFACE_ETH_B]);
+    	if(status < 0)
+        	return -1;
+
+#if (CONFIG_PILOT_NET_COUNT == 2)
+	status = tulip_init_one(INTERFACE_ETH_A,(unsigned char *)enetaddr[INTERFACE_ETH_A]);
+  	if(status < 0)
+	{
+   		tulip_remove_one(INTERFACE_ETH_B);
+   		return -1;
+	}
+#endif
+
+
+#else	// CONFIG_PILOT_SWAP_MAC
+
+    	status = tulip_init_one(INTERFACE_ETH_A,(unsigned char *)enetaddr[INTERFACE_ETH_A]);
+    	if(status < 0)
+        	return -1;
+
+#if (CONFIG_PILOT_NET_COUNT == 2)
+	status = tulip_init_one(INTERFACE_ETH_B,(unsigned char *)enetaddr[INTERFACE_ETH_B]);
+  	if(status < 0)
+	{
+   		tulip_remove_one(INTERFACE_ETH_A);
+   		return -1;
+	}
+#endif
+
+#endif // CONFIG_PILOT_SWAP_MAC
+    return 0;
+}
+
+
+static void __exit tulip_cleanup (void)
+{
+#ifdef CONFIG_PILOT_SWAP_MAC
+    	tulip_remove_one(INTERFACE_ETH_B);
+#if (CONFIG_PILOT_NET_COUNT == 2)
+   	tulip_remove_one(INTERFACE_ETH_A);
+#endif
+
+#else	// CONFIG_PILOT_SWAP_MAC
+
+    	tulip_remove_one(INTERFACE_ETH_A);
+#if (CONFIG_PILOT_NET_COUNT == 2)
+	tulip_remove_one(INTERFACE_ETH_B);
+#endif
+
+#endif // CONFIG_PILOT_SWAP_MAC
+   
+//	pci_unregister_driver (&tulip_driver);
+}
+
+
+module_init(tulip_init);
+module_exit(tulip_cleanup);
