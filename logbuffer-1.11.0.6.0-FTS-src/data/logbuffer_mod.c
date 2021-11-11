/***************************************************************************************
 **                                                                                   **
 **    (C)Copyright 2011-2012, American Megatrends Inc.                               **
 **                                                                                   **
 **            All Rights Reserved.                                                   **
 **                                                                                   **
 **        6145-F, Northbelt Parkway, Norcross,                                       **
 **                                                                                   **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.                                **
 **                                                                                   **
 ***************************************************************************************/
/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2012 - 2018 FUJITSU LIMITED
*                         ALL RIGHTS RESERVED
*
* Any reproduction of this program without the express written consent
* of Fujitsu Limited is a violation of the copyright laws and may
* subject you to civil liability and criminal prosecution.
*
* This computer program is CONFIDENTIAL and contains TRADE SECRETS of
* Fujitsu Technology Solutions and/or Fujitsu Limited which must not be disclosed.
* The receipt or possession of this program does not convey any rights to reproduce
* or disclose its contents, or to manufacture, use, or sell anything that it may
* describe, in whole or in part.
*
*****************************************************************************/

/*
 * Log buffer driver uses a portion of a persistent RAM as a log storage.
 * Log buffer consists of two parts:
 * 1) Header
 * 2) Storage
 * Header (22 bytes):
 * 1) signature (10 bytes)
 * 2) read position (4 bytes): offset from the storage base
 * 3) write position (4 bytes): offset from the storage base
 * 4) written bytes count (4 bytes)
 * Storage: Ring buffer with plain data.
 * Write position is shared between all writers.
 * Read operations on different descriptors are independent (until there is a race
 * with a writer for data in storage).
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/version.h>

//FTS includes 
#include <fts_permdat.h>

#include "logbuffer.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
#define UNLOCKED 1
#endif

#define LOGBUFFER_DRIVER_MAJOR	107
#define LOGBUFFER_DRIVER_MINOR	0
#define LOGBUFFER_MAX_DEVICES	255
#define LOGBUFFER_DEVNAME	"logbuffer"

#define LOGBUFFER_HEADER_SIZE			LOG_BUFFER_HEADER_SIZE
#define LOGBUFFER_SIGNATURE				"logbuf0001"
#define LOGBUFFER_SIGNATURE_SIZE		(sizeof(LOGBUFFER_SIGNATURE) - 1)
#define LOGBUFFER_READ_POS_OFFSET		LOGBUFFER_SIGNATURE_SIZE		// 10 bytes
#define LOGBUFFER_WRITE_POS_OFFSET		(4 + LOGBUFFER_READ_POS_OFFSET)
#define LOGBUFFER_BYTES_COUNT_OFFSET	(4 + LOGBUFFER_WRITE_POS_OFFSET)
#define LOGBUFFER_STORAGE_SIZE			LOG_BUFFER_STORAGE_SIZE

static void *mapped; /* header starts here */
static void *storage;
static struct cdev logbuffer_cdev;
static dev_t logbuffer_dev = MKDEV(LOGBUFFER_DRIVER_MAJOR, LOGBUFFER_DRIVER_MINOR);

#ifdef DEBUG_LOGBUFFER_MODULE
static struct proc_dir_entry * header_proc_file = NULL;
static struct proc_dir_entry * data_proc_file = NULL;
static struct proc_dir_entry * rawstart_proc_file = NULL;
static struct proc_dir_entry * rawend_proc_file = NULL;
static struct proc_dir_entry * header_proc_basedir = NULL;
#endif

static uint32_t hread_pos; /* local mirror for header read_pos, offset from storage base */
static uint32_t hwrite_pos; /* local mirror for header write_pos, offset from storage base */
static uint32_t hbytes_count; /* local mirror for header bytes_count */

static int write_enabled; /* enable/disable writing to log buffer */

static int logBufferErr = 0;

typedef struct {
	uint32_t read_pos; /* offset from storage base, saved on open() */
	uint32_t bytes_available; /* bytes_count corresponding to read_pos */
	char buf[MAX_LOGBUF_READ_SIZE]; /* to copy data from io to userspace */
} logbuffer_priv_t;

static inline void get_buffer(char *buf, void __iomem *addr, int count)
{
	int i;
	for (i = 0; i < count; i++)
		buf[i] = __raw_readb(addr + i);
}

