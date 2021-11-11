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
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <net/net_namespace.h>
#include <linux/fs.h>
#include "helper.h"
#include "ncsi.h"

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
static int proc_setlink(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);  
static int proc_enable(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);  
static int proc_setvlanid(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);
static int proc_vetobit(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);
static int proc_usercommand(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);
static int proc_userresponse(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos);
static int proc_detect(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);
static int proc_setflowcontrol(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,
        					loff_t* pos);
static int proc_initcomplete(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,
        					loff_t* pos);
static int proc_channellist(struct ctl_table *ctl,int write,void *buffer,size_t *lenp,loff_t* pos);
#else
static int proc_setlink(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
							loff_t* pos);  
static int proc_enable(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
							loff_t* pos);  
static int proc_setvlanid(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
							loff_t* pos);
static int proc_vetobit(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
							loff_t* pos);
static int proc_usercommand(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
							loff_t* pos);
static int proc_userresponse(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
							loff_t* pos);
static int proc_detect(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
                            loff_t* pos);  
static int proc_setflowcontrol(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
        					loff_t* pos);
static int proc_initcomplete(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp,
        					loff_t* pos);
#endif

#if 0
static ssize_t proc_read( struct file *file, char __user *buffer, size_t len, loff_t *offset);
static ssize_t proc_write( struct file *file, const char __user *buffer, size_t len, loff_t *offset);

static struct proc_dir_entry   *proc_ncsi   = NULL;
static struct proc_dir_entry   *proc_ncsi_stats = NULL;
#endif
static struct ctl_table_header *sys_ncsi  = NULL;

#ifdef NCSI_DEBUG
int verbose = 0xFF; //SHOW_MESSAGES|SHOW_SUCCESS_COMMANDS;		// SHOW_MESSAGES|DUMP_BUFFER
#else
int verbose = 0;
#endif
int UserPackageID = 0;
int UserChannelID = 0;
char UserInterface[8] = CONFIG_SPX_FEATURE_NCSI_DEFAULT_INTERFACE;
int UserEnable = 0;
int UserVlanID=0;
int UserFlowcontrol=0;
int UserSpeed=0;
int UserDuplex=1;
int UserSetLink=0;
int UserAutoNeg=0;
int UserDetect=0;
int InitComplete=1;
#ifdef CONFIG_SPX_FEATURE_NCSI_RESET_INTERFACE_IN_USER_SETTINGS
int UserInitEnabled=0;
#endif

#ifdef CONFIG_SPX_FEATURE_NCSI_KEEP_PHY_LINK_UP
int UserVetoBit=1;
#else
int UserVetoBit=0;
#endif
int UserControlVetoBit=0;

#ifdef CONFIG_SPX_FEATURE_NCSI_AUTO_FAILOVER
int UserAuto=1;
#else
int UserAuto=0;
#endif
int EnableSetLink = 1;

unsigned int UserCommand[NCSI_COMMAND_SIZE] = {0};
unsigned int UserLastResponse[NCSI_COMMAND_SIZE] = {0};

UINT32 ChannelList[MAX_PACKAGE_ID * MAX_CHANNEL_ID * 2] = {0};

static struct ctl_table SysNcsiTable[] = 
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,11))
    {"DebugLevel", &verbose,       sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"PackageID",  &UserPackageID, sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"ChannelID",  &UserChannelID, sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"Interface",  &UserInterface, sizeof(UserInterface), 0644, NULL, &proc_dostring, NULL },
    {"Enable"   ,  &UserEnable,    sizeof(int), 0644, NULL, &proc_enable, NULL },
    {"VlanID"   ,  &UserVlanID,    sizeof(int), 0644, NULL, &proc_setvlanid, NULL },
    {"UserSpeed"    ,  &UserSpeed,    sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"UserDuplex"   ,  &UserDuplex,   sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"UserAutoNeg"   ,  &UserAutoNeg,   sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"UserSetLink"  ,  &UserSetLink,   sizeof(int), 0644, NULL, &proc_setlink, NULL },
    {"EnableSetLink"   ,  &EnableSetLink,   sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"FailOver",  &UserAuto, sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"VetoBit",  &UserVetoBit, sizeof(int), 0644, NULL, &proc_dointvec, NULL },
    {"ControlVetoBit"   ,  &UserControlVetoBit,    sizeof(int), 0644, NULL, &proc_vetobit, NULL },
    {"UserCommand"   ,  UserCommand,    NCSI_COMMAND_SIZE, 0644, NULL, &proc_usercommand, NULL },
    {"UserLastResponse"   ,  UserLastResponse,    NCSI_COMMAND_SIZE, 0644, NULL, &proc_userresponse, NULL },
    {"Detect"   ,  &UserDetect,    sizeof(int), 0644, NULL, &proc_detect, NULL },
    {"Flowcontrol", &UserFlowcontrol, sizeof(int), 0644, NULL, &proc_setflowcontrol, NULL},
    {"InitComplete"   ,  &InitComplete,   sizeof(int), 0444, NULL, &proc_initcomplete, NULL },
