/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2014 Fujitsu Technology Solutions
*                         ALL RIGHTS RESERVED
*
* This computer program is CONFIDENTIAL and contains TRADE SECRETS of
* Fujitsu Technology Solutions. The receipt or possession of this program does
* not convey any rights to reproduce or disclose its contents, or to
* manufacture, use, or sell anything that it may describe, in whole or
* in part, without the specific written consent of Fujitsu Technology Solutions.
* Any reproduction of this program without the express written consent
* of Fujitsu Technology Solutions is a violation of the copyright laws and may
* subject you to civil liability and criminal prosecution.
*
*   
*   Note: Extend existing module with proc fs interface to control binds and settings.
*   	  Add multiply device support.
*         Change implementation to cover project requirements. 
*    
*
*	Based on:
*	pca954X.c from Rodolfo Giometti <giometti@linux.it>.
*
*****************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/slab.h>
#include <linux/device.h>

#include "pca954x_proc.h"
#include "pca954x.h"

typedef struct 
{
	u8 chanUsed;
	u8 instance;
	u8 maxChan;
	u8 dummy;
	struct i2c_adapter *virtAdaps[PCA954X_MAX_NCHANS];
}tCntReg;



static const struct i2c_device_id pca954x_id[] = {
	{ "pca9543", pca_9543 },
	{ "pca9546", pca_9546 },
	{ "pca9548", pca_9548 },
	{ "pca9544", pca_9544 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, pca954x_id);

/* reset function */
#define GPIO_GPDO_OFFSET			0x08
static void pca954x_setGPIOLow(u8 pinNum)
{
	u8 value;
	int	port,tpin;
	port = pinNum / 8;
	tpin = pinNum % 8;
	value = *(((u8*)SE_GPIO_VA_BASE) + GPIO_GPDO_OFFSET + port * 0x10);
	value = value & ~ ( 0x01 << tpin );
	*(((u8*)SE_GPIO_VA_BASE) + GPIO_GPDO_OFFSET + port * 0x10) = value;
}
static void pca954x_setGPIOHigh(u8 pinNum)
{
	u8 value;
	int	port,tpin;
	port = pinNum / 8;
	tpin = pinNum % 8;
	value = *(((u8*)SE_GPIO_VA_BASE) + GPIO_GPDO_OFFSET + port * 0x10);
	value = value | ( 0x01 << tpin );
	*(((u8*)SE_GPIO_VA_BASE) + GPIO_GPDO_OFFSET + port * 0x10) = value;
}
void pca954x_reset(u8 pinNum)
{
	pca954x_setGPIOLow(pinNum);
	pca954x_setGPIOHigh(pinNum);
}
/* end of reset functions */

static int pca954x_reg_write(struct i2c_adapter *adap,
			     struct i2c_client *client, u8 val)
{
	int ret = -ENODEV;

	if (adap->algo->master_xfer) {
		struct i2c_msg msg;
		char buf[1];

		msg.addr = client->addr;
		msg.flags = 0;
		msg.len = 1;
		buf[0] = val;
		msg.buf = buf;
		ret = adap->algo->master_xfer(adap, &msg, 1);
	} else {
		union i2c_smbus_data data;
		ret = adap->algo->smbus_xfer(adap, client->addr,
					     client->flags,
					     I2C_SMBUS_WRITE,
					     val, I2C_SMBUS_BYTE, &data);
	}

	return ret;
}

static int pca954x_select_chan(struct i2c_adapter *adap, void *client, u32 chan)
{
	tCntReg *cntReg =  i2c_get_clientdata(client);
	u8 regval = 0;
	u8 enabledMask = getPcaData(cntReg->instance)->chanMask;
	int ret = 0;

	regval = (u8)(1 << chan);
	//some interface are open forever after configuration
	regval |= enabledMask;
	//mask out unused bits
	regval &= cntReg->chanUsed;
	
	ret = pca954x_reg_write(adap, client, regval);
	
	return ret;
}
 
static int pca954x_deselect_chan(struct i2c_adapter *adap, void *client, u32 chan)
{
	int ret = 0;
	u8 regval = 0;
	tCntReg *cntReg =  i2c_get_clientdata(client);
	u8 enabledMask = getPcaData(cntReg->instance)->chanMask;
	
	//some interface are open forever after configuration
	regval = enabledMask;
	//mask out unused bits
	regval &= cntReg->chanUsed;
	
	ret = pca954x_reg_write(adap, client, regval);
	return ret;
}
static int pca9544_select_chan(struct i2c_adapter *adap, void *client, u32 chan)
{
//	tCntReg *cntReg =  i2c_get_clientdata(client);
	int ret = 0;
	u8 regval = 0;

	//set channel value
	regval = (chan & 0x03);
	//set enable bit
	regval |= 0x04;
	ret = pca954x_reg_write(adap, client, regval);
	
	return ret;
}
 
static int pca9544_deselect_chan(struct i2c_adapter *adap, void *client, u32 chan)
{
	int ret = 0;
	u8 regval = 0;
//	tCntReg *cntReg =  i2c_get_clientdata(client);
		
	//disable all ports
	regval = 0;
	
	ret = pca954x_reg_write(adap, client, regval);
	return ret;
}

static tMuxProp muxProp[] =
{
	{ pca_9543, PCA9543_MAX_CHAN, (u8)(sizeof("pca9543")),0, "pca9543", pca954x_select_chan, pca954x_deselect_chan},
	{ pca_9546, PCA9546_MAX_CHAN, (u8)(sizeof("pca9546")),0, "pca9546", pca954x_select_chan, pca954x_deselect_chan},
	{ pca_9548, PCA9548_MAX_CHAN, (u8)(sizeof("pca9548")),0, "pca9548", pca954x_select_chan, pca954x_deselect_chan},
	{ pca_9544, PCA9544_MAX_CHAN, (u8)(sizeof("pca9544")),0, "pca9544", pca9544_select_chan, pca9544_deselect_chan},
};

ePcaType pca954x_getType(char *name)
{
	int loop = 0;
	for (loop = 0; loop < (sizeof(muxProp)/sizeof(tMuxProp)); ++loop) {
		if( 0 == strncmp(name, muxProp[loop].name,muxProp[loop].sizeOfName) ) {
			return muxProp[loop].type;
		}
	}
	return pca_unsuported;
}
int pca954x_getMaxChan(ePcaType type)
{
	if(type >= pca_unsuported)
		return 0;
	else
		return muxProp[type].maxChn;
}
/*
 * I2C init/probing/exit functions
 */
static int pca954x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent);
	tPca954xChip *pChip = client->dev.platform_data;
	tCntReg *cntReg = NULL;
	int num;

	if(pChip == NULL) {
		return -1;
	}
	pca954x_reset(pChip->rstGpio);
	
	pChip->status = 0;
	if (!i2c_check_functionality(adap, I2C_FUNC_SMBUS_BYTE)) {
		pChip->status = -ENODEV;
		return pChip->status;
	}
	
	// Check if the device is present 
	pChip->status = i2c_smbus_write_byte(client, 0);
	if(pChip->status < 0) {
		dev_warn(&client->dev, "probe failed\n");
		return pChip->status;
	}
	if((cntReg = kmalloc((sizeof(tCntReg)), GFP_KERNEL)) == NULL) {
		pChip->status = -ENOMEM;
		return pChip->status; 
	}
	
	i2c_set_clientdata(client, cntReg);
	
	memset(cntReg,0x00,sizeof(tCntReg));
	cntReg->instance = pChip->instance;
	cntReg->maxChan = pChip->maxChan;
		
	// Now create an adapter for each channel
	for (num = 0; num < pChip->maxChan; ++num) {
		if(pChip->vBusNum[num] != 0) {
			cntReg->chanUsed |= 1 << num;
			cntReg->virtAdaps[num] = i2c_add_mux_adapter(adap,  &client->dev, client, pChip->vBusNum[num], num, 0, muxProp[pChip->type].select_chan, muxProp[pChip->type].deselect_chan);
			if (cntReg->virtAdaps[num] == NULL) {
				dev_err(&client->dev,"failed to register multiplexed adapter %d as bus %d\n", num, pChip->vBusNum[num]);
				goto virt_reg_failed;
			}
		}
		else
			cntReg->virtAdaps[num] = NULL;
		
	}
	return pChip->status;
	