static inline void set_buffer(char *buf, void __iomem *addr, int count)
{
	int i;
	for (i = 0; i < count; i++)
		__raw_writeb(buf[i], addr + i);
}

/* read global state from PRAM */
static inline void read_header(void)
{
	get_buffer((char *)&hread_pos, mapped + LOGBUFFER_READ_POS_OFFSET, sizeof(hread_pos));
	get_buffer((char *)&hwrite_pos, mapped + LOGBUFFER_WRITE_POS_OFFSET, sizeof(hwrite_pos));
	get_buffer((char *)&hbytes_count, mapped + LOGBUFFER_BYTES_COUNT_OFFSET, sizeof(hbytes_count));
}

/* write global state to PRAM */
static inline void write_header(void)
{
	set_buffer((char *)&hread_pos, mapped + LOGBUFFER_READ_POS_OFFSET, sizeof(hread_pos));
	set_buffer((char *)&hwrite_pos, mapped + LOGBUFFER_WRITE_POS_OFFSET, sizeof(hwrite_pos));
	set_buffer((char *)&hbytes_count, mapped + LOGBUFFER_BYTES_COUNT_OFFSET, sizeof(hbytes_count));
}

static int logbuffer_open(struct inode * inode, struct file * filp)
{
	logbuffer_priv_t *priv;

	priv = kmalloc(sizeof(logbuffer_priv_t), GFP_KERNEL);
	if (!priv) {
		printk(KERN_ERR "%s() unable to allocate memory\n", __FUNCTION__);
		return -ENOMEM;
	}

	priv->read_pos = hread_pos;
	priv->bytes_available = hbytes_count;

	filp->private_data = priv;
	return 0;
}

static int logbuffer_release(struct inode * inode, struct file * filp)
{
	if (filp->private_data) {
		kfree(filp->private_data);
		filp->private_data = NULL;
	}
	return 0;
}


int getNextHreadPos(unsigned int len)
{
	unsigned int recordLen;
	unsigned int pos;
	int remain;
	
	if(len > MAX_LOGBUF_READ_SIZE) 
		len = MAX_LOGBUF_READ_SIZE;
	
	pos = hread_pos;
	do {
		recordLen = __raw_readb(storage + pos);

		if(recordLen){
			len -= (len >= recordLen)?recordLen:len;
			if((pos + recordLen) >= LOGBUFFER_STORAGE_SIZE) {
				remain = pos + recordLen - LOGBUFFER_STORAGE_SIZE;
				pos = remain;
			}
			else
				pos += recordLen;
		} else {
			return -1;
		}
	} while(len > 0);
	
	return pos; 
}

static ssize_t logbuffer_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	logbuffer_priv_t *priv = filp->private_data;
	char *tmp;
	size_t remain,wrPos, rdPos;
	int entry_length;

	if (count >= sizeof(priv->buf)) {
		count = sizeof(priv->buf);
		entry_length = count + 1;
	}
	else
		/* entry length: length of record bytes + size byte */
		entry_length = count + 1;

	if (write_enabled) {
		tmp = priv->buf;

		if (copy_from_user(tmp, buf, count)) {
			printk(KERN_ERR "%s() copy_from_user() failed\n", __FUNCTION__);
			return -EFAULT;
		}
		
		wrPos = hwrite_pos;
		rdPos = hread_pos;
		//move read pointer if required
		if( (hwrite_pos < hread_pos) && ( entry_length >= hread_pos-hwrite_pos) ) { //ring buffer is full
			rdPos = getNextHreadPos(entry_length+1 -(hread_pos-hwrite_pos));
		}
		if( (hwrite_pos > hread_pos ) && ( entry_length >= LOGBUFFER_STORAGE_SIZE - hwrite_pos + hread_pos)) {
			rdPos = getNextHreadPos((entry_length+1 -(LOGBUFFER_STORAGE_SIZE - hwrite_pos + hread_pos) ));
		}
		
		if(rdPos == -1){
			logBufferErr = -EBADFD;
			return -EBADFD;
		}
		
		//move read pointer on before attempting to write to avoid possible corruption issue
		hread_pos = rdPos;
		
		/* first write entry length */
		__raw_writeb(entry_length, storage + wrPos);
		++wrPos;
		/* now copy data */
		if( (wrPos + count) >= LOGBUFFER_STORAGE_SIZE ) {
			remain = (wrPos + count) - LOGBUFFER_STORAGE_SIZE;
			memcpy_toio(storage + wrPos, &tmp[0], count - remain);
			if (remain)
				memcpy_toio(storage + 0, &tmp[count - remain], remain);
			
			wrPos = remain;

		}
		else {
			memcpy_toio(storage + wrPos, &tmp[0], count);
			wrPos += count;
		}

		hwrite_pos = wrPos;

		if( hwrite_pos >= hread_pos)
			hbytes_count = hwrite_pos - hread_pos;
		else
			hbytes_count = (LOGBUFFER_STORAGE_SIZE - hread_pos) + hwrite_pos;
		write_header();
		
		*f_pos += count + 1;

	}
	logBufferErr = 0;
	return (count + 1);
}

