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
 @file irmc_common.c
 @brief iRMC common infrastructure (/proc/irmc/ entry)
 @date 2010-02-12
 @author Vlad Shakhov(SaM)

 FTSCHG_WU: add permdata proc file support
 FTSCHG_WU: add permdata jiffies file support
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/limits.h>
#include <linux/serial_8250.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <fts_project.h>

#include "irmc_common.h"
#include "ring_buffer.h"
#include "proc.h"
#include "device.h"
#include "filter.h"
#include "helper.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
#define init_MUTEX(sem)	 sema_init(sem, 1)
#endif
//@{
/// @name Mandatory headers of the Linux Kernel module
MODULE_DESCRIPTION("iRMC common infrastructure and host console logging via /dev/irmc-cl");
MODULE_AUTHOR("Fujitsu");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.3");
//@}

struct proc_dir_entry *irmc_proc_basedir = NULL;

// Console log uses callback to serial8250 driver.
// serial8250 driver must be patched.

/** multitasking lock to prevent race conditions (global one all around the
 console logging) i.e. GiantLock. */
static struct 	semaphore cl_sem;

/// flag for logging status
static int	cl_started = 0;

#ifdef PRAM_PROC_FILE
/**
 @brief /proc filesystem interface for read() syscall
 @param page for data returning
 @param count size of page
 @param off offset in read stream
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
static struct proc_info *pram_proc_file = NULL;
static int pram_proc_read(struct file *file, char __user *page, size_t count, loff_t *offset)
#else
static struct proc_dir_entry * pram_proc_file = NULL;
static int pram_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data )
#endif
{
        int len;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
        if(*offset != 0)
            return 0;
#endif
        len=sprintf(page,"valid %X\naddress %lX\nsize %X\n",1,CONFIG_SPX_FEATURE_GLOBAL_PRAM_START_ADDR,CONFIG_SPX_FEATURE_GLOBAL_PRAM_SIZE);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
        *offset += len;
#endif
        return len;
}
#endif

#ifdef JIFFIES_PROC_FILE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
static struct proc_info *jiffies_proc_file = NULL;
static int jiffies_proc_read(struct file *file, char __user *page, size_t count, loff_t *offset)
#else
static struct proc_dir_entry * jiffies_proc_file = NULL;
static int jiffies_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data )
#endif
{
        int len;
        char jiffies_str[80] = {0};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
        if(*offset != 0)
            return 0;
#endif
        len = sprintf(jiffies_str,"%LX\n",get_jiffies_64());
        if (copy_to_user(page,jiffies_str, len))
            return -EFAULT;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
        *offset += len;
#endif
        return len;
}
#endif

#ifdef PWR_PRESS_DURING_BOOT_PROC_FILE
#define PWBTIN_STS_REG   0x37
unsigned char pwrState = 0;
unsigned char isPwrPressedDuringBoot(unsigned char initFlag )
{
	unsigned char ret = 0;
	if(initFlag) {
		pwrState = *((unsigned char *)(SE_SYS_WAKEUP_VA_BASE+PWBTIN_STS_REG));
		pwrState &= 0x03; //mask last 2 bits
		return 0;
	}else {
		ret = pwrState;
		pwrState |= 0x80; //mark status already read
		return ret;
	}
}


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
static struct proc_info * pwrPressDuringBoot_proc_file = NULL;
static int power_button_proc_read(struct file *file, char __user *page, size_t count, loff_t *offset)
#else
static struct proc_dir_entry * pwrPressDuringBoot_proc_file = NULL;
static int power_button_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data )
#endif
{
        int len;
        char pwrbtn[80] = {0};
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
        if(*offset != 0)
            return 0;
#endif
        len = sprintf(pwrbtn,"%02X",isPwrPressedDuringBoot(0));
        if (copy_to_user(page,pwrbtn, len))
            return -EFAULT;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
        *offset += len;
#endif
        return len;
}
#endif


/// @brief return logging status
int consolelog_started(void)
{
	return cl_started;
}

/// @brief enter critical section
void consolelog_lock(void)
{
	down(&cl_sem);
}

///  @brief leave critical section
void consolelog_unlock(void)
{
	up(&cl_sem);
}

///  @brief Start logging process
void consolelog_start(unsigned int tty_line)
{
#ifndef CONSOLE_LOG_DISABLED
	if (cl_started)
		consolelog_stop(); // There can be only one.

	if ( serial8250_set_console_log_callback(tty_line, irmc_cl_ring_buffer_put_char) == 0 )
	{
		printk(KERN_DEBUG "iRMC Console Logging callback registered\n");
		consolelog_tty_line = tty_line;
		cl_started = 1;
	}
#endif
}

/**
 *  @brief Start logging transmitted characters for iRMC Debug Console
 *			to ring buffer.
 */
