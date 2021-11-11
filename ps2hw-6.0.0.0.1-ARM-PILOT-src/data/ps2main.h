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
#ifndef __PS2_MAIN_H__
#define __PS2_MAIN_H__

#define PS2_MAX_PORTS		2

/* Pilot-ii PS2 Registers */
#define PS21_KMCTL_REG		0x00		/* PS2-1 Control Register */
#define PS21_KMSTS_REG		0x04		/* PS2-1 Status Register */
#define PS21_KMDAT_REG		0x08		/* PS2-1 Receive(Read)/Transimt(Write) Data Register */
#define PS21_KMCLK_REG		0x0C		/* PS2-1 Clock Control and Divisor Register */
#define PS21_KMISR_REG		0x10		/* PS2-1 Interrupt Status Register */

#define KMCIR_REG			0x14		/* PS2 Common Interrupt Status */

#define PS22_KMCTL_REG		0x20		/* PS2-2 Control Register */
#define PS22_KMSTS_REG		0x24		/* PS2-2 Status Register */
#define PS22_KMDAT_REG		0x28		/* PS2-2 Receive(Read)/Transimt(Write) Data Register */
#define PS22_KMCLK_REG		0x2C		/* PS2-2 Clock Control and Divisor Register */
#define PS22_KMISR_REG		0x30		/* PS2-2 Interrupt Status Register */

/* PS2 KM Control Register bit define */
#define KMCTL_FCLK			(1 << 0)	/* Force Clock Pin Low */
#define KMCTL_FDAT			(1 << 1)	/* Force Data Pin Low */
#define KMCTL_KMEN			(1 << 2)	/* Keyboard Mouse Interface Enable */
#define KMCTL_TXIEN			(1 << 3)	/* Transimt Interrupt Enable */
#define KMCTL_RXIEN			(1 << 4)	/* Receive Interrupt Enable */
#define KMCTL_TXNAIE		(1 << 5)	/* TX No ACK Interrupt Enable */
#define KMCTL_RXOIE			(1 << 6)	/* RX Overrun Interrupt Enable */
#define KMCTL_RXRCDE		(1 << 7)	/* Release Code Drop Enable */

/* PS2 KM Status Register bit define */
#define KMSTS_KMDI			(1 << 0)	/* Keyboard Mouse Data Input */
#define KMSTS_KMCI			(1 << 1)	/* Keyboard Mouse Clock Input */
#define KMSTS_RXPAR			(1 << 2)	/* Receive Parity */
#define KMSTS_RXBSY			(1 << 3)	/* Receive Busy */
#define KMSTS_RXFUL			(1 << 4)	/* Receive Full */
#define KMSTS_TXBSY			(1 << 5)	/* Transimt Busy */
#define KMSTS_TXEMT			(1 << 6)	/* Transimt Empty */

/* PS2 KM Interrupt Status Register bit define */
#define KMISR_RXINT			(1 << 0)	/* Keyboard Mouse Receive Interrupt */
#define KMISR_TXINT			(1 << 1)	/* Keyboard Mouse Transimt Interrupt */
#define KMISR_TX_NO_ACK		(1 << 2)	/* Tx No ACK Interrupt */
#define KMISR_RX_OVERRUN	(1 << 3)	/* Rx Overrun Interrupt */

/**
 * @def SET_PS2_DIVISOR_REG
 * @brief Reads a byte from PS2 port status register.
 * @param CHANNEL The PS2 channel number.
 * @param STATUS a byte data.
 **/
#define SET_PS2_DIVISOR_REG(CHANNEL, STATUS)                		\
do {                                                        		\
    switch (CHANNEL) {                                      		\
    case KEYBOARD: *(ps2_vadd + PS21_KMCLK_REG) = STATUS; break;	\
    case MOUSE	 : *(ps2_vadd + PS22_KMCLK_REG) = STATUS; break;  	\
    }                                                       		\
} while (0)

/**
 * @def READ_DATA
 * @brief Read data from PS2 device.
 * @param CHANNEL The PS2 channel number.
 **/
#define READ_DATA(CHANNEL, VALUE)									\
do {                                                        		\
    switch (CHANNEL) {                                      		\
    case KEYBOARD: VALUE = *(ps2_vadd + PS21_KMDAT_REG); break;		\
    case MOUSE	 : VALUE = *(ps2_vadd + PS22_KMDAT_REG); break;		\
    }                                                       		\
} while (0)

/**
 * @def WRITE_CMD
 * @brief Write command to PS2 davice.
 * @param CHANNEL The PS2 channel number.
 **/
#define WRITE_CMD(CHANNEL, VALUE)									\
do {                                                        		\
    switch (CHANNEL) {                                      		\
    case KEYBOARD: *(ps2_vadd + PS21_KMDAT_REG) = VALUE; break;		\
    case MOUSE	 : *(ps2_vadd + PS22_KMDAT_REG) = VALUE; break;		\
    }                                                       		\
} while (0)

/**
 * @def READ_PS2_STATUS_REG
 * @brief Reads a byte from PS2 port status register.
 * @param CHANNEL The PS2 channel number.
 * @param STATUS a byte data.
 **/
#define READ_PS2_STATUS_REG(CHANNEL, STATUS)                		\
do {                                                        		\
    switch (CHANNEL) {                                      		\
    case KEYBOARD: STATUS = *(ps2_vadd + PS21_KMSTS_REG); break;	\
    case MOUSE	 : STATUS = *(ps2_vadd + PS22_KMSTS_REG); break;  	\
    }                                                       		\
} while (0)