static ssize_t logbuffer_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	uint32_t avail;
	logbuffer_priv_t *priv = filp->private_data;
	char *tmp;
	size_t i, pos;

	avail = priv->bytes_available;

	if (avail <= *f_pos)
		return 0;

	avail -= *f_pos;
	if (avail < count)
		count = avail;

	tmp = priv->buf;

	if (count > sizeof(priv->buf))
		count = sizeof(priv->buf);

	pos = priv->read_pos + *f_pos;
	if (pos >=  LOGBUFFER_STORAGE_SIZE)
		pos -= LOGBUFFER_STORAGE_SIZE;
	
	if((pos + count) >=  LOGBUFFER_STORAGE_SIZE ) {
		i = (pos + count) - LOGBUFFER_STORAGE_SIZE;
		memcpy_fromio(&tmp[0], storage + pos, count - i);
		if (i) {
			pos = (pos + count - i) - LOGBUFFER_STORAGE_SIZE;
			memcpy_fromio(&tmp[count - i], storage + pos,  i);
		}
	}
	else {
		memcpy_fromio(&tmp[0], storage + pos, count);
	}

	if (copy_to_user(buf, tmp, count)) {
		printk(KERN_ERR "%s() copy_to_user() failed\n", __FUNCTION__);
		return -EFAULT;
	}

	*f_pos += count;
	return count;
}

