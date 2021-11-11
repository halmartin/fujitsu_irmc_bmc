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

#ifndef SE_USB20_H
#define SE_USB20_H

#include "board.h"
#include "usb_hw.h"


#define DEBUG_HW1				0x01
#define DEBUG_HW1_INTR			0x02
#define DEBUG_HW1_DPC			0x04
/*************************** SEUSB20 Function Prototype ************************************/


/* Macro to access SEUSB20 registers and memory */
#define READ_SEUSB20_DWORD(reg) 			*((volatile unsigned long *)(reg))
#define WRITE_SEUSB20_DWORD(reg,data) 	*((volatile unsigned long *)(reg)) = (data)

#define SEUSB20_MAX_EP			4
#define SEUSB20_EP0_SIZE		64
#define	SETUP_PKT				1
#define NOT_SETUP_PKT			0

/************************** ServEng USB 2.0 Register ************************************/

#define DEBUG_SEUSB20				0

/* Board, Hardware CPU and Architecture Specific Defines*/
#define MAX_NUM_SEUSB20				1

#define SEUSB20_0_CMD_REG  			0x0	/* Command Register */
#define SEUSB20_0_BIG_ENDIAN_DEVICE	 	0	/* Access Method 	*/
#define SEUSB20_0_INTR_COUNT		 	1	/* Number of Intrs	*/
#define SEUSB20_0_INT_PRI	  		0	/* Priority 		*/
#define SEUSB20_0_INTR	  			17
#if defined (SOC_PILOT_III)
#define SEUSB20_0_VECTOR			IRQ_USB_COMBIRQ
#else
#define SEUSB20_0_VECTOR			IRQ_USB_2_0                     
#endif

#define SEUSB20_OUT0BC   				0x0000
#define SEUSB20_IN0BC   				0x0001
#define SEUSB20_EP0CS   				0x0002

#define SEUSB20_OUT1BCL   				0x0008
#define SEUSB20_OUT1BCH   				0x0009
#define SEUSB20_OUT1CON   				0x000A
#define SEUSB20_OUT1CS   				0x000B
#define SEUSB20_IN1BCL   				0x000C
#define SEUSB20_IN1BCH   				0x000D
#define SEUSB20_IN1CON   				0x000E
#define SEUSB20_IN1CS   				0x000F

#define SEUSB20_OUT2BCL   				0x0010
#define SEUSB20_OUT2BCH   				0x0011
#define SEUSB20_OUT2CON   				0x0012
#define SEUSB20_OUT2CS   				0x0013
#define SEUSB20_IN2BCL   				0x0014
#define SEUSB20_IN2BCH   				0x0015
#define SEUSB20_IN2CON   				0x0016
#define SEUSB20_IN2CS   				0x0017

#define SEUSB20_OUT3BCL   				0x0018
#define SEUSB20_OUT3BCH   				0x0019
#define SEUSB20_OUT3CON   				0x001A
#define SEUSB20_OUT3CS   				0x001B
#define SEUSB20_IN3BCL   				0x001C
#define SEUSB20_IN3BCH   				0x001D
#define SEUSB20_IN3CON   				0x001E
#define SEUSB20_IN3CS   				0x001F

#define SEUSB20_OUT4BCL   				0x0020
#define SEUSB20_OUT4BCH   				0x0021
#define SEUSB20_OUT4CON   				0x0022
#define SEUSB20_OUT4CS   				0x0023
#define SEUSB20_IN4BCL   				0x0024
#define SEUSB20_IN4BCH   				0x0025
#define SEUSB20_IN4CON   				0x0026
#define SEUSB20_IN4CS   				0x0027

#define SEUSB20_FIFO1DAT   				0x0084
#define SEUSB20_FIFO2DAT   				0x0088
#define SEUSB20_FIFO3DAT   				0x008C
#define SEUSB20_FIFO4DAT   				0x0090

/* Int. Status reg. */
#define SEUSB20_IN07IRQ					0x0188
#define SEUSB20_OUT07IRQ				0x018A
#define SEUSB20_USBIRQ					0x018C
#define SEUSB20_OUT07PNGIRQ				0x018E
#define SEUSB20_IN07FULLIRQ				0x0190
#define SEUSB20_OUT07EMPTIRQ			0x0192

