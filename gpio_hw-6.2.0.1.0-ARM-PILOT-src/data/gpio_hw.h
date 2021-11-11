/*********************************************************
 **                                                     **
 **    (C)Copyright 2009-2015, American Megatrends Inc. **
 **                                                     **
 **            All Rights Reserved.                     **
 **                                                     **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,      **
 **                                                     **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.  **
 **                                                     **
 ********************************************************/
 
#ifndef __GPIO_HW_H
#define __GPIO_HW_H

#define SGPIO_1_DATA                0x01
#define GET_GPIO_TYPE               0X02
#define GET_GPEVST_DATA             0x03
#define SET_GPEVEN                  0x02
#define CLEAR_GPEVST                0x03
#define REG_INTR_BLOCK_GPIONUM      0X04
#define DEREG_INTR_BLOCK_GPIONUM    0X05
#define EVENT_TO_IRQ_ENABLE         0x06
#define PWRGDSTATE                  0x08
#define SWC_STAT0_REG               0x0A
#define SGPIO_2_DATA                0x0B

#endif //__GPIO_HW_H