static size_t logbuffer_ioctl_read (struct file *filp, logbuffer_ioc_read_t *logbuffer_ioc_read)
{
	logbuffer_priv_t *priv = filp->private_data;
	uint32_t avail;
	size_t count, pos, i;

	avail = priv->bytes_available;
	if (avail <= logbuffer_ioc_read->offset)
		return 0;

	count = logbuffer_ioc_read->size;
	avail -= logbuffer_ioc_read->offset;
	if (avail < count)
		count = avail;

	if (count > sizeof(logbuffer_ioc_read->buf))
		count = sizeof(logbuffer_ioc_read->buf);

	pos = priv->read_pos + logbuffer_ioc_read->offset;
	
	if (pos >=  LOGBUFFER_STORAGE_SIZE) {
		pos -= LOGBUFFER_STORAGE_SIZE;
	}
	
	
	if((pos + count) >=  LOGBUFFER_STORAGE_SIZE ) {
		i = (pos + count) - LOGBUFFER_STORAGE_SIZE;
		memcpy_fromio(&logbuffer_ioc_read->buf[0], storage + pos, count - i);
		if (i) {
			pos = (pos + count - i) - LOGBUFFER_STORAGE_SIZE;
			memcpy_fromio(&logbuffer_ioc_read->buf[count - i], storage + pos,  i);
		}
	}
	else
		memcpy_fromio(&logbuffer_ioc_read->buf[0], storage + pos, count);

	return count;
}
#ifdef UNLOCKED
static long logbuffer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
//	unsigned long i;
	logbuffer_ioc_cmd_t rwp_cmd;
	logbuffer_ioc_read_t *logbuffer_ioc_read;

	switch (cmd) {
	case LOGBUFFER_IOC_CLEAR:
		hread_pos = 0;
		hwrite_pos = 0;
		hbytes_count = 0;
		write_header();
#if 0
		for (i = 0; i < LOGBUFFER_STORAGE_SIZE; ++i)
			__raw_writeb(0, storage + i);
#endif
		memset_io(storage, 0x00 , LOGBUFFER_STORAGE_SIZE);
		break;
	case LOGBUFFER_IOC_ENABLED_LOGGING:
		write_enabled = 1;
		ret = write_enabled;
		break;
	case LOGBUFFER_IOC_DISABLED_LOGGING:
		write_enabled = 0;
		ret = write_enabled;
		break;
	case LOGBUFFER_IOC_GET_STATUS:
		ret = write_enabled;
		break;
	case LOGBUFFER_IOC_GET_RWP:
		rwp_cmd.read_p = hread_pos;
		rwp_cmd.write_p = hwrite_pos;
		if (copy_to_user( (void *)arg, (void *)&rwp_cmd, sizeof(logbuffer_ioc_cmd_t))) {
			printk(KERN_ERR "%s() copy_to_user() failed\n", __FUNCTION__);
			ret = -EFAULT;
		}
		break;
	case LOGBUFFER_IOC_READ_OFFSET:
		logbuffer_ioc_read = kmalloc(sizeof(logbuffer_ioc_read_t), GFP_USER);
		if (!logbuffer_ioc_read) {
			printk(KERN_ERR "%s() unable to allocate memory\n", __FUNCTION__);
			ret = -ENOMEM;
		} else {
			if (copy_from_user((void *)logbuffer_ioc_read, (void *)arg, sizeof(logbuffer_ioc_read_t))) {
				printk(KERN_ERR "%s() copy_from_user() failed\n", __FUNCTION__);
				ret = -EFAULT;
			} else {
				ret = logbuffer_ioctl_read(filp, logbuffer_ioc_read);
				if (ret) {
					if (copy_to_user( (void *)arg, (void *)logbuffer_ioc_read, sizeof(logbuffer_ioc_read_t))) {
						printk(KERN_ERR "%s() copy_to_user() failed\n", __FUNCTION__);
						ret = -EFAULT;
					}
				}
			}
			
			if (logbuffer_ioc_read) {
				kfree(logbuffer_ioc_read);
				logbuffer_ioc_read = NULL;
			}
		}
		break;
    case LOGBUFFER_IOC_GET_ERROR:
        ret = logBufferErr;
        break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct file_operations logbuffer_fops = {
	.owner		= THIS_MODULE,
	.open		= logbuffer_open,
	.release	= logbuffer_release,
	.read		= logbuffer_read,
	.write		= logbuffer_write,
	.unlocked_ioctl		= logbuffer_ioctl,
};
#else
static int logbuffer_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
//	unsigned long i;
	logbuffer_ioc_cmd_t rwp_cmd;
	logbuffer_ioc_read_t *logbuffer_ioc_read;

	switch (cmd) {
	case LOGBUFFER_IOC_CLEAR:
		hread_pos = 0;
		hwrite_pos = 0;
		hbytes_count = 0;
		write_header();
#if 0
		for (i = 0; i < LOGBUFFER_STORAGE_SIZE; ++i)
			__raw_writeb(0, storage + i);
#endif
		memset_io(storage, 0x00 , LOGBUFFER_STORAGE_SIZE);
		break;
	case LOGBUFFER_IOC_ENABLED_LOGGING:
		write_enabled = 1;
		ret = write_enabled;
		break;
	case LOGBUFFER_IOC_DISABLED_LOGGING:
		write_enabled = 0;
		ret = write_enabled;
		break;
	case LOGBUFFER_IOC_GET_STATUS:
		ret = write_enabled;
		break;
	case LOGBUFFER_IOC_GET_RWP:
		rwp_cmd.read_p = hread_pos;
		rwp_cmd.write_p = hwrite_pos;
		if (copy_to_user( (void *)arg, (void *)&rwp_cmd, sizeof(logbuffer_ioc_cmd_t))) {
			printk(KERN_ERR "%s() copy_to_user() failed\n", __FUNCTION__);
			ret = -EFAULT;
		}
		break;
	case LOGBUFFER_IOC_READ_OFFSET:
		logbuffer_ioc_read = kmalloc(sizeof(logbuffer_ioc_read_t), GFP_USER);
		if (!logbuffer_ioc_read) {
			printk(KERN_ERR "%s() unable to allocate memory\n", __FUNCTION__);
			ret = -ENOMEM;
		} else {
			if (copy_from_user((void *)logbuffer_ioc_read, (void *)arg, sizeof(logbuffer_ioc_read_t))) {
				printk(KERN_ERR "%s() copy_from_user() failed\n", __FUNCTION__);
				ret = -EFAULT;
			} else {
				ret = logbuffer_ioctl_read(filp, logbuffer_ioc_read);
				if (ret) {
					if (copy_to_user( (void *)arg, (void *)logbuffer_ioc_read, sizeof(logbuffer_ioc_read_t))) {
						printk(KERN_ERR "%s() copy_to_user() failed\n", __FUNCTION__);
						ret = -EFAULT;
					}
				}
				if (logbuffer_ioc_read) {
					kfree(logbuffer_ioc_read);
					logbuffer_ioc_read = NULL;
				}
			}
		}
		break;
	case LOGBUFFER_IOC_GET_ERROR:
	    ret = logBufferErr;
	    break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct file_operations logbuffer_fops = {
	.owner		= THIS_MODULE,
	.open		= logbuffer_open,
	.release	= logbuffer_release,
	.read		= logbuffer_read,
	.write		= logbuffer_write,
	.ioctl		= logbuffer_ioctl,
};
#endif

#ifdef DEBUG_LOGBUFFER_MODULE
static int header_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data )
{
	char tmp[LOGBUFFER_SIGNATURE_SIZE + 1];
	int len = 0, size;
	uint32_t tmp1 = 0;
	
    if (off > 0) {
        *eof = 1;
         return 0;
    }
	
	memset(tmp,0,LOGBUFFER_SIGNATURE_SIZE+1);
	memcpy_fromio(&tmp[0], mapped, LOGBUFFER_SIGNATURE_SIZE);
	
	len += sprintf(page + len,"Hdr: %s\n",tmp);
	len += sprintf(page + len,"read    ptr: %X",hread_pos);
	get_buffer((char *)&tmp1, mapped + LOGBUFFER_READ_POS_OFFSET, sizeof(uint32_t));
	if (tmp1!=hread_pos)
		len += sprintf(page + len,"\tHW: %X",tmp1);
	len += sprintf(page + len,"\n");
	
	len += sprintf(page + len,"write   ptr: %X",hwrite_pos);
	get_buffer((char *)&tmp1, mapped + LOGBUFFER_WRITE_POS_OFFSET, sizeof(uint32_t));
	if (tmp1!=hwrite_pos)
		len += sprintf(page + len,"\tHW: %X",tmp1);
	len += sprintf(page + len,"\n");
	
	len += sprintf(page + len,"counter ptr: %X",hbytes_count);
	get_buffer((char *)&tmp1, mapped + LOGBUFFER_BYTES_COUNT_OFFSET, sizeof(uint32_t));
	if (tmp1!=hbytes_count)
		len += sprintf(page + len,"\tHW: %X",tmp1);
	len += sprintf(page + len,"\n");
	
	len += sprintf(page + len,"total  size: %X\n\n",LOG_BUFFER_SIZE);
	
	size = __raw_readb(storage + hread_pos);
	len += sprintf(page + len,"len: %d\n",size);
	
	*eof = 1;
	
	return len;
}

static int data_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len = 0;
	int loop, i;
	uint8_t sz;
	
    if (off > 0) {
    	*eof=1;
    	return 0;
    }
    else
    {
    	loop=hread_pos;
    }
	
	while(loop!=hwrite_pos)
	{
		if (loop >= LOGBUFFER_STORAGE_SIZE)
			loop -= LOGBUFFER_STORAGE_SIZE;
		
		sz = __raw_readb(storage + loop);
		// 3+8+1 + D + 1 space is required for this, where D = sz-10 
		if (len + sz +3 >count) 
			break;
		
		len += sprintf(page + len,"%02X ", __raw_readb(storage + loop)); // Size + space
		loop+=2;
		
		
		for (i=2;i<=5 && loop!=hwrite_pos;i++,loop++) // Timestamp
		{
			if (loop >= LOGBUFFER_STORAGE_SIZE)
				loop -= LOGBUFFER_STORAGE_SIZE;
			
			len += sprintf(page + len,"%02X", __raw_readb(storage + loop));
		}
		len += sprintf(page + len," ");
		
		loop+=4;//Skip Acurate TS;
		i+=4;
		
		for (;i<sz && loop!=hwrite_pos;i++,loop++)
		{
			if (loop >= LOGBUFFER_STORAGE_SIZE)
				loop -= LOGBUFFER_STORAGE_SIZE;
			
			len += sprintf(page + len,"%c", __raw_readb(storage + loop));
		}
		len += sprintf(page + len,"\n");
	}
	if (hread_pos == loop)
	{
		*eof=1;
		return 0;
	}
	else
	{
		return len;
	}
}