void irmc_debug_uart_consolelog_start(void)
{
#ifndef CONSOLE_LOG_DISABLED
	if ( serial8250_set_console_log_callback_tx(CL_IRMC_DEBUG_UART, irmc_debug_uart_ring_buffer_put_char) == 0 )
	{
		printk(KERN_DEBUG "iRMC Debug Console UART Logging callback registered\n");
	}
#endif
}

/**
 *  @brief Stop logging transmitted characters for iRMC Debug Console
 *			to ring buffer.
 */
void irmc_debug_uart_consolelog_stop(void)
{
#ifndef CONSOLE_LOG_DISABLED
	if ( serial8250_set_console_log_callback_tx(CL_IRMC_DEBUG_UART, NULL) == 0 )
	{
		printk(KERN_DEBUG "iRMC Debug Console UART Logging stopped\n");
	}
#endif
}

///  @brief Stop logging process
void consolelog_stop(void)
{
#ifndef CONSOLE_LOG_DISABLED
	if ( serial8250_set_console_log_callback(consolelog_tty_line, NULL) == 0 )
	{
		printk(KERN_DEBUG "iRMC Console Logging stopped\n");
		cl_started = 0;
	}
#endif
}

static int consolelog_init(void)
{
#ifndef CONSOLE_LOG_DISABLED
	int ret;

	printk(KERN_INFO "Load the iRMC Console Logging, device ttyS%d\n",
		consolelog_tty_line);

	init_MUTEX(&cl_sem);
	irmc_cl_ring_buffer_init();
	irmc_debug_uart_ring_buffer_init();

	ret = consolelog_device_init();
	if (ret != 0)
		return ret;

#ifdef IRMC_DEBUG
	/// run started in debug mode
	consolelog_start(consolelog_tty_line);
#else
	consolelog_stop();
#endif

	irmc_debug_uart_consolelog_start(); //always run logging for iRMC Debug Console (UART 2)
	
	return consolelog_proc_init();

#else
	return 0;
#endif // CONSOLE_LOG_DISABLED
}

static void consolelog_exit(void)
{
#ifndef CONSOLE_LOG_DISABLED
	consolelog_stop();
	irmc_debug_uart_consolelog_stop();

	consolelog_lock();
	consolelog_proc_shutdown();
	consolelog_device_shutdown();
	irmc_cl_ring_buffer_exit();
	irmc_debug_uart_ring_buffer_exit();
	consolelog_unlock();

	printk(KERN_INFO "Shutdown the iRMC Console Logging\n");
#endif //CONSOLE_LOG_DISABLED
}

