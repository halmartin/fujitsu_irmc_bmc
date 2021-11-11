/****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
****************************************************************/

#ifndef __SE_USB_11_H__
#define __SE_USB_11_H__

#include "board.h"
#include "usb_hw.h"

#define DEBUG_HW0				0x01
#define DEBUG_HW0_INTR			0x02
#define DEBUG_HW0_DPC			0x04

#define SEUSB11_MAX_EP			7
#define SEUSB11_EP0_SIZE		64
#define	SETUP_PKT				1
#define NOT_SETUP_PKT			0
/************************** ServEng USB 1.1 Register ************************************/

#define DEBUG_SEUSB11				0

/* Board, Hardware CPU and Architecture Specific Defines*/
#define MAX_NUM_SEUSB11				1

#define SEUSB11_0_CMD_REG  			0x0	/* Command Register */
#define SEUSB11_0_BIG_ENDIAN_DEVICE	 	0	/* Access Method 	*/
#define SEUSB11_0_INTR_COUNT		 	1	/* Number of Intrs	*/
#define SEUSB11_0_INT_PRI	  		0	/* Priority 		*/
#define SEUSB11_0_INTR	  			17
#define SEUSB11_0_VECTOR			IRQ_USB_1_0                     


/* 64 bytes IN Endpoints buffer */
#define SEUSB11_IN7BUF					0x0E00
#define SEUSB11_IN6BUF					0x1000
#define SEUSB11_IN5BUF					0x1200
#define SEUSB11_IN4BUF					0x1400
#define SEUSB11_IN3BUF					0x1600
#define SEUSB11_IN2BUF					0x1800
#define SEUSB11_IN1BUF					0x1A00
#define SEUSB11_IN0BUF					0x1C00

/* 64 bytes OUT Endpoints buffer */
#define SEUSB11_OUT7BUF					0x0D00
#define SEUSB11_OUT6BUF					0x0F00
#define SEUSB11_OUT5BUF					0x1100
#define SEUSB11_OUT4BUF					0x1300
#define SEUSB11_OUT3BUF					0x1500
#define SEUSB11_OUT2BUF					0x1700
#define SEUSB11_OUT1BUF					0x1900
#define SEUSB11_OUT0BUF					0x1B00

#define SEUSB11_CLKGATE					0x1E40
#define SEUSB11_IVEC					0x1EA0
#define SEUSB11_IN07IRQ					0x1EA4
#define SEUSB11_OUT07IRQ				0x1EA8
#define SEUSB11_USBIRQ					0x1EAC
#define SEUSB11_IN07IEN					0x1EB0
#define SEUSB11_OUT07IEN   				0x1EB4
#define SEUSB11_USBIEN   				0x1EB8
#define SEUSB11_USBBAV   				0x1EBC
#define SEUSB11_EP0CS   				0x1ED0
#define SEUSB11_IN0BC   				0x1ED4
#define SEUSB11_IN1CS   				0x1ED8
#define SEUSB11_IN1BC   				0x1EDC
#define SEUSB11_IN2CS   				0x1EE0
#define SEUSB11_IN2BC   				0x1EE4
#define SEUSB11_IN3CS   				0x1EE8
#define SEUSB11_IN3BC   				0x1EEC
#define SEUSB11_IN4CS   				0x1EF0
#define SEUSB11_IN4BC   				0x1EF4
#define SEUSB11_IN5CS   				0x1EF8
#define SEUSB11_IN5BC   				0x1EFC
#define SEUSB11_IN6CS   				0x1F00
#define SEUSB11_IN6BC   				0x1F04
#define SEUSB11_IN7CS   				0x1F08
#define SEUSB11_IN7BC   				0x1F0C
#define SEUSB11_OUT0BC   				0x1F14
#define SEUSB11_OUT1CS   				0x1F18
#define SEUSB11_OUT1BC   				0x1F1C
#define SEUSB11_OUT2CS   				0x1F20
#define SEUSB11_OUT2BC   				0x1F24
#define SEUSB11_OUT3CS   				0x1F28
#define SEUSB11_OUT3BC   				0x1F2C
#define SEUSB11_OUT4CS   				0x1F30
#define SEUSB11_OUT4BC   				0x1F34
#define SEUSB11_OUT5CS   				0x1F38
#define SEUSB11_OUT5BC   				0x1F3C
#define SEUSB11_OUT6CS   				0x1F40
#define SEUSB11_OUT6BC   				0x1F44
#define SEUSB11_OUT7CS   				0x1F48
#define SEUSB11_OUT7BC   				0x1F4C
#define SEUSB11_USBCS   				0x1F58
#define	SEUSB11_TOGCTL	 		   		0x1F5C
#define	SEUSB11_FNADDR					0x1F6C
#define	SEUSB11_USBPAIR					0x1F74
#define	SEUSB11_IN07VAL					0x1F78
#define	SEUSB11_OUT07VAL   				0x1F7C
/* Setup Buff is 8 bytes register for reading setup data */
/* From 0x1FA0 to 0x1FBC */
#define	SEUSB11_SETUPBUF   				0x1FA0

