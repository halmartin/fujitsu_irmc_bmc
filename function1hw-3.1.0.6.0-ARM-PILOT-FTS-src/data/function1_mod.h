/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */
#ifndef __FUNCTION1_HW_H__
#define __FUNCTION1_HW_H__

#define FUNCTION1_BASE_ADDR			0x40440000
#define FUNCTION1_REGS_SIZE			0x10000
#define FUNCTION1_SHARED_MEM_START	(CONFIG_SPX_FEATURE_GLOBAL_MEMORY_START+CONFIG_SPX_FEATURE_GLOBAL_MEMORY_SIZE-CONFIG_SPX_FEATURE_GLOBAL_PCIE_FUNCTION1_SHARED_MEM_SIZE)
#define FUNCTION1_SHARED_MEM_LEN	CONFIG_SPX_FEATURE_GLOBAL_PCIE_FUNCTION1_SHARED_MEM_SIZE	
#define SYSCTL_REG_MEM_START			0x4010091C
#define SYSCTL_REG_MEM_LEN				0x100

#define CONFIG_SPACE_OFFSET	 0x0F00

#define HOST_TO_BMC_Q1			0xA000
#define HOST_TO_BMC_Q2			0xA010
#define BMC_TO_HOST_Q1			0xA020
#define BMC_TO_HOST_Q2			0xA030
#define BMC_TO_HOST_Q_STATUS	0xA040
#define HOST_TO_BMC_Q_STATUS	0xA044

#define MSG_Q_BUFF_SIZE		(3*4) + 1				//JS: Increased due to a large number of warnings under heavy load
#define MAX_DWORDS_PER_MSGQ	4
#define MAX_Q					2
#define MSG_Q1					0
#define MSG_Q2					1

typedef struct
{
	unsigned int Offset;
	unsigned int Len;	
	unsigned char Data [FUNCTION1_SHARED_MEM_LEN]; 
		
} __attribute__ ((packed)) Function1SharedMem_t;

typedef struct
{
	unsigned int QData[MAX_Q][MAX_DWORDS_PER_MSGQ];
	unsigned char ValidQLength[MAX_Q];
	unsigned char QNum;
} __attribute__ ((packed)) Function1MsgQ_t;

/********************************************************************
 * Circular buffer for the interrupt handler to store the dwords 
 * read from the PCIe function1 message queues
 * 
 * The buffer is assumed to be empty, if the read index RdIdx equals
 * to the write index WrIdx.
 * This means that the buffer needs to be one dword bigger than it
 * it can store dwords, because the WrIdx needs to point to the next
 * free entry.
 ********************************************************************/
typedef struct
{
	unsigned int QData[MSG_Q_BUFF_SIZE];
	unsigned char WrIdx;
	unsigned char RdIdx;
} __attribute__ ((packed)) Function1MsgQBuf_t;


#define READ_FROM_SHARED_MEM					_IOC(_IOC_WRITE,'K',0x101,0x3FFF)
#define WRITE_TO_SHARED_MEM					_IOC(_IOC_WRITE,'K',0x102,0x3FFF)
#define WAIT_FOR_HOST_MSG					_IOC(_IOC_WRITE,'K',0x103,0x3FFF)
#define CLEAR_MSG_FROM_HOST					_IOC(_IOC_WRITE,'K',0x104,0x3FFF)
#define SEND_MSG_TO_HOST					_IOC(_IOC_WRITE,'K',0x105,0x3FFF)
#define ENABLE_SHARED_MEM					_IOC(_IOC_WRITE,'K',0x106,0x3FFF)
#define DISABLE_SHARED_MEM					_IOC(_IOC_WRITE,'K',0x107,0x3FFF)
#define CLEAR_H2B_MSG_BUFFERS					_IOC(_IOC_WRITE,'K',0x108,0x3FFF)
#define CLEAR_QUEUES						_IOC(_IOC_WRITE,'K',0x109,0x3FFF)

/* Bit definitions of BMC2HOST_Q_STATUS */

#define B2H_QUEUE2_EMPTY_UNMASK			(1<<20)
#define B2H_QUEUE2_FULL_UNMASK			(1<<21)
#define B2H_QUEUE1_EMPTY_UNMASK			(1<<22)
#define B2H_QUEUE1_FULL_UNMASK			(1<<23)
#define B2H_QUEUE2_EMPTY					(1<<24)
#define B2H_QUEUE2_FULL					(1<<25)
#define B2H_QUEUE1_EMPTY					(1<<26)
#define B2H_QUEUE1_FULL					(1<<27)
#define B2H_ENABLE_INTB					(1<<30)
#define B2H_INT_STATUS_DOORBELL			(1<<31)

/* Bit definitions of HOST2BMC_Q_STATUS */

#define H2B_QUEUE2_EMPTY_UNMASK			(1<<20)
#define H2B_QUEUE2_FULL_UNMASK			(1<<21)
#define H2B_QUEUE1_EMPTY_UNMASK			(1<<22)
#define H2B_QUEUE1_FULL_UNMASK			(1<<23)
#define H2B_QUEUE2_EMPTY					(1<<24)
#define H2B_QUEUE2_FULL						(1<<25)
#define H2B_QUEUE1_EMPTY					(1<<26)
#define H2B_QUEUE1_FULL						(1<<27)
#define H2B_ENABLE_INTB						(1<<30)
#define H2B_RING_DOORBELL					(1<<31)

#endif /* __FUNCTION1_HW_H__ */

