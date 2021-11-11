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
 *********************************************************
 *********************************************************
 *
 * File name: gpiomain_ext.c
 * This driver provides common layer, independent of the hardware, for the GPIO driver.
 * 
 * Author: Revanth A <revantha@amiindia.co.in>
 * 
 *********************************************************/

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <asm-generic/current.h>
//#include <mach/platform.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/cacheflush.h>
#include "helper.h"
#include "driver_hal.h"
#include "gpio.h"
#include "dbgout.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
DEFINE_SPINLOCK(gpio_lock);
#else
spinlock_t gpio_lock = SPIN_LOCK_UNLOCKED;
#endif

typedef struct
{
    unsigned char gpio_intr_enabled;
    gpio_trigger_method_info trigger_method;
    gpio_trigger_type_info trigger_type;
    unsigned char reading_on_assertion; //indicates what the reading was after assertion. used to determine what caused the assertion
                                            //for e.g.: in both edge triggered if reading after assertion was 0 then high to low caused it
                                            //in level low if reading after assertion was 0 then level was low and hence the interrupt happened
                                            // This may be invalid and it is upto the user program to validate
                                            // for e.g.: a line may go low to high and then back to low again and if this line is cofnigured for rising edege 
                                            // then at the time of reading the pin may have gone back to low in which case the application can conclude that
                                            // the transition happened before the state was determined.
}  __attribute__((packed)) gpio_interrupt;

typedef struct _gpiolist
{
    gpio_interrupt gpio_intr[256];
    char AppName[16];
    unsigned int  pid;
    int instance;
    unsigned char wakeup;
    struct list_head list;
}__attribute__((packed)) gpiolist;

typedef struct _pending_gpio_intr_info
{
    unsigned short gpio;
    unsigned char  state; //use to determine what transition caused the interrupt.State is reading of the pin after the interrupt.
                            //THIS HAS TO BE DETERMINED IN THE IRQ ITSELF
    gpio_trigger_type_info trigger_type; //cause of the interrupt, as determined by the IRQ itself
    unsigned int pid;
    struct list_head list;
}  __attribute__((packed)) pending_gpio_intr_info;

uint32_t total_interrupt_gpio = 0;
struct list_head head_gpio_list;
struct list_head head_pending_gpio_list;
unsigned char g_gpiopinlist[256] = {0};
int instlist[MAX_IOCTL_INSTANCE+1] = {0};
static wait_queue_head_t gpio_gpioint_wq[MAX_IOCTL_INSTANCE+1];
/**
 * @fn gpio_verify_ext
 * @brief This function used find gpio_num is registered or not
 * @param gpio_num -  gpio pin number
  * return 1 gpio pin found
 *         0  gpio pin not found
**/
int gpio_verify_ext(int gpio_num)
{
    gpiolist *node = NULL;
    struct list_head *pos;

    list_for_each(pos,&head_gpio_list)
    {
        node = list_entry(pos,gpiolist,list);
        if(node->gpio_intr[gpio_num].gpio_intr_enabled == 1)
        {
            return 1;
        }
    }
    return 0;
}
/**
 * @fn ReturnAppInst
 * @brief This function used find instance of application
 * @param pid -  application pid
 * @param Appname - Application Name
 * return -1 application instance not found
 *        instance.
**/
int ReturnAppInst(unsigned int pid,char *Appname)
{
    struct list_head *pos;
    gpiolist *tmpnode = NULL;
    int inst = -1;
    list_for_each(pos,&head_gpio_list)
    {
        tmpnode = list_entry(pos,gpiolist,list);
        if((tmpnode->pid == pid) && (strncmp(tmpnode->AppName,Appname,strlen(Appname)) == 0))
        {
            inst = tmpnode->instance;
            break;
        }
    }
    return inst;
}

/**
 * @fn getFreeInst
 * @brief This function used find free instance
 * return -1 no Free instance
 *        instance.
**/
int getFreeInst(void)
{
    int i;
    for(i = 1;i<=MAX_IOCTL_INSTANCE;i++)
    {
        if(instlist[i] == 0)
            return i;
    }
    if(i > MAX_IOCTL_INSTANCE) return -1;
    return -1;
}


/**
 * @fn add_gpio_interrupt
 * @brief This function used to add GPIO interrupt details in pending list
 * @param  gpionum  - GPIO Pin Number
 * @param  pid   -  application's pid
 * @param  state  - assertion state
 * @param  trigger_type  - trigger type
 * @retval 0 success
 **/