/**
 * @def READ_PS2_INT_STATUS_REG
 * @brief Reads which one PS2 port has interrupted.
 * @param STATUS a byte data.
 **/
#define READ_PS2_INT_STATUS_REG(STATUS)               				\
do {                                                        		\
		STATUS = *(ps2_vadd + KMCIR_REG); break;					\
} while (0)

/**
 * @def READ_INT_STATUS_REG
 * @brief Reads a byte from PS2 port status register.
 * @param CHANNEL The PS2 channel number.
 * @param STATUS a byte data.
 **/
#define READ_INT_STATUS_REG(CHANNEL, STATUS)               			\
do {                                                        		\
    switch (CHANNEL) {                                      		\
    case KEYBOARD: STATUS = *(ps2_vadd + PS21_KMISR_REG); break;	\
    case MOUSE	 : STATUS = *(ps2_vadd + PS22_KMISR_REG); break;  	\
    }                                                       		\
} while (0)

/**
 * @def READ_PS2_CONTROL_REG
 * @brief Reads a byte from PS2 port control register.
 * @param CHANNEL The PS2 channel number.
 * @param STATUS a byte data.
 **/
#define READ_PS2_CONTROL_REG(CHANNEL, STATUS)                		\
do {                                                        		\
    switch (CHANNEL) {                                      		\
    case KEYBOARD: STATUS = *(ps2_vadd + PS21_KMCTL_REG); break;	\
    case MOUSE	 : STATUS = *(ps2_vadd + PS22_KMCTL_REG); break;  	\
    }                                                       		\
} while (0)

/**
 * @def READ_PS2_CONTROL_REG
 * @brief Reads a byte from PS2 port control register.
 * @param CHANNEL The PS2 channel number.
 * @param STATUS a byte data.
 **/
#define WRITE_PS2_CONTROL_REG(CHANNEL, STATUS)                		\
do {                                                        		\
    switch (CHANNEL) {                                      		\
    case KEYBOARD: *(ps2_vadd + PS21_KMCTL_REG) = STATUS; break;	\
    case MOUSE	 : *(ps2_vadd + PS22_KMCTL_REG) = STATUS; break;  \
    }                                                       		\
} while (0)

/**
 * @def ENABLE_PS2_INTERFACE
 * @brief Enable PS2 interface.
 * @param CHANNEL The PS2 channel number.
 **/
#define ENABLE_PS2_INTERFACE(CHANNEL)  				\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status |= 0x04;									\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)

/**
 * @def DISABLE_PS2_INTERFACE
 * @brief Disable PS2 interface.
 * @param CHANNEL The PS2 channel number.
 **/
#define DISABLE_PS2_INTERFACE(CHANNEL)  			\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status &= ~(0x04);								\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)

/**
 * @def ENABLE_FORCE_DATA_LOW
 * @brief Enable Force PS2 data pin low.
 * @param CHANNEL The PS2 channel number.
 **/
#define ENABLE_FORCE_DATA_LOW(CHANNEL)				\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status |= 0x02;									\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)

/**
 * @def DISABLE_FORCE_DATA_LOW
 * @brief Disable Force PS2 data pin low.
 * @param CHANNEL The PS2 channel number.
 **/
#define DISABLE_FORCE_DATA_LOW(CHANNEL)				\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status &= ~(0x02);								\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)

/**
 * @def ENABLE_FORCE_CLOCK_LOW
 * @brief Enable Force PS2 clock pin low.
 * @param CHANNEL The PS2 channel number.
 **/
#define ENABLE_FORCE_CLOCK_LOW(CHANNEL)				\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status |= 0x01;									\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)

/**
 * @def DISABLE_FORCE_CLOCK_LOW
 * @brief Disable Force PS2 clock pin low.
 * @param CHANNEL The PS2 channel number.
 **/
#define DISABLE_FORCE_CLOCK_LOW(CHANNEL)			\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status &= ~(0x01);								\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)

/**
 * @def ENABLE_PS2_INTERRUPTS
 * @brief Enable PS2 interrupts.
 * @param CHANNEL The PS2 channel number.
 **/
#define ENABLE_PS2_INTERRUPTS(CHANNEL,VALUE)		\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status |= VALUE;								\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)

/**
 * @def DISABLE_PS2_INTERRUPTS
 * @brief Disable PS2 interrupts.
 * @param CHANNEL The PS2 channel number.
 **/
#define DISABLE_PS2_INTERRUPTS(CHANNEL, VALUE)		\
do {                                                \
	u8 Status = 0;									\
	READ_PS2_CONTROL_REG ((CHANNEL), Status);		\
	Status &= ~(VALUE);								\
	WRITE_PS2_CONTROL_REG ((CHANNEL),Status);		\
} while (0)


#define	KEYBOARD		0
#define	MOUSE			1

typedef struct 
{
	wait_queue_head_t queue;
	struct timer_list tim;
	volatile u8  Timeout;
} PS2_OsSleepStruct;

typedef struct
{
	u8					PS2RcvData[2];
	u8					DataFlag;
	u8					PS2WRCmd[5];
	u8 					FirstTime;
	PS2_OsSleepStruct 	PS2ReceiveWait;
	u8					PS2IFActive;
	u8					PS2Wakeup;
} PS2Buf_T;

#endif /* __PS2_MAIN_H__ */

