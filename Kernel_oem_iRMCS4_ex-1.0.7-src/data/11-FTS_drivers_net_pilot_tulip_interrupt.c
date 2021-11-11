--- linux/drivers/net/ethernet/pilot_tulip/interrupt.c	2015-10-20 10:37:04.965668130 +0530
+++ linux_new/drivers/net/ethernet/pilot_tulip/interrupt.c	2015-10-20 10:56:30.865707370 +0530
@@ -521,15 +521,16 @@
 #ifdef __hppa__
 	struct tulip_private *tp = netdev_priv(dev);
 	int csr12 = ioread32(tp->base_addr + CSR12) & 0xff;
+	unsigned long flags;
 
 	if (csr12 != tp->csr12_shadow) {
 		/* ack interrupt */
 		iowrite32(csr12 | 0x02, tp->base_addr + CSR12);
 		tp->csr12_shadow = csr12;
 		/* do link change stuff */
-		spin_lock(&tp->lock);
+		spin_lock_irqsave(&tp->lock, flags);
 		tulip_check_duplex(dev);
-		spin_unlock(&tp->lock);
+		spin_unlock_irqrestore(&tp->lock, flags);
 		/* clear irq ack bit */
 		iowrite32(csr12 & ~0x02, tp->base_addr + CSR12);
 
