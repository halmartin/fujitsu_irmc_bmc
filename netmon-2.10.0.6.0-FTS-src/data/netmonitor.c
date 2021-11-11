/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */
/****************************************************************
  Author	: Samvinesh Christopher

  Module	: Network Interface Monitor

  Revision	: 1.0  

  Changelog : 1.0 - Initial Version  [SC]

 *****************************************************************/
#ifdef MODULE

#define NETMON_MAJOR 125

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/poll.h>

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include "netmon_io.h"

#ifdef HAVE_UNLOCKED_IOCTL
  #if HAVE_UNLOCKED_IOCTL
	#define USE_UNLOCKED_IOCTL
  #endif
#endif

#ifdef NETMON_DEBUG
int verbose=1;
#else
int verbose=0;
#endif
typedef enum
{
    ACTIVE_STATUS = 0,
    LINK_STATUS,
    PHY_STATUS
} interface_status;

typedef struct 
{
	struct list_head list;
	char name[IFNAMSIZ+1];
	unsigned char upstatus;
	unsigned char linkstatus;
	unsigned char phy_index;
	unsigned char phy_linkstate;
} ifname_list;

typedef struct _pending_phy_ints_info
{
    int phy_state;
    struct net_device *phy_dev;
    struct _pending_phy_ints_info *next;
} pending_phy_ints_info;

static int  if_count=0;
//static int netmonwakeup=NETMON_SLEEP;

static int linkup_wakeup=NETMON_SLEEP;
static int linkdown_wakeup=NETMON_SLEEP;
static int linkchange_wakeup=NETMON_SLEEP;
static int linkstate=0;
struct net_device *linkdev=NULL;

#if defined(NETDEV_PHY_LINK_DOWN) || defined(NETDEV_PHY_LINK_UP)
#define MAX_PHY_INTERRUPT 10
pending_phy_ints_info *front_phy_link_ints = NULL;
pending_phy_ints_info *rear_phy_link_ints  = NULL;
int pending_phy_ints_count = 0;
static DECLARE_WAIT_QUEUE_HEAD(phy_link_event);
#endif

static LIST_HEAD(if_list);
static DECLARE_WAIT_QUEUE_HEAD(monitor_event);
static DECLARE_WAIT_QUEUE_HEAD(link_event);

static atomic_t net_event, listener_count;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
DEFINE_SPINLOCK(phy_lock);
DEFINE_SPINLOCK(if_list_lock);
#else
spinlock_t phy_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t if_list_lock = SPIN_LOCK_UNLOCKED;
#endif

static
int
AddInterface(char *name, unsigned char phy_index)
{
	int ret = 0;
	ifname_list *iface;
	unsigned long flags;

	spin_lock_irqsave(&if_list_lock, flags);

 	list_for_each_entry (iface, &if_list, list)
        {
                if (strcmp(name,iface->name) == 0)
                {
                        ret = -EEXIST;
                        goto failed;
                }
        }

	iface = (ifname_list *)kmalloc(sizeof(ifname_list),GFP_KERNEL);
	if (!iface)
	{
		ret = -ENOMEM;
		goto failed;
	}
        memset(iface,0,sizeof(ifname_list));
	strcpy(iface->name,name);
	iface->phy_index = phy_index;
	iface->upstatus = 0;
	iface->phy_linkstate = 0;
	iface->linkstatus = -1;
	
	list_add_tail(&(iface->list),&if_list);
	
	if_count++;
failed:
	spin_unlock_irqrestore (&if_list_lock, flags);
	return ret;
}

static
int
RemoveInterface(char *name)
{
	int ret= -EINVAL;
	ifname_list *iface;
	unsigned long flags;

	spin_lock_irqsave(&if_list_lock, flags);

 	list_for_each_entry (iface, &if_list, list)
        {
                if (strcmp(name,iface->name) == 0)
		{
			list_del(&(iface->list));
			kfree(iface);		
			if_count--;
			ret= 0;
			break;	
                }
        }

	spin_unlock_irqrestore (&if_list_lock, flags);
	return ret;
}