/* Int. Enable */
#define SEUSB20_IN07IEN					0x0194
#define SEUSB20_OUT07IEN   				0x0196
#define SEUSB20_USBIEN   				0x0198
#define SEUSB20_OUT07PNGIEN				0x019A
#define SEUSB20_IN07FULLIEN				0x019C
#define SEUSB20_OUT07EMPTIEN			0x019E

#define SEUSB20_IVECT					0x01A0
#define SEUSB20_FIFOVECT				0x01A1
#define SEUSB20_ENDPRST					0x01A2
#define SEUSB20_USBCS   				0x01A3
#define SEUSB20_FRMNRL   				0x01A4
#define SEUSB20_FRMNRH   				0x01A5
#define	SEUSB20_FNADDR					0x01A6
#define SEUSB20_CLKGATE					0x01A7
#define SEUSB20_FIFOCTRL	   			0x01A8

#define SEUSB20_USBINTS_ST  			0x01B0
#define SEUSB20_USBINTS_EN  			0x01B1
#define SEUSB20_DMAADDR		  			0x01B4
#define SEUSB20_DMACTRL		  			0x01B8


/* 64 bytes IN0 Data buffer */
#define SEUSB20_EP0INDAT				0x0100 /* 0x100 to 0x13F */
/* 64 bytes OUT0 Data buffer */
#define SEUSB20_EP0OUTDAT				0x0140 /* 0x140 to 0x17F */
/* 8 bytes Setup Data buffer */
#define	SEUSB20_SETUPDAT   				0x0180 /* 0x180 to 0x187 */

/************************** ServEng USB 2.0 Register Bit Definitions************************************/

/* SEUSB20_EP0CS */
#define	SEUSB20_EP0STALL				0x01
#define	SEUSB20_HSNAK					0x02
#define	SEUSB20_IN0BSY					0x04
#define	SEUSB20_OUT0BSY					0x08


/* SEUSB20_OUTXCS & INXCS */
#define	SEUSB20_OUT_ERR	   				0x01
#define	SEUSB20_OUT_BUSY   				0x02
#define	SEUSB20_OUT_NPAK   				0x0C
#define	SEUSB20_OUT_AUTOOUT				0x10

/* SEUSB20_OUTXCON & INXCON */
#define	SEUSB20_SINGLEBUF				0x03
#define	SEUSB20_EPTYPE					0x0C
#define	SEUSB20_EPTYPE_BULK				0x08
#define	SEUSB20_EPTYPE_INT				0x0C
#define	SEUSB20_STALL					0x40
#define	SEUSB20_VAL						0x80

/* SEUSB20_OUT07IRQ */
#define	SEUSB20_OUT0IRQ					0x01
#define	SEUSB20_OUT1IRQ					0x02
#define	SEUSB20_OUT2IRQ					0x04
#define	SEUSB20_OUT3IRQ					0x08
#define	SEUSB20_OUT4IRQ					0x10
#define	SEUSB20_OUT5IRQ					0x20
#define	SEUSB20_OUT6IRQ					0x40
#define	SEUSB20_OUT7IRQ					0x80

/* SEUSB20_IN07IRQ */
#define	SEUSB20_IN0IRQ					0x01
#define	SEUSB20_IN1IRQ					0x02
#define	SEUSB20_IN2IRQ					0x04
#define	SEUSB20_IN3IRQ					0x08
#define	SEUSB20_IN4IRQ					0x10
#define	SEUSB20_IN5IRQ					0x20
#define	SEUSB20_IN6IRQ					0x40
#define	SEUSB20_IN7IRQ					0x80

/* SEUSB20_USBIRQ */
#define	SEUSB20_SUDAVIR					0x01
#define	SEUSB20_SOFIR					0x02
#define	SEUSB20_SUTOKIR					0x04
#define	SEUSB20_SUSPIR					0x08
#define	SEUSB20_URESIR				    0x10
#define	SEUSB20_HSPEEDIR			    0x20

/* SEUSB20_OUT07PNGIRQ */
#define	SEUSB20_OUT0PNG					0x01
#define	SEUSB20_OUT1PNG					0x02
#define	SEUSB20_OUT2PNG					0x04
#define	SEUSB20_OUT3PNG					0x08
#define	SEUSB20_OUT4PNG					0x10
#define	SEUSB20_OUT5PNG					0x20
#define	SEUSB20_OUT6PNG					0x40
#define	SEUSB20_OUT7PNG					0x80