static void add_gpio_interrupt(unsigned short gpionum, unsigned int pid, unsigned char state, unsigned char trigger_type)
{
    pending_gpio_intr_info *newnode = NULL;      /* New node to be inserted */

    //checking for interrupt is already exits in pending list
    pending_gpio_intr_info *node;
    struct list_head *pos;
    list_for_each(pos,&head_pending_gpio_list)
    {
        node = list_entry(pos,pending_gpio_intr_info,list);
        if (node->gpio == gpionum && node->trigger_type == trigger_type && node->pid == pid) 
        {
            //dbgprint("Level %s interrupt already queued for GPIO %d, ignoring\n", type, gpionum);
//                printk("Already Exists\n");
            //touch_softlockup_watchdog();
            return;
        }
    }
    
    newnode=(pending_gpio_intr_info*)kmalloc(sizeof(pending_gpio_intr_info),GFP_ATOMIC);
    if (newnode == NULL)
        return;
    
    newnode->gpio=gpionum;
    newnode->state=state;
    newnode->trigger_type=trigger_type;
    newnode->pid=pid;
    
    list_add(&newnode->list,&head_pending_gpio_list);
}

/**
 * @fn process_gpio_intr_ext
 * @brief This function used to check occurred interrupt of gpio pin is belong to any application, if yes add it in to pending list
 * @param  gpio_pin_number  - GPIO Pin Number
 * @param  state  - assertion state
 * @param  trigger_type  - trigger type
 * @retval 0 success
 **/
int
process_gpio_intr_ext (int gpio_pin_number,unsigned char state, unsigned char trigger_type)
{
    unsigned long flags;
    gpiolist *node = NULL;
    struct list_head *pos;

    //gpio Interrupt list
    node = NULL;
    list_for_each(pos,&head_gpio_list)
    {
        node = list_entry(pos,gpiolist,list);
        if(node->gpio_intr[gpio_pin_number].gpio_intr_enabled == 1)
        {
            node->gpio_intr[gpio_pin_number].reading_on_assertion=state;
            spin_lock_irqsave(&gpio_lock, flags);
            add_gpio_interrupt(gpio_pin_number,node->pid, state, trigger_type);
            node->wakeup = 1;
            spin_unlock_irqrestore (&gpio_lock, flags);
        }
    }
    return 0;
}

/**
 * @fn remove_gpio_interrupt
 * @brief This function used to delete GPIO interrupt from Pending list
 * @param  gpio_num  - GPIO Pin Number
 * @retval void
 **/
void remove_gpio_interrupt(unsigned short gpio_num)
{
    pending_gpio_intr_info *node;
    struct list_head *pos;
begin:
    list_for_each(pos,&head_pending_gpio_list)
    {
        node = list_entry(pos,pending_gpio_intr_info,list);
        if (node->gpio == gpio_num && node->pid == current->tgid) {
            list_del(pos);
            kfree(node);
            goto begin;
        }
    }
}
/**
 * @fn status_gpio_interrupt
 * @brief This function used to get the status GPIO interrupt from Pending list
 * @param  gpio_num  - GPIO Pin Number
 * @retval 1 interrupt triggered
 *         0 interrupt Not triggered
 **/
unsigned char status_gpio_interrupt(unsigned short gpio_num)
{
    pending_gpio_intr_info *node = NULL;
    struct list_head *pos;

    list_for_each(pos,&head_pending_gpio_list)
    {
        node = list_entry(pos,pending_gpio_intr_info,list);
        if (node->gpio == gpio_num && current->tgid == node->pid)
               return 1;
    }
    return 0;
}
/**
 * @fn process_pending_gpio_interrupt
 * @brief This function used to collect all GPIO pins in pending list which are matched with application pid
 * @param  pid  - application pid
 * @param triggered_ints - pointer array which is used hold all gpio pins matched with applicatio pid
 * @retval void
  **/
void process_pending_gpio_interrupt(unsigned int pid,unsigned char *triggered_ints)
{
    pending_gpio_intr_info *node;
    struct list_head *pos;
    int idx;

    list_for_each(pos,&head_pending_gpio_list)
    {
        node = list_entry(pos,pending_gpio_intr_info,list);
        idx = 0;
        if(node->pid == pid)
        {
            if(node->gpio >= 8)
                idx = node->gpio/8;
            else
                idx = 0;
                
            triggered_ints[idx] = (1 << (node->gpio - (8 *idx)));
        }
    }
}