#define USB_INTR_REGISTER				0x2000
#define USB_INTR_ENABLE					0x2004

/************************** ServEng USB 1.1 Register Bit Definitions************************************/
/* Int vector register ivec */
#define SEUSB11_SUDAV					0x00
#define SEUSB11_SOF						0x01
#define SEUSB11_SUTOK					0x02
#define SEUSB11_SUSPEND					0x03
#define SEUSB11_USBRESET				0x04
#define SEUSB11_EP0IN					0x06
#define SEUSB11_EP0OUT					0x07
#define SEUSB11_EP1IN					0x08
#define SEUSB11_EP1OUT					0x09
#define SEUSB11_EP2IN					0x0A
#define SEUSB11_EP2OUT					0x0B
#define SEUSB11_EP3IN					0x0C
#define SEUSB11_EP3OUT					0x0D
#define SEUSB11_EP4IN					0x0E
#define SEUSB11_EP4OUT					0x0F
#define SEUSB11_EP5IN					0x10
#define SEUSB11_EP5OUT					0x11
#define SEUSB11_EP6IN					0x12
#define SEUSB11_EP6OUT					0x13
#define SEUSB11_EP7IN					0x14
#define SEUSB11_EP7OUT					0x15

/* SEUSB11_IN07IRQ */
#define	SEUSB11_IN0IR					0x01
#define	SEUSB11_IN1IR					0x02
#define	SEUSB11_IN2IR					0x04
#define	SEUSB11_IN3IR					0x08
#define	SEUSB11_IN4IR					0x10
#define	SEUSB11_IN5IR					0x20
#define	SEUSB11_IN6IR					0x40
#define	SEUSB11_IN7IR					0x80

/* SEUSB11_OUT07IRQ */
#define	SEUSB11_OUT0IR					0x01
#define	SEUSB11_OUT1IR					0x02
#define	SEUSB11_OUT2IR					0x04
#define	SEUSB11_OUT3IR					0x08
#define	SEUSB11_OUT4IR					0x10
#define	SEUSB11_OUT5IR					0x20
#define	SEUSB11_OUT6IR					0x40
#define	SEUSB11_OUT7IR					0x80

/* SEUSB11_USBIRQ */
#define	SEUSB11_SUDAVIR					0x01
#define	SEUSB11_SOFIR					0x02
#define	SEUSB11_SUTOKIR					0x04
#define	SEUSB11_SUSPIR					0x08
#define	SEUSB11_URESIR				    0x10

/*SEUSB11_IN07IEN */
#define SEUSB11_IN0IEN					0x01
#define SEUSB11_IN1IEN					0x02
#define SEUSB11_IN2IEN					0x04
#define SEUSB11_IN3IEN					0x08
#define SEUSB11_IN4IEN					0x10
#define SEUSB11_IN5IEN					0x20
#define SEUSB11_IN6IEN					0x40
#define SEUSB11_IN7IEN					0x80

/* SEUSB11_OUT07IEN  */
#define SEUSB11_OUT0IEN					0x01
#define SEUSB11_OUT1IEN					0x02
#define SEUSB11_OUT2IEN					0x04
#define SEUSB11_OUT3IEN					0x08
#define SEUSB11_OUT4IEN					0x10
#define SEUSB11_OUT5IEN					0x20
#define SEUSB11_OUT6IEN					0x40
#define SEUSB11_OUT7IEN					0x80

