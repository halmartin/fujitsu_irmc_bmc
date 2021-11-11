/*****************************************************************************
*
*                               NOTICE
*
*                COPYRIGHT 2010 - 2018 FUJITSU LIMITED
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
/**
 @file device.c
 @brief /dev/irmc-cl Character device operations
 @date 2010-01-20
 @author Vlad Shakhov

 Implement character device /dev/irmc-cl.

 */
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "device.h"
#include "ring_buffer.h"
#include "filter.h"
#include "irmc_common.h"

/// for future extension (if more than one device must be logged)
#define CONSOLELOG_DEVICE_COUNT	2	//1: Host UART (via virtual UART), 2: iRMC Debug UART
#define IRMC_CL_MAJOR		254
#define IRMC_CL_MINOR		0

#ifdef HAVE_UNLOCKED_IOCTL  
  #if HAVE_UNLOCKED_IOCTL  
        #define USE_UNLOCKED_IOCTL  
  #endif
#endif

/// store number of logging device (ttyS*), kernel module param
unsigned int consolelog_tty_line = CL_DEFAULT_UART;

/// storage for character device essential params
struct irmc_console_log_dev
{
	dev_t dev;
	struct cdev cdev;
	int use_count;
};

struct debug_uart_console_log_buffer_info{
	char *read_buffer;
	int read_buffer_size;
};

/// all persistent character device data after driver loading
static struct irmc_console_log_dev console_log_dev;

/**
 @brief Open Console Log device
 @return 0 in success
 @param inode pointer to INODE structure, used to extract log device struct
 @param filp file data pointer, used to store log device struct to file opened

 Point read HEAD to the very beginning of ring buffer (on every open).

 Log device can be opened only once a time (to prevent race conditions on
 read HEAD movements).
 */
static int consolelog_open_locked(struct inode *inode, struct file *filp)
{
	struct irmc_console_log_dev * log;

	// get the log params, assigned to device
	log = container_of(inode->i_cdev, struct irmc_console_log_dev, cdev);

	// exclusive usage of device file
	if ( log->use_count )
	{
		printk(KERN_ERR "%s already opened\n", CONSOLELOG_LOG_NAME);
		return -EBUSY;
	}

	log->use_count = 1;
	irmc_cl_ring_buffer_open();
	return 0;
}

static int consolelog_open(struct inode *inode, struct file *filp)
{
	int ret;
	consolelog_lock();
	ret = consolelog_open_locked(inode, filp);
	consolelog_unlock();
	return ret;
}

/**
 @brief Do finalization when device closed by user process
 @param inode Pointer to inode assigned to device file
 @param filp File data pointer

 Now only clear use flag
 */
static int consolelog_release(struct inode *inode, struct file *filp)
{
	struct irmc_console_log_dev * log;
	consolelog_lock();
	log = container_of(inode->i_cdev, struct irmc_console_log_dev, cdev);
	log->use_count = 0;
	irmc_cl_ring_buffer_release();
	consolelog_unlock();
	return 0;
}

/**
 @brief read() syscall implementation for Console Log device
 @param filp file data structure
 @param buf buffer to  put the data readed
 @param count size of read buffer
 @param f_pos pointer to current position in file
 @return count of characters readed

 Executed every time when someone try to read /dev/irmc-cl device.
 */
static ssize_t consolelog_read(struct file *filp, char __user *to,
	size_t count, loff_t *f_pos)
{
	static char buf[512];
	static unsigned data_size, read_pos;
	ssize_t ret;

	if (read_pos >= data_size) {
		ret = irmc_cl_ring_buffer_read(buf, sizeof(buf));
		if (ret < 0)
			return ret;
		data_size = ret;
		read_pos = 0;
	}

	ret = (count < (data_size - read_pos)) ? count : (data_size - read_pos);

	if (ret > 0) {
		if (copy_to_user(to, &buf[read_pos], ret) > 0) {
			printk(KERN_ERR "%s: copy_to_user() failed\n", __FUNCTION__);
			return -EFAULT;
		}
		read_pos += ret;
	}

	return ret;
}

