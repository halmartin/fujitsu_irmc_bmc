--- linux-3.14.17/drivers/mmc/host/sdhci-pilot.c	2014-09-12 16:25:46.253062198 +0000
+++ linux/drivers/mmc/host/sdhci-pilot.c	2014-09-12 15:34:38.453024452 +0000
@@ -0,0 +1,2062 @@
+#include <linux/version.h>
+#include <linux/delay.h>
+#include <linux/highmem.h>
+#include <linux/io.h>
+#include <linux/dma-mapping.h>
+#include <linux/scatterlist.h>
+#include <linux/platform_device.h>
+#include <linux/export.h>
+#include <linux/slab.h>
+#include <linux/module.h>
+#include <linux/leds.h>
+#include <linux/mmc/host.h>
+
+#include "sdhci-pilot.h"
+#include "sdhci.h"
+
+#if defined(CONFIG_SOC_SE_PILOT4)
+#define MULTIBLOCK_WRITE	1
+
+static int SD_DMA_MODE = 1;
+module_param(SD_DMA_MODE, int, S_IRUGO|S_IWUSR);
+static uint txdttmr_flg = 0;
+module_param(txdttmr_flg, uint, S_IRUGO|S_IWUSR);
+#endif
+static int pilot_sd_remove(struct platform_device *pdev);
+
+#ifdef CONFIG_PM
+static const struct dev_pm_ops pilot_sd_pmops = {
+        /* Use standard power save option (if enabled) by using core
+         * SD/MMC stack function call itself, as PILOT register set aligns
+         * perfectly with the call */
+        .resume  = sdhci_resume_host,
+        .suspend = sdhci_suspend_host,
+};
+#endif
+
+#if defined(CONFIG_SOC_SE_PILOT4)
+
+struct dma_desc
+{
+	volatile unsigned int_enb: 1;       //Enable Interrupt
+	volatile unsigned chain: 1;         //0-last entry, 1-NOT last entry
+	volatile unsigned dir: 1;           //0-SDHC-DDR, 1-DDR-SDHC
+	volatile unsigned crc_en: 1;        //Enable CRC
+	volatile unsigned rsvd1: 4;
+	volatile unsigned len: 24;          //Max 16MB
+	volatile unsigned long crc_addr;    //CRC address
+	volatile unsigned long data_addr;   //src/dest Address
+	volatile unsigned long rsvd2;
+};
+
+struct dma_desc *gstruc[NO_OF_SDHCS];
+struct dma_desc *astru[NO_OF_SDHCS];
+dma_addr_t dma_addr[NO_OF_SDHCS];
+
+struct sdhci_host * pilot_sdhc[NO_OF_SDHCS];
+static void pilot_sdhc_card_reinit(struct sdhci_host * host);
+static void pilot_set_clkctl(struct sdhci_host * host, u16 fsclk_divisor, u8 sdclk_polarity, u8 sdclk_enable, u8 sdclk_select);
+static void pilot_setup_lemepagecfg (struct sdhci_host * host, u8 high_wm, u8 low_wm, u16 max_num_pages, u8 page_size_dw);
+static void pilot_enable_all_interrupts(struct sdhci_host * host);
+#endif
+
+static unsigned int pilot_get_max_clk(struct sdhci_host *host)
+{
+        return PILOT_SD_MAX_CLOCK;
+}
+
+static unsigned int pilot_get_min_clk(struct sdhci_host *host)
+{
+        return PILOT_SD_MIN_CLOCK;
+}
+
+static void sdhci_pilot_set_clock(struct sdhci_host *host, unsigned int clock)
+{
+	u16 clk=0;
+	u16 timeout;
+	
+
+	if (clock == host->clock)
+		return;
+
+	writew(0, host->ioaddr + SDHCI_CLOCK_CONTROL);
+
+	if (clock == 0)
+		goto out;
+
+	if (clock >= 50000000)
+        clk |= 0x2 << 4;
+	else if (clock >= 25000000)
+        clk |= 0x4 << 4;
+	else
+        clk |= 0xFA << 4;
+	writew(clk, host->ioaddr + SDHCI_CLOCK_CONTROL);
+
+
+
+	/* Wait max 10 ms */
+	timeout = 10;
+	while (!((clk = readw(host->ioaddr + SDHCI_CLOCK_CONTROL))
+		& SDHCI_CLOCK_INT_STABLE)) {
+		if (timeout == 0) {
+			printk(KERN_ERR "%s: Internal clock never "
+				"stabilised.\n", mmc_hostname(host->mmc));
+			return;
+		}
+		timeout--;
+		mdelay(1);
+	}
+
+	clk |= SDHCI_CLOCK_CARD_EN;
+	writew(clk, host->ioaddr + SDHCI_CLOCK_CONTROL);
+
+out:
+	host->clock = clock;
+}
+
+#if defined(CONFIG_SOC_SE_PILOT4)
+
+unsigned char pilot_mmc_crc7 (char *data, unsigned char length)
+{
+	unsigned char i, ibit, c, crc;
+	crc = 0x00;
+
+	for (i = 0; i < length; i++, data++)
+	{
+		c = *data;
+
+		for (ibit = 0; ibit < 8; ibit++)
+		{
+			crc = crc << 1;
+			if ((c ^ crc) & 0x80) 
+				crc = crc ^ 0x09;
+			c = c << 1;
+		}
+		crc = crc & 0x7F;
+	}
+
+	c = ((crc<<1)|1);
+	crc = c;
+
+	return crc;
+}
+
+static void pilot_set_cmdctl (struct sdhci_host *host, u8 multiblock,
+		u8 dir, u8 autocmd12en, u8 mode, u8 cmdindex, u8 datapresent, 
+		u8 cmdindexcheck, u8 resptype, u8 crcchkenable)
+{
+	u32 tempData;
+
+	tempData = readl(host->ioaddr + SDHCI_TRANSFER_MODE);
+	//0xC SDHCI_TRANSFER_MODE, 0xE SDHCI_COMMAND
+	//clear all the required fields
+	tempData &= (~(MODE));
+	tempData &= (~(AUTOCMD12EN));
+	tempData &= (~(DIRECTION));
+	tempData &= (~(MULTIBLOCK));
+	tempData &= (~(RESPTYPE));
+	tempData &= (~(CMDINDEXCHECK));
+	tempData &= (~(DATAPRESENT));
+	tempData &= (~(CMDINDEX));
+
+	//store all the required values
+	tempData |= ((datapresent & DATAPRESENT_BITS) << DATAPRESENT_POS);
+	tempData |= ((cmdindexcheck & CMDINDEXCHECK_BITS) << CMDINDEXCHECK_POS);
+	tempData |= ((resptype & RESPTYPE_BITS) << RESPTYPE_POS);
+	tempData |= ((multiblock & MULTIBLOCK_BITS) << MULTIBLOCK_POS);
+	tempData |= ((dir & DIRECTION_BITS) << DIRECTION_POS);
+	tempData |= ((autocmd12en & AUTOCMD12EN_BITS) << AUTOCMD12EN_POS);
+	tempData |= ((mode & MODE_BITS) << MODE_POS);
+
+	if (crcchkenable){
+		tempData |= CRCCHECKENABLE;
+	} else {
+		tempData &= ~CRCCHECKENABLE;
+	}
+
+	writel(tempData, host->ioaddr + SDHCI_TRANSFER_MODE);
+
+	tempData = readl(host->ioaddr + SDHCI_TRANSFER_MODE);
+	//clear all the required fields
+	tempData &= (~(CMDINDEX));
+	//store all the required values
+	tempData |= ((cmdindex & CMDINDEX_BITS) << CMDINDEX_POS);
+	tempData |= ((CMDSET_BITS) << CMDSET_POS);
+
+	writel(tempData, host->ioaddr + SDHCI_TRANSFER_MODE);
+
+}
+
+static void pilot_set_cmdarg (struct sdhci_host * host, u32 cmdarg)
+{
+	writel(cmdarg, host->ioaddr + SDHCI_ARGUMENT);
+}
+
+static void pilot_sdhc_card_reinit(struct sdhci_host * host)
+{
+	volatile u32 temp;
+	volatile int timeout = 1000000;
+	
+	/* Reset SDHC controller */
+	if(host->sdhc_id == SDHC0)
+	{
+		*(volatile unsigned int *)IO_ADDRESS(0x40100858) = 0;
+		*(volatile unsigned int *)IO_ADDRESS(0x40100854) = 1<<30;
+	}
+	if(host->sdhc_id == SDHC1)
+	{
+		*(volatile unsigned int *)IO_ADDRESS(0x40100858) = 1;
+		*(volatile unsigned int *)IO_ADDRESS(0x40100854) = 0;
+	}
+
+	*(volatile unsigned int *)IO_ADDRESS(0x40100850) = 0xD01;
+
+	mdelay(5);
+
+	do
+	{
+		temp = readl(host->ioaddr + SDHCI_LMEMCTL);
+		timeout--;
+		if(timeout <= 0) 
+		{
+			printk("%s %d Condition NOT met even after wait!!! Breaking while loop!!!\n", __FUNCTION__, __LINE__);
+			break;
+		}
+		udelay(1);
+	} while(temp != 0x3010000);
+
+	pilot_set_clkctl(host, PILOT_SDHC_CLK_400KHz, 0x0, 0x1, 0x01);
+
+	//increase the Ncr timer
+	writel(0xF0, host->ioaddr + SDHCI_RSPTMR);
+
+	//set Tac
+	writel(0x7fffffff, host->ioaddr + SDHCI_RXDTTMR);
+
+	//Change crc turnaround timer
+	writel(0x1854232, host->ioaddr + SDHCI_TXDTTMR);
+
+	//Programn LMEM registers
+	pilot_setup_lemepagecfg (host, 0x2, 0x1, 0x4, 0x80);
+
+	pilot_enable_all_interrupts(host);
+}
+
+static int pilot_send_command(struct sdhci_host * host, struct mmc_command *cmd,
+		struct mmc_request * mrq)
+{
+	struct mmc_data *data = cmd->data;
+	u8 multiblock = 0; 
+	u8 dir = 0; 
+	u8 autocmd12en = 0; 
+	u8 mode = 0; 
+	u8 cmdindex = 0; 
+	u8 datapresent = 0;
+	u8 cmdindexcheck = 0; 
+	u8 resptype = 0; 
+	u8 crcchkenable = 0;
+	u8 resp_type = mmc_resp_type(cmd);
+	u32 tempData;	
+	volatile u32 make_sure;
+	volatile int timeout = 1000000;
+
+
+	if(resp_type == MMC_RSP_NONE)
+	{
+		resptype = RESP_NONE;
+		cmdindex = cmd->opcode;
+	}
+	else if(resp_type == MMC_RSP_R1)
+	{
+		cmdindex = cmd->opcode;
+		cmdindexcheck = 1;
+		resptype = RESP_48BIT;
+		crcchkenable = 1;
+	}
+	else if(resp_type == MMC_RSP_R1B)
+	{
+		cmdindex = cmd->opcode;
+		cmdindexcheck = 1;
+		resptype = RESP_48BIT_BUSY;
+		crcchkenable = 1;
+	}
+	else if(resp_type == MMC_RSP_R2)
+	{
+		cmdindex = cmd->opcode;
+		cmdindexcheck = 0;
+		resptype = RESP_136BIT;
+		crcchkenable = 0;
+	}
+	else if(resp_type == MMC_RSP_R3)
+	{
+		cmdindex = cmd->opcode;
+		cmdindexcheck = 0;
+		resptype = RESP_48BIT;
+		crcchkenable = 0;
+	}
+#if 0
+	 /* MMC response R1,R5, R6 and R7 are same. Hence this code will be deadcode */
+	else if(resp_type == MMC_RSP_R6)
+	{
+		cmdindex = cmd->opcode;
+		cmdindexcheck = 0;
+		resptype = RESP_48BIT;
+		crcchkenable = 1;
+	}
+	else if(resp_type == MMC_RSP_R7)
+	{ 	
+		cmdindex = cmd->opcode;
+		cmdindexcheck = 0;
+		resptype = RESP_48BIT;
+		crcchkenable = 1;
+	}
+#endif
+	else
+	{
+		printk("Unknown Response type\n");
+	}
+	
+	host->expected_irqs = 0;
+	host->expected_irqs = EXPECT_CMD_INTR; 
+
+	if(data)
+	{
+		if(SD_DMA_MODE)
+		{
+			mode = 1; //dma_mode
+		}
+		else
+		{
+			mode = 0; //pio_mode
+			host->expected_irqs |= EXPECT_NO_INTR_PIO;
+		}
+		host->expected_irqs |= EXPECT_DATA_INTR;
+		datapresent = 1; //Data is present
+
+		if(data->blocks > 1)
+		{
+			autocmd12en = 1;
+			multiblock = 1;
+			host->expected_irqs |= EXPECT_AUTOCMD12_INTR;
+		}
+
+		if(data->flags & MMC_DATA_READ){
+			dir = 1;
+		}
+		else
+		{
+			dir = 0;
+			host->timer.expires = jiffies + host->timer_val;
+			host->timed_out = 0;
+			host->timer_added = 1;
+		}
+		
+		if(host->sdhc_id != EMMC)
+		{
+			if(host->bus_width == MMC_BUS_WIDTH_4)
+			{
+				tempData = readl(host->ioaddr + SDHCI_HOST_CONTROL);
+				tempData |= 0x2;
+				writel(tempData, host->ioaddr + SDHCI_HOST_CONTROL);
+			}
+			else
+			{
+				tempData = readl(host->ioaddr + SDHCI_HOST_CONTROL);
+				tempData &= ~(0x00000002);
+				writel(tempData, host->ioaddr + SDHCI_HOST_CONTROL);
+			}
+		}
+		else
+		{
+			if(host->bus_width == MMC_BUS_WIDTH_8)
+			{
+				writel(0x04, host->ioaddr + SDHCI_HOST_CONTROL);
+			}
+			else if(host->bus_width == MMC_BUS_WIDTH_4)
+			{
+				writel(0x02, host->ioaddr + SDHCI_HOST_CONTROL);
+			}
+			else
+			{
+				writel(0x00, host->ioaddr + SDHCI_HOST_CONTROL);
+			}
+		}
+	}
+
+	if (host->sdhc_id != EMMC)
+	{
+		if(host->flags == HOST_F_STOP)
+		{
+			host->cmd->error = -ETIMEDOUT;
+			host->data = NULL;
+			host->expected_irqs = 0;
+	
+			pilot_sdhc_card_reinit(host);
+	
+			tasklet_schedule(&host->finish_tasklet);
+			return -1;
+		}
+	}
+	
+	//Send the command to the hardware
+	pilot_set_cmdarg(host, cmd->arg);
+	//Change as per shaileshs mail dated 29th June 2009 to fix bug 5149
+	do
+	{
+		make_sure = readl(host->ioaddr + SDHCI_PRESENT_STATE);
+		timeout--;
+		if(timeout <= 0) 
+		{
+			printk("%s %d Condition NOT met even after wait!!! Breaking while loop!!!\n", __FUNCTION__, __LINE__);
+			break;
+		}
+		udelay(1);
+	}while((make_sure & 0x00F00000) != 0x00F00000);
+	//End Change as per shaileshs mail dated 29th June 2009 to fix bug 5149
+	pilot_set_cmdctl(host, multiblock, dir, autocmd12en, mode, cmdindex, datapresent, cmdindexcheck, resptype, crcchkenable);
+
+	return 0;
+} 
+
+static void pilot_setup_lemepagecfg (struct sdhci_host * host, u8 high_wm,
+		u8 low_wm, u16 max_num_pages, u8 page_size_dw)
+{
+	u32 tempData;
+
+	tempData = readl(host->ioaddr + SDHCI_LMEMCTL);
+	//clear all the required fields
+	tempData &= (~(LMEM_FREE_PAGE_LOW_WM));
+	tempData &= (~(LMEM_FREE_PAGE_HIGH_WM));
+
+	//store all the required values
+	tempData |= ((high_wm & LMEM_FREE_PAGE_HIGH_WM_BITS) << LMEM_FREE_PAGE_HIGH_WM_POS);
+	tempData |= ((low_wm & LMEM_FREE_PAGE_LOW_WM_BITS) << LMEM_FREE_PAGE_LOW_WM_POS);
+	writel(tempData, host->ioaddr + SDHCI_LMEMCTL);
+
+	tempData = readl(host->ioaddr + SDHCI_LMEMCFG);
+	//clear all the required fields
+	tempData &= (~(LMEM_PAGE_SIZE_DWORDS));
+	tempData &= (~(LMEM_MAX_NUM_PAGES));
+
+	//store all the required values
+	tempData |= ((max_num_pages & LMEM_MAX_NUM_PAGES_BITS) << LMEM_MAX_NUM_PAGES_POS);
+	tempData |= ((page_size_dw & LMEM_PAGE_SIZE_DWORDS_BITS) << LMEM_PAGE_SIZE_DWORDS_POS);
+	writel(tempData, host->ioaddr + SDHCI_LMEMCFG);
+}
+
+static void pilot_set_clkctl(struct sdhci_host * host, u16 fsclk_divisor,
+		u8 sdclk_polarity, u8 sdclk_enable, u8 sdclk_select)
+{
+	volatile u32 tempData;
+	volatile int timeout = 1000000;
+	tempData = readl(host->ioaddr + SDHCI_CLOCK_CONTROL);
+
+	//clear all the required fields
+	tempData &= (~(SDCLK_SELECT));
+	tempData &= (~(SDCLK_CLK_ENABLE));
+	tempData &= (~(SDCLK_POLARITY));
+	tempData &= (~(FSCLK_DIVISOR));
+
+	//store all the required values
+	tempData |= ((fsclk_divisor & FSCLK_DIVISOR_BITS)<<FSCLK_DIVISOR_POS);
+	tempData |= ((sdclk_polarity & SDCLK_POLARITY_BITS)<<SDCLK_POLARITY_POS);
+	tempData |= ((sdclk_enable & SDCLK_CLK_ENABLE_BITS)<<SDCLK_CLK_ENABLE_POS);
+	tempData |= ((sdclk_select & SDCLK_SELECT_BITS)<<SDCLK_SELECT_POS);
+
+	writel(tempData, host->ioaddr + SDHCI_CLOCK_CONTROL);
+	if(sdclk_enable == 0)
+	{
+		do
+		{
+			tempData = readl(host->ioaddr + SDHCI_CLOCK_CONTROL);
+			timeout--;
+			if(timeout <= 0) 
+			{
+				printk("%s %d Condition NOT met even after wait!!! Breaking while loop!!!\n", __FUNCTION__, __LINE__);
+				break;
+			}
+			udelay(1);
+		}while((tempData & 0x2) != 0x2); 
+	}
+}
+
+static void pilot_set_blkctl (struct sdhci_host * host, u16 blkcnt, u16 blksize)
+{
+	u32 tempData;
+
+	blkcnt &= 0x1ff;
+	blksize &=0xfff;
+	tempData = readl(host->ioaddr+ SDHCI_BLOCK_SIZE);
+	//clear all the required fields
+	tempData &= (~(BLKLEN));
+	tempData &= (~(BLKCNT));
+
+	//store all the required values
+	tempData |= ((blksize & BLKLEN_BITS) << BLKLEN_POS);
+	tempData |= ((blkcnt & BLKCNT_BITS) << BLKCNT_POS);
+
+	writel(tempData, host->ioaddr+ SDHCI_BLOCK_SIZE);
+}
+
+static int pilot_prepare_data(struct sdhci_host * host, struct mmc_data *data)
+{
+	int i;
+	u32 dma_phys_addr;
+	unsigned long timer_val=250;
+	struct scatterlist *sg = &data->sg[0];
+
+	if (data->flags & MMC_DATA_READ)
+	{
+		host->dma.dir = DMA_FROM_DEVICE;
+	}
+	else
+	{
+		host->dma.dir = DMA_TO_DEVICE;
+	}
+
+	host->dma.len = dma_map_sg(mmc_dev(host->mmc), data->sg,
+			data->sg_len, host->dma.dir);
+
+	if (host->dma.len == 0)
+		return -ETIMEDOUT;
+
+	if(data->timeout_ns == 0) //May be a sdhc card
+	{ 
+		if(data->flags & MMC_DATA_READ)
+		{
+			timer_val = 100*1000*1000;
+			writel(timer_val, host->ioaddr+ SDHCI_RXDTTMR);
+		}
+		else
+		{
+			timer_val = 250; //For write we track it using a software timer whose
+			//value is programmed in millsecs and according to the spec
+			//for writes the TAAC shoudl be 250ms fixed value
+			host->timer_val = msecs_to_jiffies(timer_val);
+		}
+	}
+	else
+	{
+		if(data->flags & MMC_DATA_WRITE)
+		{
+			timer_val = (usecs_to_jiffies(data->timeout_ns/1000));
+			timer_val *= data->blocks;
+			host->timer_val = timer_val;
+		}
+		else
+		{
+			switch (host->clock)
+			{
+				case PILOT_SDHC_CLK_50MHz:
+					//DIV factor is 20 ns
+					timer_val = (data->timeout_ns/20);
+					break;
+				case PILOT_SDHC_CLK_25MHz: 
+				case PILOT_SDHC_CLK_20MHz:
+					//DIV factor is 40 ns
+					timer_val = (data->timeout_ns/40);
+					break;
+				case PILOT_SDHC_CLK_400KHz:
+					//DIV factor is 2500 ns
+					timer_val = (data->timeout_ns/2500);
+					break;
+				default:
+					printk("Unknown clock not programming RXDTTMR register\n");
+					break;
+			}
+			writel(timer_val, host->ioaddr+ SDHCI_RXDTTMR);
+		}
+	}
+
+	dma_phys_addr = sg_dma_address(sg);
+
+	if(SD_DMA_MODE)
+	{
+		if(host->dma.len > 64){
+			printk(KERN_WARNING "list greater than 64 %d \n", host->dma.len);
+		}
+
+		if (!(data->flags & MMC_DATA_READ))
+		{
+			int i;
+			for(i=0; i<host->dma.len; i++)
+			{
+				struct scatterlist *sg = &data->sg[i];
+				gstruc[host->id]->data_addr = sg_dma_address(sg);
+				if((gstruc[host->id]->data_addr & 0xFF) != 0x00)
+				{
+					printk(KERN_WARNING "Write Address is not 256 byte alligned for SD%d dmalen %d data_addr %x\n", host->id, host->dma.len, (unsigned int)gstruc[host->id]->data_addr);
+				}
+
+				gstruc[host->id]->len = sg->length-1;
+				gstruc[host->id]->crc_en = 0;
+				gstruc[host->id]->dir = 1;
+				gstruc[host->id]->chain = 1;
+				gstruc[host->id]->int_enb = 0;
+				gstruc[host->id]++;
+			}
+			gstruc[host->id]--;
+			gstruc[host->id]->chain=0;
+			writel(dma_addr[host->id], host->ioaddr + SDHCI_DMA_ADDRESS);//Trigger DMA
+			gstruc[host->id] = astru[host->id];
+		}
+
+		if ((host->data->flags & MMC_DATA_READ))
+		{
+			for(i = 0; i< host->dma.len; i++)
+			{
+				struct scatterlist *sg = &host->data->sg[i];
+				gstruc[host->id]->data_addr = sg_dma_address(sg);
+				if((gstruc[host->id]->data_addr & 0xFF) != 0x00)
+				{
+					printk(KERN_WARNING "Read Address is not 256 byte alligned for SD%d dmalen %d data_addr %x\n", host->id, host->dma.len, (unsigned int)gstruc[host->id]->data_addr);
+				}
+				gstruc[host->id]->len = sg->length-1;
+				gstruc[host->id]->dir = 0;
+				gstruc[host->id]->crc_en = 0;
+				gstruc[host->id]->int_enb = 0;
+				gstruc[host->id]->chain = 1;
+				gstruc[host->id]++;
+			}
+			gstruc[host->id]--;
+			gstruc[host->id]->chain=0;
+			writel(dma_addr[host->id], host->ioaddr + SDHCI_DMA_ADDRESS);//Trigger DMA
+			gstruc[host->id] = astru[host->id];
+		}
+		pilot_set_blkctl (host, data->blocks, (data->blksz));
+	}
+	else
+	{
+		pilot_set_blkctl (host, data->blocks, (data->blksz));
+	}
+	return 0;
+}
+
+static void pilot_load_pio_data_task (struct sdhci_host * host, u16 blklen,
+		u32 hostAddr, u32 LMEMAddr)
+{
+	u16 tmp_blklen;
+	u16 tmp_blklen_cnt;
+	u32 hostData;
+	u8 hostDataByte;
+	u32 tempAddr;
+	u32 tempData;
+	u32 LMEMFullAddr;
+	u32 lmem_start_addr = SDHC0_LMEM_START_ADDR;
+
+	if(host->sdhc_id == SDHC0)
+		lmem_start_addr = SDHC0_LMEM_START_ADDR;
+	else if (host->sdhc_id ==  SDHC1)
+		lmem_start_addr = SDHC1_LMEM_START_ADDR;
+	else 
+		lmem_start_addr = EMMC_LMEM_START_ADDR;
+
+	tempData = ((lmem_start_addr) & (0x00000fff));
+	LMEMFullAddr = (u32)(host->lmem_v + (LMEMAddr - tempData));
+
+
+	tmp_blklen = (blklen >> 2);
+	tmp_blklen_cnt = 0;
+
+	while (tmp_blklen_cnt < tmp_blklen) 
+	{
+		tempAddr = (LMEMFullAddr + (tmp_blklen_cnt << 2) );
+		tempAddr = (hostAddr + (tmp_blklen_cnt << 2));
+
+		hostData = 0;
+		hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0));
+		hostData |= (hostDataByte << 0);
+		hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 1));
+		hostData |= (hostDataByte << 8);
+		hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 2));
+		hostData |= (hostDataByte << 16);
+		hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 3));
+		hostData |= (hostDataByte << 24);
+
+		*((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2))) = hostData;
+
+		tmp_blklen_cnt += 1;
+	}
+
+	if ((blklen & 0x3)) 
+	{
+
+		tempAddr = (LMEMFullAddr + (tmp_blklen_cnt << 2));
+		tempAddr = (hostAddr + (tmp_blklen_cnt << 2));
+
+		hostData = 0;
+		switch((blklen & 0x3))
+		{
+			case 1:
+				hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0));
+				hostData |= (hostDataByte << 0);
+				*((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2))) = hostData;
+				break;
+			case 2:
+				hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0));
+				hostData |= (hostDataByte << 0);
+				hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 1));
+				hostData |= (hostDataByte << 8);
+				*((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2))) = hostData;
+				break;
+			case 3:
+				hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0));
+				hostData |= (hostDataByte << 0);
+				hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 1));
+				hostData |= (hostDataByte << 8);
+				hostDataByte = *((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 2));
+				hostData |= (hostDataByte << 16);
+				*((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2))) = hostData;
+				break;
+			default:
+				printk("Can't reach here in load_pio_data_task\n");
+				break;
+		}
+	}
+}
+
+static u16 pilot_check_lmem_pgavailable(struct sdhci_host * host)
+{
+	volatile u32 tempData; 
+	volatile int timeout = 1000000;
+	tempData = LMEM_ALL_PAGES_ALLOCATED;
+
+	while ((tempData & LMEM_ALL_PAGES_ALLOCATED) != 0x00000000) 
+	{
+		tempData = readl(host->ioaddr + SDHCI_LMEMSTATUS);
+		timeout--;
+		if(timeout <= 0) 
+		{
+			printk("%s %d Condition NOT met even after wait!!! Breaking while loop!!!\n", __FUNCTION__, __LINE__);
+			break;
+		}
+		udelay(1);
+	}
+
+	tempData = readl(host->ioaddr + SDHCI_LMEMADDR);
+
+	tempData &= LMEM_FREE_PTR;
+	tempData = tempData >> LMEM_FREE_PTR_POS;
+	return(tempData & LMEM_FREE_PTR_BITS);
+}
+
+static u16 pilot_get_lmem_baseaddr (void) 
+{
+
+	return (0x400);
+}
+
+static u8 pilot_get_lmem_pgsize (struct sdhci_host * host) 
+{
+	volatile u32 tempData;
+
+	tempData = readl(host->ioaddr + SDHCI_LMEMCFG);
+	tempData &= LMEM_PAGE_SIZE_DWORDS;
+	tempData = tempData >> LMEM_PAGE_SIZE_DWORDS_POS;
+
+	return(tempData & LMEM_PAGE_SIZE_DWORDS_BITS);
+}
+
+static u16 pilot_get_lmem_limitaddr (void) 
+{
+
+	return (0xBFF);
+}
+
+static u8 pilot_check_rxdtrdy (struct sdhci_host * host) 
+{
+	volatile u32 tempData;
+	volatile u32 readData;
+	volatile u32 vldEmpty;
+	volatile int timeout = 1000000;
+
+	tempData  =  readl(host->ioaddr + SDHCI_PRESENT_STATE);
+	readData  = readl(host->ioaddr + SDHCI_LTXSTS);
+	vldEmpty  = ((readData & LMEM_PG_VALID_EMPTY) >> LMEM_PG_VALID_EMPTY_POS);
+
+	//Here first check if the "Write transfer bit" is set in the PRSNTST_REG 
+	//and also see if the VALID_EMPTY is set in LTXSTS_REG
+	while (((tempData & 0x00000800) != 0x00000800) && (vldEmpty == 1))
+	{
+		tempData  = readl(host->ioaddr + SDHCI_PRESENT_STATE);
+		readData  = readl(host->ioaddr + SDHCI_LTXSTS);
+		vldEmpty  = ((readData & LMEM_PG_VALID_EMPTY) >> LMEM_PG_VALID_EMPTY_POS);
+		timeout--;
+		if(timeout <= 0) 
+		{
+			printk("%s %d Condition NOT met even after wait!!! Breaking while loop!!!\n", __FUNCTION__, __LINE__);
+			break;
+		}
+		udelay(1);
+	}
+	
+	return(0);
+}
+
+static u16 pilot_get_lmem_pgvalidcnt (struct sdhci_host * host)
+{
+	u32 tempData;
+	tempData = readl(host->ioaddr + SDHCI_LMEMCNT);
+	tempData &= LMEM_VALID_DATA_PAGES_CNT;
+	tempData = tempData >> LMEM_VALID_DATA_PAGES_CNT_POS;
+	return(tempData & LMEM_VALID_DATA_PAGES_CNT_BITS);
+}
+
+static u16 pilot_get_lmem_pgcompletionptr (struct sdhci_host * host)
+{
+	u32 tempData;
+	tempData = readl(host->ioaddr + SDHCI_LMEMSTATUS);
+	tempData &= LMEM_COMPLETION_PTR;
+	tempData = tempData >> LMEM_COMPLETION_PTR_POS;
+	return(tempData & LMEM_COMPLETION_PTR_BITS);
+}
+
+static void pilot_store_pio_data_task (struct sdhci_host * host, u16 blklen,
+		u32 hostAddr, u16 LMEMAddr)
+{
+	volatile u16 tmp_blklen;
+	volatile u16 tmp_blklen_cnt;
+	volatile u32 tempData;
+	volatile u32 hostData;
+
+	volatile u32 LMEMFullAddr;
+
+	tempData = ((host->lmem_p) & (0x00000fff));
+	LMEMFullAddr = (u32)((host->lmem_v + (LMEMAddr - tempData)));
+
+	//Dwords
+	tmp_blklen = (blklen >> 2);
+	tmp_blklen_cnt = 0;
+	while (tmp_blklen_cnt < tmp_blklen)
+	{
+		hostData = *((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2)));
+
+		*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0)) = (hostData & (0xff));
+		hostData = (hostData >> 8);
+		*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 1)) = (hostData & (0xff));
+		hostData = (hostData >> 8);
+		*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 2)) = (hostData & (0xff));
+		hostData = (hostData >> 8);
+		*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 3)) = (hostData & (0xff));
+		hostData = (hostData >> 8);
+		tmp_blklen_cnt += 1;
+	}
+	if ((blklen & 0x3))
+	{
+		switch((blklen & 0x3))
+		{
+			case 1:
+				hostData = *((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2)));
+				hostData &= 0xff;
+
+				*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0)) = (hostData & (0xff));
+				hostData = (hostData >> 8);
+				break;
+			case 2:
+				hostData = *((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2)));
+				hostData &= 0xffff;
+
+				*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0)) = (hostData & (0xff));
+				hostData = (hostData >> 8);
+				*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 1)) = (hostData & (0xff));
+				hostData = (hostData >> 8);
+				break;
+			case 3:
+				hostData = *((volatile u32*)(LMEMFullAddr + (tmp_blklen_cnt << 2)));
+				hostData &= 0xffffff;
+
+				*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 0)) = (hostData & (0xff));
+				hostData = (hostData >> 8);
+				*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 1)) = (hostData & (0xff));
+				hostData = (hostData >> 8);
+				*((volatile u8*)(hostAddr + (tmp_blklen_cnt << 2) + 2)) = (hostData & (0xff));
+				hostData = (hostData >> 8);
+				break;
+			default:
+				break;
+		}
+	}
+}
+
+static void pilot_clear_rxdtrdy (struct sdhci_host * host)
+{
+	u32 tempData;
+
+	tempData = readl(host->ioaddr + SDHCI_PRESENT_STATE);
+	tempData = tempData | ((0x1) << 11);
+	writel(tempData, host->ioaddr + SDHCI_PRESENT_STATE);
+}
+
+static int pilot_do_pio_read(struct sdhci_host * host)
+{
+	u16 tmp_blkcnt;
+	u32 hostmemAddr;
+	u16 LMEMPGValidCnt;
+	u16 LMEMCompPtr;
+	u16 tmp_LMEMPGValidCnt;
+	u16 rollover_LMEMPGValidCnt;
+	u16 LMEMBaseAddr;
+	u16 LMEMLimitAddr;
+	u16 LMEMAddr;
+	u8 LMEMPGSize;
+	u16 new_blkcnt;
+	u16 new_blklen;
+	u8  rxDtResult;
+	u32 starthostmemAddr;
+
+	struct scatterlist *sg = &host->data->sg[0];
+	u16 blkcnt = host->data->blocks;
+	u16 blklen = host->data->blksz;
+
+	//supports for 4K bytes block support
+	if ((blkcnt & 0x1ff) == 0) {
+		new_blkcnt = 0x200;
+	} else {
+		new_blkcnt = (blkcnt & 0x1ff);
+	}
+
+	if ((blklen & 0xfff) == 0) {
+		new_blklen = 0x1000;
+	} else {
+		new_blklen = (blklen & 0xfff);
+	}
+
+	hostmemAddr = (u32)(page_address(sg_page(sg)) + sg->offset);
+	starthostmemAddr = hostmemAddr;
+	tmp_blkcnt = 0;
+
+	LMEMBaseAddr = pilot_get_lmem_baseaddr();
+	LMEMLimitAddr = pilot_get_lmem_limitaddr();
+	LMEMPGSize = pilot_get_lmem_pgsize(host);
+
+	while (tmp_blkcnt < new_blkcnt) 
+	{
+		rollover_LMEMPGValidCnt = 0;
+
+		//Check Rx Data Ready
+		rxDtResult = pilot_check_rxdtrdy(host);
+		if (rxDtResult != 0)
+		{
+			printk("Pio data read fail\n");
+			return -1;
+		}
+		LMEMPGValidCnt = pilot_get_lmem_pgvalidcnt(host);
+		if(LMEMPGValidCnt == 0)
+		{
+			printk("In %s RxDtis ready but count is ZERO\n", __FUNCTION__);
+		}
+		LMEMCompPtr = pilot_get_lmem_pgcompletionptr(host);
+		tmp_LMEMPGValidCnt = 0;
+		while (tmp_LMEMPGValidCnt < LMEMPGValidCnt)
+		{
+			if (rollover_LMEMPGValidCnt == 0)
+			{
+				if ((LMEMBaseAddr + ((LMEMCompPtr + (tmp_LMEMPGValidCnt * LMEMPGSize)) << 2)) > LMEMLimitAddr) {
+					rollover_LMEMPGValidCnt = tmp_LMEMPGValidCnt;
+					LMEMAddr = LMEMBaseAddr;
+				}
+				else
+				{
+					LMEMAddr = LMEMBaseAddr + ((LMEMCompPtr + (tmp_LMEMPGValidCnt * LMEMPGSize)) << 2);
+				}
+			}
+			else
+			{
+				if ((LMEMBaseAddr + (((tmp_LMEMPGValidCnt - rollover_LMEMPGValidCnt) * LMEMPGSize) << 2)) > LMEMLimitAddr){
+					rollover_LMEMPGValidCnt = tmp_LMEMPGValidCnt;
+					LMEMAddr = LMEMBaseAddr;
+				}
+				else
+				{
+					LMEMAddr = LMEMBaseAddr + (((tmp_LMEMPGValidCnt - rollover_LMEMPGValidCnt) * LMEMPGSize) << 2);
+				}
+			}
+
+			pilot_store_pio_data_task(host, new_blklen, hostmemAddr, LMEMAddr);
+
+			hostmemAddr += new_blklen;
+			tmp_LMEMPGValidCnt = tmp_LMEMPGValidCnt + 1;
+			tmp_blkcnt = tmp_blkcnt + 1;
+			pilot_clear_rxdtrdy(host);
+		} 
+	}
+	tmp_blkcnt = 0;
+
+	__cpuc_flush_dcache_area((void *)starthostmemAddr,  (new_blklen * new_blkcnt));
+	return 0;
+}
+
+static void pilot_set_txdtrdy (struct sdhci_host * host) 
+{
+	volatile u32 tempData;
+
+	tempData = readl(host->ioaddr + SDHCI_PRESENT_STATE);
+	tempData = tempData | ((0x1) << 10);
+	writel(tempData, host->ioaddr + SDHCI_PRESENT_STATE);
+}
+
+static int pilot_do_pio_write(struct sdhci_host * host)
+{
+	u16 new_blkcnt = host->data->blocks;
+	u16 new_blklen = host->data->blksz;
+	unsigned char * sg_ptr;
+	struct scatterlist *sg = &host->data->sg[0];
+	u16 AllocIndex;
+	u16 tmp_blkcnt=0;
+	u16 LMEMAddr;
+	u16 LMEMBaseAddr;
+	u8  LMEMPGSize;
+	u32 sg_len;
+	
+	//supports 4K blocks in the hardware
+	if ((new_blkcnt & 0x1ff) == 0) {
+		new_blkcnt = 0x200;
+	} else {
+		new_blkcnt &= 0x1ff;
+	}
+
+	if ((new_blklen & 0xfff) == 0) {
+		new_blklen = 0x1000;
+	} else {
+		new_blklen &= 0xfff;
+	}
+
+	sg_ptr = page_address(sg_page(sg)) + sg->offset;
+	sg_len = sg_dma_len(sg);
+	tmp_blkcnt = 0;
+	LMEMBaseAddr = pilot_get_lmem_baseaddr();
+	LMEMPGSize = pilot_get_lmem_pgsize(host);
+
+	while (tmp_blkcnt < new_blkcnt) {
+		AllocIndex = pilot_check_lmem_pgavailable(host);
+		LMEMAddr = (LMEMBaseAddr + (AllocIndex << 2));
+
+		//SetupPIOData
+		pilot_load_pio_data_task(host, new_blklen, (u32)sg_ptr, LMEMAddr);
+
+		//Set Tx Data Ready
+		pilot_set_txdtrdy(host);
+		sg_ptr += new_blklen;
+		tmp_blkcnt = tmp_blkcnt + 1;
+	}
+	tmp_blkcnt = 0;
+
+	return 0;
+}
+
+static void pilot_enable_all_interrupts(struct sdhci_host * host)
+{
+	writel(0x1ffffff, host->ioaddr + SDHCI_SIGNAL_ENABLE);
+	writel(0x1ffffff, host->ioaddr + SDHCI_INT_ENABLE);
+}
+
+static void pilot_disable_all_interrupts(struct sdhci_host * host)
+{
+	writel(0x0, host->ioaddr + SDHCI_SIGNAL_ENABLE);
+	writel(0x0, host->ioaddr + SDHCI_INT_ENABLE);
+}
+
+static void pilot_sdhc_request(struct mmc_host* mmc, struct mmc_request* mrq)
+{
+	unsigned long flags;
+	struct sdhci_host * host = mmc_priv(mmc);
+	u32 int_status = 0;
+
+	host->mrq = mrq;
+	host->cmd = mrq->cmd;
+	host->data = mrq->data;
+	host->stop = mrq->stop;
+
+	if (host->sdhc_id == EMMC)
+	{
+		//Check if card is present
+		if(host->flags == HOST_F_STOP)
+		{
+			host->cmd->error = -ETIMEDOUT;
+			host->data = NULL;
+			tasklet_schedule(&host->finish_tasklet);
+			return;
+		}
+	}
+
+	//Check if previous command is in progress
+	if(host->mrq->data)
+	{
+		if (host->data->flags & MMC_DATA_STREAM)
+		{
+			host->cmd->error = -EINVAL;
+			host->data = NULL;
+			tasklet_schedule(&host->finish_tasklet);
+			return;
+		}
+
+		int_status = 1;
+		host->data->error =  PILOT_SDHC_ERR_NONE;
+		pilot_prepare_data(host, mrq->data);
+	}
+
+	spin_lock_irqsave(&host->lock, flags);
+
+	if(host->mrq->cmd)
+	{
+		host->cmd->error = PILOT_SDHC_ERR_NONE;
+		pilot_send_command(host, host->mrq->cmd, mrq);
+	}
+
+	//Check if we are in PIO mode 
+	if((!SD_DMA_MODE) && (int_status))
+	{
+		if(host->mrq->data)
+		{
+			if(host->mrq->data->flags & MMC_DATA_READ){
+				pilot_do_pio_read(host);
+			}else{
+				pilot_do_pio_write(host);
+			}
+		}
+	}
+	spin_unlock_irqrestore(&host->lock, flags);
+}
+
+static void pilot_sdhc_set_ios(struct mmc_host* mmc, struct mmc_ios* ios)
+{
+	struct sdhci_host * host = mmc_priv(mmc);
+	u16 clk = PILOT_SDHC_CLK_400KHz;
+
+	spin_lock_bh(&host->lock);
+
+	//Find out if there is a way to power on and reset our device, or is it done automatically when system is restarted
+	if (ios->clock >= 50000000)
+	{
+		//Read the current clock value
+		clk = ((readl(host->ioaddr + SDHCI_CLOCK_CONTROL) & FSCLK_DIVISOR_BITS) >> FSCLK_DIVISOR_POS);
+		pilot_set_clkctl(host, PILOT_SDHC_CLK_400KHz, 0x0, 0x0, 0x01);
+
+		//set Tac
+		writel(0x7fffffff, host->ioaddr + SDHCI_RXDTTMR);
+		//Change crc turnaround timer
+		if(txdttmr_flg)
+		{
+			writel(0x1844242, host->ioaddr + SDHCI_TXDTTMR);
+		}
+		else
+		{
+			writel(0x1854232, host->ioaddr + SDHCI_TXDTTMR);
+		}
+		clk = PILOT_SDHC_CLK_50MHz;
+
+		pilot_set_clkctl(host, clk, 0x0, 0x0, 0x01);
+		
+		host->clock = PILOT_SDHC_CLK_50MHz;
+		//0x40c00084 = 0xc0c0_0000; //Only in 50 Mhz Mode
+		writel(0xc0c00000, host->ioaddr + SDHCI_FSBLK_CTRL);
+	}
+	else if (ios->clock >= 25000000)
+	{
+		//Read the current clock value
+		clk = ((readl(host->ioaddr + SDHCI_CLOCK_CONTROL) & FSCLK_DIVISOR_BITS) >> FSCLK_DIVISOR_POS);
+		pilot_set_clkctl(host, PILOT_SDHC_CLK_400KHz, 0x0, 0x0, 0x01);
+		
+		//set Tac
+		writel(0x7fffffff, host->ioaddr+ SDHCI_RXDTTMR);
+		//Change crc turnaround timer
+		writel(0x1854232, host->ioaddr+ SDHCI_TXDTTMR);
+		clk = PILOT_SDHC_CLK_25MHz;
+
+		pilot_set_clkctl(host, clk, 0x0, 0x0, 0x01);
+		
+		host->clock = PILOT_SDHC_CLK_25MHz;
+		//0x40c00084 = 0xc0c0_0000; //Only in 50 Mhz Mode
+		writel(0x00000, host->ioaddr+ SDHCI_FSBLK_CTRL);
+	}
+	else if( ios->clock >= 20000000)
+	{
+		//Read the current clock value
+		clk = ((readl(host->ioaddr + SDHCI_CLOCK_CONTROL) & FSCLK_DIVISOR_BITS) >> FSCLK_DIVISOR_POS);
+		pilot_set_clkctl(host, PILOT_SDHC_CLK_400KHz, 0x0, 0x0, 0x01);
+
+		//set Tac
+		writel(0x7fffffff, host->ioaddr+ SDHCI_RXDTTMR);
+		//Change crc turnaround timer
+		writel(0x1854232, host->ioaddr+ SDHCI_TXDTTMR);
+		clk = PILOT_SDHC_CLK_20MHz;
+
+		pilot_set_clkctl(host, clk, 0x0, 0x0, 0x01);
+		
+		host->clock = PILOT_SDHC_CLK_20MHz;
+		//0x40c00084 = 0xc0c0_0000; //Only in 50 Mhz Mode
+		writel(0x00000, host->ioaddr+ SDHCI_FSBLK_CTRL);
+	}
+	else if (ios->clock == 0) 
+	{
+		//Read the current clock value
+		clk = ((readl(host->ioaddr + SDHCI_CLOCK_CONTROL) & FSCLK_DIVISOR_BITS) >> FSCLK_DIVISOR_POS);
+		pilot_set_clkctl(host, clk, 0x0, 0x0, 0x01);
+		
+		//set Tac
+		writel(0x7fffffff, host->ioaddr+ SDHCI_RXDTTMR);
+		//Change crc turnaround timer
+		writel(0x1854232, host->ioaddr+ SDHCI_TXDTTMR);
+		clk = 0;
+		
+		pilot_set_clkctl(host, clk, 0x0, 0x0, 0x01);
+		
+		host->clock = 0;
+		//0x40c00084 = 0xc0c0_0000; //Only in 50 Mhz Mode
+		writel(0x00000, host->ioaddr+ SDHCI_FSBLK_CTRL);
+		
+		host->bus_width = ios->bus_width;
+		spin_unlock_bh(&host->lock);
+		return;
+	}
+	else 
+	{
+		//Read the current clock value
+		clk = (((*((volatile u32*)(host->ioaddr + SDHCI_CLOCK_CONTROL))) & FSCLK_DIVISOR_BITS) >> FSCLK_DIVISOR_POS);
+		pilot_set_clkctl(host, clk, 0x0, 0x0, 0x01);
+		
+		//set Tac
+		writel(0x7fffffff, host->ioaddr+ SDHCI_RXDTTMR);
+		//Change crc turnaround timer
+		writel(0x1854232, host->ioaddr+ SDHCI_TXDTTMR);
+		clk = PILOT_SDHC_CLK_400KHz;
+		pilot_set_clkctl(host, clk, 0x0, 0x0, 0x01);
+		
+		host->clock = PILOT_SDHC_CLK_400KHz;
+		//0x40c00084 = 0xc0c0_0000; //Only in 50 Mhz Mode
+		writel(0x00000, host->ioaddr+ SDHCI_FSBLK_CTRL);
+	}
+
+	pilot_set_clkctl(host, clk, 0x0, 0x1, 0x01);
+	host->bus_width = ios->bus_width;
+	spin_unlock_bh(&host->lock);
+
+}
+
+static int pilot_sdhc_get_ro(struct mmc_host *mmc)
+{
+	struct sdhci_host * host = mmc_priv(mmc);
+	u32 tempData;
+	//Here read the Write protected bit in the PRSNTSTS register and return 1 if WP else 0
+	tempData = readl(host->ioaddr + SDHCI_PRESENT_STATE);
+	if((tempData & WRITE_PROTECT_PIN_STATUS) == WRITE_PROTECT_PIN_STATUS){
+		return 1;
+	}
+	return 0;
+}
+
+static void pilot_tasklet_card(unsigned long param)
+{
+	struct sdhci_host *host = (struct sdhci_host *) param;
+	int delay = 0;
+
+	if(host->flags == HOST_F_STOP){
+		//delay = 500;
+	}
+	mmc_detect_change(host->mmc, msecs_to_jiffies(delay));	
+}
+
+static void pilot_tasklet_finish (unsigned long param)
+{
+	struct sdhci_host *host = (struct sdhci_host *) param;
+	struct mmc_request * mrq = NULL;
+	unsigned long flags;
+
+	if(host->timer_added)
+	{
+		host->timer_added = 0;
+	}
+	spin_lock_irqsave(&host->lock, flags);
+	if(host->cmd == NULL)
+	{
+		//Someone else has completed this request 
+		spin_unlock_irqrestore(&host->lock, flags);
+		return;
+	}
+
+	host->cmd->resp[0] = readl(host->ioaddr + SDHCI_RESPONSE);
+	host->cmd->resp[1] = readl(host->ioaddr + SDHCI_RESPONSE + 0x04);
+	host->cmd->resp[2] = readl(host->ioaddr + SDHCI_RESPONSE + 0x08);
+	host->cmd->resp[3] = readl(host->ioaddr + SDHCI_RESPONSE + 0x0C);
+
+	if(mmc_resp_type(host->cmd) == MMC_RSP_R2)
+	{
+		unsigned char tmp_buf[15] = {0};
+		tmp_buf[14] = *(((unsigned char *)host->cmd->resp)+1);
+		tmp_buf[13] = *(((unsigned char *)host->cmd->resp)+2);
+		tmp_buf[12] = *(((unsigned char *)host->cmd->resp)+3);
+		tmp_buf[11] = *(((unsigned char *)host->cmd->resp)+4);
+		tmp_buf[10] = *(((unsigned char *)host->cmd->resp)+5);
+		tmp_buf[9] = *(((unsigned char *)host->cmd->resp)+6);
+		tmp_buf[8] = *(((unsigned char *)host->cmd->resp)+7);
+		tmp_buf[7] = *(((unsigned char *)host->cmd->resp)+8);
+		tmp_buf[6] = *(((unsigned char *)host->cmd->resp)+9);
+		tmp_buf[5] = *(((unsigned char *)host->cmd->resp)+10);
+		tmp_buf[4] = *(((unsigned char *)host->cmd->resp)+11);
+		tmp_buf[3] = *(((unsigned char *)host->cmd->resp)+12);
+		tmp_buf[2] = *(((unsigned char *)host->cmd->resp)+13);
+		tmp_buf[1] = *(((unsigned char *)host->cmd->resp)+14);
+		tmp_buf[0] = *(((unsigned char *)host->cmd->resp)+15);
+
+		if(pilot_mmc_crc7(tmp_buf, 15) != (((volatile unsigned char *) host->cmd->resp)[0]))
+			host->cmd->error = -EILSEQ;
+		host->cmd->resp[0] = readl(host->ioaddr + SDHCI_RESPONSE + 0x0C);
+		host->cmd->resp[1] = readl(host->ioaddr + SDHCI_RESPONSE + 0x08);
+		host->cmd->resp[2] = readl(host->ioaddr + SDHCI_RESPONSE + 0x04);
+		host->cmd->resp[3] = readl(host->ioaddr + SDHCI_RESPONSE);
+
+	}
+	else if(mmc_resp_type(host->cmd) == MMC_RSP_NONE){
+		//Dont care for this 
+	}
+	else
+	{
+		host->cmd->resp[0] = host->cmd->resp[1];
+		host->cmd->resp[1] = host->cmd->resp[2];
+		host->cmd->resp[2] = host->cmd->resp[3];
+	}
+
+	if(host->data)
+	{
+		if(!SD_DMA_MODE)
+		{
+			u32 lmem_status = *((u32 *) (host->ioaddr + SDHCI_LMEMSTATUS));
+			if((lmem_status & (1<< 23)) == (1<< 23)){
+				printk(KERN_ERR "LMEM STATUS Bit 23 is 1");
+			}
+			if((lmem_status & (1<< 22)) == (1<< 22)){
+				printk(KERN_ERR "LMEM STATUS Bit 22 is 1");
+			}
+			if((lmem_status & (1<< 21)) == (1<< 21)){
+				printk(KERN_ERR "LMEM STATUS Bit 21 is 1");
+			}
+			if((lmem_status & (1<< 20)) != (1<< 20)){
+				printk(KERN_ERR "LMEM STATUS Bit 20 is 0");
+			}
+			if((lmem_status & (1<< 19)) == (1<< 19)){
+				printk(KERN_ERR "LMEM STATUS Bit 19 is 1");
+			}
+			if((lmem_status & (1<< 18)) == (1<< 18)){
+				printk(KERN_ERR "LMEM STATUS Bit 18 is 1");
+			}
+			if((lmem_status & (1<< 17)) == (1<< 17)){
+				printk(KERN_ERR "LMEM STATUS Bit 17 is 1");
+			}
+			if((lmem_status & (1<< 16)) != (1<< 16)){
+				printk(KERN_ERR "LMEM STATUS Bit 16 is 0");
+			}
+		}
+
+		dma_unmap_sg(mmc_dev(host->mmc), host->data->sg, host->data->sg_len, host->dma.dir);
+	}
+
+	host->cmd = NULL;
+	host->data = NULL;
+	mrq = host->mrq;
+	host->mrq = NULL;
+	spin_unlock_irqrestore(&host->lock, flags);
+	mmc_request_done(host->mmc, mrq);
+}
+
+static void pilot_access_timer_event(unsigned long arg)
+{
+	struct sdhci_host * host = (struct sdhci_host *) arg;
+
+	if(host->cmd == NULL)
+		return;
+	host->cmd->error = -ETIMEDOUT;
+	host->timed_out = 1;
+	pilot_tasklet_finish((unsigned long)host);
+}
+
+struct mmc_host_ops pilot_mmc_host_ops = {
+	.request = pilot_sdhc_request,
+	.set_ios = pilot_sdhc_set_ios,
+	.get_ro	= pilot_sdhc_get_ro,
+};
+
+static inline irqreturn_t sdhc_isr(int irq, void *dev_id)
+{
+	u32 int_status = 0;
+	struct sdhci_host *host = (struct sdhci_host *)dev_id;
+
+	int_status = readl(host->ioaddr + SDHCI_INT_STATUS);
+
+	if((int_status & PILOT_SDHC_MMC_BOOT_ACK) == PILOT_SDHC_MMC_BOOT_ACK)
+	{
+		writel(PILOT_SDHC_MMC_BOOT_ACK, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_WRITE_CMD_RSP_ERR) == PILOT_SDHC_WRITE_CMD_RSP_ERR)
+	{
+		writel(PILOT_SDHC_WRITE_CMD_RSP_ERR, host->ioaddr + SDHCI_INT_STATUS);
+		if(host->cmd == NULL)
+			goto ack_intr;
+		host->cmd->error = -ETIMEDOUT;
+	}
+
+	if((int_status & PILOT_SDHC_XDMA_COMPL_ERR) == PILOT_SDHC_XDMA_COMPL_ERR)
+	{
+		writel(PILOT_SDHC_XDMA_COMPL_ERR, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_MMC_BOOT_COMPL) == PILOT_SDHC_MMC_BOOT_COMPL)
+	{
+		writel(PILOT_SDHC_MMC_BOOT_COMPL, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_AUTO_CMD12_COMPL) == PILOT_SDHC_AUTO_CMD12_COMPL)
+	{
+		writel(PILOT_SDHC_AUTO_CMD12_COMPL, host->ioaddr + SDHCI_INT_STATUS);
+
+		host->expected_irqs &= ~(EXPECT_AUTOCMD12_INTR);
+		if(host->cmd == NULL)
+			goto ack_intr;
+
+		if(host->expected_irqs == 0){
+			tasklet_schedule(&host->finish_tasklet);
+		}
+	}
+
+	if((int_status & PILOT_SDHC_LIMIT_ERR) == PILOT_SDHC_LIMIT_ERR)
+	{
+		writel(PILOT_SDHC_LIMIT_ERR, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_DATA_END_BIT_ERR) == PILOT_SDHC_DATA_END_BIT_ERR)
+	{
+		writel(PILOT_SDHC_DATA_END_BIT_ERR, host->ioaddr + SDHCI_INT_STATUS);
+
+		if(host->cmd == NULL)
+			goto ack_intr;
+		host->cmd->error = -EILSEQ;
+		host->data->error = -EILSEQ;
+	}
+
+	if((int_status & PILOT_SDHC_DATA_CRC_ERR) == PILOT_SDHC_DATA_CRC_ERR)
+	{
+		writel(PILOT_SDHC_DATA_CRC_ERR, host->ioaddr + SDHCI_INT_STATUS);
+
+		if(host->cmd == NULL)
+			goto ack_intr;
+		host->cmd->error = -EILSEQ;
+		host->data->error = -EILSEQ;
+	}
+
+	if((int_status & PILOT_SDHC_DATA_TIMEOUT_ERR) == PILOT_SDHC_DATA_TIMEOUT_ERR)
+	{
+		writel(PILOT_SDHC_DATA_TIMEOUT_ERR, host->ioaddr + SDHCI_INT_STATUS);
+
+		if(host->cmd == NULL)
+			goto ack_intr;
+		host->cmd->error = -ETIMEDOUT;
+		host->data->error = -ETIMEDOUT;
+	}
+
+	if((int_status & PILOT_SDHC_WRITE_PROTCT_ERR) == PILOT_SDHC_WRITE_PROTCT_ERR)
+	{
+		writel(PILOT_SDHC_WRITE_PROTCT_ERR, host->ioaddr + SDHCI_INT_STATUS);
+		if(host->cmd == NULL)
+			goto ack_intr;
+		host->cmd->error = -ETIMEDOUT;
+		host->data->error = -ETIMEDOUT;
+	}
+
+	if((int_status & PILOT_SDHC_TRANSFER_COMPL) == PILOT_SDHC_TRANSFER_COMPL)
+	{
+		writel(PILOT_SDHC_TRANSFER_COMPL, host->ioaddr + SDHCI_INT_STATUS);
+		host->expected_irqs &= ~(EXPECT_DATA_INTR);
+		if(host->cmd == NULL)
+			goto ack_intr;
+
+		host->data->bytes_xfered = (host->data->blocks * (host->data->blksz));
+
+		if(host->expected_irqs == 0){
+			tasklet_schedule(&host->finish_tasklet);
+		}
+	}
+
+	if((int_status & PILOT_SDHC_BUFFER_READ_RDY) == PILOT_SDHC_BUFFER_READ_RDY)
+	{
+		writel(PILOT_SDHC_BUFFER_READ_RDY, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_BUFFER_WRITE_RDY) == PILOT_SDHC_BUFFER_WRITE_RDY)
+	{
+		writel(PILOT_SDHC_BUFFER_WRITE_RDY, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_DMA_INTERRUPT) == PILOT_SDHC_DMA_INTERRUPT)
+	{
+		writel(PILOT_SDHC_DMA_INTERRUPT, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_CARD_INTRPT) == PILOT_SDHC_CARD_INTRPT)
+	{
+		writel(PILOT_SDHC_CARD_INTRPT, host->ioaddr + SDHCI_INT_STATUS);
+	}
+
+	if((int_status & PILOT_SDHC_CARD_REMOVAL) == PILOT_SDHC_CARD_REMOVAL)
+	{
+		writel(PILOT_SDHC_CARD_REMOVAL, host->ioaddr + SDHCI_INT_STATUS);
+		host->flags = HOST_F_STOP;
+
+		tasklet_schedule(&host->card_tasklet);
+	}
+
+
+	if((int_status & PILOT_SDHC_CARD_INSERTION) == PILOT_SDHC_CARD_INSERTION)
+	{
+		writel(PILOT_SDHC_CARD_INSERTION, host->ioaddr + SDHCI_INT_STATUS);
+		host->flags = HOST_F_ACTIVE;
+
+		tasklet_schedule(&host->card_tasklet);
+	}
+
+	if((int_status & PILOT_SDHC_CMD_TIMEOUT_ERR) == PILOT_SDHC_CMD_TIMEOUT_ERR)
+	{
+		writel(PILOT_SDHC_CMD_TIMEOUT_ERR, host->ioaddr + SDHCI_INT_STATUS);
+		if(host->cmd == NULL)
+			goto ack_intr;
+		host->cmd->error = -ETIMEDOUT;
+	}
+
+	if((int_status & PILOT_SDHC_CMD_CRC_ERR) == PILOT_SDHC_CMD_CRC_ERR)
+	{
+		writel(PILOT_SDHC_CMD_CRC_ERR, host->ioaddr + SDHCI_INT_STATUS);
+		if(host->cmd == NULL)
+			goto ack_intr;
+		if(host->cmd->flags & MMC_RSP_CRC)
+			host->cmd->error = -EILSEQ;
+	}
+
+	if((int_status & PILOT_SDHC_CMD_END_BIT_ERR) == PILOT_SDHC_CMD_END_BIT_ERR)
+	{
+		writel(PILOT_SDHC_CMD_END_BIT_ERR, host->ioaddr + SDHCI_INT_STATUS);
+		if(host->cmd == NULL)
+			goto ack_intr;
+		if(host->cmd->flags & MMC_RSP_CRC)
+			host->cmd->error = -EILSEQ;
+	}
+
+	if((int_status & PILOT_SDHC_CMD_INDEX_ERR) == PILOT_SDHC_CMD_INDEX_ERR)
+	{
+		writel(PILOT_SDHC_CMD_INDEX_ERR, host->ioaddr + SDHCI_INT_STATUS);
+		if(host->cmd == NULL)
+			goto ack_intr;
+		if(host->cmd->flags & MMC_RSP_CRC)
+			host->cmd->error = -EILSEQ;
+	}
+
+	if((int_status & PILOT_SDHC_COMMAND_COMPL) == PILOT_SDHC_COMMAND_COMPL)
+	{
+		writel(PILOT_SDHC_COMMAND_COMPL, host->ioaddr + SDHCI_INT_STATUS);
+		host->expected_irqs &= ~(EXPECT_CMD_INTR);
+		if(host->expected_irqs == 0){
+			tasklet_schedule(&host->finish_tasklet);
+		}
+	}
+
+	if((SD_DMA_MODE == 0) && (host->expected_irqs == 0xFFFFFFF0)){
+		tasklet_schedule(&host->finish_tasklet);
+	}
+ack_intr:
+	return IRQ_HANDLED;
+}
+
+static irqreturn_t pilot_sdhc_irq(int irq, void *dev_id)
+{
+	return sdhc_isr(irq, dev_id);
+}
+
+static irqreturn_t pilot_sdhc1_irq(int irq, void *dev_id)
+{
+	return sdhc_isr(irq, dev_id);
+}
+
+static irqreturn_t pilot_emmc_irq(int irq, void *dev_id)
+{
+	return sdhc_isr(irq, dev_id);
+}
+#endif
+
+static struct sdhci_ops pilot_sd_ops =
+{
+        .get_max_clock = pilot_get_max_clk,
+        .get_min_clock = pilot_get_min_clk,
+        .set_clock = sdhci_pilot_set_clock,
+};
+
+
+static struct resource pilot_sdhci_resource[] = {
+#if defined(CONFIG_SOC_SE_PILOT4)
+        [0] = {
+                .start = SDHC0_BASE_ADDR,
+                .end = SDHC0_BASE_ADDR + 0x1FF,
+                .flags = IORESOURCE_MEM,
+				.name = "pilot_sdhci0",				
+        },
+        [1] = {
+                .start = IRQ_SDHC_INT,
+                .end = IRQ_SDHC_INT,
+                .flags = IORESOURCE_IRQ,
+				.name = "pilot_sdhci0",
+        },
+        [2] = {
+                .start = SDHC1_BASE_ADDR,
+                .end = SDHC1_BASE_ADDR + 0x1FF,
+                .flags = IORESOURCE_MEM,
+				.name = "pilot_sdhci1",
+        },
+        [3] = {
+                .start = IRQ_SDHC1_INT,
+                .end = IRQ_SDHC1_INT,
+                .flags = IORESOURCE_IRQ,
+				.name = "pilot_sdhci1",
+        },
+        [4] = {
+                .start = EMMC_BASE_ADDR,
+                .end = EMMC_BASE_ADDR + 0x1FF,
+                .flags = IORESOURCE_MEM,
+				.name = "pilot_emmc",
+        },
+        [5] = {
+                .start = IRQ_EMMC_INT,
+                .end = IRQ_EMMC_INT,
+                .flags = IORESOURCE_IRQ,
+				.name = "pilot_emmc",
+        },
+#else
+        [0] = {
+                .start = 0x40C00000,
+                .end = 0x40C00000 + 0xFFFFF,
+                .flags = IORESOURCE_MEM,
+        },
+        [1] = {
+                .start = (64 + 5),
+                .end = (64 + 5),
+                .flags = IORESOURCE_IRQ,
+        },
+#endif		
+};
+
+static void pilot_sdhci_release(struct device * dev)
+{
+	/* Currently not doing any operation on release */
+	return;
+}
+
+static struct platform_device pilot_sdhci_device = {
+        .name   = "pilot_sdhci",
+#if defined(CONFIG_SOC_SE_PILOT4)
+		.num_resources  = ARRAY_SIZE(pilot_sdhci_resource),
+#endif
+        .resource = pilot_sdhci_resource,
+    .dev = {
+        .release = pilot_sdhci_release,
+    }
+};
+
+static struct platform_driver pilot_sdhci_driver = {
+        .driver = {
+                .name = "pilot_sdhci",
+                .owner = THIS_MODULE,
+        },
+        #ifdef CONFIG_PM
+        .pm = &pilot_sd_pmops,
+        #endif
+        .remove = pilot_sd_remove
+};
+
+static int pilot_hw_init(struct sdhci_host *host)
+{
+#if defined(CONFIG_SOC_SE_PILOT4)
+	u32	lmem_start_addr = SDHC0_LMEM_START_ADDR;
+
+	if(SD_DMA_MODE)
+	{
+		gstruc[host->id] = (struct dma_desc *)dma_alloc_coherent(NULL,
+				sizeof(struct dma_desc) * 512, &dma_addr[host->id], GFP_KERNEL);
+
+		astru[host->id] = gstruc[host->id];
+		if(gstruc[host->id] == NULL)
+		{
+			printk("ERROR: gstruc alloc coherent failed");
+		}
+	}	
+
+	//Set clock to 400 KHz
+	pilot_set_clkctl(host, PILOT_SDHC_CLK_400KHz, 0x0, 0x1, 0x01);
+	
+	//increase the Ncr timer
+	writel(0xF0, host->ioaddr + SDHCI_RSPTMR);
+
+	//set Tac
+	writel(0x7fffffff, host->ioaddr + SDHCI_RXDTTMR);
+	//Change crc turnaround timer
+	writel(0x1854232, host->ioaddr + SDHCI_TXDTTMR);
+
+	//Programn LMEM registers
+	pilot_setup_lemepagecfg (host, 0x2, 0x1, 0x4, 0x80);
+
+	if(!SD_DMA_MODE)
+	{
+		if(host->sdhc_id == SDHC0)
+			lmem_start_addr = SDHC0_LMEM_START_ADDR;
+		else if (host->sdhc_id ==  SDHC1)
+			lmem_start_addr = SDHC1_LMEM_START_ADDR;
+		else 
+			lmem_start_addr = EMMC_LMEM_START_ADDR;
+
+		//host->lmem_p = LMEM_START_ADDRESS;
+		host->lmem_p = lmem_start_addr;
+		host->lmem_length = LMEM_LENGTH;
+		host->lmem_v = (unsigned char *) ioremap_nocache(host->lmem_p, host->lmem_length); 
+	}	
+	
+	//Initialize the write access timer
+	init_timer(&host->timer);
+	host->timer.data = (unsigned long) host;
+	host->timer.function = pilot_access_timer_event;
+	host->timer_added = 0;
+#else
+	u32 intmask;
+	u32 setval=0;
+	uint32_t reg;
+
+	host->ioaddr = ioremap(0x40C00000, SZ_1K);
+	if (!host->ioaddr) {
+		printk(KERN_ERR "failed to remap registers\n");
+		return -ENOMEM;
+	}
+
+
+	setval=0x00000480;
+	writel(setval,host->ioaddr+SDHCI_LMEMCFG);
+	setval=0x03010000;
+	writel(setval,host->ioaddr+SDHCI_LMEMCTL);
+
+	intmask = SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
+		SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
+		SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
+		SDHCI_INT_CARD_REMOVE | SDHCI_INT_CARD_INSERT |
+		SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL |
+		SDHCI_INT_DMA_END | SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE |
+		SDHCI_INT_ADMA_ERROR;
+
+	writel(intmask, host->ioaddr + SDHCI_INT_ENABLE);
+	writel(intmask, host->ioaddr + SDHCI_SIGNAL_ENABLE);
+
+
+
+	/* enable LDMA engine for LMEM */
+	reg = ioread32((void __iomem*)(SE_FGE_VA_BASE + 0x300 + 0x00));
+	reg |= 0x00000001;
+	iowrite32(reg, (void __iomem*)(SE_FGE_VA_BASE + 0x300 + 0x00));
+
+
+	setval=0x00384A88;
+	writel(setval, host->ioaddr + SDHCI_CMDTMR);
+	setval=0x00000064;
+	writel(setval, host->ioaddr + SDHCI_RSPTMR);
+
+	setval=0x01854232;
+	writel(setval, host->ioaddr + SDHCI_TXDTTMR);
+	setval=0x7FFFFFFF;
+	writel(setval, host->ioaddr + SDHCI_RXDTTMR);
+
+	writel(0,host->ioaddr+SDHCI_CLOCK_CONTROL);
+	setval=0x00000044;
+	writel(setval,host->ioaddr + SDHCI_CLOCK_CONTROL);
+
+	host->ops = &pilot_sd_ops;
+
+
+
+	host->irq = pilot_sdhci_device.resource[1].start;
+	host->hw_name = pilot_sdhci_device.name;
+
+	host->max_clk = 50 * 1000000;
+	host->flags |= SDHCI_USE_SDMA;
+	host->caps |= SDHCI_CAN_VDD_330;
+	host->timeout_clk = 0xE;
+	host->mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ | MMC_CAP_SD_HIGHSPEED ;
+	host->mmc->max_req_size = 524288;
+	host->mmc->max_seg_size = host->mmc->max_req_size;
+	host->mmc->max_blk_size = 4096;
+	host->mmc->max_blk_count = 512;
+	host->mmc->f_min = host->max_clk / 256;
+	host->mmc->f_max = host->max_clk;
+	//host->mmc->ocr_avail = MMC_VDD_32_33 | MMC_VDD_33_34;
+
+	/* Added the following quirk to inform SD host controller
+	 * not to get clock info from capability registers */
+	host->quirks |= SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN;
+	host->quirks |= SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK;
+	host->quirks |= SDHCI_QUIRK_MISSING_CAPS;
+	host->quirks |= SDHCI_QUIRK_NONSTANDARD_CLOCK;
+	host->quirks |= SDHCI_QUIRK_INVERTED_WRITE_PROTECT;
+
+	/* AMI related quirk support needed for Pilot-III soc*/
+	host->ami_quirks |= SDHCI_AMI_QUIRK_INHIBIT_ABSENT;
+	host->ami_quirks |= SDHCI_AMI_QUIRK_RSP_136_IGN_CRC_ERR;
+	host->ami_quirks |= SDHCI_AMI_QUIRK_RSP_136_ONLY;
+	host->ami_quirks |= SDHCI_AMI_QUIRK_NO_SOFTWARE_RESET;
+	host->ami_quirks |= SDHCI_AMI_QUIRK_NO_TIMEOUT_CNTL_REG;
+	host->ami_quirks |= SDHCI_AMI_QUIRK_BROKEN_BLK_CNTL_REG;
+#endif
+	
+	printk("pilot_hw_init success \n");
+
+	return 0;
+}
+
+static int pilot_sd_probe(struct platform_device *pdev)
+{
+#if defined(CONFIG_SOC_SE_PILOT4)
+	struct sdhci_host *host[NO_OF_SDHCS];
+	int ret = -EINVAL;
+	struct mmc_host *mmc[NO_OF_SDHCS];	
+	u32 iobase;
+	u32 tempData;	
+	u32 i;
+
+	for (i=0; i<NO_OF_SDHCS; i++)
+	{
+		mmc[i] = mmc_alloc_host(sizeof(struct sdhci_host), &pdev->dev);
+		if (!mmc[i])
+		{
+			printk(KERN_ERR "Cannot allocate host \n");
+		}
+
+		mmc[i]->ops = &pilot_mmc_host_ops;
+		
+		mmc[i]->f_min =   400000;
+		mmc[i]->f_max = 50000000;
+		
+		mmc[i]->max_blk_count = 511;
+		mmc[i]->max_blk_size = 4096 * 511;
+		mmc[i]->max_req_size = 4096 * 511;
+		mmc[i]->max_seg_size = mmc[i]->max_req_size;
+		mmc[i]->max_segs = 1;
+		if(SD_DMA_MODE)
+			mmc[i]->max_segs = 511;
+		if(SD_DMA_MODE==0)
+		{
+#ifndef	MULTIBLOCK_WRITE
+			mmc[i]->max_blk_size = 512;
+			mmc[i]->max_blk_count = 1;
+			mmc[i]->max_req_size = 4096;
+			mmc[i]->max_segs = 1;
+			mmc[i]->max_seg_size = mmc[i]->max_req_size;
+#else
+			mmc[i]->max_blk_size = 4096;
+			mmc[i]->max_blk_count = 512;
+			mmc[i]->max_req_size = 4096 * 512;
+#endif
+		}
+		
+		mmc[i]->ocr_avail = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31
+			| MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34;
+
+		host[i] = mmc_priv(mmc[i]);
+		
+		host[i]->ops = &pilot_sd_ops;
+		
+		if(CONFIG_SPX_FEATURE_GLOBAL_SD_SLOT_COUNT)
+		{
+			if(i == SDHC0)
+			{
+				host[i]->sdhc_id = SDHC0;
+				iobase = pilot_sdhci_device.resource[0].start;
+				host[i]->hw_name = pilot_sdhci_device.resource[0].name;
+				host[i]->irq = pilot_sdhci_device.resource[1].start;
+				ret = request_irq(host[i]->irq, pilot_sdhc_irq, IRQF_SHARED, "Pilot-SDHC0", host[i]);
+			}
+			else if((i == SDHC1) && (i < CONFIG_SPX_FEATURE_GLOBAL_SD_SLOT_COUNT))
+			{
+				host[i]->sdhc_id = SDHC1;
+				iobase = pilot_sdhci_device.resource[2].start;
+				host[i]->hw_name = pilot_sdhci_device.resource[2].name;
+				host[i]->irq = pilot_sdhci_device.resource[3].start;
+				ret = request_irq(host[i]->irq, pilot_sdhc1_irq, IRQF_SHARED, "Pilot-SDHC1", host[i]);
+			}
+		}
+		if (CONFIG_SPX_FEATURE_GLOBAL_EMMC_FLASH_COUNT)
+		{
+			if(i == CONFIG_SPX_FEATURE_GLOBAL_SD_SLOT_COUNT)
+			{
+				host[i]->sdhc_id = EMMC;
+				iobase = pilot_sdhci_device.resource[4].start;
+				host[i]->hw_name = pilot_sdhci_device.resource[4].name;
+				host[i]->irq = pilot_sdhci_device.resource[5].start;
+				ret = request_irq(host[i]->irq, pilot_emmc_irq, IRQF_SHARED, "Pilot-EMMC", host[i]);
+			}
+		}	
+		if (ret)
+		{
+			printk("ERROR: Couldn't get int %d: %d\n", host[i]->irq, ret);
+			return -ENXIO;
+		}
+		host[i]->id = i;
+		
+		disable_irq(host[i]->irq);
+		host[i]->mmc = mmc[i];
+		
+		if(host[i]->sdhc_id != EMMC)
+		{
+			mmc[i]->caps = (MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED); 
+		}
+		else
+		{
+			mmc[i]->caps = (MMC_CAP_8_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED);
+		}
+		
+		host[i]->ioaddr = ioremap(iobase, SZ_1K);
+		host[i]->clock = 0;
+		host[i]->power_mode = MMC_POWER_OFF;
+		
+		//See if the card is already inserted and update our status accordingly
+		host[i]->flags = HOST_F_STOP;
+		tempData = readl(host[i]->ioaddr + SDHCI_PRESENT_STATE);
+		if((tempData & CARD_INSERTED) == CARD_INSERTED)
+		{
+			host[i]->flags = HOST_F_ACTIVE;
+		}
+		
+		tasklet_init(&host[i]->card_tasklet, pilot_tasklet_card,	(unsigned long) host[i]);
+		tasklet_init(&host[i]->finish_tasklet, pilot_tasklet_finish, (unsigned long) host[i]);
+		
+		spin_lock_init(&host[i]->lock);
+		
+		ret = pilot_hw_init(host[i]);
+		if(ret != 0)
+		{
+			printk(KERN_ERR "SDHC%d: Failed to initialize PILOT SD/MMC Hardware\n", i);
+			goto pilot_probe_exit;
+		}
+		pilot_enable_all_interrupts(host[i]);
+		enable_irq(host[i]->irq);
+		mmc_add_host(mmc[i]);
+		
+		pilot_sdhc[i] = host[i];
+	}
+	return 0;
+
+pilot_probe_exit:
+	if(ret != 0 && host != NULL)
+	{
+		free_irq(host[i]->irq, host[i]);
+		tasklet_kill(&host[i]->card_tasklet);
+		tasklet_kill(&host[i]->finish_tasklet);
+		iounmap(&host[i]->ioaddr);
+		mmc_free_host(host[i]->mmc);
+		platform_set_drvdata(pdev, NULL);
+	}
+#else
+	struct sdhci_host *host = NULL;
+	int ret = -EINVAL;
+
+	host = sdhci_alloc_host(&pdev->dev,sizeof(struct sdhci_host));
+	if(IS_ERR(host))
+	{
+		printk(KERN_ERR "Cannot allocate host \n");
+		return ret;
+	}
+
+	platform_set_drvdata(pdev, host->mmc);
+
+	ret = pilot_hw_init(host);
+	if(ret != 0)
+	{
+		printk(KERN_ERR "Failed to initialize PILOT SD/MMC Hardware\n");
+		goto pilot_probe_exit;
+	}
+
+	ret = sdhci_add_host(host);
+	if(ret != 0)
+	{
+		printk(KERN_ERR "Failed to add host\n");
+		goto pilot_probe_exit;
+	}
+
+pilot_probe_exit:
+	if(ret != 0 && host != NULL)
+	{
+		sdhci_free_host(host);
+		platform_set_drvdata(pdev, NULL);
+	}
+#endif
+
+	return ret;
+}
+
+static int pilot_sd_remove(struct platform_device *pdev)
+{
+#if defined(CONFIG_SOC_SE_PILOT4)
+	struct sdhci_host *host[NO_OF_SDHCS];
+	u32 i;
+	for(i=0; i<NO_OF_SDHCS; i++)
+	{
+		host[i] = pilot_sdhc[i];
+
+		if(host[i] == NULL)
+			return 0;
+
+		if(SD_DMA_MODE)
+			dma_free_coherent(NULL, sizeof(struct dma_desc) * 512, (void *)gstruc[i], dma_addr[i]);
+
+		mmc_remove_host(host[i]->mmc);
+		pilot_disable_all_interrupts(host[i]);
+		free_irq(host[i]->irq, host[i]);
+		tasklet_kill(&host[i]->card_tasklet);
+		tasklet_kill(&host[i]->finish_tasklet);
+		iounmap((u32 *)host[i]->ioaddr);
+		mmc_free_host(host[i]->mmc);
+		
+		pilot_sdhc[i] = NULL;
+	}
+#else
+	struct mmc_host *mmc = platform_get_drvdata(pdev);
+	struct sdhci_host *host;
+
+	if(mmc == NULL)
+	{
+		printk(KERN_ERR "Couldn't get the driver data\n");
+		return -EINVAL;
+	}
+
+	host = mmc_priv(mmc);
+	sdhci_remove_host(host, 1);
+	iounmap(host->ioaddr);
+	sdhci_free_host(host);
+	platform_set_drvdata(pdev, NULL);
+#endif
+	
+	return 0;
+}
+
+static void __exit pilot_sd_exit(void)
+{
+	platform_device_unregister(&pilot_sdhci_device);
+	platform_driver_unregister(&pilot_sdhci_driver);
+}
+
+static int __init pilot_sd_init(void)
+{
+	printk(KERN_INFO "PILOT SoC SD/MMC Driver \n");
+	
+	platform_device_register(&pilot_sdhci_device);
+	platform_driver_probe(&pilot_sdhci_driver,pilot_sd_probe);
+
+	return 0;
+}
+
+module_init(pilot_sd_init);
+module_exit(pilot_sd_exit);
+
+MODULE_AUTHOR("American Megatrends Inc.");
+MODULE_LICENSE("GPL");
+MODULE_DESCRIPTION("SD/MMC driver for PILOT SoC Boards");
+
+