static
int
UpdateInterfaceStatus(char *name, int info, unsigned char status)
{
	int ret = 0;
	ifname_list *iface;
    unsigned long flags;

    spin_lock_irqsave(&if_list_lock, flags);

 	list_for_each_entry (iface, &if_list, list)
        {
                if (strcmp(name,iface->name) == 0)
                {
            switch (info)
            {
            case ACTIVE_STATUS:
                iface->upstatus = status;
                break;
            case LINK_STATUS:
                if(iface->linkstatus != status) /* Update it only if there is a link change */
                {
                    iface->linkstatus = status;
                    ret = 1;
                }
                break;
            case PHY_STATUS:
                iface->phy_linkstate = status;
                break;
            }
        }
    }

    spin_unlock_irqrestore (&if_list_lock, flags);
    return ret;
}

static
int
GetInterfaceStatus(char *name, int info)
{
    int ret = 0;
    ifname_list *iface;
    unsigned long flags;

    spin_lock_irqsave(&if_list_lock, flags);
    list_for_each_entry (iface, &if_list, list)
    {
        if (strcmp(name,iface->name) == 0)
        {
            switch (info)
            {
            case ACTIVE_STATUS:
                ret = iface->upstatus;
                break;
            case LINK_STATUS:
                ret = iface->linkstatus;
                break;
            case PHY_STATUS:
                ret = iface->phy_linkstate;
                break;
            }
        }
    }
    spin_unlock_irqrestore (&if_list_lock, flags);
    
    return ret;
}

// Some platform will not have PHY monitor
#if defined(NETDEV_PHY_LINK_DOWN) || defined(NETDEV_PHY_LINK_UP)

/* Removes the interrupt item in the front of the Q */
void remove_phy_interrupt(void)
{
    pending_phy_ints_info *delnode;      /* Node to be deleted */
    if(front_phy_link_ints == NULL && rear_phy_link_ints == NULL)
        printk("NETMON: Queue is empty to delete any element\n");
    else
    {
        delnode = front_phy_link_ints;
        front_phy_link_ints = front_phy_link_ints->next;
        kfree(delnode);
        if(front_phy_link_ints == NULL)
            rear_phy_link_ints = NULL;
        pending_phy_ints_count--;
    }
}


/* Add the interrupt item at the rear end of the Q */
void add_phy_interrupt(int phy_state, struct net_device *phy_dev)
{
    pending_phy_ints_info *newnode;      /* New node to be inserted */

    newnode = (pending_phy_ints_info*)kmalloc(sizeof(pending_phy_ints_info), GFP_ATOMIC);
    if (newnode == NULL)
        return;

    if (pending_phy_ints_count == MAX_PHY_INTERRUPT)
    {
        remove_phy_interrupt();
    }

    newnode->phy_state = phy_state;
    newnode->phy_dev = phy_dev;
    newnode->next = NULL;
    if(front_phy_link_ints == NULL && rear_phy_link_ints == NULL)
    {
        front_phy_link_ints = newnode;
        rear_phy_link_ints = newnode;
    }
    else
    {
        rear_phy_link_ints->next = newnode;
        rear_phy_link_ints = newnode;
    }
    pending_phy_ints_count++;
}

int gpio_waitforphyint (LINK_CHANGE_INFO *linkinfo)
{
    unsigned long flags;
    if(front_phy_link_ints == NULL)
    {
        if(wait_event_interruptible(phy_link_event, (front_phy_link_ints != NULL)))
            return -ERESTARTSYS;
    }
    
    spin_lock_irqsave(&phy_lock, flags);
    /* Read info from the pending interrupts */
    linkinfo->linkstate = front_phy_link_ints->phy_state;
    linkinfo->phy_index = front_phy_link_ints->phy_dev->ifindex - 2;
    strncpy (linkinfo->ifname, front_phy_link_ints->phy_dev->name, IFNAMSIZ);
    linkinfo->ifname [IFNAMSIZ - 1] = '\0';

    /* Remove the read interrupt entry from the pending interrupts */
    remove_phy_interrupt();
    spin_unlock_irqrestore(&phy_lock, flags);

    return 0; 
}
#endif