/**
 @brief ioctl() syscall implementation for Console Log device
 @param inode inode for open device file
 @param filp file data structure
 @param cmd command id
 @param arg argument for command (shoud be anything, from integer to buffer pointer)
 @return operation result, 0 in success
*/
#ifdef USE_UNLOCKED_IOCTL
static long consolelog_ioctl(struct file *filp, u_int cmd, unsigned long arg)
#else
static int consolelog_ioctl(struct inode * inode, struct file *filp, u_int cmd, unsigned long arg)
#endif
{
	int ret = 0;
	unsigned int tty_line;
	char *tmpBuffer = NULL;
	int tmpBufferSize = 0;
	int count = 0;
	struct debug_uart_console_log_buffer_info debug_uart_console_log_buffer;
	
	switch (cmd) {
	// start logging
	case CONSOLELOG_IOC_GET_START:
		if (copy_from_user( &tty_line, (unsigned int *) arg, sizeof(unsigned int) ) != 0 )
			ret = -EINVAL;
		else {
			consolelog_start(tty_line);
			ret = consolelog_started() ? 0 : -EACCES;
		}
		break;

	// stop logging
	case CONSOLELOG_IOC_GET_STOP:
		consolelog_stop();
		ret = consolelog_started() ? -EACCES : 0;
		break;

	// return logging status (0 or 1)
	case CONSOLELOG_IOC_GET_STATUS:
		ret = consolelog_started();
		break;

	// trigger the all escape sequences filter (0->1 or 1->0)
	case CONSOLELOG_IOC_SET_FILTER:
		consolelog_lock();
		if (copy_from_user( &consolelog_filter_all, (unsigned int *) arg, sizeof(unsigned int) ) != 0 )
			ret = - EINVAL;
		consolelog_unlock();
		break;

	// get filter status
	case CONSOLELOG_IOC_GET_FILTER:
		if ( copy_to_user( (void *)arg, &consolelog_filter_all,
				sizeof(consolelog_filter_all)) != 0)
		{
			ret = -EINVAL;
		}

		break;

	// clear the log
	case CONSOLELOG_IOC_CLEAR_LOG:
		consolelog_lock();
		irmc_cl_ring_buffer_clear();
		consolelog_unlock();
		break;

	case CONSOLELOG_IOC_DEBUG_UART_READ:
		//lock the buffer

		if (copy_from_user( (void *)&debug_uart_console_log_buffer, (void *) arg, sizeof(debug_uart_console_log_buffer) ) != 0 ){
			ret = 0;
		} else {
			irmc_debug_uart_lock();
	
			//make a temporary buffer, size equal to the count of the valid data
			tmpBufferSize = irmc_debug_uart_ring_buffer_count();
	
			if(!tmpBufferSize){
				irmc_debug_uart_unlock();
				return 0; // there is no new data
			}
			
			if(tmpBufferSize > debug_uart_console_log_buffer.read_buffer_size){
				tmpBufferSize = debug_uart_console_log_buffer.read_buffer_size;
			}
	
			tmpBuffer = kmalloc(tmpBufferSize, GFP_KERNEL);
	
			if(tmpBuffer == NULL){
				irmc_debug_uart_unlock();
				return 0;
			}
				
			while(count < tmpBufferSize){
				tmpBuffer[count++] = irmc_debug_uart_ring_buffer_get_char();
			}
			
			//memcpy(debug_uart_console_log_buffer.read_buffer, tmpBuffer, tmpBufferSize);
			if(copy_to_user((void*)debug_uart_console_log_buffer.read_buffer, tmpBuffer, tmpBufferSize)){
				kfree(tmpBuffer);
				irmc_debug_uart_unlock();
				return 0;
			}

			if(copy_to_user((void *)arg, &debug_uart_console_log_buffer, sizeof(debug_uart_console_log_buffer))){
				kfree(tmpBuffer);
				irmc_debug_uart_unlock();
				return 0;
			}
			kfree(tmpBuffer);
			
			ret = tmpBufferSize;
			irmc_debug_uart_unlock();
		}
		break;

	// unknown request
	default:
		printk("%s() unknown cmd %u\n", __FUNCTION__, cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/// file operations with console log device
static struct file_operations consolelog_fops =
{
	.open		= consolelog_open,
	.release	= consolelog_release,
	.read		= consolelog_read,
#ifdef USE_UNLOCKED_IOCTL
	.unlocked_ioctl = consolelog_ioctl,
#else
	.ioctl		= consolelog_ioctl,
#endif
};

/**
 @brief Initialization of /dev/irmc-cl character device
 @return 0 in success
 Create device in kernel namespace (not the file on /dev/ filesystem!).
 */
int consolelog_device_init(void)
{
	int err;

	console_log_dev.dev = MKDEV(IRMC_CL_MAJOR, IRMC_CL_MINOR);

	err = register_chrdev_region(console_log_dev.dev,
		CONSOLELOG_DEVICE_COUNT, CONSOLELOG_CHRDEV_NAME);
	if (err < 0)
	{
		printk(KERN_ERR "failed to register chardev with major %u, minor %u\n",
			IRMC_CL_MAJOR, IRMC_CL_MINOR);
		return err;
	}

	// initialize character device class
	cdev_init( &console_log_dev.cdev, &consolelog_fops );
	console_log_dev.cdev.owner = THIS_MODULE;
	console_log_dev.cdev.ops = &consolelog_fops;

	// add device itself
	err = cdev_add( &console_log_dev.cdev, console_log_dev.dev, CONSOLELOG_DEVICE_COUNT );
	if ( err < 0 )
	{
		printk(KERN_ERR "Can't add char device to system\n");
		unregister_chrdev_region( console_log_dev.dev, CONSOLELOG_DEVICE_COUNT);
		return err;
	}

	console_log_dev.use_count = 0;

	printk(KERN_INFO "console log chardev registered with major %d, minor %d\n",
		MAJOR(console_log_dev.dev), MINOR(console_log_dev.dev) );

	return 0;
}

/// Shutdown /dev/irmc-cl device (on kernel module exit)
void consolelog_device_shutdown(void)
{
	cdev_del(&console_log_dev.cdev);
	unregister_chrdev_region( console_log_dev.dev, CONSOLELOG_DEVICE_COUNT);
}

