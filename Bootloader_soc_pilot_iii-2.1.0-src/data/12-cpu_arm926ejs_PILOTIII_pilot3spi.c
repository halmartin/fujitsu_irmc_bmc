--- u-boot-1.1.6/cpu/arm926ejs/PILOTIII/pilot3spi.c	1970-01-01 08:00:00.000000000 +0800
+++ uboot/cpu/arm926ejs/PILOTIII/pilot3spi.c	2012-11-21 17:30:00.000000000 +0800
@@ -0,0 +1,759 @@
+/*
+ * Copyright (C) 2007 American Megatrends Inc
+ *
+ * This program is free software; you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation; either version 2 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program; if not, write to the Free Software
+ * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
+ */
+
+#include "spiflash.h"
+#ifdef __UBOOT__	
+#include "soc_hw.h"
+#endif
+
+#define SPI_INPUT_CLOCK 100		/* Input Frequency MHZ */
+#define MAX_CLOCK	25		/* Max SPI Frequency MHZ supported by Controller */
+#define MAX_READ	4
+
+#define CS0_MASK 0x0000001F
+#define CS1_MASK 0x00001F00
+#define CS2_MASK 0x001F0000
+
+#define CS_8MB   0x02
+#define CS_16MB  0x04
+#define CS_32MB  0x08
+#define CS_64MB  0x10
+
+#define FLASH_8MB  0x00800000
+#define FLASH_16MB 0x01000000
+#define FLASH_32MB 0x02000000
+#define FLASH_64MB 0x04000000
+
+#define Enable_ADDRESS_4BYTE 0x1000000 
+
+// Pilot-II supports multiple SPI busses.  Bank 0 is the Boot SPI, Bank 1 is the Backup/Data SPI
+// Route any access to bank 1 to the backup SPI code
+extern int pilot3spi_bkup_init(void);
+extern int pilot3spi_bkup_transfer(int bank,unsigned char *cmd,int cmdlen, SPI_DIR dir,
+                                            unsigned char *data, unsigned long datalen);
+extern  void pilot_change_spi_source_clk100mhz(unsigned int spi_no);
+extern  void pilot_change_spi_source_clk40mhz(unsigned int spi_no);
+static  int ValidateInputs(unsigned char *cmd,int cmdlen, SPI_DIR dir,unsigned char *data, int datalen);
+
+extern flash_info_t flash_info[CFG_MAX_FLASH_BANKS];
+extern int spi_verbose;
+
+typedef struct 
+{
+        volatile unsigned char Addr0;           /* LSB */
+        volatile unsigned char Addr1;
+        volatile unsigned char Addr2;           /* MSB */
+        volatile unsigned char Addr3;
+		volatile unsigned long Dummy0;	
+        volatile union {
+                volatile struct {
+                        volatile unsigned char Opcode;
+                        volatile unsigned char CmdType;
+                        volatile unsigned char Dummy1;
+                        volatile unsigned char CmdCtrl;
+                };
+                volatile unsigned long Command;
+        };
+        volatile unsigned char Trec;
+        volatile unsigned char Tcs;
+        volatile unsigned char Tcsl;
+        volatile unsigned char Tcsh;
+        volatile unsigned char SpiCtrl;
+        volatile unsigned char Rsvd2[3];
+        volatile unsigned char ClkDivL;
+        volatile unsigned char ClkDivH;
+        volatile unsigned char Rsvd3[2];
+        volatile unsigned long SpiStatus;
+        volatile unsigned long bmisc;
+		volatile unsigned char Rsvd5[16];//Pilot3 needs so much of offset as the data register is at offset 0x30
+        volatile union {
+                volatile unsigned char Data8[4];
+                volatile unsigned long Data32;
+        } Data;
+} BOOT_SPI_REGS;
+
+
+volatile BOOT_SPI_REGS *Regs = (volatile BOOT_SPI_REGS *)(SE_BOOT_SPI_VA_BASE);
+void pilot_set_dual_spi_bit(void)
+{
+  Regs->CmdCtrl |= (1<<3);
+}
+void pilot_clear_dual_spi_bit(void)
+{
+  Regs->CmdCtrl &= (~(1<<3));
+}
+
+static void inline
+WaitForSpiReady(void)
+{
+	while (Regs->SpiStatus & 0x01);
+	return;
+}
+
+static void inline
+WaitForWriteFifo(void)
+{
+	/* Wait till Write Fifo Ready Bit is set */
+	while (!(Regs->SpiStatus & 0x08));
+	return;
+}
+
+static void inline
+WaitForWriteFifoEmpty(void)
+{
+	/* Wait till Fifo Empty Bit is set */
+	while (!(Regs->SpiStatus & 0x20));
+	return;
+}
+
+static int inline
+IsWriteFifoFull(void)
+{
+	return (Regs->SpiStatus & 0x10);
+}
+
+static void
+SpiEnable(int en)
+{
+	if (en) 
+		Regs->SpiCtrl |= 0x01;
+	else
+		Regs->SpiCtrl &= 0xFE;
+	return;
+}
+
+static
+void
+InitializeCtrl(void)
+{
+  unsigned int strap_sts;
+  if (spi_verbose)
+  {
+    printk("Fast clk is set\n");
+    //pilot_change_spi_source_clk100mhz(0);
+    Regs->CmdCtrl|=0x4;
+  }
+	/* Disable the ctrl */
+	SpiEnable(0);
+	
+	/* Set the clock to 25MHZ */ //We might have to fix it for ASIC
+	// TODO
+	//Regs->ClkDivH = 0;
+	//Regs->ClkDivL = (SPI_INPUT_CLOCK/MAX_CLOCK);
+
+	/* Enable the ctrl */
+	SpiEnable(1);
+
+	/* Wait for the SPI internal state to be ready */
+	WaitForSpiReady();
+	
+  strap_sts = (int)IO_ADDRESS(0x4010010c);
+  if(strap_sts&0x40)
+    {
+      printk("Strap is not set for back up spi\n");
+    }
+  else
+    {
+      pilot3spi_bkup_init();
+    }
+	return;
+}
+	
+
+
+static
+int
+ValidateInputs(unsigned char *cmd,int cmdlen, SPI_DIR dir, 
+				unsigned char *data, int datalen)
+{
+	if ((cmdlen < 1) || (cmdlen > 6)) // vaild command length range is 1~6
+	{
+		printk("pilot3spi : Invalid Command Len %d\n",cmdlen);
+		return -1;
+	}
+	if (cmd == NULL)
+	{
+		printk("pilot3spi : Invalid : Null Command\n");
+		return -1;
+	}
+	if (dir != SPI_NONE)
+	{
+		if ( (data == NULL) || (datalen == 0))
+		{
+			printk("pilot3spi : Invalid data or datalen \n");
+			return -1;
+		}
+	}
+	if ((dir == SPI_READ) && (datalen > MAX_READ))
+	{
+		printk("pilot3spi : Invalid : Reads cannot exceed %d bytes\n",MAX_READ);
+		return -1;
+	}
+	
+	return 0;
+
+}
+
+static
+void
+DoSpiFifoWrite(unsigned char *data, int datalen)
+{
+
+	/* Wait for Fifo Mode Enabled */
+	//WaitForWriteFifo();			
+
+	/* Transfer all data to Fifo */
+	while (datalen --)
+	{
+		Regs->Data.Data8[0] = *data;
+		data++;
+		/* Wait for Fifo not full */
+		//while (IsWriteFifoFull());
+	}
+
+	/* Tell ctrl data is over by deselect CS */
+
+	/* Wait for the Fifo to be transferrred */
+	//WaitForWriteFifoEmpty();		
+	return;
+}
+
+#if 0
+#define SHOWREG8(x) printk("Register %s @ 0x%08lx = 0x%02x\n",#x,(unsigned long)&(Regs->x),Regs->x)
+#define SHOWREG32(x) printk("Register %s @ 0x%08lx = 0x%08lx\n",#x,(unsigned long)&(Regs->x),Regs->x)
+static
+void
+DumpRegisters(void)
+{
+	SHOWREG8(Addr0);
+	SHOWREG8(Addr1);
+	SHOWREG8(Addr2);
+	SHOWREG32(Command);
+	SHOWREG8(ClkDivH);
+	SHOWREG8(ClkDivL);
+	SHOWREG32(SpiStatus);
+	SHOWREG32(Data.Data32);
+	return;
+}
+#endif
+
+#ifdef __UBOOT__	
+typedef struct 
+{
+	ulong length;
+	ulong rsvd;
+	ulong src;
+	ulong dest;
+} GDMA_DESCRIPTOR;
+
+typedef struct 
+{
+	volatile ulong GDMA_CTRL;
+	volatile ulong GDMA_STAT;
+	volatile ulong GDMA_DESC;
+	volatile ulong GDMA_CHK;
+} GDMA_REGS;
+
+volatile GDMA_REGS *gdma_regs = (volatile GDMA_REGS *)(SE_SECURITY_VA_BASE + 0x80);
+
+/*Align at 16 Byte Boundary */
+GDMA_DESCRIPTOR gdma_desc __attribute__ ((aligned (16)));
+
+#define PILOT_3_SPI_MAX_CHIP_SELECT	4
+const unsigned int ChipSelectBit[PILOT_3_SPI_MAX_CHIP_SELECT] = {0x00000000, 0x40000000, 0x80000000, 0xC0000000};
+
+static
+int  
+pilot3spi_split_burst_read(int bank, unsigned char addrmode, unsigned char *cmd,int cmdlen, SPI_DIR dir, 
+				unsigned char *data, unsigned long datalen, unsigned long flashsize)
+{
+	dma_addr_t dmabuff;
+	unsigned long src;
+	//unsigned long timeout =100000;
+	unsigned long timeout =800000;
+
+	/* Fast Read Bit */
+	// TODO
+
+	if (addrmode == 4)
+	{
+		/*switch to 4 byte mode*/
+		Regs->bmisc |= Enable_ADDRESS_4BYTE;
+		/*set this bit for direct memory read access to the upper 16MB of a 32MB SPI */
+		*((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x90)) |= 0x10000;
+		src = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
+	}
+	else
+	{
+		/*switch to 3 byte mode */
+		Regs->bmisc &= ~(Enable_ADDRESS_4BYTE);	  
+		*((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x90)) &= ~(0x10000);
+		src = cmd[0] << 24 | cmd[1] << 16 | cmd[2] << 8 | cmd[3];
+	}
+
+	/* Convert Buffer to DMA Buffer */
+	dmabuff = dma_map_single(NULL,data,datalen,DMA_FROM_DEVICE);
+	if (dmabuff == 0)
+	{
+		printk("ERROR:pilot3spi dma_map_single() failed\n");
+		return -1;
+	}
+
+	/* Fill the Descriptors */
+	gdma_desc.dest = dmabuff;
+	gdma_desc.src  = src;
+	gdma_desc.rsvd = 0;
+	gdma_desc.length = (datalen-1) << 8;
+
+	/* Enable GDMA */
+	gdma_regs->GDMA_CTRL |= 0x1;
+
+	/* Load the Descriptor to GDMA . This starts transfer */
+	gdma_regs->GDMA_DESC = (ulong)virt_to_phys(&gdma_desc);
+
+	/* Wait for completion */	
+	while ((gdma_regs->GDMA_STAT & 0x1) && timeout)
+		timeout --;
+
+	/* Disable GDMA */
+	gdma_regs->GDMA_CTRL &= ~(0x1);
+
+	/* Release DMA Memory */
+	dma_unmap_single(NULL,dmabuff,datalen,DMA_FROM_DEVICE);
+
+	if (timeout == 0)
+	{
+		printk("ERROR: pilot3spi_burst_read timeout for read at 0x%08lx for %ld bytes\n", src, datalen);
+		return -1;
+	}
+
+	return 0;
+}
+
+#define MAX_BURST_READ 0xFFF
+
+static
+int  
+pilot3spi_burst_read(int bank,unsigned char *cmd,int cmdlen, SPI_DIR dir, 
+				unsigned char *data, unsigned long datalen,unsigned long flashsize)
+{
+	unsigned long transfer;
+	int retval = 0;
+	unsigned long src;
+	unsigned char addrmode;
+
+	// Pass any access to highest bank number to the Backup/Data SPI handler
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS
+	if (bank == CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS)
+	{
+		if (CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS == 1)
+			return pilot3spi_bkup_transfer(bank, cmd, cmdlen, dir, data, datalen);
+		else
+			return 0;
+	}
+#else
+	if (CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS > 1)
+	{
+		if (bank == (CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS - 1))
+			return pilot3spi_bkup_transfer(bank, cmd, cmdlen, dir, data, datalen);
+	}
+#endif
+
+	addrmode = 3; //default to 24-bits address mode
+	if (cmd[0] == 0x0B) // fast read
+	{
+		if (cmdlen == 6) addrmode = 4; // change to 32-bits address mode
+	}
+	else
+	{
+		if (cmdlen == 5) addrmode = 4; // change to 32-bits address mode
+	}
+
+	// chip select # for the current flash
+	Regs->bmisc = ((Regs->bmisc & 0x3FFFFFFF) | ChipSelectBit[bank]);
+
+	while (datalen)	
+	{
+		if (datalen > MAX_BURST_READ)
+			transfer = MAX_BURST_READ;
+		else
+			transfer = datalen;
+
+		retval = pilot3spi_split_burst_read(bank,addrmode,cmd,cmdlen,dir,data,transfer,flashsize);
+		if (retval != 0) break;
+ 
+		if (addrmode == 4)
+ 		{
+			/* Increment Source address */
+			src = cmd[1] << 24 | cmd[2] << 16 | cmd[3] << 8 | cmd[4];
+			src+=transfer;
+			cmd[1] = src >> 24;
+			cmd[2] = src >> 16;
+			cmd[3] = src >> 8;
+			cmd[4] = src;
+		}
+		else
+		{
+			/* Increment Source address */
+			src = cmd[1] << 16 | cmd[2] << 8 | cmd[3];
+			src+=transfer;
+			cmd[1] = src >> 16;
+			cmd[2] = src >> 8;
+			cmd[3] = src;
+		}
+
+		/* Increment buffer */
+		data+=transfer;
+
+		/* Decrement remain len */
+		datalen-=transfer;
+	}
+
+	if (addrmode == 4)
+	{
+		/*switch to 3 byte mode */
+		Regs->bmisc &= ~(Enable_ADDRESS_4BYTE);
+	}
+	*((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x90)) &= ~(0x10000);
+	Regs->bmisc = ((Regs->bmisc & 0x3FFFFFFF) | ChipSelectBit[PILOT_3_SPI_MAX_CHIP_SELECT - 1]);
+
+	return 0;
+}
+#endif // __UBOOT__	
+
+static
+int  
+pilot3spi_transfer(int bank,unsigned char *cmd,int cmdlen, SPI_DIR dir, 
+				unsigned char *data, unsigned long datalen, unsigned long flashsize)
+{
+	unsigned char Opcode;
+	unsigned char Type;
+	unsigned char Dummy;
+	unsigned char Ctrl;
+	unsigned long Command;
+	unsigned char addrmode;
+
+	// Pass any access to highest bank number to the Backup/Data SPI handler
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS
+	if (bank == CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS)
+	{
+		if (CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS == 1)
+			return pilot3spi_bkup_transfer(bank, cmd, cmdlen, dir, data, datalen);
+		else
+			return 0;
+	}
+#else
+	if (CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS > 1)
+	{
+		if (bank == (CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS - 1))
+		return pilot3spi_bkup_transfer(bank, cmd, cmdlen, dir, data, datalen);
+	}
+#endif
+
+	if (ValidateInputs(cmd,cmdlen,dir,data,datalen) == -1)
+		return -1;
+
+	Dummy = 0;
+
+	/* Fill in Command And Address */
+
+  //if read then change it to fast read or dual i/o read based on switches
+  //if write then change it to dual i/o write based on switches
+  //
+#if defined  (DUAL_SPI)   
+  
+  if(cmd[0]==0x03)
+    {
+      pilot_set_dual_spi_bit();
+      Opcode = 0x3b;   //Dual io read
+      cmdlen=cmdlen+1;
+    }
+  else if( (cmd[0]==0x02))
+    {
+      pilot_set_dual_spi_bit();
+      Opcode = 0xA2;   //Dual io write
+    }
+  else
+    {
+      Opcode = cmd[0];
+    }
+#else
+  
+  if( (cmd[0]==0x03))
+    {
+      Opcode = 0x0B;   //fast read
+      cmdlen=cmdlen+1;
+    }
+  else
+    {
+      Opcode = cmd[0];
+    }
+
+#endif
+
+	Ctrl = ( (Regs->CmdCtrl) | (0x80) );
+
+	addrmode = 3; // default to 24-bits address mode
+	if (Opcode == 0x0B) // fast read
+	{
+		if (cmdlen == 6) addrmode = 4; // change to 32-bits address mode
+	}
+	else // other commands
+	{
+		if (cmdlen == 5) addrmode = 4; // change to 32-bits address mode
+	}
+
+	// chip select # for the current flash
+	Regs->bmisc = ((Regs->bmisc & 0x3FFFFFFF) | ChipSelectBit[bank]);
+
+	if (addrmode == 4)
+	{
+		/*switch to 4 byte mode*/
+		Regs->bmisc |= Enable_ADDRESS_4BYTE;
+		Regs->Addr0 = cmd[4];
+		Regs->Addr1 = cmd[3];
+		Regs->Addr2 = cmd[2];
+		Regs->Addr3 = cmd[1];
+	}
+	else
+	{
+		/*switch to 3 byte mode */
+		//Regs->bmisc &= ~(Enable_ADDRESS_4BYTE);
+		Regs->Addr0 = cmd[3];
+		Regs->Addr1 = cmd[2];
+		Regs->Addr2 = cmd[1];
+	}
+
+	/* Fill in Command Len and data len */
+	if (dir == SPI_READ) Type = 0x00;
+	else Type = 0x10;
+	Type |= (unsigned char)(0x0F & cmdlen);
+
+	/* Write Command . Enable Write Fifo */
+	if (dir == SPI_WRITE) Type |= 0x10;
+
+	Command = (Opcode) | (Type << 8) | (Dummy << 16) | (Ctrl << 24);
+
+	/* Issue Command */
+	WaitForSpiReady();
+	Regs->Command = Command;
+
+	WaitForSpiReady();
+
+	/* Do FIFO write for write commands */
+	if (dir == SPI_WRITE)
+		DoSpiFifoWrite(data,datalen);
+
+	/* Wait for SPI Controller become Idle */
+	WaitForSpiReady();
+
+//	DumpRegisters();
+
+	/* Read the data from the data register */
+	if (dir == SPI_READ)
+	{
+		int i;
+
+		for(i=0;i<datalen;i++)
+			data[i] = Regs->Data.Data8[0];	
+	}
+
+	if (addrmode == 4)
+	{
+		/* switch to 3 byte mode */
+		Regs->bmisc = Regs->bmisc & ~(Enable_ADDRESS_4BYTE);
+	}
+
+	/* Switch back to non-register mode and disable Write Fifo mode */
+	Regs->CmdCtrl &= 0x77;
+	Regs->bmisc = ((Regs->bmisc & 0x3FFFFFFF) | ChipSelectBit[PILOT_3_SPI_MAX_CHIP_SELECT - 1]);
+
+	return 0;
+}
+
+static int pilot3spi_configure_clock(int bank, unsigned int clock) 
+{ 
+  unsigned int clk_ctrl;
+  unsigned int temp;
+  unsigned char flsize;
+
+  flsize = 0x00;
+  if (flash_info[bank].size >= FLASH_8MB)  flsize = CS_8MB;
+  if (flash_info[bank].size >= FLASH_16MB) flsize = CS_16MB;
+  if (flash_info[bank].size >= FLASH_32MB) flsize = CS_32MB;
+  if (flash_info[bank].size >= FLASH_64MB) flsize = CS_64MB;
+
+  if (bank == 0)
+  {
+    Regs->bmisc &= ~(CS0_MASK);
+    Regs->bmisc |= flsize;
+  }
+  else if (bank == 1)
+  {
+    unsigned long cs0_upper_address = (Regs->bmisc & CS0_MASK);
+
+    Regs->bmisc &= ~(CS1_MASK);
+    Regs->bmisc |= ((cs0_upper_address + flsize) << 8);
+  }
+  else if (bank == 2)
+  {
+    unsigned long cs1_upper_address = ((Regs->bmisc & CS1_MASK) >> 8);
+
+    Regs->bmisc &= ~(CS2_MASK);
+    Regs->bmisc |= ((cs1_upper_address + flsize) << 16);
+  }
+  else ; // nothing to do
+
+  if ((clock / 1000000) > 50) return 0;
+
+  clk_ctrl = (int)IO_ADDRESS(0x40100120);
+  temp = *(volatile int *)(clk_ctrl);
+  if (temp == 0) return 0;
+
+  if (bank == 0) {
+    temp &= 0xfffffff0;
+    temp |= 0x00000003;
+  } else if (bank == 1) {
+    temp &= 0xffffff0f;
+    temp |= 0x00000030;
+  } else if (bank == 2) {
+    temp &= 0xfffff0ff;
+    temp |= 0x00000300;
+  }
+
+  *(volatile int *)(clk_ctrl)=temp;
+               return 0; 
+} 
+
+#ifdef __UBOOT__        
+extern void ll_serial_init(void); 
+static int  
+pilot3spi_init(void) 
+{ 
+//     ll_serial_init(); 
+       InitializeCtrl();        
+    pilot3spi_bkup_init(); 
+       return 0; 
+} 
+#endif 
+
+#ifndef __UBOOT__	/* linux */
+struct spi_ctrl_driver pilot3spi_driver =
+#else
+/* 
+   Uboot supports only one spi controller and has to be defined 
+   in the structure "SPI_CTRL_DRIVER"
+*/
+struct spi_ctrl_driver SPI_CTRL_DRIVER =
+#endif
+{
+	.name 		= "pilot3spi",
+	.module 	= THIS_MODULE,
+	.max_read	= MAX_READ,
+	.fast_read	= 1,
+	.spi_transfer	= pilot3spi_transfer,
+#ifdef __UBOOT__	
+	.spi_burst_read	= pilot3spi_burst_read,
+	.spi_init	= pilot3spi_init,
+#else
+    .spi_burst_read = NULL,
+#endif
+	.spi_configure_clock = pilot3spi_configure_clock,
+};
+
+#ifdef __UBOOT__
+void
+SPI_CTRL_DRIVER_INIT(void)
+{
+	InitializeCtrl();	
+	init_MUTEX(&SPI_CTRL_DRIVER.lock);
+	register_spi_ctrl_driver(&SPI_CTRL_DRIVER);
+    pilot3spi_bkup_init();
+	return;
+}
+
+#else		/* Linux */
+
+static int 
+pilot3spi_init(void)
+{
+	InitializeCtrl();	
+	init_MUTEX(&pilot3spi_driver.lock);
+	register_spi_ctrl_driver(&pilot3spi_driver);
+	pilot3spi_bkup_init();
+	return 0;
+}
+
+
+static void 
+pilot3spi_exit(void)
+{
+  pilot_change_spi_source_clk40mhz(0);
+  pilot_change_spi_source_clk40mhz(1);
+	unregister_spi_ctrl_driver(&pilot3spi_driver);
+	return;
+}
+
+void pilot_change_spi_source_clk100mhz(unsigned int spi_no)
+{
+  unsigned int clk_ctrl;
+  unsigned int temp;
+  clk_ctrl = (int)IO_ADDRESS(0x40100120);
+  if(spi_no==0x0) //boot_spi
+    {
+      temp=*(volatile int *)(clk_ctrl);
+      temp=temp &0xfffffff0;
+      temp=temp|0x1;
+      *(volatile int *)(clk_ctrl)=temp;
+    }
+  if(spi_no==0x1) //backup_spi
+    {
+      temp=*(volatile int *)(clk_ctrl);
+      temp=temp &0xffffff0f;
+      temp=temp|0x10;
+      *(volatile int *)(clk_ctrl)=temp;
+    }
+}
+void pilot_change_spi_source_clk40mhz(unsigned int spi_no)
+{
+  unsigned int clk_ctrl;
+  unsigned int temp;
+  clk_ctrl = (int)IO_ADDRESS(0x40100120);
+  if(spi_no==0x0) //boot_spi
+    {
+      temp=*(volatile int *)(clk_ctrl);
+      temp=temp &0xfffffff0;
+      temp=temp|0x4;
+      *(volatile int *)(clk_ctrl)=temp;
+    }
+  if(spi_no==0x1) //backup_spi
+    {
+      temp=*(volatile int *)(clk_ctrl);
+      temp=temp &0xffffff0f;
+      temp=temp|0x40;
+      *(volatile int *)(clk_ctrl)=temp;
+    }
+}
+module_init(pilot3spi_init);
+module_exit(pilot3spi_exit);
+
+MODULE_LICENSE("GPL");
+MODULE_AUTHOR("American Megatrends Inc");
+MODULE_DESCRIPTION("PILOT-II SOC SPI Controller driver");
+
+#endif
