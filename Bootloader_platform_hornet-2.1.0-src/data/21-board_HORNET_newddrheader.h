--- olduboot/include/pilotiii/ddr.h	2011-09-22 15:36:43.000000000 +0800
+++ uboot/include/pilotiii/ddr.h	2011-09-22 15:32:36.000000000 +0800
@@ -77,6 +77,7 @@
 #define HM_FIFO_DELAY_2     (DDR_REG_BASE + 0x278)
 #define HM_FIFO_ALIGN       (DDR_REG_BASE + 0x27C)
 
+#define  DDRBASE              (0x80000000) 
 
 
 #define LSI_FIFO_TRAINING   (0x10007000)
@@ -103,7 +104,7 @@
 void ddrcfg(tU32, tU8);
 void prg_gate(tU32);
 int gateon_training(void); 
-void redeye_training(void);
+int  redeye_training(void);
 extern void Burst_Write(tPVU32 , tU32, tU32);
 extern int Burst_Read(tPVU32, tU32, tU32);
 extern void Enable_stby_intr_vic(void);
@@ -125,7 +126,7 @@
 extern void arm_invalidate_icache(void);
 extern int  Burst_Read_LSI_FIFO(int);
 extern int  Burst_Read_PHY_TRAINING(int);
-void lsi_phy_fifo_training(void);
+int  lsi_phy_fifo_training(void);
 void clean_fifo(void);
 void ddrcfg_asic(tU8);
 
