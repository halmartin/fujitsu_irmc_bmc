--- linux_old/drivers/mtd/maps/ractrends.c	1970-01-01 05:30:00.000000000 +0530
+++ linux/drivers/mtd/maps/ractrends.c	2017-10-05 17:01:55.371983314 +0530
@@ -0,0 +1,386 @@
+/*
+ * ractrends.c - MTD mapper for FMH Compatible images used in ractrends
+ * 			     series of boards.
+ * 			     Creates MTD Devices for each of the JFFS or JFFS2 modules
+ * 			     present in the FMH Image
+ * 			     
+ * Copyright 2003 American Megatrends Inc.
+ *
+ */
+
+
+#include <linux/module.h>
+#include <linux/types.h>
+#include <linux/kernel.h>
+#include <linux/mtd/mtd.h>
+#include <linux/mtd/map.h>
+#include <linux/mtd/concat.h>
+#include <linux/mtd/partitions.h>
+#include <asm/io.h>
+#include "fmh.h"
+#include "../mtdcore.h"
+#include <linux/init.h>
+
+
+#if defined (CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT)
+#define IMAGE_1  "imagebooted=1"
+#define IMAGE_2  "imagebooted=2"
+#endif
+
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS
+#define MAX_BANKS (CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS + CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS)
+#else
+#ifdef CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_ACCESS
+#define MAX_BANKS 1
+#else
+#define MAX_BANKS CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS
+#endif
+#endif
+static struct mtd_info *ractrends_mtd[MAX_BANKS];
+
+static struct mtd_partition ractrends_flash_partitions[16];
+static char partition_name [16][9];
+unsigned long PartNameCnt=0;
+
+#define FULL_FLASH_PARTITION_NAME	"fullpart"
+unsigned char total_active_spi_banks;
+
+unsigned long
+GetFMHPartitions(unsigned long FlashStart, uint64_t FlashSize, uint32_t SectorSize,struct mtd_info *ractrendsmtd,unsigned long *PartCount)
+{
+
+	FMH *fmh;
+	MODULE_INFO *mod;
+	unsigned short ModuleType;
+
+	unsigned long i;
+	unsigned long startingsector=0,endingsector=0,startaddress=0;
+
+#if defined (CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT)
+	unsigned long startingsector_p=0,endingsector_p=0,startaddress_p=0;
+	int scan_flag=1;
+
+#if defined CONFIG_SPX_FEATURE_DEDICATED_SPI_FLASH_BANK
+    unsigned long img2_offset=0;
+    for(i=0;i<CONFIG_SPX_FEATURE_SECONDARY_IMAGE_SPI;i++){
+        img2_offset+=ractrends_mtd[i]->size;
+    }
+
+#endif
+	if(strstr(boot_command_line,IMAGE_1)){
+    	endingsector=(unsigned long)div_u64(CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE,SectorSize);
+
+ #if defined CONFIG_SPX_FEATURE_DEDICATED_SPI_FLASH_BANK
+       
+        startingsector_p=(unsigned long)div_u64(img2_offset,SectorSize);
+        endingsector_p=(unsigned long)div_u64(img2_offset+CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE,SectorSize);
+        startaddress_p=img2_offset;
+#else
+		startingsector_p=endingsector;
+		endingsector_p=2*endingsector;
+		startaddress_p=CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
+#endif  
+	}
+	else if(strstr(boot_command_line,IMAGE_2)){
+#if defined CONFIG_SPX_FEATURE_DEDICATED_SPI_FLASH_BANK
+        startingsector=(unsigned long)div_u64(img2_offset,SectorSize);
+        endingsector=(unsigned long)div_u64(img2_offset+CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE,SectorSize);
+        startaddress=img2_offset;
+        endingsector_p=(unsigned long)div_u64(CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE,SectorSize);
+#else
+		startingsector=(unsigned long)div_u64(CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE,SectorSize);
+		endingsector=2*startingsector;
+		startaddress=CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE;
+		endingsector_p=startingsector;
+#endif  
+    }
+#else
+	//address computation for single image
+	endingsector=(unsigned long)div_u64(FlashSize,SectorSize);
+#endif
+
+	printk("*PartCount is %ld\n",*PartCount);
+	
+	/* Create a MTD for full Flash */
+	strncpy(partition_name[PartNameCnt],FULL_FLASH_PARTITION_NAME,8);
+	partition_name[PartNameCnt][8] = 0;
+	ractrends_flash_partitions[*PartCount].name   = partition_name[PartNameCnt];
+	ractrends_flash_partitions[*PartCount].offset = 0;
+	ractrends_flash_partitions[*PartCount].size 	 = FlashSize;
+	ractrends_flash_partitions[*PartCount].mask_flags = 0;
+	printk("MTD Partition %ld : %s @ 0x%lx of Size 0x%lx\n",*PartCount,
+						ractrends_flash_partitions[*PartCount].name,
+						(unsigned long)ractrends_flash_partitions[*PartCount].offset,
+						(unsigned long)ractrends_flash_partitions[*PartCount].size);
+	(*PartCount)++;
+	PartNameCnt++;
+
+#if defined (CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT)
+START_SCAN:
+#endif
+
+	for (i=startingsector;i<endingsector;)
+	{
+		fmh = ScanforFMH((unsigned char *)(FlashStart+(i*SectorSize)),(unsigned long)SectorSize,ractrendsmtd);
+		if (fmh == NULL)
+		{
+			i++;
+			continue;	
+		}
+		mod = &(fmh->Module_Info);
+
+		/* Check if module type is JFFS or JFFS2 */
+		ModuleType = le16_to_host(mod->Module_Type);
+		if ((ModuleType != MODULE_JFFS) && (ModuleType != MODULE_JFFS2) && (ModuleType != MODULE_INITRD_CRAMFS) &&
+		    (ModuleType != MODULE_CRAMFS)  && (ModuleType != MODULE_SQUASHFS) && (ModuleType != MODULE_INITRD_SQUASHFS) &&
+		    (ModuleType != MODULE_CACHE))
+		{
+			if ((i*SectorSize) > le32_to_host(mod->Module_Location))		/* AltFMH */
+				i++;
+			else
+				i+=(unsigned long)div_u64(le32_to_host(fmh->FMH_AllocatedSize),SectorSize);
+			continue;
+		}
+
+		/* Copy the Name String */
+		strncpy(partition_name[PartNameCnt],mod->Module_Name,8);
+		partition_name[PartNameCnt][8] = 0;
+
+		/* Fill the parition information */
+		ractrends_flash_partitions[*PartCount].name   = partition_name[PartNameCnt];
+
+		if ((fmh->FMH_Ver_Major == 1) && (fmh->FMH_Ver_Minor >= 8))
+			ractrends_flash_partitions[*PartCount].offset = startaddress + le32_to_host(mod->Module_Location);
+		else
+			ractrends_flash_partitions[*PartCount].offset = (i*SectorSize)+ le32_to_host(mod->Module_Location);
+
+//		ractrends_flash_partitions[*PartCount].size = le32_to_host(fmh->FMH_AllocatedSize) - le32_to_host(mod->Module_Location);
+		ractrends_flash_partitions[*PartCount].size = le32_to_host(mod->Module_Size);
+
+		ractrends_flash_partitions[*PartCount].mask_flags = 0;
+		printk("MTD Partition %ld : %s @ 0x%lx of Size 0x%lx\n",*PartCount,
+						ractrends_flash_partitions[*PartCount].name,
+						(unsigned long)ractrends_flash_partitions[*PartCount].offset,
+						(unsigned long)ractrends_flash_partitions[*PartCount].size);
+
+		if ((i*SectorSize) > le32_to_host(mod->Module_Location))   /* AltFMH */
+			i++;
+		else
+			i+=(unsigned long)div_u64(le32_to_host(fmh->FMH_AllocatedSize),SectorSize);
+		(*PartCount)++;
+		PartNameCnt++;
+	}
+#if defined (CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT)
+	if(scan_flag== 1){
+		scan_flag=2;
+		startingsector=startingsector_p;
+		endingsector=endingsector_p;
+		startaddress=startaddress_p;
+		goto START_SCAN;// NOW START SCANNING FOR INACTIVE IMAGE
+	}
+#endif
+
+	return *PartCount;
+}
+
+EXPORT_SYMBOL(ractrends_mtd) ;
+
+struct map_info ractrends_flash_map[MAX_BANKS];
+static struct mtd_info *concat_mtd = NULL;
+static unsigned long bankcount= 0;
+
+
+#ifdef CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_ACCESS
+struct map_info individual_spi_flash_map;
+static struct mtd_info *individual_spi_mtd = NULL;
+static struct mtd_partition individual_spi_flash_partitions;
+static char individual_spi_partition_name[16];
+
+void create_individual_spi_partition(void)
+{
+	unsigned long SPI_FlashAddr = CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_FLASH_START;
+	unsigned long SPI_FlashSize = CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_FLASH_SIZE;
+	unsigned long SPI_bank = CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_BANK_NUMBER;
+	int status = 0;
+
+	printk("Probing for Individual SPI Flash at Bank # %ld\n",SPI_bank);
+	individual_spi_flash_map.name = "Individual SPI Flash";
+    individual_spi_flash_map.bankwidth = 2;
+    individual_spi_flash_map.size = SPI_FlashSize;
+    individual_spi_flash_map.phys = SPI_FlashAddr;
+    individual_spi_flash_map.map_priv_1 = SPI_bank;
+    individual_spi_mtd = NULL;
+    simple_map_init(&individual_spi_flash_map);
+
+	#if defined CONFIG_SPX_FEATURE_GLOBAL_FLASH_TYPE_SPI
+		individual_spi_mtd = do_map_probe("spi_probe",&individual_spi_flash_map);
+	#else
+	#if defined CONFIG_SPX_FEATURE_GLOBAL_FLASH_TYPE_NOR
+		individual_spi_mtd = do_map_probe("cfi_probe",&individual_spi_flash_map);
+	#else
+	#error	CONFIG_SPX_FEATURE_GLOBAL_FLASH_TYPE is not defined or defined with unknown flash type
+	#endif
+	#endif
+	if (!individual_spi_mtd) 
+	{
+		printk("ERROR: init_ractrends_flash: flash probe failed\n");
+		//return -ENXIO;
+	}
+	individual_spi_mtd->owner = THIS_MODULE;
+
+	/* Create a MTD for full HOST SPI Flash */
+	strncpy(individual_spi_partition_name,CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_MTD_NAME,strlen(CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_MTD_NAME));
+	
+	individual_spi_flash_partitions.name   = individual_spi_partition_name;
+	individual_spi_flash_partitions.offset = 0;
+	individual_spi_flash_partitions.size 	 = SPI_FlashSize;
+	individual_spi_flash_partitions.mask_flags = 0;
+	printk("MTD Partition %s @ 0x%lx of Size 0x%lx\n",
+						individual_spi_flash_partitions.name,
+						(unsigned long)individual_spi_flash_partitions.offset,
+						(unsigned long)individual_spi_flash_partitions.size);
+
+	status = add_mtd_partitions(individual_spi_mtd,&individual_spi_flash_partitions,1);
+}
+#endif
+
+int __init 
+init_ractrends_flash(void)
+{
+	unsigned long FlashAddr = CONFIG_SPX_FEATURE_GLOBAL_FLASH_START;
+	unsigned long FlashSize = CONFIG_SPX_FEATURE_GLOBAL_FLASH_SIZE;
+	unsigned long PartitionCount;
+	unsigned long bank;
+	unsigned long PartCount=0;
+	int status = 0;
+	printk(KERN_NOTICE "Ractrends Flash mapping: 0x%08lx at 0x%08lx\n",
+					FlashSize, FlashAddr);
+
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS
+	printk(KERN_NOTICE "Flash total banks (%d) - BOOT(%d) BACKUP(%d)\n",
+		MAX_BANKS, CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS, CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS);
+#else
+	printk(KERN_NOTICE "Flash total banks (%d)\n", MAX_BANKS);
+#endif
+
+//	FlashSize = (unsigned long)div_u64(FlashSize,MAX_BANKS);
+	for (bank = 0; bank < MAX_BANKS; bank++)
+		ractrends_mtd[bank] = 0;
+
+	for (bank = 0; bank < MAX_BANKS; bank++)
+	{
+		printk("Probing for Flash at Bank # %ld\n",bank);
+		ractrends_flash_map[bank].name = "Ractrends";
+		ractrends_flash_map[bank].bankwidth = 2;
+		ractrends_flash_map[bank].size = FlashSize;
+		ractrends_flash_map[bank].phys = FlashAddr + (bank * FlashSize);
+		ractrends_flash_map[bank].map_priv_1 = bank;
+		ractrends_mtd[bank] = NULL;
+		simple_map_init(&ractrends_flash_map[bank]);	
+
+#if defined CONFIG_SPX_FEATURE_GLOBAL_FLASH_TYPE_SPI
+		ractrends_mtd[bank] = do_map_probe("spi_probe",&ractrends_flash_map[bank]);
+#else
+#if defined CONFIG_SPX_FEATURE_GLOBAL_FLASH_TYPE_NOR
+		ractrends_mtd[bank] = do_map_probe("cfi_probe",&ractrends_flash_map[bank]);
+#else
+#error	CONFIG_SPX_FEATURE_GLOBAL_FLASH_TYPE is not defined or defined with unknown flash type
+#endif
+#endif
+		if (!ractrends_mtd[bank]) 
+		{
+			if (bank == 0)
+			{
+				printk("ERROR: init_ractrends_flash: flash probe failed\n");
+				return -ENXIO;
+			}
+			break;
+		}
+		ractrends_mtd[bank]->owner = THIS_MODULE;
+	}
+	bankcount = bank;
+	total_active_spi_banks = bankcount;
+
+	if (bank > 1)	
+		concat_mtd = mtd_concat_create(ractrends_mtd,bank,"Concat Ractrends");
+	else	
+		concat_mtd = ractrends_mtd[0];
+	if (!concat_mtd){
+		printk("ERROR: init_ractrends_flash: flash concat failed\n");
+		return -ENXIO;}
+	PartitionCount = GetFMHPartitions(0,concat_mtd->size,
+				concat_mtd->erasesize,concat_mtd,&PartCount);
+
+	status = add_mtd_partitions(concat_mtd,ractrends_flash_partitions,PartitionCount);
+	if(status !=0) 
+	{
+		printk("Error in adding partitions \n");
+		return status;
+	}
+
+#if defined (CONFIG_SPX_FEATURE_GLOBAL_DUAL_IMAGE_SUPPORT) && defined (CONFIG_SPX_FEATURE_COMMON_CONF_SECTION)
+	printk("Creating Partition for conf \n");
+	ractrends_flash_partitions[PartCount].name   		= "conf";
+#if defined CONFIG_SPX_FEATURE_DEDICATED_SPI_FLASH_BANK
+    ractrends_flash_partitions[PartCount].offset        = CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE + ractrends_mtd[0]->size;
+#else
+    ractrends_flash_partitions[PartCount].offset 		= (CONFIG_SPX_FEATURE_GLOBAL_USED_FLASH_SIZE * 2);//concat_mtd->size - CONFIG_SPX_FEATURE_GLOBAL_FLASH_SIZE; //starting of SPI-3
+#endif
+	ractrends_flash_partitions[PartCount].size   		= CONFIG_SPX_FEATURE_GLOBAL_CONF_SIZE;
+	ractrends_flash_partitions[PartCount].mask_flags 	= 0;
+	printk("MTD Partition %ld : %s @ 0x%lx of Size 0x%lx\n",PartCount,
+                                                ractrends_flash_partitions[PartCount].name,
+                                                (unsigned long)ractrends_flash_partitions[PartCount].offset,
+                                                (unsigned long)ractrends_flash_partitions[PartCount].size);
+	status = add_mtd_partitions(concat_mtd,&ractrends_flash_partitions[PartCount],1);
+	if(status !=0) 
+		return status;
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_FAILSAFECONF
+	ractrends_flash_partitions[PartCount].name   		= "conf";
+	ractrends_flash_partitions[PartCount].offset        = ractrends_flash_partitions[PartCount-1].offset + CONFIG_SPX_FEATURE_GLOBAL_CONF_SIZE;
+    ractrends_flash_partitions[PartCount].size   		= CONFIG_SPX_FEATURE_GLOBAL_CONF_SIZE;
+	ractrends_flash_partitions[PartCount].mask_flags 	= 0;
+	printk("MTD Partition %ld : %s @ 0x%lx of Size 0x%lx\n",PartCount,
+                                                ractrends_flash_partitions[PartCount].name,
+                                                (unsigned long)ractrends_flash_partitions[PartCount].offset,
+                                                (unsigned long)ractrends_flash_partitions[PartCount].size);
+	status = add_mtd_partitions(concat_mtd,&ractrends_flash_partitions[PartCount],1);
+	if(status!=0) 
+		return status;
+#endif
+#endif
+
+#ifdef CONFIG_SPX_FEATURE_INDIVIDUAL_SPI_ACCESS
+	create_individual_spi_partition();
+#endif
+	return status;
+}
+
+static void __exit 
+cleanup_ractrends_flash(void)
+{
+	unsigned long bank;
+	if (bankcount > 1)
+	{
+		del_mtd_partitions(concat_mtd);
+		map_destroy(concat_mtd);
+		concat_mtd = NULL;
+
+	}
+	for (bank = 0; bank < bankcount; bank++)
+	{
+		if (ractrends_mtd[bank])
+		{
+			del_mtd_partitions(ractrends_mtd[bank]);
+			map_destroy(ractrends_mtd[bank]);
+			ractrends_mtd[bank] = NULL;
+		}
+	}
+
+}
+
+module_init(init_ractrends_flash);
+module_exit(cleanup_ractrends_flash);
+
+MODULE_AUTHOR("Samvinesh Christopher. American Megatrends Inc.");
+MODULE_DESCRIPTION("MTD map driver for the Ractrends Series of Boards");