static int 
netmon_netdev_event(struct notifier_block *this, unsigned long event, void *ptr)
{
#if (LINUX_VERSION_CODE >KERNEL_VERSION(3,4,11))
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
#else
	struct net_device *dev = ptr;
#endif
#if defined(NETDEV_PHY_LINK_DOWN) || defined(NETDEV_PHY_LINK_UP)
	unsigned long flags;
#endif

	/* Handle only Ethernet devices. Don't handle others */
	if (dev->type != ARPHRD_ETHER)
		return NOTIFY_DONE;

	//Allow only physical interfaces that can get IP. Do not allow virtual interfaces!!!!!
	if (strncmp(dev->name,"eth", 3) && strncmp(dev->name, "bond", 4)  && strncmp(dev->name, "lo", 2) && strncmp(dev->name, "usb", 3))
		return NOTIFY_DONE;

	switch (event) 
	{
		case NETDEV_REGISTER:
			if (verbose)
				printk("NETMON: NetDev Register Event for %s\n",dev->name);
			AddInterface(dev->name, dev->ifindex - 2);
			atomic_inc(&net_event);
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_UNREGISTER:
			if (verbose)
				printk("NETMON: NetDev UnRegister Event for %s\n",dev->name);
			RemoveInterface(dev->name);
			atomic_inc(&net_event);
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_UP:
			if (verbose)
				printk("NETMON: NetDev Up Event for %s\n",dev->name);
			UpdateInterfaceStatus(dev->name, ACTIVE_STATUS, 1);
			atomic_inc(&net_event);
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_DOWN:
			if (verbose)
				printk("NETMON: NetDev Down Event for %s\n",dev->name);
			UpdateInterfaceStatus(dev->name, ACTIVE_STATUS, 0);
			atomic_inc(&net_event);
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_LINK_DOWN:
			if (verbose)
				printk("NETMON: NetDev Link Down Event for %s\n",dev->name);
			linkdev=dev;
			linkstate=0;
			if(UpdateInterfaceStatus(dev->name, LINK_STATUS, linkstate))  // AMI_OEM
			{
				linkdown_wakeup= NETMON_WAKE_UP;
				linkchange_wakeup= NETMON_WAKE_UP;
				wake_up_interruptible_all(&link_event);
			}
			break;
		case NETDEV_LINK_UP:
			if (verbose)
				printk("NETMON: NetDev Link Up Event for %s\n",dev->name);
			linkdev=dev;
			linkstate=1;
			if(UpdateInterfaceStatus(dev->name, LINK_STATUS, linkstate))  // AMI_OEM
			{
				linkup_wakeup= NETMON_WAKE_UP;
				linkchange_wakeup= NETMON_WAKE_UP;
				wake_up_interruptible_all(&link_event);
			}
			break;
#ifdef NETDEV_PHY_LINK_DOWN  // Some platform will not have PHY monitor
		case NETDEV_PHY_LINK_DOWN:
			if (verbose)
				printk("NETMON: NetDev PHY Link Down Event for %s\n", dev->name);
			UpdateInterfaceStatus (dev->name, PHY_STATUS, 0);
			spin_lock_irqsave(&phy_lock, flags);
			add_phy_interrupt(0, dev);
			spin_unlock_irqrestore (&phy_lock, flags);
			wake_up_interruptible_all(&phy_link_event);
			break;
#endif
#ifdef NETDEV_PHY_LINK_UP   // Some platform will not have PHY monitor
		case NETDEV_PHY_LINK_UP:
			if (verbose)
				printk("NETMON: NetDev PHY Link Up Event for %s\n", dev->name);
			UpdateInterfaceStatus (dev->name, PHY_STATUS, 1);
			spin_lock_irqsave(&phy_lock, flags);
			add_phy_interrupt(1, dev);
			spin_unlock_irqrestore (&phy_lock, flags);
			wake_up_interruptible_all(&phy_link_event);
			break;
#endif
#if 0		// Change to 0 after debugging and testing is completed 
		case NETDEV_CHANGEADDR:
			printk("NETMON: NetDev Addr Change Event for %s\n",dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_CHANGEMTU:
			printk("NETMON: NetDev MTU  Change Event for %s\n",dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_CHANGENAME:
			printk("NETMON: NetDev Name Change Event for %s\n",dev->name);
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_CHANGE:
			printk("NETMON: NetDev State Change Event for %s\n",dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_GOING_DOWN:	
			printk("NETMON: NetDev Going Down Event for %s\n",dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_REBOOT:	
			printk("NETMON: NetDev Reboot Event for %s\n",dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_FEAT_CHANGE:
			printk("NETMON: NetDev Feature Change Event for %s\n",dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
		case NETDEV_BONDING_FAILOVER:
			printk("NETMON: NetDev Bonding Failover Event for %s\n",dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
		default:
			printk("NETMON: WARNING: NetDev Unhandled (%ld) Event for %s\n",event,dev->name);
			netmonwakeup = NETMON_WAKE_UP;
			wake_up_interruptible_all(&monitor_event);
			break;
#endif
	}
	return NOTIFY_DONE;
}


static long     
NetmonIoctlUnlocked(struct file *file, unsigned int cmd,unsigned long arg)
{
	INTERFACE_LIST *iface_arg;
	char *ifname;
	char *ifupstatus;
	LINK_CHANGE_INFO linkinfo;
	
 	switch (cmd)
        {
                case NETMON_GET_INTERFACE_COUNT:
			if (__copy_to_user((void *)arg,(void*)&if_count,sizeof(int)))
			{		
				printk("ERROR: NETMON: Get Interface Count Failed\n");
				return -EFAULT;
			}
			break;

                case NETMON_GET_INTERFACE_LIST:
			{
				ifname_list *iface;

				iface_arg=(INTERFACE_LIST *)arg;

				if (__copy_to_user((void *)&iface_arg->count,(void*)&if_count,sizeof(int)))	
				{
					printk("ERROR: NETMON: Get Interface List Failed. Unable to copy length\n");
					return -EFAULT;
				}
			
				ifname=iface_arg->ifname;
				ifupstatus = iface_arg->ifupstatus;
		 		list_for_each_entry (iface, &if_list, list)
				{
					if (__copy_to_user((void *)ifname, (void *)iface->name,IFNAMSIZ+1))
					{
						printk("ERROR: NETMON: Get Interface List Failed. Unable to interface %s\n",iface->name);
						return -EFAULT;
					}
					if (__copy_to_user((void *)ifupstatus,(void *)&iface->upstatus,sizeof(unsigned char)))	
					{
						printk("ERROR: NETMON: Get Interface List Failed. Unable to interface %s up status\n",iface->name);
						return -EFAULT;
					}
					ifname+=IFNAMSIZ+1;
					ifupstatus+=1;
				}				
			}
			break;

		case NETMON_WAIT_FOR_INTERFACE_CHANGE:

			/*EAGAIN is returned to avoid awakening same listener twice for one event*/
			if((atomic_read(&net_event) > 0) && (atomic_read(&listener_count)> 0))
			{
				return -EAGAIN;
			}
			atomic_inc (&listener_count);
			
			wait_event_interruptible(monitor_event,(atomic_read(&net_event) > 0));
			if ((atomic_read (&listener_count) > 0))
			{
				atomic_dec (&listener_count);
			}
			if ((atomic_read (&listener_count) == 0))
			{
				atomic_dec(&net_event);
			}
			break;

		case NETMON_WAIT_FOR_LINK_CHANGE:
			wait_event_interruptible(link_event,(linkchange_wakeup == NETMON_WAKE_UP));
			linkchange_wakeup = NETMON_SLEEP;
			linkinfo.linkstate=linkstate;
			if(linkdev != NULL)
				memcpy(linkinfo.ifname,linkdev->name,IFNAMSIZ);
			if (__copy_to_user((void *)arg,(void*)&linkinfo,sizeof(linkinfo)))
			{		
				printk("ERROR: NETMON: Wait for Link Change Failed\n");
				return -EFAULT;
			}
			break;

		case NETMON_WAIT_FOR_LINK_UP:
			wait_event_interruptible(link_event,(linkup_wakeup == NETMON_WAKE_UP));
			linkup_wakeup = NETMON_SLEEP;
			if (__copy_to_user((void *)arg,(void*)&linkdev->name,IFNAMSIZ))
			{		
				printk("ERROR: NETMON: Wait for Link Up Failed\n");
				return -EFAULT;
			}
			break;

		case NETMON_WAIT_FOR_LINK_DOWN:
			wait_event_interruptible(link_event,(linkdown_wakeup == NETMON_WAKE_UP));
			linkdown_wakeup = NETMON_SLEEP;
			if (__copy_to_user((void *)arg,(void*)&linkdev->name,IFNAMSIZ))
			{		
				printk("ERROR: NETMON: Wait for Link Down Failed\n");
				return -EFAULT;
			}
			break;
	case NETMON_GET_INTERFACE_LINK_STATE:
		if (__copy_from_user(&linkinfo, (void*)arg, sizeof(LINK_CHANGE_INFO)))
		{
			printk("ERROR: NETMON: Get PHY Link state failed\n");
			return -EFAULT;
		}

		linkinfo.linkstate = GetInterfaceStatus (linkinfo.ifname, LINK_STATUS);
		if (__copy_to_user((void *)arg, (void*)&linkinfo, sizeof(LINK_CHANGE_INFO)))
		{
			printk("ERROR: NETMON: Get Interface Link state failed\n");
			return -EFAULT;
		}
		break;

	case NETMON_GET_PHY_LINK_STATE:
		if (__copy_from_user(&linkinfo, (void*)arg, sizeof(LINK_CHANGE_INFO)))
		{
			printk("ERROR: NETMON: Get PHY Link state failed\n");
			return -EFAULT;
		}

		linkinfo.linkstate = GetInterfaceStatus (linkinfo.ifname, PHY_STATUS);
		if (__copy_to_user((void *)arg, (void*)&linkinfo, sizeof(LINK_CHANGE_INFO)))
		{
			printk("ERROR: NETMON: Get PHY Link state failed\n");
			return -EFAULT;
		}
		break;

	case NETMON_WAIT_FOR_PHY_LINK_CHANGE:
#if defined(NETDEV_PHY_LINK_DOWN) || defined(NETDEV_PHY_LINK_UP)
		gpio_waitforphyint (&linkinfo);
		if (__copy_to_user((void *)arg, (void*)&linkinfo, sizeof(linkinfo)))
		{
			printk("ERROR: NETMON: Wait for Link Change Failed\n");
			return -EFAULT;
		}
#else
		return(-EINVAL);
#endif
		break;

                default:
                        printk("ERROR: Netmon: Unknown ioctl\n");
                        return(-EINVAL);
        }

	return 0;
}

#ifndef USE_UNLOCKED_IOCTL 
static int     
NetmonIoctl(struct inode * inode, struct file * file, unsigned int cmd,unsigned long arg)
{
	return NetmonIoctlUnlocked(file,cmd,arg);
}
#endif

#if 0
static
ssize_t
NetmonRead(struct file * file , char * buf, size_t count, loff_t *ppos)
{
	ifname_list *iface;
	unsigned long flags;

	spin_lock_irqsave(&if_list_lock, flags);
	printk("Number of Interfaces = %d\n",if_count);

 	list_for_each_entry (iface, &if_list, list)
        {
		printk("Interface : [%s]\n",iface->name);
        }

	spin_unlock_irqrestore (&if_list_lock, flags);
	return (-EIO);
}
#endif

static unsigned int
NetmonPoll(struct file* file, poll_table *wait)
{
    unsigned int mask = 0;

#if defined(NETDEV_PHY_LINK_DOWN) || defined(NETDEV_PHY_LINK_UP)
    poll_wait(file, &phy_link_event, wait);

    //we also have to put the conditions here that help us determine if there is a interrupt recd
    if(front_phy_link_ints != NULL)
    {
        mask |= POLLPRI;
    }
#endif

    return mask;

}



static struct notifier_block netmon_netdev_notifier = 
{
	.notifier_call = netmon_netdev_event,
};

struct file_operations netmon_fops =
{
#ifdef USE_UNLOCKED_IOCTL 
       	.unlocked_ioctl  =     NetmonIoctlUnlocked,
#else
       	.ioctl  	=      NetmonIoctl, 
#endif
       .poll            =   NetmonPoll,
//	.read	=	NetmonRead,
};



static int
init_netmon_module(void)
{
	printk("Network Interface Monitor Version %d.%d.%d\n",PKG_MAJOR,PKG_MINOR,PKG_AUX);
	printk("Copyright (c) 2009-2015 American Megatrends Inc.\n");
        if (register_chrdev(NETMON_MAJOR, "netmon",  &netmon_fops) < 0)
        {
                printk("ERROR: Unable to register Netmon driver\n");
                return -EBUSY;
        }

	atomic_set(&net_event,0);
	atomic_set(&listener_count,0);
	register_netdevice_notifier(&netmon_netdev_notifier);

	return 0;
}


static void
exit_netmon_module(void)
{
	unregister_netdevice_notifier(&netmon_netdev_notifier);
        unregister_chrdev(NETMON_MAJOR,"netmon");
	return;
}

module_init (init_netmon_module);
module_exit (exit_netmon_module);

MODULE_AUTHOR("Samvinesh Christopher- American Megatrends Inc");
MODULE_DESCRIPTION("Network Interface Monitor");
MODULE_LICENSE("GPL");

#endif