virt_reg_failed:
	for (num--; num >= 0; num--)
		if(cntReg->virtAdaps[num] != NULL)
			i2c_del_mux_adapter(cntReg->virtAdaps[num]);

	kfree(cntReg);
	pChip->status = -ENODEV;
	return pChip->status;

}

static int pca954x_remove(struct i2c_client *client)
{
	tCntReg *cntReg = i2c_get_clientdata(client);
	int num;
	
	for (num = 0; num < cntReg->maxChan; ++num) {
		if (cntReg->virtAdaps[num]) {
			i2c_del_mux_adapter(cntReg->virtAdaps[num]);
			cntReg->virtAdaps[num] = NULL;
		}
	}

	kfree(cntReg);
	return 0;
}

static struct i2c_driver pca954x_driver = {
	.driver		= {
		.name	= "pca954x",
		.owner	= THIS_MODULE,
	},
	.probe		= pca954x_probe,
	.remove		= pca954x_remove,
	.id_table	= pca954x_id,
};
struct i2c_client *pca954x_addChip(u8 bus, struct i2c_board_info *board_info)
{
	struct i2c_adapter *adap = NULL;
	struct i2c_client *regI2Client = NULL;
	tPca954xChip *pChip = board_info->platform_data;
		
	if(pChip == NULL)
		return NULL;
	
	adap = i2c_get_adapter(bus);
    if( adap != NULL )
    {
		regI2Client = i2c_new_device(adap, board_info);
   		if(pChip->status == 0) {
   			return regI2Client;
   		}
   		else { 
   			i2c_unregister_device(regI2Client);
   			return NULL;
   		}
    }
    else {
            return NULL;
    }
	
}
void pca954x_removeChip(struct i2c_client *regI2Client)
{
	if(regI2Client != NULL)
		i2c_unregister_device(regI2Client);
}
static int __init pca954x_init(void)
{
    int ret = 0;
   
	i2c_add_driver(&pca954x_driver);
    if((ret = pca954x_createProc()) != 0)  {
    	i2c_del_driver(&pca954x_driver);
    }
    return ret;
    
}

static void __exit pca954x_exit(void)
{
	pca954x_removeProc();
	i2c_del_driver(&pca954x_driver);
   
}

module_init(pca954x_init);
module_exit(pca954x_exit);

MODULE_AUTHOR("Fujitsu");
MODULE_DESCRIPTION("PCA9546/43 I2C switch driver");
MODULE_LICENSE("GPL v2");