static int rawdatastart_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len = 0;
	int loop;
	
    if (off > 0) {
    	*eof=1;
    	return 0;
    }

   	loop=0;
	
	while(len < count-10)
	{
		len += sprintf(page + len,"%02X ", __raw_readb(mapped + loop)); // Size + space
		if ((loop%32) ==31) len += sprintf(page + len,"\n");
		loop++;
	}

	len += sprintf(page + len,"\n");
	
	*eof=1;
	return len;
}

static int rawdataend_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int len = 0;
	int loop;
	int i;
	
    if (off > 0) {
    	*eof=1;
    	return 0;
    }

   	loop=LOGBUFFER_STORAGE_SIZE-20*16;
	
	for(i=0;len < count-200 && loop < LOGBUFFER_STORAGE_SIZE;i++)
	{
		len += sprintf(page + len,"%02X ", __raw_readb(storage + loop)); // Size + space
		if ((i%32) ==31) len += sprintf(page + len,"\n");
		loop++;
	}

	len += sprintf(page + len,"\n");
	
	*eof=1;
	return len;
}
#endif
void createProcFsForTest(void)
{
#ifdef DEBUG_LOGBUFFER_MODULE
	header_proc_basedir = proc_mkdir("logbuf", NULL);
	if (header_proc_basedir == NULL)
	{
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n","logbuf");
		goto no_dir;
	}

	header_proc_basedir->owner	= THIS_MODULE;
	header_proc_basedir->uid 	 	= 0;
	header_proc_basedir->gid 	 	= 0;
	
	header_proc_file = create_proc_read_entry("header",0444, header_proc_basedir,header_proc_read,NULL);
	if (header_proc_file == NULL) {
		printk(KERN_ALERT "Error: Could not initialize %s\n","/proc/logbuf/header");
		goto no_file;
	}

	header_proc_file->owner = THIS_MODULE;
	
	data_proc_file = create_proc_read_entry("data",0444, header_proc_basedir, data_proc_read, NULL);
	if (data_proc_file == NULL) {
		printk(KERN_ALERT "Error: Could not initialize %s\n","/proc/logbuf/data");
		goto no_file;
	}
	data_proc_file->owner = THIS_MODULE;
	
	rawstart_proc_file = create_proc_read_entry("rawstart",0444, header_proc_basedir, rawdatastart_proc_read, NULL);
	if (rawstart_proc_file == NULL) {
		printk(KERN_ALERT "Error: Could not initialize %s\n","/proc/logbuf/raw");
		goto no_file;
	}
	rawstart_proc_file->owner = THIS_MODULE;
	
	rawend_proc_file = create_proc_read_entry("rawend",0444, header_proc_basedir, rawdataend_proc_read, NULL);
	if (rawend_proc_file == NULL) {
		printk(KERN_ALERT "Error: Could not initialize %s\n","/proc/logbuf/raw");
		goto no_file;
	}
	rawend_proc_file->owner = THIS_MODULE;
no_file:
no_dir:
	return;
#endif
}
void removeProcFsForTest(void) 
{
#ifdef DEBUG_LOGBUFFER_MODULE
	if(header_proc_file != NULL) {
		remove_proc_entry("header",header_proc_basedir);
		header_proc_file = NULL;
	}
	
	if(data_proc_file != NULL) {
		remove_proc_entry("data",header_proc_basedir);
		data_proc_file = NULL;
	}
	
	if(rawstart_proc_file != NULL) {
		remove_proc_entry("rawstart",header_proc_basedir);
		rawstart_proc_file = NULL;
	}
	
	if(rawend_proc_file != NULL) {
		remove_proc_entry("rawend",header_proc_basedir);
		rawend_proc_file = NULL;
	}
	
	if(header_proc_basedir) {
		remove_proc_entry("logbuf", NULL);
		header_proc_basedir = NULL;
	}
#endif
}

