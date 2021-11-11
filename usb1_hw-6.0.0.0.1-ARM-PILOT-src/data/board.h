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

#ifndef BOARD_H
#define BOARD_H


#define MAX_INTRS_PER_DEVICE		 	1


/* Device Configuration Structures */
typedef struct 
{
	uint8	IntrPri;
	uint8	ExtIntr;
	uint8	Vector;	
	uint32  IntrFlags; 
} USB_INTR_CONF;

typedef struct
{
	uint32	CmdReg;
	uint32	DataReg;
	uint8 	DeviceNo;
	uint8   DevSpeed;
	uint8	BigEndian;
	uint8   NumIntrs;
	USB_INTR_CONF IntrConf[MAX_INTRS_PER_DEVICE];		
} USB_DEV_CONF;


#endif /* BOARD_H */