/* SEUSB20_OUT07EMPTIRQ */
#define	SEUSB20_OUT1EMPT				0x02
#define	SEUSB20_OUT2EMPT				0x04
#define	SEUSB20_OUT3EMPT				0x08
#define	SEUSB20_OUT4EMPT				0x10
#define	SEUSB20_OUT5EMPT				0x20
#define	SEUSB20_OUT6EMPT				0x40
#define	SEUSB20_OUT7EMPT				0x80

/* SEUSB20_IN07FULLIRQ */
#define	SEUSB20_IN1FULL					0x02
#define	SEUSB20_IN2FULL					0x04
#define	SEUSB20_IN3FULL					0x08
#define	SEUSB20_IN4FULL					0x10
#define	SEUSB20_IN5FULL					0x20
#define	SEUSB20_IN6FULL					0x40
#define	SEUSB20_IN7FULL					0x80

/* SEUSB20_OUT07IEN  */
#define SEUSB20_OUT0IEN					0x01
#define SEUSB20_OUT1IEN					0x02
#define SEUSB20_OUT2IEN					0x04
#define SEUSB20_OUT3IEN					0x08
#define SEUSB20_OUT4IEN					0x10
#define SEUSB20_OUT5IEN					0x20
#define SEUSB20_OUT6IEN					0x40
#define SEUSB20_OUT7IEN					0x80

/*SEUSB20_IN07IEN */
#define SEUSB20_IN0IEN					0x01
#define SEUSB20_IN1IEN					0x02
#define SEUSB20_IN2IEN					0x04
#define SEUSB20_IN3IEN					0x08
#define SEUSB20_IN4IEN					0x10
#define SEUSB20_IN5IEN					0x20
#define SEUSB20_IN6IEN					0x40
#define SEUSB20_IN7IEN					0x80

/* SEUSB20_USBIEN */
#define	SEUSB20_SUDAVIE					0x01
#define	SEUSB20_SOFIE					0x02
#define	SEUSB20_SUTOKIE					0x04
#define	SEUSB20_SUSPIE					0x08
#define	SEUSB20_URESIE					0x10
#define	SEUSB20_HSPIE					0x20

/* SEUSB20_OUT07PNGIEN */
#define	SEUSB20_OUT0PIE					0x01
#define	SEUSB20_OUT1PIE					0x02
#define	SEUSB20_OUT2PIE					0x04
#define	SEUSB20_OUT3PIE					0x08
#define	SEUSB20_OUT4PIE					0x10
#define	SEUSB20_OUT5PIE					0x20
#define	SEUSB20_OUT6PIE					0x40
#define	SEUSB20_OUT7PIE					0x80

/* SEUSB20_OUT07EMPTIEN */
#define	SEUSB20_OUT1EMIE				0x02
#define	SEUSB20_OUT2EMIE				0x04
#define	SEUSB20_OUT3EMIE				0x08
#define	SEUSB20_OUT4EMIE				0x10
#define	SEUSB20_OUT5EMIE				0x20
#define	SEUSB20_OUT6EMIE				0x40
#define	SEUSB20_OUT7EMIE				0x80

/*SEUSB20_IN07FULLIEN */
#define SEUSB20_IN1FUIE					0x02
#define SEUSB20_IN2FUIE					0x04
#define SEUSB20_IN3FUIE					0x08
#define SEUSB20_IN4FUIE					0x10
#define SEUSB20_IN5FUIE					0x20
#define SEUSB20_IN6FUIE					0x40
#define SEUSB20_IN7FUIE					0x80

/*SEUSB20_ENDPRST */
#define	SEUSB20_EPN						0x0F
#define	SEUSB20_DIR						0x10
#define SEUSB20_DIR_OUT    				(0<<4)
#define SEUSB20_DIR_IN     				(1<<4)  
#define	SEUSB20_TOGRST					0x20
#define	SEUSB20_FIFORST					0x40
#define	SEUSB20_TOGGLE					0x80

/* SEUSB20_USBCS */
#define	SEUSB20_SLFPWR					0x04
#define	SEUSB20_RWAKEN					0x08
#define	SEUSB20_ENUM					0x10
#define	SEUSB20_SIGRSUME				0x20
#define	SEUSB20_DISCON					0x40
#define	SEUSB20_WAKESRC					0x80

