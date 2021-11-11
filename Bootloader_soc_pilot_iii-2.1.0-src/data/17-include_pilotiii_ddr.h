--- u-boot-1.1.6/include/pilotiii/ddr.h	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/include/pilotiii/ddr.h	2010-04-05 16:47:25.000000000 -0400
@@ -0,0 +1,145 @@
+//////////////////////////////////////////////////
+//#ServerEngines Public License
+//#This license governs use of the accompanying software. If you use the software, you
+//#accept this license. If you do not accept the license, do not use the software.
+//#
+//#1. Definitions
+//#The terms "reproduce," "reproduction," "derivative works," and "distribution" have the
+//#same meaning here as under U.S. copyright law.
+//#A "contribution" is the original software, or any additions or changes to the software.
+//#A "contributor" is any person that distributes its contribution under this license.
+//#"Licensed patents" are a contributor's patent claims that read directly on its contribution.
+//#
+//##2. Grant of Rights
+//###(A) Copyright Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free copyright license to reproduce its contribution, prepare derivative works of its contribution, and distribute its contribution or any derivative works that you create.
+//##(B) Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the contribution in the software.
+//##
+//##3. Conditions and Limitations
+//##(A) No Trademark License- This license does not grant you rights to use any contributors' name, logo, or trademarks.
+//##(B) If you bring a patent claim against any contributor over patents that you claim are infringed by the software, your patent license from such contributor to the software ends automatically.
+//#(C) If you distribute any portion of the software, you must retain all copyright, patent, trademark, and attribution notices that are present in the software.
+//#(D) If you distribute any portion of the software in source code form, you may do so only under this license by including a complete copy of this license with your distribution. If you distribute any portion of the software in compiled or object code form, you may only do so under a license that complies with this license.
+//#(E) The software is licensed "as-is." You bear the risk of using it. The contributors give no express warranties, guarantees or conditions. You may have additional consumer rights under your local laws which this license cannot change. To the extent permitted under your local laws, the contributors exclude the implied warranties of merchantability, fitness for a particular purpose and non-infringement.
+//##
+//###       Project  : Pilot
+
+
+
+
+
+
+#include "hwmap.h"
+#include "hwreg.h"
+#include "hwdef.h"
+
+
+#define MC_REG_ADDRESS00 (DDR_REG_BASE + 0x0)
+#define MC_REG_ADDRESS04 (DDR_REG_BASE + 0x04) 
+#define MC_REG_ADDRESS08 (DDR_REG_BASE + 0x08)
+#define MC_REG_ADDRESS0C (DDR_REG_BASE + 0x0C)
+#define MC_REG_ADDRESS10 (DDR_REG_BASE + 0x10)
+#define MC_REG_ADDRESS14 (DDR_REG_BASE + 0x14)
+#define MC_REG_ADDRESS18 (DDR_REG_BASE + 0x18)
+#define MC_REG_ADDRESS1C (DDR_REG_BASE + 0x1C)
+#define MC_REG_ADDRESS20 (DDR_REG_BASE + 0x20)
+#define MC_REG_ADDRESS24 (DDR_REG_BASE + 0x24)
+#define MC_REG_ADDRESS28 (DDR_REG_BASE + 0x28)
+#define MC_REG_ADDRESS2C (DDR_REG_BASE + 0x2C)
+#define MC_REG_ADDRESS30 (DDR_REG_BASE + 0x30)
+#define MC_REG_ADDRESS34 (DDR_REG_BASE + 0x34)
+#define MC_REG_ADDRESS38 (DDR_REG_BASE + 0x38)
+#define MC_REG_ADDRESS3C (DDR_REG_BASE + 0x3C)
+#define MC_REG_ADDRESS40 (DDR_REG_BASE + 0x40)
+#define MC_REG_ADDRESS44 (DDR_REG_BASE + 0x44)
+#define MC_REG_ADDRESS48 (DDR_REG_BASE + 0x48)
+#define MC_REG_ADDRESS4C (DDR_REG_BASE + 0x4C)
+#define MC_REG_ADDRESS50 (DDR_REG_BASE + 0x50)
+#define MC_REG_ADDRESS54 (DDR_REG_BASE + 0x54)
+#define MC_REG_ADDRESS58 (DDR_REG_BASE + 0x58)
+#define MC_REG_ADDRESS5C (DDR_REG_BASE + 0x5C)
+#define HM_MISC_CTRL        (DDR_REG_BASE + 0x200)
+#define HM_IOBZ_CTRL        (DDR_REG_BASE + 0x204)
+#define HM_RDPAT            (DDR_REG_BASE + 0x230)
+#define HM_DQSCLOCKSKEW     (DDR_REG_BASE + 0x260)
+#define HM_PROGGATEON       (DDR_REG_BASE + 0x290)
+#define HM_ADRCLOCK         (DDR_REG_BASE + 0x2B8)
+#define HM_DPBASEDDELAY     (DDR_REG_BASE + 0x2E0)
+#define HM_TRAINREQ         (DDR_REG_BASE + 0x2E4)
+#define HM_ALLOWTRAIN       (DDR_REG_BASE + 0x2E8)
+#define HM_DPLOADDELAY      (DDR_REG_BASE + 0x360)
+#define HM_DPREADDELAY      (DDR_REG_BASE + 0x364)
+#define HM_CORETYPE         (DDR_REG_BASE + 0x3EC)
+#define HM_PHYCOMPILE       (DDR_REG_BASE + 0x3F0)
+#define HM_PHYSYSTEMCODE    (DDR_REG_BASE + 0x3F4)
+#define HM_PHYMODULEVERSION (DDR_REG_BASE + 0x3F8)
+#define HM_PHYMODULECODE    (DDR_REG_BASE + 0x3FC)
+#define HM_FIFO_DELAY_1     (DDR_REG_BASE + 0x274)
+#define HM_FIFO_DELAY_2     (DDR_REG_BASE + 0x278)
+#define HM_FIFO_ALIGN       (DDR_REG_BASE + 0x27C)
+
+
+
+#define LSI_FIFO_TRAINING   (0x10007000)
+#define MAGM_VALUE       0x007fffff
+#define UAGM_VALUE       0x00000000
+#if defined (FPGA_DUT)
+#define LOWER_ECC_ADDRESS  (Bank1start + 0x2000)
+#define UPPER_ECC_ADDRESS  (LOWER_ECC_ADDRESS + 4*1024*1024 - 1)
+#else
+#define LOWER_ECC_ADDRESS 0x82000000
+#define UPPER_ECC_ADDRESS 0x83ffffff
+#endif
+#define QEMPTY_VALUE      0x081c0000
+#define PAGE_OPEN_TIME    0x00004c30
+#define DDR_LPWR_FUNCTION_INSPI   0x40
+#define DDR_LPWR_FUNCTION_INLMEM  0x10005000
+tU32 (* ddr_lpwr)(void);
+void ddr_lower_power_mode(tU8, tU8, tU8, tU8, tU8);
+void chg_ddrfreq(void);
+void ddr_go_to_lp(void);
+void ddr_selfrefresh (void);
+void ddr_activemode (void);
+void DELAY(tU32);
+void ddrcfg(tU32, tU8);
+void prg_gate(tU32);
+int gateon_training(void); 
+void redeye_training(void);
+extern void Burst_Write(tPVU32 , tU32, tU32);
+extern int Burst_Read(tPVU32, tU32, tU32);
+extern void Enable_stby_intr_vic(void);
+extern void Enable_standbyfi_intr(void);
+extern void Wait_For_PLLock(void);
+extern void DDR_Pll_In_Standby(void);
+extern void DDR_Pll_In_Runmode(void);
+extern void Switch_Clk_DDR_Pll(void);
+extern void Switch_Clk_DDR_OSC(void);
+extern void Pll_In_Standby(void);
+extern void Disable_standbyfi_intr(void);
+extern void save_and_change_stack_to_intram(tU32);
+extern void retrive_and_change_stack_to_ddr(tU32);
+extern void put_arm_in_stdby(void);
+extern void Disable_MMU_Cache(void);
+extern void Enable_MMU_Cache(void);
+extern void TCCacheD_922(void);
+extern void TCCacheD_926(void);
+extern void arm_invalidate_icache(void);
+extern int  Burst_Read_LSI_FIFO(int);
+extern int  Burst_Read_PHY_TRAINING(int);
+void lsi_phy_fifo_training(void);
+void clean_fifo(void);
+void ddrcfg_asic(tU8);
+
+typedef struct 
+	{
+	 tU8 mode;
+	 tU8 on;
+	 tU8 keepfreq;
+	 tU8 multof25mhz;
+	 tU8 timeout;
+	 tU8 reg_18_19;
+	 tU8 reg_10_3031;
+	 } DdrlpDataType;
+
+//#define Ddrlp  ((DdrlpDataType *)(INTRAMBASE))
+#define Ddrlp  ((DdrlpDataType *)(Bank1start+0x10000))
+ 
