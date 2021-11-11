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
*****************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/i2c-mux.h>

#include <helper.h>

#include "pca954x.h"


#define PCA954X_PARAM_COUNT 	4
#define I2C_MUX_PROC_DIR		"i2c_mux"
#define PCA954X_FILE_STATUS		"status"
#define PCA954X_FILE_PRESENT	"present"
#define PCA954X_FILE_BIND		"bind"
#define PCA954X_FILE_UNBIND		"unbind"
#define PCA954X_FILE_RESET		"reset"
#define PCA954X_FILE_MASK		"mask"

typedef enum
{
	PCA954X_STATUS_FILE = 0,
	PCA954X_BIND_FILE,
	PCA954X_UNBIND_FILE,
	PCA954X_RESET_FILE,
	PCA954X_MASK_FILE,
	PCA954X_PRESENT_FILE
}eProcFileType;

typedef enum
{
	PCA954X_BUS = 0,
	PCA954X_ADDR,
	PCA954X_MASK,
	PCA954X_RST_GPIO,
	PCA954X_TYPE,
	PCA954X_CHANNEL_1,
	PCA954X_CHANNEL_2,
	PCA954X_CHANNEL_3,
	PCA954X_CHANNEL_4,
	PCA954X_CHANNEL_5,
	PCA954X_CHANNEL_6,
	PCA954X_CHANNEL_7,
	PCA954X_CHANNEL_8
}eParam;

static tPca954xChip *gChips = NULL;
static struct proc_dir_entry *i2c_mux_dir = NULL;
static struct i2c_board_info *board_info = NULL;

static tPca954xChip *findChip(u8 bus, u8 addr)
{
	u8 num;
	for(num = 0; num < PCA954X_MAX_COUNT; ++num) {
		if( ( gChips[num].enabled == 1 ) && ( gChips[num].bus == (u8)bus) && ( board_info[num].addr == (u16)addr) ) 
			return &gChips[num];
		}
	return NULL;
}