/* SEUSB20_FIFOCTRL */
#define	SEUSB20_FIFOCTRL_EP				0x0F
#define	SEUSB20_FIFODIR					0x10
#define	SEUSB20_FIFOAUTO				0x20
#define	SEUSB20_FIFOCMIT				0x40
#define	SEUSB20_FIFOACC					0x80

/* SEUSB20_USBINTS_EN */
#define SEUSB20_USBINT					0x01
#define SEUSB20_DMAINT					0x02
#define SEUSB20_WUINT					0x04
#define SEUSB20_FIFOINT					0x08

/* SEUSB20_IN07IRQ */
#define	SEUSB20_IN0IR					0x01
#define	SEUSB20_IN1IR					0x02
#define	SEUSB20_IN2IR					0x04
#define	SEUSB20_IN3IR					0x08
#define	SEUSB20_IN4IR					0x10
#define	SEUSB20_IN5IR					0x20
#define	SEUSB20_IN6IR					0x40
#define	SEUSB20_IN7IR					0x80

/* SEUSB20_OUT07IRQ */
#define	SEUSB20_OUT0IR					0x01
#define	SEUSB20_OUT1IR					0x02
#define	SEUSB20_OUT2IR					0x04
#define	SEUSB20_OUT3IR					0x08
#define	SEUSB20_OUT4IR					0x10
#define	SEUSB20_OUT5IR					0x20
#define	SEUSB20_OUT6IR					0x40
#define	SEUSB20_OUT7IR					0x80


/* Int vector register ivec */
#define SEUSB20_SUDAV					0
#define SEUSB20_SOF						1
#define SEUSB20_SUTOK					2
#define SEUSB20_SUSPEND					3
#define SEUSB20_USBRESET				4
#define SEUSB20_HSPEED					5
#define SEUSB20_EP0IN					6
#define SEUSB20_EP0OUT					7
#define SEUSB20_EP0PING					8
#define SEUSB20_EP1IN					9
#define SEUSB20_EP1OUT					10
#define SEUSB20_EP1PING					11
#define SEUSB20_EP2IN					12
#define SEUSB20_EP2OUT					13
#define SEUSB20_EP2PING					14
#define SEUSB20_EP3IN					15
#define SEUSB20_EP3OUT					16
#define SEUSB20_EP3PING					17
#define SEUSB20_EP4IN					18
#define SEUSB20_EP4OUT					19
#define SEUSB20_EP4PING					20
#define SEUSB20_EP5IN					21
#define SEUSB20_EP5OUT					22
#define SEUSB20_EP5PING					23


/* Intr Mask Values */
#define SEUSB20_INT_DISABLE_ALL		0x00
#define SEUSB20_INT_ENABLE_ALL		0xFF
#define SEUSB20_INT_CLEAR_ALL		0xFF

#define SE_CONNECT_VA				IO_ADDRESS(0x40100134)


#define BIT0						0x01
#define BIT1						0x02
#define BIT2						0x04
#define BIT3						0x08
#define BIT4						0x10

/************************** SEUSB20 Register Macros ************************************/

#define READ_SEUSB20_BYTE(reg) 			*((volatile unsigned char *)(reg))

#define WRITE_SEUSB20_BYTE(reg,data) 	*((volatile unsigned char *)(reg)) = (data)


#define DMA_EPT1               (1<<16)
#define DMA_EPT2               (2<<16)
#define DMA_EPT3               (3<<16)
#define DMA_EPT4               (4<<16)
#define DMA_SETSTOP            (1<<24)  
#define DMA_SETPAUSE           (1<<25)
  
#define DMA_DIR_IN             (1<<30)
#define DMA_DIR_OUT            (0<<30)
#define DMA_START              (1<<31)

#if defined (SOC_PILOT_III)
	#define USB2_DMA_EN (1<<4)
#endif


#define MAX_DMA_DATA    512

typedef struct
{
    uint8   devicenum;
    uint8   ep;
    uint16  len;
    uint8   buffer [MAX_DMA_DATA];

} dma_data_t;


#define ACTION_USB_SETUP_PACKET		1
#define ACTION_USB_EP0_TX_DONE		2
#define ACTION_USB_EP0_RX_DONE		3
#define ACTION_USB_BUS_SUSPEND		5
#define ACTION_USB_BUS_RESET		4
#define ACTION_DMA_TRANSFER		6
#define ACTION_USB_CONFIG_HISPEED	7




#endif


