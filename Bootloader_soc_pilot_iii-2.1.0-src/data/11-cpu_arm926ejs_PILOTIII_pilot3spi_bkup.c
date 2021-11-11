--- uboot.old/cpu/arm926ejs/PILOTIII/pilot3spi_bkup.c	1969-12-31 19:00:00.000000000 -0500
+++ uboot/cpu/arm926ejs/PILOTIII/pilot3spi_bkup.c	2012-05-11 17:30:36.519098258 +0800
@@ -0,0 +1,293 @@
+/******************************************************************************
+*
+*   Abstract:   Implements the low level access routines for the Pilot-II Server Engines BMC Backup/Data
+*               SPI flash interface.
+*
+******************************************************************************/
+
+#include "spiflash.h"
+#ifdef __UBOOT__        
+#include "soc_hw.h"
+#endif
+
+extern  void pilot_change_spi_source_clk100mhz(unsigned int spi_no);
+typedef struct
+{
+        volatile unsigned char Addr0;           /* LSB */
+        volatile unsigned char Addr1;
+        volatile unsigned char Addr2;           /* MSB */
+        volatile unsigned char Rsvd1;
+        volatile unsigned long pilot3_Dummy;
+        volatile union {
+                volatile struct {
+                        volatile unsigned char Opcode;
+                        volatile unsigned char CmdType;
+                        volatile unsigned char Dummy;
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
+        volatile unsigned long Rsvd4;
+	volatile unsigned char Rsvd5[16];//Pilot3 needs so much of offset as the data register is at offset 0x30
+        volatile union {
+                volatile unsigned char Data8[4];
+                volatile unsigned long Data32;
+        } Data;
+//End Changed by shivah for Pilot3
+} BKUP_SPI_REGS;
+
+volatile BKUP_SPI_REGS *Bkup_Regs = (volatile BKUP_SPI_REGS *)(SE_BACKUP_SPI_VA_BASE);
+
+#if 1
+int pilot3spi_bkup_init(void);
+int pilot3spi_bkup_transfer(int bank,unsigned char *cmd,int cmdlen, SPI_DIR dir,
+                                            unsigned char *data, unsigned long datalen);
+void bkup_pilot_set_dual_spi_bit(void)
+{
+  Bkup_Regs->CmdCtrl |= (1<<3);
+}
+void bkup_pilot_clear_dual_spi_bit(void)
+{
+  Bkup_Regs->CmdCtrl &= (~(1<<3));
+}
+static
+int
+BkupValidateInputs(int bank, unsigned char *cmd,int cmdlen, SPI_DIR dir,
+                                unsigned char *data, int datalen);
+
+static void inline
+BkupWaitForSpiReady(void)
+{
+        while (Bkup_Regs->SpiStatus & 0x01);
+        return;
+}
+
+static void inline
+BkupWaitForWriteFifo(void)
+{
+        /* Wait till Write Fifo Ready Bit is set */
+        while (!(Bkup_Regs->SpiStatus & 0x08));
+        return;
+}
+
+static void inline
+BkupWaitForWriteFifoEmpty(void)
+{
+        /* Wait till Fifo Empty Bit is set */
+        while (!(Bkup_Regs->SpiStatus & 0x20));
+        return;
+}
+
+static int inline
+BkupIsWriteFifoFull(void)
+{
+        return (Bkup_Regs->SpiStatus & 0x10);
+}
+static void
+Bkup_SpiEnable(int en)
+{
+  if (en) 
+    Bkup_Regs->SpiCtrl |= 0x01;
+  else
+    Bkup_Regs->SpiCtrl &= 0xFE;
+  return;
+}
+static
+int
+BkupValidateInputs(int bank, unsigned char *cmd,int cmdlen, SPI_DIR dir, unsigned char *data, int datalen)
+{
+#ifdef CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS
+    if ((CONFIG_SPX_FEATURE_GLOBAL_BKUP_FLASH_BANKS == 0) && (bank != CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS))
+#else
+    if (bank != (CONFIG_SPX_FEATURE_GLOBAL_FLASH_BANKS - 1))
+#endif
+    {
+        printk("pilot3spi_bkup : Called for wrong bank: %d\n", bank);
+        return -1;
+    }
+
+        if ((cmdlen != 1) && (cmdlen != 4) && (cmdlen !=5))
+        {
+                printk("pilot3spi : Invalid Command Len %d\n",cmdlen);
+                return -1;
+        }
+        if (cmd == NULL)
+        {
+                printk("pilot3spi : Invalid : Null Command\n");
+                return -1;
+        }
+        if (dir != SPI_NONE)
+        {
+                if ( (data == NULL) || (datalen == 0))
+                {
+                        printk("pilot3spi : Invalid data or datalen \n");
+                        return -1;
+                }
+        }
+
+        return 0;
+
+}
+
+static
+void
+BkupDoSpiFifoWrite(unsigned char *data, int datalen)
+{
+
+        /* Wait for Fifo Mode Enabled */
+        //BkupWaitForWriteFifo();
+
+        /* Transfer all data to Fifo */
+        while (datalen --)
+        {
+                Bkup_Regs->Data.Data8[0] = *data;
+                data++;
+                /* Wait for Fifo not full */
+                //while (BkupIsWriteFifoFull());
+        }
+
+        /* Tell ctrl data is over by deselect CS */
+//      SpiChipDeselect();
+
+        /* Wait for the Fifo to be transferrred */
+        //BkupWaitForWriteFifoEmpty();
+        return;
+}
+
+
+int pilot3spi_bkup_init(void)
+{
+#if defined  (BKUP_DUAL_SPI) || (BKUP_FAST_READ_EN)
+      {
+	pilot_change_spi_source_clk100mhz(1);
+      }
+#endif
+  Bkup_SpiEnable(0);
+  Bkup_SpiEnable(1);
+  BkupWaitForSpiReady();
+	return 0;
+
+}
+
+int pilot3spi_bkup_transfer(int bank,unsigned char *cmd,int cmdlen, SPI_DIR dir,
+                                            unsigned char *data, unsigned long datalen)
+{
+        unsigned char Opcode;
+        unsigned char Type;
+        unsigned char Dummy;
+        unsigned char Ctrl;
+        unsigned long Command;
+
+	int i;
+
+        if (BkupValidateInputs(bank,cmd,cmdlen,dir,data,datalen) == -1)
+                return -1;
+
+        //      Ctrl = 0x84;
+        Ctrl = 0x80;
+        Dummy = 0;
+
+        /* Fill in Command And Address */
+#if defined  (BKUP_DUAL_SPI)   
+  if(cmd[0]==0x03)
+    {
+      bkup_pilot_set_dual_spi_bit();
+      Opcode = 0x3b;   //Dual io read
+      cmdlen=cmdlen+1;
+    }
+  else if( (cmd[0]==0x02))
+    {
+      bkup_pilot_set_dual_spi_bit();
+      Opcode = 0xA2;   //Dual io write
+    }
+  else
+    {
+        Opcode = cmd[0];
+    }
+#else
+#if defined  (BKUP_FAST_READ_EN)
+  if( (cmd[0]==0x03))
+    {
+      Opcode = 0x0B;   //fast read
+      cmdlen=cmdlen+1;
+    }
+  else
+    {
+      Opcode = cmd[0];
+    }
+#else
+  Opcode = cmd[0];
+#endif
+
+#endif
+
+        Bkup_Regs->Rsvd1 = 0x0;
+
+        if (cmdlen >= 4)
+        {
+                Bkup_Regs->Addr0 = cmd[3];
+                Bkup_Regs->Addr1 = cmd[2];
+                Bkup_Regs->Addr2 = cmd[1]&0x7F;
+        }
+
+        /* Fill in Command Len and data len */
+        Type = 0;
+        if (dir == SPI_READ)
+        {
+          Type = 0x0;
+        }
+        else
+        {
+                Type |= 0x10;
+        }
+
+
+        Type |= cmdlen;
+
+        /* Write Command . Enable Write Fifo */
+        if (dir == SPI_WRITE)
+        {
+                Type |= 0x10;
+        }
+
+        Command = (Opcode) | (Type << 8) | (Dummy << 16) | (Ctrl << 24);
+        /* Issue Command */
+        BkupWaitForSpiReady();
+        Bkup_Regs->Command = Command;
+
+        BkupWaitForSpiReady();
+        /* Do FIFO write for write commands */
+        if (dir == SPI_WRITE)
+          BkupDoSpiFifoWrite(data,datalen);
+
+        /* Wait for SPI Controller become Idle */
+        BkupWaitForSpiReady();
+
+
+          {
+            /* Read the data from the data register */
+            if (dir == SPI_READ)
+              {
+                for(i=0;i<datalen;i++)
+                  data[i] = Bkup_Regs->Data.Data8[0];
+              }
+          }
+
+
+        /* Switch back to non-register mode and disable Write Fifo mode */
+  Bkup_Regs->CmdCtrl &= 0x77;
+
+
+        return 0;
+}
+#endif
