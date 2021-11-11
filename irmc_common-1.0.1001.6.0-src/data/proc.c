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
 @file proc.c
 @brief /proc/irmc/console-log operations
 @date 2010-01-20
 @author Vlad Shakhov

 Implementation for /proc filesystem interface.
 Operations via /proc/irmc/console-log (Start/Stop/Get Status/Clear)
 	- $ cat /proc/irmc/console-log - return 0 (stopped), 1 (running)
 	- $ echo 0 >/proc/irmc/console-log - stop the logging
 	- $ echo 1 >/proc/irmc/console-log - start the logging
	- $ echo 2 >/proc/irmc/console-log - clear the log
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "irmc_common.h"
#include "proc.h"
#include "ring_buffer.h"
#include "helper.h"
#define CONSOLELOG_PROCFS_NAME	"console-log"

/// pointer to procfile structure
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
static struct proc_dir_entry * cl_proc_file = NULL;
#endif
/**
 @brief /proc filesystem interface for read() syscall
 @param buffer for data returning
 @param buffer_length size of buffer
 @param offset offset in read stream
 
 Not documented arguments are ignored. Write "0" or "1" in buffer,
 depend on status of logging process.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
static ssize_t consolelog_proc_read (struct file *filp, char * buffer,size_t buffer_length,loff_t *offset)

#else
static int consolelog_proc_read(
		char *buffer, char **buffer_location,
		off_t offset, int buffer_length, 
		int *eof, void *data )
#endif
{
	/* mean EOF, we have finished to read, return 0 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	if (*offset > 0)
                return 0;
#else
	if (offset > 0)
		return 0;
#endif

	if (buffer_length < 2)
		return -1;

	/* fill the buffer, return the buffer size */
	buffer[0] = consolelog_started() ? '1' : '0';
	buffer[1] = '\n';
	return 2;
}

extern unsigned int consolelog_tty_line;

/**
 @brief Do the control operations via log
 @param buffer for string
 @param buffer_length size of buffer
 @return count of symbols processed 

 Not documented arguments are ignored.

 Start/stop the logging  if '1' or '0' received from user.

 Clear log on '2' received.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
static ssize_t consolelog_proc_write(struct file *file, const char *buffer, size_t buffer_length, loff_t *ppos)
#else
static int consolelog_proc_write (
		struct file *file, const char __user *buffer,
     		unsigned long buffer_length, void *data )
#endif
{
	int ret = -1;

	if (buffer_length < 2)
		return 0;

	if (buffer[1] == '\n')
	{
		switch (buffer[0])
		{
		case '2':
			consolelog_lock();
			irmc_cl_ring_buffer_clear();
			consolelog_unlock();
			ret = 2;
			break;
		case '1':
			consolelog_start(consolelog_tty_line);
			ret = 2;
			break;
		case '0':
			consolelog_stop();
			ret = 2;
			break;
		}
	}

	return ret;
}

/** 
 @brief Create and initialize /proc/irmc/console-log file
 @return 0 in success

 irmc_proc_basedir inherited from irmc-common Linux kernel module
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
int consolelog_proc_init(void)
{
        mode_t mode = S_IFREG;
        struct proc_info *procinfo;
	if ( irmc_proc_basedir == NULL )
	{
		printk(KERN_ERR "/proc/irmc dir not found, load aborted");
                return -ENOMEM;
	}
	        /* Allocate memory */
        procinfo  = kmalloc(sizeof (struct proc_info), GFP_KERNEL);
        if (procinfo == NULL)
        {
                return -ENOMEM;
        }
        procinfo->module_fops = kmalloc(sizeof (struct file_operations),GFP_KERNEL);
        if (procinfo->module_fops == NULL)
        {
                kfree(procinfo);
                return -ENOMEM;
        }
        memset(procinfo->module_fops,0,sizeof(struct file_operations));


        procinfo->PrivateData = NULL;
	mode = S_IFREG | S_IRUGO;
        /* Create proc entry */
        procinfo->module_fops->read = consolelog_proc_read;
        procinfo->module_fops->write = consolelog_proc_write;
        procinfo->module_proc = proc_create(CONSOLELOG_PROCFS_NAME,mode,irmc_proc_basedir,procinfo->module_fops);
        if (!procinfo->module_proc)
        {
                kfree(procinfo->module_fops);
                kfree(procinfo);
                return -ENOMEM;
        }
	return 0;
}
#else
int consolelog_proc_init(void)
{
	// make /proc entries. 
	if ( irmc_proc_basedir != NULL )
	{
		cl_proc_file = create_proc_entry(CONSOLELOG_PROCFS_NAME, 0644,
			irmc_proc_basedir);

		if (cl_proc_file == NULL) {
			remove_proc_entry(CONSOLELOG_PROCFS_NAME, irmc_proc_basedir);
			printk(KERN_ALERT "Error: Could not initialize /proc/irmc/%s\n",
					CONSOLELOG_PROCFS_NAME);
			return -ENOMEM;
		}
	} 
	else 
	{
		printk(KERN_ERR "/proc/irmc dir not found, load aborted");
		return -ENOMEM;
	}

	// specify read/write callbacks
	cl_proc_file->read_proc	= consolelog_proc_read;
	cl_proc_file->write_proc = consolelog_proc_write;

	// specify file attributes for file
	cl_proc_file->owner	= THIS_MODULE;
	cl_proc_file->mode 	= S_IFREG | S_IRUGO;
	cl_proc_file->uid 	= 0;
	cl_proc_file->gid 	= 0;
	cl_proc_file->size 	= 2;

	return 0;
}
#endif
/// remove /proc/irmc/console-log file
void consolelog_proc_shutdown(void)
{
	if (irmc_proc_basedir != NULL)
	{
		remove_proc_entry(CONSOLELOG_PROCFS_NAME, irmc_proc_basedir);
		printk(KERN_INFO "/proc/irmc/%s removed\n", CONSOLELOG_PROCFS_NAME);
	}
}