static int pca954x_bind(u8 bus, u8 addr, u8 chanMask, u8 rstGpio, char *typeStr, u8 *channel,  u8 maxChan)
{
	int k,num;

	//check if device already bind
	if( NULL != findChip(bus, addr) )
		return -1;
	
	//bind device, search for free entry in topology structure
	for(num = 0; num < PCA954X_MAX_COUNT; ++num) {
		if(gChips[num].enabled == 0)
		{
			memset(&gChips[num],0x00,sizeof(tPca954xChip));
			gChips[num].type = pca954x_getType(typeStr);
			if(gChips[num].type == pca_unsuported)
				return (-1);
			gChips[num].bus = (u8)bus;
			gChips[num].instance = num;
			gChips[num].chanMask = chanMask;
			gChips[num].rstGpio = (u8)rstGpio;
			gChips[num].enabled = 1;
			gChips[num].maxChan = pca954x_getMaxChan(gChips[num].type);
			if(maxChan > gChips[num].maxChan)
				maxChan = gChips[num].maxChan;
			for(k = 0; k < maxChan; ++k)
				gChips[num].vBusNum[k] =  channel[k];
			
			memset(&board_info[num],0x00,sizeof(struct i2c_board_info));
			strncpy(board_info[num].type,typeStr,8);
			board_info[num].addr = (u16)addr;
			board_info[num].platform_data = &gChips[num];
			gChips[num].client = pca954x_addChip(gChips[num].bus, &board_info[num]);
			if(gChips[num].client != NULL)
				gChips[num].bind = 1;
			return 0;
		}
		
	}
	return -1;
}
static int pca954x_unbind(u8 bus, u8 addr)
{
	tPca954xChip *pChip = findChip(bus, addr);
	if(pChip != NULL) {
		pca954x_removeChip(pChip->client);
		gChips->bind = gChips->enabled = 0;
		return 0;
	}
	else
		return -1;
		
}
static int pca954x_resetChip(u8 bus, u8 addr)
{
	tPca954xChip *pChip = findChip(bus, addr);
	if(pChip != NULL) {
		pca954x_reset(pChip->rstGpio);
		return 0;
	}
	else
		return -1;
}
static int pca954x_setMask(u8 bus, u8 addr, u8 mask )
{
	tPca954xChip *pChip = findChip(bus, addr);
	if(pChip != NULL) {
		pChip->chanMask = mask;
		return 0;
	}
	else
		return -1;
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
static inline void *PDE_DATA(const struct inode *inode)
{
	return PDE(inode)->data;
}
static inline struct inode *file_inode(struct file *f)
{
	return f->f_dentry->d_inode;
}
#endif
static ssize_t pca954x_proc_write(struct file *filp, const char *buff, size_t len, loff_t *offp)
{
	char *kBuf = NULL, *pBuf = NULL, *oneMuxSettings = NULL, *param = NULL;
	u8 bus = 0, addr = 0, rstGpio = 0, chanMask = 0;
	u8 chan[PCA954X_MAX_NCHANS];
	u8 chanNum, count;
	unsigned long tmp;
	char type[I2C_NAME_SIZE];
	int ret = 0;
	eProcFileType fileType = (eProcFileType)PDE_DATA(file_inode(filp));
	
	kBuf = kmalloc(len + 1, GFP_KERNEL);
	if(kBuf == NULL)
		return -EFAULT;
	
	if(copy_from_user(kBuf, buff, len)) {
		kfree(kBuf);
		return -EFAULT;
	}
	kBuf[len] = 0;
	//printk (KERN_ERR "Kernel: <%s: val: %s, %i, I2C_NAME_SIZE: %i>\n",__FUNCTION__,kBuf,(int)len,I2C_NAME_SIZE);
	
	for(count = 0; count < len; ++count) //remove spaces on begin
		if(kBuf[count != ' '])
			break;
	
	pBuf = &kBuf[count];
	while((oneMuxSettings = strsep(&pBuf, " ")) != NULL) {
		count = 0;
		chanNum = 0;
		while((param = strsep(&oneMuxSettings, ",")) != NULL) {
			switch(count) {
				case PCA954X_BUS:
					if(strict_strtoul(param, 10, &tmp))
						ret = -EFAULT;
					else
						bus = (u8)tmp;
					break;
				case PCA954X_ADDR:
					if(strict_strtoul(param, 16, &tmp))
						ret = -EFAULT;
					else {
						addr = (u8)tmp;
						addr >>= 1;
					}
					break;
				case PCA954X_MASK:
					if(strict_strtoul(param, 16, &tmp))
						ret = -EFAULT;
					else
						chanMask = (u8)tmp;
					break;
				case PCA954X_RST_GPIO:
					if(strict_strtoul(param, 10, &tmp))
						ret = -EFAULT;
					else
						rstGpio = (u8)tmp;
					break;
				case PCA954X_TYPE:
					strncpy(type,param,I2C_NAME_SIZE);
					break;
				case PCA954X_CHANNEL_1:
				case PCA954X_CHANNEL_2:
				case PCA954X_CHANNEL_3:
				case PCA954X_CHANNEL_4:
				case PCA954X_CHANNEL_5:
				case PCA954X_CHANNEL_6:
				case PCA954X_CHANNEL_7:
				case PCA954X_CHANNEL_8:
					if(strict_strtoul(param, 10, &tmp))
						ret = -EFAULT;
					else
						chan[chanNum++] = (u8)tmp;
					break;
				default:
					break;
			}
			++count;
		}
		if(ret ==0) {
			switch(fileType)
			{
				case PCA954X_BIND_FILE:
					//printk (KERN_ERR "<BIND bus: %i, addr: %02X, mask: %02X, gpio: %i, type: %s chanNum: %i>\n",bus,(addr<<1),chanMask, rstGpio,type,chanNum);
					pca954x_bind(bus,addr,chanMask,rstGpio,type,chan,chanNum);
					break;
				case PCA954X_UNBIND_FILE:
					//printk (KERN_ERR "<UNBIND bus: %i, addr: %02X>\n",bus,(addr<<1));
					pca954x_unbind(bus,addr);
					break;
				case PCA954X_RESET_FILE:
					//printk (KERN_ERR "<RESET bus: %i, addr: %02X>\n",bus,(addr<<1));
					pca954x_resetChip(bus,addr);
					break;
				case PCA954X_MASK_FILE:
					//printk (KERN_ERR "<SET MASK bus: %i, addr: %02X, mask: %02X>\n",bus,(addr<<1),chanMask);
					pca954x_setMask(bus,addr,chanMask);
					break;
				default:
					break;
			}
		}
	}
	kfree(kBuf);
	return len;
}
static ssize_t pca954x_proc_read_status (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    int len = 0;
    int i,j;
    eProcFileType fileType = (eProcFileType)PDE_DATA(file_inode(filp));
    
	if( *f_pos == 1 ) //count is typically 4kb. so we could read all data in one call  
		return 0; 
	else {
		if(fileType == PCA954X_STATUS_FILE) {
			for(i = 0; i < PCA954X_MAX_COUNT; ++i) {
				if(gChips[i].enabled == 1)
				{
					len += sprintf(buf+len,"st: %08X, bus: %02X, addr: %02X, gpio: %d, type: %s",
						gChips[i].status,gChips[i].bus,(board_info[i].addr<<1),gChips[i].rstGpio,board_info[i].type);
					for(j=0;j<gChips[i].maxChan;++j) {
						len += sprintf(buf+len,",%s%i", ((gChips[i].vBusNum[j] == 0)?"":"i2c-"),gChips[i].vBusNum[j]);
					}
					len += sprintf(buf+len,"\n");
				}
			}    
		}
		else if(fileType == PCA954X_PRESENT_FILE ) {
			for(i = 0; i < PCA954X_MAX_COUNT; ++i) {
				if(gChips[i].enabled == 1){
					len += sprintf(buf+len,"%02X,%02X,%02X\n",gChips[i].bus,(board_info[i].addr<<1),((gChips[i].status==0)?1:0));
				}
			}
		}
		else {
			len = 0;
		}
		*f_pos = 1;
		return len;

	}
}
inline tPca954xChip *getPcaData(unsigned char id)
{
	return &gChips[id];
}

static struct file_operations pca954x_read_fops = {
		.owner     	=  THIS_MODULE,
		.read = pca954x_proc_read_status,	
		.write = NULL,
};
static struct file_operations pca954x_write_fops = {
		.owner     	=  THIS_MODULE,
		.read = NULL,
		.write = pca954x_proc_write,		
};

static void pca954x_removeAllProc(void)
{
	remove_proc_entry(PCA954X_FILE_STATUS,	i2c_mux_dir);
	remove_proc_entry(PCA954X_FILE_PRESENT,	i2c_mux_dir);
	remove_proc_entry(PCA954X_FILE_BIND,	i2c_mux_dir);
	remove_proc_entry(PCA954X_FILE_UNBIND,	i2c_mux_dir);
	remove_proc_entry(PCA954X_FILE_MASK,	i2c_mux_dir);
	remove_proc_entry(PCA954X_FILE_RESET,	i2c_mux_dir);
	remove_proc_entry(I2C_MUX_PROC_DIR, 	NULL);
}


int pca954x_createProc(void)
{
	int ret = 0, loop;
	
    if((gChips = kmalloc((sizeof(tPca954xChip) * PCA954X_MAX_COUNT), GFP_KERNEL)) == NULL)
       	return -ENOMEM;
       
    if((board_info = kmalloc((sizeof(struct i2c_board_info) * PCA954X_MAX_COUNT), GFP_KERNEL)) == NULL) {
    	kfree(gChips);
    	return -ENOMEM;
    }
    for(loop = 0; loop < PCA954X_MAX_COUNT; ++loop) 
    	gChips[loop].bind = gChips[loop].enabled = 0;	
    
    i2c_mux_dir = proc_mkdir(I2C_MUX_PROC_DIR, NULL);
    if (i2c_mux_dir == NULL)
    {
    	printk(KERN_ERR "Error: Could not initialize /proc/%s\n",I2C_MUX_PROC_DIR);
    	ret = -EFAULT;
    	goto no_dir;
    }
	if( (NULL == proc_create_data(PCA954X_FILE_STATUS,  (S_IFREG |S_IRUGO),i2c_mux_dir,&pca954x_read_fops,(void*)PCA954X_STATUS_FILE) ) ||
		(NULL == proc_create_data(PCA954X_FILE_PRESENT, (S_IFREG |S_IRUGO),i2c_mux_dir,&pca954x_read_fops,(void*)PCA954X_PRESENT_FILE)) ||
		(NULL == proc_create_data(PCA954X_FILE_BIND,	(S_IFREG |S_IWUGO),i2c_mux_dir,&pca954x_write_fops,(void*)PCA954X_BIND_FILE)  ) || 
		(NULL == proc_create_data(PCA954X_FILE_UNBIND,	(S_IFREG |S_IWUGO),i2c_mux_dir,&pca954x_write_fops,(void*)PCA954X_UNBIND_FILE)) ||
		(NULL == proc_create_data(PCA954X_FILE_MASK,	(S_IFREG |S_IWUGO),i2c_mux_dir,&pca954x_write_fops,(void*)PCA954X_MASK_FILE)  ) ||
		(NULL == proc_create_data(PCA954X_FILE_RESET,	(S_IFREG |S_IWUGO),i2c_mux_dir,&pca954x_write_fops,(void*)PCA954X_RESET_FILE) ) )
	{
		printk(KERN_ERR "Error: Could not initialize /proc/%s files\n",I2C_MUX_PROC_DIR);
		pca954x_removeAllProc();
		ret = -EFAULT;
		goto no_file;
	}
	return 0;
	
no_file:
	pca954x_removeAllProc();
no_dir:
	kfree(gChips);
	kfree(board_info);
	
	return ret;
}
int pca954x_removeProc(void)
{
	u8 num;
	pca954x_removeAllProc();
	
	for(num = 0; num < PCA954X_MAX_COUNT; ++num) {
		if(( gChips[num].enabled == 1 ) && (gChips[num].bind == 1) )  {
			pca954x_removeChip(gChips[num].client);
			gChips[num].bind = gChips[num].enabled = 0;
		}
	}
	kfree(gChips);
	kfree(board_info);
	return 0;
}