#ifdef CONFIG_SPX_FEATURE_NCSI_RESET_INTERFACE_IN_USER_SETTINGS
    {"UserInitEnabled"   ,  &UserInitEnabled,   sizeof(int), 0644, NULL, &proc_dointvec, NULL },
#endif
    {"ChannelList"   ,  ChannelList,    sizeof(ChannelList), 0644, NULL, &proc_channellist, NULL },
    { 0 }
#else	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
	{"DebugLevel", &verbose,       sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{"PackageID",  &UserPackageID, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{"ChannelID",  &UserChannelID, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{"Interface",  &UserInterface, sizeof(UserInterface), 0644, NULL, NULL, &proc_dostring },
	{"Enable"   ,  &UserEnable,    sizeof(int), 0644, NULL, NULL, &proc_enable },	
	{"VlanID"	,  &UserVlanID,    sizeof(int), 0644, NULL, NULL,&proc_setvlanid },
	{"UserSpeed"	,  &UserSpeed,    sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{"UserDuplex"	,  &UserDuplex,   sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{"UserAutoNeg"   ,  &UserAutoNeg,   sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{"UserSetLink"	,  &UserSetLink,   sizeof(int), 0644, NULL, NULL,&proc_setlink },
	{"EnableSetLink"   ,  &EnableSetLink,   sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{"FailOver",  &UserAuto, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{"VetoBit",  &UserVetoBit, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{"ControlVetoBit"   ,  &UserControlVetoBit,    sizeof(int), 0644, NULL, NULL, &proc_usercontrolvetobit },
	{"UserCommand"   ,  UserCommand,    NCSI_COMMAND_SIZE, 0644, NULL, NULL, &proc_usercommand },
	{"UserLastResponse"   ,  UserLastResponse,    NCSI_COMMAND_SIZE, 0644, NULL, NULL, &proc_userresponse },
	{"Detect"   ,  &UserDetect,    sizeof(int), 0644, NULL, NULL, &proc_detect },
	{"Flowcontrol", &UserFlowcontrol, sizeof(int), 0644, NULL, NULL, &proc_setflowcontrol},
	{"InitComplete"   ,  &InitComplete,    sizeof(int), 0444, NULL, NULL, &proc_initcomplete },
#ifdef CONFIG_SPX_FEATURE_NCSI_RESET_INTERFACE_IN_USER_SETTINGS
	{"UserInitEnabled"   ,  &UserInitEnabled,    sizeof(int), 0644, NULL, NULL, &proc_dointvec },
#endif
	{ 0 }
#else
	{CTL_UNNUMBERED, "DebugLevel", &verbose,       sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{CTL_UNNUMBERED, "PackageID",  &UserPackageID, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{CTL_UNNUMBERED, "ChannelID",  &UserChannelID, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{CTL_UNNUMBERED, "Interface",  &UserInterface, sizeof(UserInterface), 0644, NULL, NULL, &proc_dostring },
	{CTL_UNNUMBERED, "Enable"   ,  &UserEnable,    sizeof(int), 0644, NULL, NULL, &proc_enable },	
	{CTL_UNNUMBERED, "VlanID"	,  &UserVlanID,    sizeof(int), 0644, NULL, NULL,&proc_setvlanid },
	{CTL_UNNUMBERED, "UserSpeed"	,  &UserSpeed,    sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{CTL_UNNUMBERED, "UserDuplex"	,  &UserDuplex,   sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{CTL_UNNUMBERED, "UserAutoNeg"   ,  &UserAutoNeg,   sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{CTL_UNNUMBERED, "UserSetLink"	,  &UserSetLink,   sizeof(int), 0644, NULL, NULL,&proc_setlink },
	{CTL_UNNUMBERED, "EnableSetLink"   ,  &EnableSetLink,   sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{CTL_UNNUMBERED, "FailOver",  &UserAuto, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
	{CTL_UNNUMBERED, "VetoBit"	,  &UserVetoBit,   sizeof(int), 0644, NULL, NULL,&proc_dointvec },
	{CTL_UNNUMBERED, "ControlVetoBit"   ,  &UserControlVetoBit,    sizeof(int), 0644, NULL, NULL, &proc_vetobit },
	{CTL_UNNUMBERED, "UserCommand"   ,  UserCommand,    NCSI_COMMAND_SIZE, 0644, NULL, NULL, &proc_usercommand },
	{CTL_UNNUMBERED, "UserLastResponse"   ,  UserLastResponse,    NCSI_COMMAND_SIZE, 0644, NULL, NULL, &proc_userresponse },
	{CTL_UNNUMBERED, "Detect"   ,  &UserDetect,    sizeof(int), 0644, NULL, NULL, &proc_detect },
	{CTL_UNNUMBERED, "Flowcontrol",  &UserFlowcontrol,  sizeof(int), 0644, NULL, NULL, &proc_setflowcontrol},
	{CTL_UNNUMBERED, "InitComplete",  &InitComplete, sizeof(int), 0444, NULL, NULL, &proc_initcomplete },
#ifdef CONFIG_SPX_FEATURE_NCSI_RESET_INTERFACE_IN_USER_SETTINGS
	{CTL_UNNUMBERED, "UserInitEnabled",  &UserInitEnabled, sizeof(int), 0644, NULL, NULL, &proc_dointvec },
#endif
	{ 0 }
#endif
#endif
};

/* 
 * This is a function Called when something is written to sysctl "UserCommand"
 * Note : First byte is the command number and followed by number of data bytes(n) and n bytes of data.
 * */
static
int 
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_usercommand(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else
proc_usercommand(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	int retval = 0;
	
	if (UserCommand[1] != 0)		// Already given command is in progress.
		return retval;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))	
	retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
	retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif
	
	UserCommand[1]++;	// Increment for command.
	SendUserCommand();
 	return retval;
} 


/* 
 * This is a function Called when something is written to sysctl "UserCommand"
 * Note : First byte is the length(n) and followed by n bytes for command.
 * */
static
int 
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_userresponse(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else
proc_userresponse(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	static char read_flag = 0;
	int retval = 0, length = *lenp, i,ret;
	
	*lenp = 0;
	if (write)
		return retval;

	if (read_flag)
	{
		read_flag = 0;
		memset (UserLastResponse, 0, NCSI_COMMAND_SIZE);
		return retval;
	}

	if (UserLastResponse[1])
	{
		(*lenp) = snprintf (buffer, length, "Command : %02X, Response Length : %02X\n", UserLastResponse[0], UserLastResponse[1]);
		read_flag = 1;
	}
	for (i = 2; i < UserLastResponse[1] + 2; i++)
	{
		ret = 0;
		if (((i - 1) % 4) == 0)
		{
			ret = snprintf (&((char *)buffer)[(*lenp)], length, "%02X\n", UserLastResponse[i]);
			if (ret >= length)
			{
				(*lenp) += length;
			}
			else
			{
				(*lenp) += ret;
			}
		}
		else
		{
			ret = snprintf (&((char *)buffer)[(*lenp)], length, "%02X ", UserLastResponse[i]);
			if (ret >= length)
			{
				(*lenp) += length;
			}
			else
			{
				(*lenp) += ret;
			}
		}
	}
	if (((i - 2) % 4) != 0)
		 snprintf (&((char *)buffer)[(*lenp)], length, "\n");
	
 	return retval;
} 

/* This is a function Called when something is written to sysctl "Enable " */
static
int 
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_enable(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else
proc_enable(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	int retval;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))	
	retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
	retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif

	if (UserEnable == 0)
		return retval;
	
	if (UserEnable != 1)
	{
		printk(KERN_DEBUG "NCSI: Sysctl \"Enable\" should be 1 or 0 \n");
		return retval;
	}
	
	if( UserAuto != 0 && UserAuto != 1 )
	{
		printk(KERN_DEBUG "NCSI: Sysctl \"Auto\" should be 1 or 0 \n");
		return retval;
	}
	
	/* UserEnable == 1 */
	SetUserSettings(UserVlanID);
 	return retval;
} 

static
int 
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_setlink(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else
proc_setlink(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	int retval = -1;
	if (EnableSetLink != 0)
	{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))	
		retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
		retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif	

#ifdef CONFIG_SPX_FEATURE_NCSI_FORCE_LAN_SPEED_10G
		printk(KERN_DEBUG "NCSI: Sysctl \"SetLink\" is not supported on 10G mode\n");
		return retval;
#else

		if (UserSetLink == 0)
			return retval;
	
		if (UserSetLink != 1)
		{
			printk(KERN_DEBUG "NCSI: Sysctl \"SetLink\" should be 1 or 0 \n");
			return retval;
		}

		if (UserSpeed != 0)			/* 0 = Enable AutoNeg */
		{
			if ((UserDuplex != 0) && (UserDuplex != 1))
			{
				printk(KERN_DEBUG "NCSI: Sysctl \"Duplex\" should be 1 (Full) or 0  (Half)\n");
				return retval;
			}

			if ((UserSpeed != 10) && (UserSpeed != 100) && ( UserSpeed != 1000))
			{
				printk(KERN_DEBUG "NCSI: Sysctl \"Speed\" should be 10/100/1000 Mbps\n");
				return retval;
			}
		
			if ((UserAutoNeg != 0) && (UserAutoNeg != 1))
			{
				printk(KERN_DEBUG "NCSI: Sysctl \"AutoNeg\" should be 1 (Enable) or 0  (Disable)\n");
				return retval;
			}
		}

		SetUserLink();
#endif			
	}
 	return retval;

} 

/* This is a function Called when something is written to sysctl "Detect " */
static
int 
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_detect(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else
proc_detect(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
    int retval;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))  
    retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
    retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif

    if (UserDetect == 0)
        return retval;
    
    if (UserDetect != 1)
    {
        printk("NCSI: Sysctl \"Detect\" should be 1 or 0 \n");
        return retval;
    }

#ifdef CONFIG_SPX_FEATURE_NCSI_MANUAL_DETECT
    /* UserDetect == 1 */
    SetUserDetect();
#endif
    return retval;
} 


/* This is a function Called when something is written to sysctl "VlanID " */
static
int
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_setvlanid(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else
proc_setvlanid(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	int retval;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))	
	retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
	retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif	

	/* Set VLAN Filter , Enable VLAN */
	SetUserSettings(UserVlanID);
 	return retval;
}

#if 0
static struct file_operations proc_ops = 
{
	.read = proc_read,
	.write = proc_write
};
#endif


/* This is a function Called when something is written to sysctl "Detect " */
static
int 
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_vetobit(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else
proc_vetobit(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	int retval;
#if (LINUX_VERSION_CODE >KERNEL_VERSION(3,4,11))	
	retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
	retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif

	if (UserControlVetoBit== 0)
		return retval;
	
	if (UserControlVetoBit!= 1)
	{
		printk(KERN_DEBUG "NCSI: Sysctl \"ControlVetoBit\" should be 1 or 0 \n");
		return retval;
	}
	
/*	if( UserAuto != 0 && UserAuto != 1 )
	{
		printk("NCSI: Sysctl \"Auto\" should be 1 or 0 \n");
		return retval;
	}
*/	
	/* UserControlVetoBit== 1 */
	SetUserVetoBit();
 	return retval;
} 

/* This is a function Called when something is written to sysctl "Flow Control "*/
static
int
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_setflowcontrol(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else 
proc_setflowcontrol(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	int retval;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))	
	retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
	retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif
	if ((UserFlowcontrol < 0) || (UserFlowcontrol > 3))
	{
		printk("Invalid Flow Control value\n");
		return retval;
	}
#ifdef CONFIG_SPX_FEATURE_NCSI_FLOW_CONTROL
	Setflowcontrol(UserFlowcontrol);
#else
	printk("Setting Flow Control Feature is not Enabled\n");
#endif
	return 0;
	
}

/* This is a function Called when something is written to proc entry "InitComplete" */

static
int
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))
proc_initcomplete(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
#else 
proc_initcomplete(struct ctl_table *ctl,int write,struct file *filep,void *buffer,size_t *lenp, loff_t *pos)
#endif
{
	int retval;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,4,11))	
	retval = proc_dointvec(ctl,write,buffer,lenp,pos);
#else
	retval = proc_dointvec(ctl,write,filep,buffer,lenp,pos);
#endif
	return 0;
	
}

static
int
proc_channellist(struct ctl_table *ctl,int write,void *buffer,size_t *lenp, loff_t *pos)
{
	NCSI_IF_INFO *info = NULL;
	int i, j;

	if(write)
		return 0;

	info = GetInterfaceInfoByName(UserInterface);
	if(info == NULL)
	{
		printk("Can't find ncsi info of %s\n", UserInterface);
		*lenp = 0;
		return 0;
	}

	memset (ChannelList, 0, sizeof(ChannelList));

	ctl->maxlen = info->TotalChannels * 2 * sizeof(int);
        for(i = 0, j = 0; i < info->TotalChannels; i++)
	{
		ChannelList[j++] = info->ChannelInfo[i].PackageID;
		ChannelList[j++] = info->ChannelInfo[i].ChannelID;
	}

	return proc_dointvec(ctl,write,buffer,lenp,pos);
}

#if 0
static ssize_t 
proc_read( struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
/*
	loff_t pos = *offset;
	struct proc_data *priv = (struct proc_data*)file->private_data;

	if (!priv->rbuffer)
		return -EINVAL;

	if (pos < 0)
		return -EINVAL;
	if (pos >= priv->readlen)
		return 0;
	if (len > priv->readlen - pos)
		len = priv->readlen - pos;
	if (copy_to_user(buffer, priv->rbuffer + pos, len))
		return -EFAULT;
	*offset = pos + len;
	return len;
*/
	printk(KERN_DEBUG "NCSI Statistics: Read : UnImplemented \n");
	return 0;
	
}

static ssize_t 
proc_write( struct file *file, const char __user *buffer, size_t len , loff_t *offset)
{
	/*
	loff_t pos = *offset;
	struct proc_data *priv = (struct proc_data*)file->private_data;

	if (!priv->wbuffer)
		return -EINVAL;

	if (pos < 0)
		return -EINVAL;
	if (pos >= priv->maxwritelen)
		return 0;
	if (len > priv->maxwritelen - pos)
		len = priv->maxwritelen - pos;
	if (copy_from_user(priv->wbuffer + pos, buffer, len))
		return -EFAULT;
	if ( pos + len > priv->writelen )
		priv->writelen = len + file->f_pos;
	*offset = pos + len;
	return len;
	*/
	printk(KERN_DEBUG "NCSI Statistics: Write : UnImplemented \n");
	return -EINVAL;
}
#endif

int 
CreateProcSys(void)
{
#if 0
	proc_ncsi = proc_mkdir("ncsi",init_net.proc_net);
	if (proc_ncsi == NULL)
	{
		printk("NCSI: ERROR: Unable to /proc/net/ncsi \n");
		return 1;
	}
	proc_ncsi_stats =create_proc_entry("stats",0600,proc_ncsi);
	if (proc_ncsi_stats == NULL)
	{
		printk("NCSI: ERROR: Unable to /proc/net/ncsi/stats\n");
		remove_proc_entry("ncsi",init_net.proc_net);
		return 1;
	}
	proc_ncsi_stats->proc_fops = &proc_ops;
#endif
	
	sys_ncsi = AddSysctlTable("ncsi",&SysNcsiTable[0]);
	if (sys_ncsi == NULL)
	{
		printk("NCSI: ERROR: Unable to create /proc/sys/ractrends/ncsi \n");
#if 0
		if (proc_ncsi_stats)
			remove_proc_entry("stats",proc_ncsi);
		if (proc_ncsi)
			remove_proc_entry("ncsi",init_net.proc_net);
#endif
	}
	return 0;
}


int
RemoveProcSys(void)
{
	if (sys_ncsi)
		RemoveSysctlTable(sys_ncsi);
#if 0 
	if (proc_ncsi_stats)
		remove_proc_entry("stats",proc_ncsi);
	if (proc_ncsi)
		remove_proc_entry("ncsi",init_net.proc_net);
#endif
	return 0;
}