/* SEUSB11_USBIEN */
#define	SEUSB11_SUDAVIE					0x01
#define	SEUSB11_SOFIE					0x02
#define	SEUSB11_SUTOKIE					0x04
#define	SEUSB11_SUSPIE					0x08
#define	SEUSB11_URESIE					0x10

/* SEUSB11_EP0CS */
#define	SEUSB11_EP0STALL				0x01
#define	SEUSB11_HSNAK					0x02
#define	SEUSB11_IN0BSY					0x04
#define	SEUSB11_OUT0BSY					0x08

/* SEUSB11_IN1CS  */
#define SEUSB11_INXSTL   				0x01
#define SEUSB11_INXBSY   				0x02

/* SEUSB11_OUT1CS */
#define SEUSB11_OUTXSTL   				0x01
#define SEUSB11_OUTXBSY   				0x02

/* SEUSB11_USBCS */
#define	SEUSB11_SIGRESUME				0x01
#define	SEUSB11_DISCON					0x08
#define	SEUSB11_WAKESRC					0x80

/* SEUSB11_TOGCTL */
#define	SEUSB11_TOG_EPN					0x07
#define	SEUSB11_TOG_IO					0x10
#define	SEUSB11_TOG_R					0x20
#define	SEUSB11_TOG_S					0x40
#define	SEUSB11_TOG_Q					0x80

/* SEUSB11_USBPAIR */
#define	SEUSB11_PR2IN					0x01
#define	SEUSB11_PR4IN					0x02
#define	SEUSB11_PR6IN					0x04
#define	SEUSB11_PR2OUT					0x08
#define	SEUSB11_PR4OUT					0x10
#define	SEUSB11_PR6OUT					0x20

/* SEUSB11_IN07VAL	 */
#define	SEUSB11_IN0VAL					0x01
#define	SEUSB11_IN1VAL					0x02
#define	SEUSB11_IN2VAL					0x04
#define	SEUSB11_IN3VAL					0x08
#define	SEUSB11_IN4VAL					0x10
#define	SEUSB11_IN5VAL					0x20
#define	SEUSB11_IN6VAL					0x40
#define	SEUSB11_IN7VAL					0x80

/*SEUSB11_OUT07VAL */
#define	SEUSB11_OUT0VAL					0x01
#define	SEUSB11_OUT1VAL					0x02
#define	SEUSB11_OUT2VAL					0x04
#define	SEUSB11_OUT3VAL					0x08
#define	SEUSB11_OUT4VAL					0x10
#define	SEUSB11_OUT5VAL					0x20
#define	SEUSB11_OUT6VAL					0x40
#define	SEUSB11_OUT7VAL					0x80

/*USB_INTR_REGISTER	*/
#define	SEUSB11_USBINTREQ				0x01
#define	SEUSB11_DMAINTREQ				0x02
#define	SEUSB11_WUINTREQ				0x04

/*USB_INTR_ENABLE	*/
#define	SEUSB11_USBINTEN				0x01
#define	SEUSB11_DMAINTEN				0x02
#define	SEUSB11_WUINTEN					0x04


/* Intr Mask Values */
#define SEUSB11_INT_DISABLE_ALL		0x00
#define SEUSB11_INT_ENABLE_ALL		0xFF

/* SEUSB11_TOGCTL */
#define  SEUSB11_TOGCTL_R_BIT		0x20
#define  SEUSB11_TOGCTL_S_BIT		0x40


#define SEUSB11_INT_CLEAR_ALL		0xFF

#define SE_CONNECT_VA				IO_ADDRESS(0x40100134)


#define BIT0						0x01
#define BIT1						0x02
#define BIT2						0x04
#define BIT3						0x08
#define BIT4						0x10

/************************** SEUSB11 Register Macros ************************************/
#define READ_SEUSB11_BYTE(reg) 			*((volatile unsigned char *)(reg))
#define WRITE_SEUSB11_BYTE(reg,data) 	*((volatile unsigned char *)(reg)) = (data)

#define ACTION_USB_BUS_SUSPEND		5
#define ACTION_USB_BUS_RESET		4
#define ACTION_USB_SETUP_PACKET		1
#define ACTION_USB_EP0_TX_DONE		2
#define ACTION_USB_EP0_RX_DONE		3

#endif /* __SE_USB_11_H__ */