static int logbuffer_init(void)
{
	int ret;
	char header[LOGBUFFER_SIGNATURE_SIZE];
	unsigned int pram_version = 0;
	
	//init. header
	memset(header, 0, sizeof(header));

	if (CONFIG_SPX_FEATURE_GLOBAL_PRAM_SIZE <= LOG_BUFFER_SIZE) {
		printk(KERN_ERR "%s() PRAM size <%u> is not enough\n", __FUNCTION__, CONFIG_SPX_FEATURE_GLOBAL_PRAM_SIZE);
		return -EINVAL;
	}

	/* first 4 byte of PRAM is a magic number (PERMDAT_VERSION): */
	mapped = ioremap_nocache(CONFIG_SPX_FEATURE_GLOBAL_PRAM_START_ADDR, sizeof(PERMDAT_VERSION));
	if (!mapped) {
		printk(KERN_ERR "%s() failed to map PERMDAT_VERSION IO space (addr 0x%ld, size %u)\n",
			__FUNCTION__, (unsigned long)CONFIG_SPX_FEATURE_GLOBAL_PRAM_START_ADDR, sizeof(PERMDAT_VERSION));
		return -EIO;
	}
	get_buffer((char*)&pram_version, mapped, sizeof(PERMDAT_VERSION));
	if (mapped) {
		iounmap(mapped);
		mapped = NULL;
	}

	/* assume logbuffer is at the end of PRAM: */
	mapped = ioremap_nocache(((CONFIG_SPX_FEATURE_GLOBAL_PRAM_START_ADDR + CONFIG_SPX_FEATURE_GLOBAL_PRAM_SIZE) - LOG_BUFFER_SIZE), LOG_BUFFER_SIZE);
	if (!mapped) {
		printk(KERN_ERR "%s() failed to map logbuffer IO space (addr 0x%ld, size %u)\n",
			__FUNCTION__,(unsigned long) ((CONFIG_SPX_FEATURE_GLOBAL_PRAM_START_ADDR + CONFIG_SPX_FEATURE_GLOBAL_PRAM_SIZE) - LOG_BUFFER_SIZE), LOG_BUFFER_SIZE);
		return -EIO;
	}

	storage = mapped + LOGBUFFER_HEADER_SIZE;

	get_buffer(header, mapped, LOGBUFFER_SIGNATURE_SIZE);

	if ((memcmp(header, LOGBUFFER_SIGNATURE, LOGBUFFER_SIGNATURE_SIZE) == 0) && (pram_version == PERMDAT_VERSION)) {
		read_header();
	} else {
		printk(KERN_INFO "%s() pram is missing signature or wrong PERMDAT_VERSION, so initializing logbuffer\n", __FUNCTION__);
		set_buffer(LOGBUFFER_SIGNATURE, mapped, LOGBUFFER_SIGNATURE_SIZE);
		hread_pos = 0;
		hwrite_pos = 0;
		hbytes_count = 0;
		write_header();
	}

	ret = register_chrdev_region(logbuffer_dev, LOGBUFFER_MAX_DEVICES, LOGBUFFER_DEVNAME);
	if (ret < 0) {
		printk(KERN_ERR "%s() failed to register logbuffer device <%s> (err %d)\n",
			__FUNCTION__, LOGBUFFER_DEVNAME, ret);
		iounmap(mapped);
		mapped = NULL;
		return ret;
	}

	cdev_init(&logbuffer_cdev, &logbuffer_fops);
	ret = cdev_add(&logbuffer_cdev, logbuffer_dev, LOGBUFFER_MAX_DEVICES);
	if (ret < 0) {
		printk(KERN_ERR "%s() failed to add <%s> char device\n", __FUNCTION__, LOGBUFFER_DEVNAME);
		unregister_chrdev_region(logbuffer_dev, LOGBUFFER_MAX_DEVICES);
		iounmap(mapped);
		mapped = NULL;
		return ret;
	}

	write_enabled = 1;
	
	createProcFsForTest();
	
	return 0;
}

static void logbuffer_exit(void)
{
	write_enabled = 0;
	cdev_del(&logbuffer_cdev);
	unregister_chrdev_region(logbuffer_dev, LOGBUFFER_MAX_DEVICES);
	if (mapped) {
		iounmap(mapped);
		mapped = NULL;
	}
	removeProcFsForTest();
}

module_init(logbuffer_init);
module_exit(logbuffer_exit);

MODULE_DESCRIPTION("Logbuffer interface to PRAM");
MODULE_AUTHOR("Maxim Gorbachyov, SaM Solutions");
MODULE_LICENSE("GPL");