/**
 * @fn wakeup_intr_queue_ext
 * @brief This function used to wake the waiting state of application
 * @retval void
**/
void
wakeup_intr_queue_ext (int intr_type)
{
    gpiolist *node = NULL;
    struct list_head *pos;

    node = NULL;
    list_for_each(pos,&head_gpio_list)
    {
        node = list_entry(pos,gpiolist,list);
        if(node->wakeup == 1)
        {
            wake_up_interruptible(&gpio_gpioint_wq[node->instance]);
            node->wakeup = 0;
        }
    }
}
/**
 * @fn isRegisteredAllowed
 * @brief This function used to check the GPIO Pin interrupt is already enabled by other application or not
 * @parm gpio_int_sensor - gpio pin interrupt configuration details
 * @retval -1 already register with different method and type
 *         1 already register with same details.
 *         0 not resistered.
**/
static int isRegisteredAllowed(gpio_interrupt_sensor *gpio_int_sensor)
{
    int j=0;
    gpiolist *node = NULL;
    int found = 0;
    struct list_head *pos;

    /* Check if it's a sensor interrupt */
    for(j=0;j<total_interrupt_sensors;j++)
    {
        if(gpio_int_sensor->gpio_number == intr_sensors[j].gpio_number)
        {
            if((gpio_int_sensor->trigger_method != intr_sensors[j].trigger_method) || (gpio_int_sensor->trigger_type != intr_sensors[j].trigger_type))
                return -1;
            found = 1;
        }
    }
   /* Check if it's a chassis interrupt */
    for(j=0;j<total_chassis_interrupts;j++)
    {
        if(gpio_int_sensor->gpio_number == intr_chassis[j].gpio_number)
        {
            if((gpio_int_sensor->trigger_method != intr_chassis[j].trigger_method) || (gpio_int_sensor->trigger_type != intr_chassis[j].trigger_type))
                return -1;
            found = 1;
        }
    }
    node = NULL;
    j=0;
    list_for_each(pos,&head_gpio_list)
    {
        node = list_entry(pos,gpiolist,list);
        if(total_interrupt_gpio <= j)break;
        if(node->gpio_intr[gpio_int_sensor->gpio_number].gpio_intr_enabled == 1)
        {
            if((gpio_int_sensor->trigger_method != node->gpio_intr[gpio_int_sensor->gpio_number].trigger_method) || (gpio_int_sensor->trigger_type != node->gpio_intr[gpio_int_sensor->gpio_number].trigger_type))
                return -1;
            found = 1;
        }
        j++;
    }
    if(found == 1) return 1;
    return 0;

}
/**
 * @fn unregistergpio_int
 * @brief This function used to unregister all gpio pin's interrupt if pid matched with applications pid.
 * @parm *pdev - gpio device pointer
 * @retval void
**/
void unregistergpio_int(struct gpio_dev *pdev)
{
    gpiolist *tmpnode = NULL;
    struct list_head *pos;
    unsigned long flags;
    Gpio_data_t gpin_data = {0};
    int j;

    spin_lock_irqsave(&gpio_lock, flags);
    if(list_empty(&head_gpio_list) == 1)
    {
        spin_unlock_irqrestore(&gpio_lock,flags);
        return;
    }
    list_for_each(pos,&head_gpio_list)
    {
        tmpnode = list_entry(pos,gpiolist,list);

        if(tmpnode->pid == current->tgid)
        {
            for(j=0;j<256;j++)
            {
                if(tmpnode->gpio_intr[j].gpio_intr_enabled == 1) 
                {
                    if(g_gpiopinlist[j] == 1)//regsiter more than one application
                    {
                        gpin_data.PinNum = j;
                        if (pdev->pgpio_hal->pgpio_hal_ops->unregsensorints)
                            pdev->pgpio_hal->pgpio_hal_ops->unregsensorints((void*)&gpin_data );
                    }
                    g_gpiopinlist[j] -= 1; //decrease application count
                    remove_gpio_interrupt(j);//remove gpio interrupt from the pending list
                }
            }
            total_interrupt_gpio--;
            instlist[tmpnode->instance] = 0;//free instance , to avail for new application
            list_del(pos);
            kfree(tmpnode);
            break;
        }
    }
    spin_unlock_irqrestore(&gpio_lock,flags);
}
/**
 * @fn gpio_ioctlUnlocked_ext
 * @brief IOCTL function
 * @parm *pdev - gpio device pointer
 * @param cmd - IOCTL command
 * @param arg - input/output argument
 * @retval -1 failed
 *         -EINVAL invaild ioctl cmd
 *         -EFAULT failed to read/write of user space.
**/
long gpio_ioctlUnlocked_ext(struct gpio_dev *pdev, unsigned int cmd, unsigned long arg)
{
    Gpio_data_t gpin_data;
    gpio_interrupt_sensor gpio_intr;
    unsigned char ret_gpio_int_triggered[64];
    unsigned long flags;
    int instance;

    switch (cmd)
    {
        case IOCTL_GPIO_INT_REGISTER:
        {
            int ret = 0;
            if ( copy_from_user(&gpio_intr, (void*)arg, sizeof(gpio_interrupt_sensor)) )
                            return -EFAULT;
            //Check for GPIO is already register by other application
            if((ret = isRegisteredAllowed(&gpio_intr)) == -1)
            {
                printk("GPIO PIN %d Already Registered with different triggered method and type\n",gpio_intr.gpio_number);
                printk("Registered not allowed for GPIO PIN %d\n",gpio_intr.gpio_number);
                return -1;
            } 
            
            if((ret == 0) && (g_gpiopinlist[gpio_intr.gpio_number] == 0))
            {
                if (pdev->pgpio_hal->pgpio_hal_ops->regsensorints) {
                    if (0 != pdev->pgpio_hal->pgpio_hal_ops->regsensorints((void*)&gpin_data, 1, (void *)&gpio_intr))
                        return -1;
                    }
            }
            //check if it is New Application trying to register
            if(ReturnAppInst(current->tgid,current->comm) == -1)
            {
                gpiolist *newnode = NULL;
                newnode=(gpiolist*)kmalloc(sizeof(gpiolist),GFP_ATOMIC); //create new node
                if(newnode == NULL)
                    return -1;
                newnode->instance = getFreeInst(); // get free instance to use for wakeup the waiting state application.
                if(newnode->instance == -1)
                {
                    printk("Reached Max App limit %d\n",MAX_IOCTL_INSTANCE);
                    kfree(newnode);
                    return -1;
                }
                instlist[newnode->instance] = 1; //used instance
                total_interrupt_gpio += 1;
                newnode->pid = current->tgid; //process ID
                memset(&newnode->AppName,0,16);
                memset(&newnode->gpio_intr,0,sizeof(gpio_interrupt)*256);
                strncpy(newnode->AppName,current->comm,strlen(current->comm));
                newnode->gpio_intr[gpio_intr.gpio_number].gpio_intr_enabled = 1;
                newnode->gpio_intr[gpio_intr.gpio_number].trigger_method = gpio_intr.trigger_method;
                newnode->gpio_intr[gpio_intr.gpio_number].trigger_type = gpio_intr.trigger_type;
                
                g_gpiopinlist[gpio_intr.gpio_number] += 1;//application count to find same gpio pin interrupt enabled
    
                spin_lock_irqsave(&gpio_lock, flags);
    
                list_add(&newnode->list,&head_gpio_list); // add to head_gpio_list linked
                
                spin_unlock_irqrestore (&gpio_lock, flags);
            }
            else
            {   //application already exists
                gpiolist *tmpnode = NULL;
                struct list_head *pos;
                spin_lock_irqsave(&gpio_lock, flags);
                list_for_each(pos,&head_gpio_list)//traverse node by node to find application's node  
                {
                    tmpnode = list_entry(pos,gpiolist,list);
                    if((tmpnode->pid == current->tgid) && (strncmp(tmpnode->AppName,current->comm,strlen(current->comm)) == 0))
                    {
                        tmpnode->gpio_intr[gpio_intr.gpio_number].gpio_intr_enabled = 1;
                        tmpnode->gpio_intr[gpio_intr.gpio_number].trigger_method = gpio_intr.trigger_method;
                        tmpnode->gpio_intr[gpio_intr.gpio_number].trigger_type = gpio_intr.trigger_type;
                        g_gpiopinlist[gpio_intr.gpio_number] += 1;
                        break;
                    }
                }
                spin_unlock_irqrestore(&gpio_lock,flags);
                total_interrupt_gpio += 1;
            }
        }
        break;
        case IOCTL_GPIO_INT_WAIT_FOR:
        {
            instance = ReturnAppInst(current->tgid,current->comm); //Get Instance of the application to use in wait_event_interruptible
            if(instance == -1)
                return -EFAULT;

            if(wait_event_interruptible(gpio_gpioint_wq[instance],!list_empty(&head_pending_gpio_list)))
                return -ERESTARTSYS;
    
            spin_lock_irqsave(&gpio_lock, flags);
            memset(&ret_gpio_int_triggered,0,64);
            process_pending_gpio_interrupt(current->tgid,ret_gpio_int_triggered); // collect all triggered gpio's interrupt of application.  
            spin_unlock_irqrestore(&gpio_lock,flags);
    
            
            if(copy_to_user((void __user *)arg, (void*) &ret_gpio_int_triggered, sizeof(ret_gpio_int_triggered)))
                        return -EFAULT;
        }
            break;
        case IOCTL_GPIO_INT_GET_STATUS:
        {
            
            if ( copy_from_user(&gpin_data, (void*)arg, sizeof(Gpio_data_t)) )
                                    return -EFAULT;
            spin_lock_irqsave(&gpio_lock, flags);
            
            if(!list_empty(&head_pending_gpio_list))
                gpin_data.data = status_gpio_interrupt(gpin_data.PinNum);
            else
                gpin_data.data = 0;
            spin_unlock_irqrestore(&gpio_lock,flags);
            
            if(copy_to_user((void __user *)arg, (void*) &gpin_data, sizeof(Gpio_data_t)))
                return -EFAULT;
        }
            break;
        case IOCTL_GPIO_INT_CLEAR:
            if ( copy_from_user(&gpin_data, (void*)arg, sizeof(Gpio_data_t)) )
                            return -EFAULT;
    
            spin_lock_irqsave(&gpio_lock, flags);
            remove_gpio_interrupt(gpin_data.PinNum);
            spin_unlock_irqrestore(&gpio_lock,flags);
    
            break;
        case IOCTL_GPIO_INT_UNREGISTER:
            {
                gpiolist *tmpnode = NULL;
                struct list_head *pos;
                int j;
                if ( copy_from_user(&gpin_data, (void*)arg, sizeof(Gpio_data_t)) )
                                return -EFAULT;
                spin_lock_irqsave(&gpio_lock, flags);
                list_for_each(pos,&head_gpio_list)
                {
                    tmpnode = list_entry(pos,gpiolist,list);
                    if(tmpnode->pid == current->tgid && tmpnode->gpio_intr[gpin_data.PinNum].gpio_intr_enabled == 1) 
                    {
                        if(g_gpiopinlist[gpin_data.PinNum] == 1)
                        {
                            if (pdev->pgpio_hal->pgpio_hal_ops->unregsensorints)
                            {
                                if (0 != pdev->pgpio_hal->pgpio_hal_ops->unregsensorints((void*)&gpin_data ))
                                {
                                    spin_unlock_irqrestore(&gpio_lock,flags);
                                    return -1;
                                }
                            }
                        }
                        g_gpiopinlist[gpin_data.PinNum] -= 1;
                        total_interrupt_gpio -= 1;
                        tmpnode->gpio_intr[gpin_data.PinNum].gpio_intr_enabled = 0;
                        tmpnode->gpio_intr[gpin_data.PinNum].trigger_method = 0;
                        tmpnode->gpio_intr[gpin_data.PinNum].trigger_type = 0;
                        remove_gpio_interrupt(gpin_data.PinNum);//remove pending gpio interrupts 
                        break;
                    }
                }
                tmpnode = NULL;
                list_for_each(pos,&head_gpio_list)
                {
                    tmpnode = list_entry(pos,gpiolist,list);
                    if(tmpnode->pid == current->tgid) 
                    {
                        for(j=0;j<256;j++)
                        {
                            if(tmpnode->gpio_intr[j].gpio_intr_enabled == 1) 
                            {
                                break;
                            }
                        }
                        if(j == 256)
                        {
                            instlist[tmpnode->instance] = 0;
                            tmpnode->instance = 0;
                            list_del(pos);
                            kfree(tmpnode);
                            tmpnode = NULL;
                            break;
                        }
                    }
                }
                spin_unlock_irqrestore(&gpio_lock,flags);
            }
            break;
        default:
            return -EINVAL;
        }
    return 0;
}
/**
 * @fn gpio_initialize
 * @brief This function is used initailize wait queue and gpio linked list head node. 
 * @retval void
**/
void gpio_initialize(void)
{
    int Instance;
    
    for (Instance = 1; Instance <= MAX_IOCTL_INSTANCE; Instance++)
                init_waitqueue_head(&gpio_gpioint_wq[Instance]);
    
    INIT_LIST_HEAD(&head_gpio_list);
    INIT_LIST_HEAD(&head_pending_gpio_list);
}
