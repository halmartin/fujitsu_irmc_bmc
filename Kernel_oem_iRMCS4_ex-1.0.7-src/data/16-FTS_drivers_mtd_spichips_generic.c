diff -Naur linux_orig/drivers/mtd/spichips/generic.c linux/drivers/mtd/spichips/generic.c
--- linux_orig/drivers/mtd/spichips/generic.c	2018-08-14 19:34:51.931382343 +0530
+++ linux/drivers/mtd/spichips/generic.c	2018-08-14 19:13:12.058892794 +0530
@@ -483,7 +483,7 @@
  	int protect_state ;
  	size_t bytes = spansion_erase_size;
 
-	down(&priv->chip_drv->lock);
+	down(&priv->ctrl_drv->lock);
 	
 
  	/* Spantion SPI unprotect sector */
@@ -499,7 +499,7 @@
 	/* Wait until finished previous command. */
 	if (wait_till_ready(bank,ctrl_drv))
 	{
-		up(&priv->chip_drv->lock);
+		up(&priv->ctrl_drv->lock);
 		return -1;
 	}
 
@@ -522,7 +522,7 @@
 			{
 				spi_generic_select_die( bank, 0,ctrl_drv);
 			}
-			up(&priv->chip_drv->lock);
+			up(&priv->ctrl_drv->lock);
 			return spi_error(retval);
 		}
 	}
@@ -562,7 +562,7 @@
 		/* requires the read flag status with at latest one byte. */
 		if (require_read_flag_status(bank,ctrl_drv))
 		{
-			up(&priv->chip_drv->lock);
+			up(&priv->ctrl_drv->lock);
 			return -1;
 		}
 	}
@@ -585,7 +585,7 @@
 		{
 			spi_generic_select_die( bank, 0,ctrl_drv);
 		}
-		up(&priv->chip_drv->lock);
+		up(&priv->ctrl_drv->lock);
 		return spi_error(retval);
 	}
 
@@ -604,7 +604,7 @@
 	{
 		spi_generic_select_die( bank, 0,ctrl_drv);
 	}
-	up(&priv->chip_drv->lock);
+	up(&priv->ctrl_drv->lock);
 
 	/* Spantion SPI protect sector */
 	if (ractrends_spiflash_flash_id[bank] == 0x00011902)
@@ -659,14 +659,14 @@
 			if (bytes==0) return 0;
 		}
 	}
-	down(&priv->chip_drv->lock);
+	down(&priv->ctrl_drv->lock);
 	
 	
 	
 	/* Wait until finished previous command. */
 	if (wait_till_ready(bank,ctrl_drv))
 	{
-		up(&priv->chip_drv->lock);
+		up(&priv->ctrl_drv->lock);
 		return -1;
 	}
 
@@ -699,7 +699,7 @@
 			{
 				spi_generic_select_die( bank, 0,ctrl_drv);
 			}
-			up(&priv->chip_drv->lock);
+			up(&priv->ctrl_drv->lock);
 			return spi_error(retval);
 		}
 	}
@@ -938,7 +938,7 @@
 			{
 				spi_generic_select_die( bank, 0,ctrl_drv);
 			}
-			up(&priv->chip_drv->lock);
+			up(&priv->ctrl_drv->lock);
 			return spi_error(retval);
 		}
 
@@ -964,7 +964,7 @@
 	{
 		spi_generic_select_die( bank, 0,ctrl_drv);
 	}
-	up(&priv->chip_drv->lock);
+	up(&priv->ctrl_drv->lock);
 
 	/*Spasion  disable extended address*/
 	if (ractrends_spiflash_flash_id[bank] == 0x00011902) 
@@ -1036,7 +1036,7 @@
 		}
 	}
 
-	down(&priv->chip_drv->lock);
+	down(&priv->ctrl_drv->lock);
 	
 	if (address32 == ADDRESS_DIE_LO3_HI4_BYTE){
 		if(addr >= ADDR_32MB){
@@ -1058,7 +1058,7 @@
 			{
 				spi_generic_select_die( bank, 0,ctrl_drv);
 			}
-			up(&priv->chip_drv->lock);
+			up(&priv->ctrl_drv->lock);
 			return spi_error(retval);
 		}
 	}
@@ -1074,7 +1074,7 @@
 			{
 				spi_generic_select_die( bank, 0,ctrl_drv);
 			}
-			up(&priv->chip_drv->lock);
+			up(&priv->ctrl_drv->lock);
 			return -1;
 		}
 
@@ -1142,7 +1142,7 @@
 			/* requires the read flag status with at latest one byte. */
 			if (require_read_flag_status(bank,ctrl_drv))
 			{
-				up(&priv->chip_drv->lock);
+				up(&priv->ctrl_drv->lock);
 				return -1;
 			}
 		}
@@ -1165,7 +1165,7 @@
 			{
 				spi_generic_select_die( bank, 0,ctrl_drv);
 			}
-			up(&priv->chip_drv->lock);
+			up(&priv->ctrl_drv->lock);
 			return spi_error(retval);
 		}
 		addr+=(transfer-retval);
@@ -1189,7 +1189,7 @@
 	{
 		spi_generic_select_die( bank, 0,ctrl_drv);
 	}
-	up(&priv->chip_drv->lock);
+	up(&priv->ctrl_drv->lock);
 
 	/* Spantion SPI protect sector */
 	if (ractrends_spiflash_flash_id[bank] == 0x00011902)
