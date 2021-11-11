--- uboot.org/cpu/arm926ejs/PILOTIII/kcsregs.h	1969-12-31 19:00:00.000000000 -0500
+++ uboot/cpu/arm926ejs/PILOTIII/kcsregs.h	2007-04-05 12:38:11.000000000 -0400
@@ -0,0 +1,194 @@
+/*****************************************************************
+ *****************************************************************
+ ***                                                            **
+ ***    (C)Copyright 2002-2003, American Megatrends Inc.        **
+ ***                                                            **
+ ***            All Rights Reserved.                            **
+ ***                                                            **
+ ***        6145-F, Northbelt Parkway, Norcross,                **
+ ***                                                            **
+ ***        Georgia - 30071, USA. Phone-(770)-246-8600.         **
+ ***                                                            **
+ *****************************************************************
+ *****************************************************************
+ *****************************************************************
+ * $Header: $
+ *
+ * $Revision: $
+ *
+ * $Date: $
+ *
+ ******************************************************************
+ ******************************************************************
+ * 
+ * KCSRegs.h
+ * KCS Register definitions
+ *
+ *  Author: Rama Rao Bisa <ramab@ami.com>
+ *			Modified by Anurag Bhatia for Pilot-II porting
+ ******************************************************************/
+#ifndef KCS_REGS_H
+#define KCS_REGS_H
+
+// 0x40426300
+/* LPC Base Address */ 
+//SE_KCS4_BASE
+#define LPC_BASE_ADDR	0x40426300
+
+
+/* Register Definitions */
+ #define KCS4_STATUS        0x03
+ #define KCS4_DATA_IN       0x00
+ #define KCS4_CMD_IN        0x02
+ #define KCS4_DATA_OUT      0x00
+
+
+/*** Local Macro Definitions ***/
+/**
+ * @def WRITE_KCS_STATUS_REG
+ * @brief Writes a byte into KCS port status register.
+ * @param STATUS a byte data.
+ **/
+#define WRITE_KCS_STATUS_REG(STATUS)	 *(lpc_base + KCS4_STATUS) = STATUS
+
+/**
+ * @def READ_KCS_STATUS_REG
+ * @brief Reads a byte from KCS port status register.
+ * @param STATUS a byte data.
+ **/
+#define READ_KCS_STATUS_REG(STATUS)       STATUS = *(lpc_base + KCS4_STATUS)
+
+/**
+ * @def KCS_DATA_OUT_REG
+ * @brief Writes a byte into KCS port data register.
+ * @param DATA a byte data.
+ **/
+#define KCS_DATA_OUT_REG(DATA)   		*(lpc_base + KCS4_DATA_OUT) = DATA 
+
+/**
+ * @def KCS_DATA_IN_REG
+ * @brief Reads a byte from KCS port data register.
+ * @param DATA a byte data.
+ **/
+#define KCS_DATA_IN_REG(DATA)            DATA = *(lpc_base + KCS4_DATA_IN)
+
+/**
+ * @def KCS_CMD_REG
+ * @brief Reads a byte from KCS port command register.
+ * @param CMD a byte data.
+ **/
+#define KCS_CMD_REG(CMD)				CMD = *(lpc_base + KCS4_CMD_IN)
+
+
+/**
+ * @def CLEAR_IBF_STATUS
+**/
+#define CLEAR_IBF_STATUS()                       \
+do {                                             \
+        u8 Status = 0;                           \
+        READ_KCS_STATUS_REG (Status);            \
+        Status = Status & (~0x02);               \
+        WRITE_KCS_STATUS_REG (Status);           \
+} while (0)
+
+/**
+ * @def SET_OBF_STATUS
+**/
+#define SET_OBF_STATUS()                         \
+do {                                                                                    \
+        u8 Status = 0;                                                  \
+        READ_KCS_STATUS_REG (Status);                \
+        Status = Status | 0x01 ;                \
+        WRITE_KCS_STATUS_REG (Status);                \
+} while (0)
+
+/**
+ * @def INIT_KCS
+ * @brief Enables KCS channel 1
+ **/
+#define INIT_KCS_HARDWARE() 			                    					\
+do {                                                        					\
+	CLEAR_IBF_STATUS();															\
+} while (0) 
+
+
+/** 
+ * @def SMS_KCS_BASE_ADDR
+ * @brief SMS KCS Base address
+**/
+#define SMS_KCS_BASE_ADDR		0xCA2
+
+/** 
+ * @def OBF_BIT_SET
+ * @brief Macro to check if output buffer is full
+**/
+#define OBF_BIT_SET(STATUS_REG) (0 != ((STATUS_REG) & 0x01))
+
+/** 
+ * @def IBF_BIT_SET
+ * @brief Macro to check if input buffer is full
+**/
+#define IBF_BIT_SET(STATUS_REG) (0 != ((STATUS_REG) & 0x02))
+
+/** 
+ * @def GET_KCS_STATE
+ * @brief Macro to return the current state of the KCS Interface.
+**/
+#define GET_KCS_STATE(STATUS_REG) ((STATUS_REG) & 0xC0)
+
+/** 
+ * @def SET_KCS_STATE
+ * @brief Macro to set the current state of the KCS Interface.
+**/
+#define SET_KCS_STATE(STATE)			 			\
+do {												\
+	unsigned char Status = 0;						\
+	READ_KCS_STATUS_REG (Status);					\
+	Status = ((Status & (~0xC0)) | (STATE));		\
+	WRITE_KCS_STATUS_REG (Status);					\
+} while (0)
+
+/** 
+ * @def IS_WRITE_TO_CMD_REG
+ * @brief Macro that returns true if last write was to command register.
+**/
+#define IS_WRITE_TO_CMD_REG(STATUS_REG) (0 != ((STATUS_REG) & 0x08))
+
+/** 
+ * @def IS_WRITE_TO_DATA_REG
+ * @brief Macro that returns true if last write was to data register.
+**/
+#define IS_WRITE_TO_DATA_REG(STATUS_REG) (0 == ((STATUS_REG) & 0x08))
+
+
+#if 0
+/**
+ * @def LPC_SOFT_RESET
+ **/
+#define LPC_SOFT_RESET()				                    					\
+do {                                                        					\
+	volatile unsigned char Temp;												\
+	Temp = lpc_base [HICR1];													\
+	Temp &= 0xEF;																\
+	lpc_base [HICR1] = Temp;													\
+	Temp |= 0x10;																\
+	lpc_base [HICR1] = Temp;													\
+	Temp &= 0xEF;																\
+	lpc_base [HICR1] = Temp;													\
+} while (0) 
+
+/**
+ * @def LPC_CORE_PRESENCE
+ **/
+#define LPC_CORE_PRESENCE(COREPRESENCE)	                    					\
+do {                                                        					\
+	volatile unsigned char HiAddr,LoAddr;										\
+	LoAddr = lpc_base [LADR12L];												\
+	HiAddr = lpc_base [LADR12H];												\
+	COREPRESENCE = 0;															\
+	if ((LoAddr == 0x60) && (HiAddr == 0x00)) COREPRESENCE = 1;					\
+} while (0) 
+
+#endif
+
+#endif  /* KCS_REGS_H */
