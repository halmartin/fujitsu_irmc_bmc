--- u-boot-1.1.6/include/pilotiii/macreg.h	1969-12-31 19:00:00.000000000 -0500
+++ u-boot-1.1.6-PilotIII/include/pilotiii/macreg.h	2010-04-05 16:47:25.000000000 -0400
@@ -0,0 +1,391 @@
+#ifndef AST2000_MAC_REG_H
+#define AST2000_MAC_REG_H
+
+
+/* -------------------------------------------------------------------------------
+ * 			PILOTII Ethernet MAC definitions amd structures
+ * -------------------------------------------------------------------------------
+ */
+
+#define AST_MAC_CSR0		0x00			/* Bus Mode */
+#define AST_MAC_CSR1		0x08			/* Xmit Poll Demand */
+#define AST_MAC_CSR2		0x10			/* Recv Poll Demand */
+#define AST_MAC_CSR3		0x18			/* Recv list base address */
+#define AST_MAC_CSR4		0x20			/* Xmit list base address */
+#define AST_MAC_CSR5		0x28			/* Status */
+#define AST_MAC_CSR6		0x30			/* Operation Mode */
+#define AST_MAC_CSR7		0x38			/* Interrupt Enable */
+#define AST_MAC_CSR8		0x40			/* Missed frames and overflow counters */
+#define AST_MAC_CSR9		0x48			/* MII management */
+#define AST_MAC_CSR11		0x58			/* Timer and interrupt mitigation control */
+
+#define AST_MAC_CSR(x)		((x)*8)
+
+/*---------------------------------------------------------------------------------
+ * 							Bit Definitions of MAC Registers
+ *---------------------------------------------------------------------------------*/
+#define SETUP_FRAME_SIZE 	192
+
+/*---------------------------------------------------------------------------------*/
+						/* CSR0 Bus Mode Register */
+/*---------------------------------------------------------------------------------*/
+#define CSR0_DBO		((u32)0x01 << 20)	//Descriptor byte ordering mode
+
+#define CSR0_TAP_MSK	((u32)0x07 << 17)	// Transmit automatic polling mask */
+#define CSR0_TAP_VAL(x)	(((u32)(x) << 17) & CSR0_TAP_MSK)
+
+#define CSR0_PBL_MSK	((u32)0x3F << 8)	// Dma burst length mask
+#define CSR0_PBL_VAL(x)	(((u32)(x) << 8) & CSR0_PBL_MSK)
+
+#define CSR0_PBL_0		CSR0_PBL_VAL(0 )	// 0 longwords DMA'ed
+#define CSR0_PBL_1		CSR0_PBL_VAL(1 )	// 1 longwords DMA'ed
+#define CSR0_PBL_2		CSR0_PBL_VAL(2 )	// 2 longwords DMA'ed
+#define CSR0_PBL_4		CSR0_PBL_VAL(4 )	// 4 longwords DMA'ed
+#define CSR0_PBL_8		CSR0_PBL_VAL(8 )	// 8 longwords DMA'ed
+#define CSR0_PBL_16		CSR0_PBL_VAL(16)	// 16 longwords DMA'ed
+#define CSR0_PBL_32		CSR0_PBL_VAL(32)	// 32 longwords DMA'ed
+
+#define CSR0_BLE		((u32)0x01 << 7)	// Big/little endian
+
+#define CSR0_DSL_MSK	((u32)0x1F << 2)	// Descriptor skip length
+#define CSR0_DSL_VAL(x) (((u32)(x) << 2) & CSR0_DSL_MSK)
+
+#define	CSR0_BAR_EQU	0x00000000
+#define	CSR0_BAR_RXH	0x00000002
+#define CSR0_BAR_VAL(x) (((u32)(x) << 1) & CSR0_BAR_RXH)
+
+#define CSR0_SWR		((u32)0x01 << 0)	// software reset
+
+
+/*---------------------------------------------------------------------------------*/
+				/* CSR1 Transmit Poll Demand Register */
+/*---------------------------------------------------------------------------------*/
+#define CSR1_TPD		0x00000001			// Transmit poll demand
+
+/*---------------------------------------------------------------------------------*/
+				/* CSR2 Recieve Poll Demand Register */
+/*---------------------------------------------------------------------------------*/
+#define CSR2_RPD		0x00000001			// Transmit poll demand
+
+/*---------------------------------------------------------------------------------*/
+				/* CSR3 Receive List Base address Register */
+/*---------------------------------------------------------------------------------*/
+#define CSR3_RDBA_MSK	0xFFFFFFFC			// long word aligned
+#define CSR3_RDBA_VAL(x) ((x) & CSR3_RDBA_MSK)
+
+/*---------------------------------------------------------------------------------*/
+				/* CSR4 Transmit List Base address Register */
+/*---------------------------------------------------------------------------------*/
+#define CSR4_TDBA_MSK	0xFFFFFFFC			// long word aligned
+#define CSR4_TDBA_VAL(x) ((x) & CSR4_TDBA_MSK)
+
+/*---------------------------------------------------------------------------------*/
+						/* CSR5 Status register */
+/*---------------------------------------------------------------------------------*/
+#define CSR5_TS_MSK		((u32)0x07 << 20)
+#define CSR5_TS_ST		0x00000000			// Stopped
+#define CSR5_TS_RFTD	0x00100000			// Running Fetch xmit descriptor
+#define CSR5_TS_RWET	0x00200000			// Running Wait for end of Xmission
+#define CSR5_TS_RRBM	0x00300000			// Running Read buff from memory
+#define CSR5_TS_RSP		0x00500000			// Running Set up packet
+#define CSR5_TS_STFU	0x00600000			// Suspended xmit FIFO underflow
+#define CSR5_TS_RCTD	0x00700000			// Running Close xmit descriptor
+
+#define CSR5_RS_MSK		((u32)0x07 << 17)
+#define CSR5_RS_ST		0x00000000			// stopped reset or stop rcv command
+#define CSR5_RS_RFRD	0x00020000			// Running Fetch rcv descriptor
+#define CSR5_RS_RCEP	0x00040000			// Running Check end of rcv packet
+#define CSR5_RS_RWRP	0x00060000			// Running Wait for rcv packet
+#define CSR5_RS_SURB	0x00080000			// Suspended - unavailable rcv buff
+#define CSR5_RS_RCRD	0x000A0000			// Running close rcv descriptor
+#define CSR5_RS_RFFF	0x000C0000			// flush frame from rcv FIFO
+#define CSR5_RS_RQRF	0x000E0000			// queue the rcv frame into rcv buff
+
+#define CSR5_NIS		((u32)0x01 << 16)	// normal interrupt summary
+#define CSR5_AIS		((u32)0x01 << 15)	// abnormal interrupt summary
+#define CSR5_ERI		((u32)0x01 << 14)	// early rcv interrupt
+#define CSR5_GPTE		((u32)0x01 << 11)	// General Purpose Timer Expire
+#define CSR5_ETI		((u32)0x01 << 10)	// early xmit interrupt
+#define CSR5_RPS		((u32)0x01 <<  8)	// rcv process stopped
+#define CSR5_RU			((u32)0x01 <<  7)	// rcv buffer unavailable
+#define CSR5_RI			((u32)0x01 <<  6)	// rcv interrupt
+#define CSR5_UNF		((u32)0x01 <<  5)	// xmit underflow
+#define CSR5_TU			((u32)0x01 <<  2)	// xmit buffer unavailable
+#define CSR5_TPS		((u32)0x01 <<  1)	// xmit Process stopped
+#define CSR5_TI			((u32)0x01 <<  0)	// xmit interrupt
+#define CSR5_ALLINT_MASK	( 0				\
+							| CSR5_NIS		\
+							| CSR5_AIS		\
+							| CSR5_ERI		\
+							| CSR5_GPTE		\
+							| CSR5_ETI		\
+							| CSR5_RPS		\
+							| CSR5_RU		\
+							| CSR5_RI		\
+							| CSR5_UNF		\
+							| CSR5_TU		\
+							| CSR5_TPS		\
+							| CSR5_TI		\
+                            )
+
+/*---------------------------------------------------------------------------------*/
+				/* CSR6 Operation Mode Register */
+/*---------------------------------------------------------------------------------*/
+//#define CSR6_SPEED		((u32)0x01 << 31)	// SPEED 1:100M 0:10M
+
+#define CSR6_RA			((u32)0x01 << 30)	// Receive all
+#define	CSR6_TTM		((u32)0x01 << 22)	// transmit thresold mode
+#define	CSR6_SF			((u32)0x01 << 21)	// store forward mode
+
+#define CSR6_TR_STEP	((u32)0x01 << 14)	// Threshold Control Bit step size
+#define CSR6_TR_MSK		((u32)0x03 << 14)	// Threshold Control Bit
+#define CSR6_TR_VAL(x)	(((u32)(x) << 14) & CSR6_TR_MSK)
+
+#define	CSR6_ST			((u32)0x01 << 13)	// start/stop Xmit command
+#define	CSR6_FD			((u32)0x01 <<  9)	// Full Duplex mode
+#define	CSR6_PM			((u32)0x01 <<  7)	// Pass all multicast
+#define	CSR6_PR			((u32)0x01 <<  6)	// promiscuous mode
+#define	CSR6_IF			((u32)0x01 <<  4)	// inverse filtering
+#define	CSR6_PB			((u32)0x01 <<  3)	// pass bad frames
+#define	CSR6_HO			((u32)0x01 <<  2)	// hash only filtering mode
+#define	CSR6_SR			((u32)0x01 <<  1)	// start/stop receive command
+#define	CSR6_HP			((u32)0x01 <<  0)	// hash/perfect recv filtering mode
+
+/*---------------------------------------------------------------------------------*/
+					/* CSR7 Interrupt Mask register */
+/*---------------------------------------------------------------------------------*/
+#define	CSR7_NIS		CSR5_NIS			// normal interrupt summary
+#define	CSR7_AIS		CSR5_AIS			// abnormal interrupt summary
+#define	CSR7_ERI		CSR5_ERI			// early rcv interrupt
+#define	CSR7_GPTE		CSR5_GPTE			// General Purpose Timer Expire
+#define	CSR7_ETI		CSR5_ETI			// early xmit interrupt
+#define	CSR7_RPS		CSR5_RPS			// rcv process stopped
+#define	CSR7_RU			CSR5_RU				// rcv buffer unavailable
+#define	CSR7_RI			CSR5_RI				// rcv interrupt
+#define	CSR7_UNF		CSR5_UNF			// xmit underflow
+#define	CSR7_TU			CSR5_TU				// xmit buffer unavailable
+#define	CSR7_TPS		CSR5_TPS			// xmit Process stopped
+#define	CSR7_TI			CSR5_TI				// xmit interrupt
+
+/*---------------------------------------------------------------------------------*/
+					/* CSR8 Missing Frame Counter */
+/*---------------------------------------------------------------------------------*/
+#define	CSR8_OCO		((u32)0x01 << 28)	// overflow conter overflow
+#define CSR8_FOC_SHF	17					// FIFO overflow counter shift bits
+#define CSR8_FOC_MSK	((u32)0x7FF << CSR8_FOC_SHF)	// FIFO overflow counter counter mask
+#define CSR8_FOC_GET(x)	(((x) & CSR8_FOC_MSK) >> CSR8_FOC_SHF)
+#define CSR8_OCOFOC_GET(x)	(((x) & (CSR8_FOC_MSK|CSR8_OCO)) >> CSR8_FOC_SHF)
+#define	CSR8_MFO		((u32)0x01 << 16)	// missed frame overflow
+#define CSR8_MFC_MSK	0x0000FFFF			// Missed frame counter mask
+
+/*---------------------------------------------------------------------------------*/
+					/* CSR9 MII Management Register */
+/*---------------------------------------------------------------------------------*/
+#define	CSR9_MDI		0x00080000			// MII mgmt data in
+#define	CSR9_MDI_SHF	19
+#define	CSR9_MII_RD		0x00000000			// MII mgmt read mode
+#define	CSR9_MII_WR		0x00040000			// MII mgmt write mode
+#define	CSR9_MDO		0x00020000			// MII mgmt write data
+#define	CSR9_MDO_SHF	17
+#define	CSR9_MDC		0x00010000			// MII mgmt clock - 21140
+
+#define	CSR9_MII_DBIT_RD(x)	(((x) & CSR9_MDI) >> CSR9_MDI_SHF)
+#define CSR9_MII_DBIT_WR(x)	(((x) & 0x1) << CSR9_MDO_SHF)
+
+#define	CSR9_SDO		(1<<3)		//GPIO Out
+#define	CSR9_SDI		(1<<2)		//GPIO In
+#define	CSR9_SCLK		(1<<1)		//GPIO Out
+#define	CSR9_SCE		(1<<0)		//GPIO Out
+
+
+/*---------------------------------------------------------------------------------*/
+					/* CSR11 Full Duplex Register */
+/*---------------------------------------------------------------------------------*/
+#define	CSR11_CS			((u32)0x01 << 31)	// Cycle size
+#define CSR11_CS_VAL(x)		(((u32)(x) << 31) & CSR11_CS)
+
+#define CSR11_TT_MSK		((u32)0x0F << 27)	// Transmit timer
+#define CSR11_TT_VAL(x) 	(((u32)(x) << 27) & CSR11_TT_MSK)
+#define CSR11_TT_GET(x)		(((x) & CSR11_TT_MSK) >> 27)
+
+#define CSR11_NTP_MSK		((u32)0x07 << 24)	// Number of transmit packet
+#define CSR11_NTP_VAL(x)	(((u32)(x) << 24) & CSR11_NTP_MSK)
+#define CSR11_NTP_GET(x)	(((x) & CSR11_NTP_MSK) >> 24)
+
+#define CSR11_RT_MSK		((u32)0x0F << 20)	// Receive timer
+#define CSR11_RT_VAL(x) 	(((u32)(x) << 20) & CSR11_RT_MSK)
+#define CSR11_RT_GET(x)		(((x) & CSR11_RT_MSK) >> 20)
+
+#define CSR11_NRP_MSK		((u32)0x07 << 17)	// Transmit timer
+#define CSR11_NRP_VAL(x)	(((u32)(x) << 17) & CSR11_NRP_MSK)
+#define CSR11_NRP_GET(x)	(((x) & CSR11_NRP_MSK) >> 17)
+
+#define	CSR11_CON_MODE		0x00010000			// GPT Continuous Mode
+#define CSR11_CON_VAL(x) 	(((u32)(x) << 16) & CSR11_CON_MODE)
+#define	CSR11_TIM_MASK		0x0000FFFF			// GPT Timer Value Mask
+#define CSR11_TIM_VAL(x) 	(((u32)(x)      ) & CSR11_TIM_MASK)
+
+/*---------------------------------------------------------------------------------*/
+/*------------------- Defines for Recv and Xmit Descriptors -----------------------*/
+/*---------------------------------------------------------------------------------*/
+
+/* receive descriptor 0 */
+#define RDESC0_OWN			0x80000000	// Own
+#define RDESC0_FF			0x40000000	// Filtering Fail
+
+#define RDESC0_FL_MSK		0x3FFF0000	// Frame length mask
+#define RDESC0_FL_GET(x)	(((x) & RDESC0_FL_MSK) >> 16)
+#define RDESC0_FL_PUT(x)	(((x) << 16) & RDESC0_FL_MSK)
+
+#define RDESC0_ES			0x00008000	// Error summary
+#define RDESC0_DE			0x00004000  // Descriptor error
+#define RDESC0_RF			0x00000800	// runt frame
+#define RDESC0_MF			0x00000400	// multicast frame
+#define RDESC0_FD			0x00000200	// first descriptor
+#define RDESC0_LS			0x00000100	// last descriptor
+#define RDESC0_TL			0x00000080	// frame too long
+#define RDESC0_CS			0x00000040	// collision seen
+#define RDESC0_FT			0x00000020	// frame type
+#define RDESC0_RE			0x00000008	// Report MII error
+#define RDESC0_DB			0x00000004	// dribbling bit
+#define RDESC0_CE			0x00000002	// crc error
+#define RDESC0_FIFOERR		0x00000001	// Legal length
+#define	RDESC0_FT_GET(x)	(((x) & RDESC0_FT) >> 5)
+
+/* receive descriptor 1 */
+#define RDESC1_RER			0x02000000	// recv end of ring
+#define RDESC1_RCH			0x01000000	// second address chained
+
+#define RDESC1_RBS2_MSK		0x003FF800	// RBS2 buffer 2 size
+#define RDESC1_RBS1_MSK		0x000007FF	// RBS1 buffer 1 size
+
+#define RDESC1_RBS1_VAL(x)	((x) & RDESC1_RBS1_MSK)	// multiple of 4
+#define RDESC1_RBS2_VAL(x)	(((x) << 11) & RDESC1_RBS2_MSK)
+
+/* xmit descriptor 0 */
+#define TDESC0_OWN			0x80000000	// own */
+#define TDESC0_ES			0x00008000	// error summary
+#define TDESC0_LO			0x00000800	// loss of carrier
+#define TDESC0_NC			0x00000400	// NC No carrier
+#define TDESC0_LC			0x00000200	// late collision
+#define TDESC0_EC			0x00000100	// excessive collision
+
+#define TDESC0_CC_MSK		0x00000078	// collision count
+#define TDESC0_CC_GET(x)    (((x) & TDESC0_CC_MSK) >> 3)
+
+#define TDESC0_UF			0x00000002	// underflow error
+#define TDESC0_DE			0x00000001	// deffered
+
+/* xmit descriptor 1 */
+#define TDESC1_IC			0x80000000	// interrupt on completion
+#define TDESC1_LS			0x40000000	// last segment
+#define TDESC1_FS			0x20000000	// first segment
+#define TDESC1_FT1			0x10000000	// filtering type
+#define TDESC1_SET			0x08000000	// setup packet
+#define TDESC1_AC			0x04000000	// add crc disable
+#define TDESC1_TER			0x02000000	// xmit end of ring
+#define TDESC1_TCH			0x01000000	// second address chained
+#define TDESC1_DPD			0x00800000	// disabled padding
+#define TDESC1_FT0			0x00400000	// filtering type
+
+#define TDESC1_TBS2_MSK		0x003FF800	// TBS2 buffer 2 size
+#define TDESC1_TBS1_MSK		0x000007FF	// TBS2 buffer 1 size
+
+#define TDESC1_PERFECT		0x00000000
+#define TDESC1_FT0			0x00400000
+#define TDESC1_FT1			0x10000000
+#define TDESC1_HASH			TDESC1_FT0
+#define TDESC1_INVERSE		TDESC1_FT1
+#define TDESC1_HASH_ONLY	(TDESC1_FT0|TDESC1_FT1)
+
+/*---------------------------------------------------------------------------------*/
+/*-------------------------------- Setup Frame  -----------------------------------*/
+/*---------------------------------------------------------------------------------*/
+#define FLTR_FRM_SIZE			0xC0		// filter frm size 192 bytes
+#define FLTR_FRM_SIZE_ULONGS	(FLTR_FRM_SIZE / sizeof (u32))
+#define FLTR_FRM_ADRS_NUM		0x10		// filter frm holds 16 addrs
+#define FLTR_FRM_ADRS_SIZE		0x06		// size of each phys addrs
+#define FLTR_FRM_DEF_ADRS		0xFFFFFFFF	// enet broad cast address
+
+#define FLTR_HASH_FRM_SIZE			0x80		// Hash filter frm size 128 bytes
+#define FLTR_HASH_FRM_SIZE_ULONGS	(FLTR_HASH_FRM_SIZE / sizeof (u32))
+
+/*---------------------------------------------------------------------------------*/
+/*------------------------------- MII/PHY defines ---------------------------------*/
+/*---------------------------------------------------------------------------------*/
+#define	MAC_MAX_PHY			32	// max number of PHY devices
+#define	MAC_MAX_LINK_TOUT	16	// max link timeout (in secs)
+#define MII_PREAMBLE		((u32) 0xFFFFFFFF)
+
+/* MII frame header format */
+#define MII_SOF				0x4	// start of frame
+#define MII_RD				0x2	// op-code: Read
+#define	MII_WR				0x1	// op-code: Write
+
+/* MII PHY registers */
+#define MII_PHY_CR			0x00	// Control Register
+#define MII_PHY_SR			0x01	// Status Register
+#define MII_PHY_ID0			0x02	// Identifier Register 0
+#define MII_PHY_ID1			0x03	// Identifier Register 1
+#define MII_PHY_ANA			0x04	// Auto Negot'n Advertisement
+#define MII_PHY_ANLPA		0x05	// Auto Negot'n Link Partner Ability
+#define MII_PHY_ANE			0x06	// Auto Negot'n Expansion
+#define MII_PHY_ANP			0x07	// Auto Negot'n Next Page TX
+/* SW ISSUE */
+#define	MII_PHY_LBR         0x11	// Loopback Register
+#define	MII_PHY_TEST        0x19	// Test Register
+
+/* MII_PHY control register */
+#define	MII_PHY_CR_RESET	0x8000	/* reset */
+#define	MII_PHY_CR_LOOP		0x4000	/* loopback enable */
+#define	MII_PHY_CR_100M		0x2000	/* speed 100Mbps */
+#define	MII_PHY_CR_10M		0x0000	/* speed 10Mbps */
+#define	MII_PHY_CR_AUTO		0x1000	/* auto speed enable */
+#define	MII_PHY_CR_OFF		0x0800	/* powerdown mode */
+#define	MII_PHY_CR_RAN		0x0200	/* restart auto negotiate */
+#define	MII_PHY_CR_FDX		0x0100	/* full duplex mode */
+#define	MII_PHY_CR_HDX		0x0000	/* Half duplex mode */
+
+/* MII PHY status register */
+#define MII_PHY_SR_100T4	0x8000	/* 100BASE-T4 capable */
+#define MII_PHY_SR_100TX_FD	0x4000	/* 100BASE-TX Full Duplex capable */
+#define MII_PHY_SR_100TX_HD	0x2000	/* 100BASE-TX Half Duplex capable */
+#define MII_PHY_SR_10TFD	0x1000	/* 10BASE-T Full Duplex capable */
+#define MII_PHY_SR_10THD	0x0800	/* 10BASE-T Half Duplex capable */
+#define MII_PHY_SR_MF		0x0040  /* Management Frams preamble suppressed */
+#define MII_PHY_SR_ASS		0x0020	/* Auto Speed Selection Complete*/
+#define MII_PHY_SR_RFD		0x0010	/* Remote Fault Detected */
+#define MII_PHY_SR_AN		0x0008	/* Auto Negotiation capable */
+#define MII_PHY_SR_LNK		0x0004	/* Link Status */
+#define MII_PHY_SR_JABD		0x0002	/* Jabber Detect */
+#define MII_PHY_SR_XC		0x0001	/* Extended Capabilities */
+
+/* MII PHY Auto Negotiation Advertisement Register */
+#define MII_PHY_ANA_TAF		0x03e0	/* Technology Ability Field */
+/* SW ISSUE */
+#define MII_PHY_ANA_T4AM	0x0200	/* T4 Technology Ability Mask */
+#define MII_PHY_ANA_100F	0x0100
+#define MII_PHY_ANA_100H	0x0080
+#define MII_PHY_ANA_10F		0x0040
+#define MII_PHY_ANA_10H		0x0020
+#define MII_PHY_ANA_FDAM	(MII_PHY_ANA_100F|MII_PHY_ANA_10F)	/* Full Duplex Technology Ability Mask */
+#define MII_PHY_ANA_HDAM	(MII_PHY_ANA_100H|MII_PHY_ANA_10H)	/* Half Duplex Technology Ability Mask */
+#define MII_PHY_ANA_100M	(MII_PHY_ANA_100F|MII_PHY_ANA_100H)	/* 100Mb Technology Ability Mask */
+#define MII_PHY_ANA_10M		(MII_PHY_ANA_10F|MII_PHY_ANA_10H)	/* 10Mb Technology Ability Mask */
+#define MII_PHY_ANA_ALLAM	(MII_PHY_ANA_T4AM|MII_PHY_ANA_100M|MII_PHY_ANA_10M)
+#define MII_PHY_ANA_CSMA	0x0001	/* CSMA-CD Capable */
+
+/* MII PHY Auto Negotiation Remote End Register */
+#define MII_PHY_ANLPA_NP	0x8000	/* Next Page (Enable) */
+#define MII_PHY_ANLPA_ACK	0x4000	/* Remote Acknowledge */
+#define MII_PHY_ANLPA_RF	0x2000	/* Remote Fault */
+#define MII_PHY_ANLPA_TAF	0x03e0	/* Technology Ability Field */
+#define MII_PHY_ANLPA_T4AM	0x0200	/* T4 Technology Ability Mask */
+#define MII_PHY_ANLPA_TXAM	0x0180	/* TX Technology Ability Mask */
+#define MII_PHY_ANLPA_FDAM	0x0140	/* Full Duplex Technology Ability Mask */
+#define MII_PHY_ANLPA_HDAM	0x02a0	/* Half Duplex Technology Ability Mask */
+#define MII_PHY_ANLPA_100M	0x0380	/* 100Mb Technology Ability Mask */
+#define MII_PHY_ANLPA_10M	0x0060	/* 10Mb Technology Ability Mask */
+#define MII_PHY_ANLPA_CSMA	0x0001	/* CSMA-CD Capable */
+
+#endif