static int irmc_common_init(void)
{
	int ret = 0;

	irmc_proc_basedir = proc_mkdir(IRMC_PROC_BASEDIR, NULL);
	if (irmc_proc_basedir == NULL)
	{
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",IRMC_PROC_BASEDIR);
		ret = -ENOMEM;
		goto no_dir;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
	irmc_proc_basedir->owner	= THIS_MODULE;
	irmc_proc_basedir->uid 	 	= 0;
	irmc_proc_basedir->gid 	 	= 0;
#endif

#ifdef PRAM_PROC_FILE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	pram_proc_file = AddProcEntry(irmc_proc_basedir,PRAM_PROCFS_NAME,pram_proc_read,NULL,NULL);
#else
	pram_proc_file = create_proc_read_entry(PRAM_PROCFS_NAME,0444, irmc_proc_basedir,pram_proc_read,NULL);
#endif
	if (pram_proc_file == NULL)
	{
		printk(KERN_ALERT "Error: Could not initialize /proc/irmc/%s\n",PRAM_PROCFS_NAME);
		ret = -ENOMEM;
		goto no_pram;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
	pram_proc_file->owner = THIS_MODULE;
#endif
#endif

#ifdef JIFFIES_PROC_FILE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	jiffies_proc_file = AddProcEntry(irmc_proc_basedir,JIFFIES_PROCFS_NAME,jiffies_proc_read,NULL,NULL);
#else
        jiffies_proc_file = create_proc_read_entry(JIFFIES_PROCFS_NAME,0444, irmc_proc_basedir,jiffies_proc_read,NULL);
#endif
        if (jiffies_proc_file == NULL)
        {
        	printk(KERN_ALERT "Error: Could not initialize /proc/irmc/%s\n",JIFFIES_PROCFS_NAME);
        	ret = -ENOMEM;
        	goto no_jiffies_data;
        }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
	jiffies_proc_file->owner = THIS_MODULE;
#endif
#endif
	
#ifdef PWR_PRESS_DURING_BOOT_PROC_FILE
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
	pwrPressDuringBoot_proc_file = AddProcEntry(irmc_proc_basedir, PWR_PRESS_DURING_BOOT_PROCFS_NAME,power_button_proc_read,NULL,NULL);
#else
	pwrPressDuringBoot_proc_file = create_proc_read_entry(PWR_PRESS_DURING_BOOT_PROCFS_NAME,0444, irmc_proc_basedir,power_button_proc_read,NULL);
#endif
        if (pwrPressDuringBoot_proc_file == NULL)
        {
        	printk(KERN_ALERT "Error: Could not initialize /proc/irmc/%s\n",PWR_PRESS_DURING_BOOT_PROCFS_NAME);
        	ret = -ENOMEM;
        	goto no_pwrPressDuringBoot;
        }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,11))
        pwrPressDuringBoot_proc_file->owner = THIS_MODULE;
#endif
        isPwrPressedDuringBoot(1);
#endif
	

	ret = consolelog_init();
	if (0 == ret)
		return 0;

#ifdef PWR_PRESS_DURING_BOOT_PROC_FILE
no_pwrPressDuringBoot:
	remove_proc_entry(PWR_PRESS_DURING_BOOT_PROCFS_NAME, irmc_proc_basedir);
#endif

#ifdef JIFFIES_PROC_FILE
no_jiffies_data:
	remove_proc_entry(JIFFIES_PROCFS_NAME, irmc_proc_basedir);
#endif

#ifdef PRAM_PROC_FILE
no_pram:
   	remove_proc_entry(PRAM_PROCFS_NAME, irmc_proc_basedir);
#endif

no_dir:
   	remove_proc_entry(IRMC_PROC_BASEDIR, NULL);
	return ret;
}

static void irmc_common_exit(void)
{
	consolelog_exit();
#ifdef PWR_PRESS_DURING_BOOT_PROC_FILE
	remove_proc_entry(PWR_PRESS_DURING_BOOT_PROCFS_NAME, irmc_proc_basedir);
	printk(KERN_INFO "/proc/%s/%s removed\n", IRMC_PROC_BASEDIR, PWR_PRESS_DURING_BOOT_PROCFS_NAME);
#endif
#ifdef JIFFIES_PROC_FILE
	remove_proc_entry(JIFFIES_PROCFS_NAME, irmc_proc_basedir);
	printk(KERN_INFO "/proc/%s/%s removed\n", IRMC_PROC_BASEDIR, JIFFIES_PROCFS_NAME);
#endif
#ifdef PERMDATA_PROC_FILE
	remove_proc_entry(PERMDATA_PROCFS_NAME,irmc_proc_basedir);
	printk(KERN_INFO "/proc/%s/%s removed\n", IRMC_PROC_BASEDIR, PERMDATA_PROCFS_NAME);
#endif
	remove_proc_entry(IRMC_PROC_BASEDIR, NULL);
	irmc_proc_basedir = NULL;
	printk(KERN_INFO "/proc/%s removed\n", IRMC_PROC_BASEDIR);
}

EXPORT_SYMBOL(irmc_proc_basedir);
module_init(irmc_common_init);
module_exit(irmc_common_exit);

/// @defgroup ModuleParams Linux kernel module parameters
/** @ingroup ModuleParams
 number of tty line (tty_line=4 for ttyS4)  */
module_param_named( tty_line, consolelog_tty_line, uint, S_IRUSR );
/** @ingroup ModuleParams
 flag to filter all esc sequences (filter_all=1 to filter anything in esc sequences) */
module_param_named( filter_all, consolelog_filter_all, uint, S_IRUSR );

