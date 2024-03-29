#! /bin/sh
### BEGIN INIT INFO
# Provides:          mountall
# Required-Start:    checkfs
# Required-Stop: 
# Default-Start:     S
# Default-Stop:
# Short-Description: Mount all filesystems.
# Description:
### END INIT INFO

PATH=/sbin:/bin:/usr/bin:/usr/sbin

. /lib/init/vars.sh

. /lib/lsb/init-functions
. /lib/init/mount-functions.sh

LD_LIB_PATH=$(ls /lib/ld-*.so)
FLASH_ERASE="$LD_LIB_PATH /usr/sbin/flash_erase"
MKFSJFFS2="$LD_LIB_PATH /usr/sbin/mkfs.jffs2"

CONF_DIR="conf"
TMP_DIR="/var/tmp"
DEFCONF_DIR="/etc/defconfig"
BKUPCONF_DIR="bkupconf"

# for ntfs-3g to get correct file name encoding
if [ -r /etc/default/locale ]; then
	. /etc/default/locale
	export LANG
fi

do_start() {
	#
	# Mount local file systems in /etc/fstab.
	#
	mount_all_local() {
		if [ -r /proc/cmdline ] && grep -q 'root=\/dev\/nfs' /proc/cmdline 
		then
			# Booting from NFS. mount all except mtd flash filesystems
 	    		mount -a -t sysfs,proc,devpts,nfs4,smbfs,cifs,ncp,ncpfs,coda,ocfs2,gfs,gfs2,nocramfs 2>/dev/null || true

		else
			#Unmount the old root automatically created by init
			#Ractrends 4.0 Busybox mount does not support -O option
			#	    mount -a -t nonfs,nfs4,smbfs,cifs,ncp,ncpfs,coda,ocfs2,gfs,gfs2 \
			#		-O no_netdev
			#Ractrends 4.0 : Mount return non-zero exit code even on success
 	    		mount -a -t nonfs,nfs4,smbfs,cifs,ncp,ncpfs,coda,ocfs2,gfs,gfs2 2>/dev/null || true
		fi
	}
		
	#
	# function get_mtdinfo, 
	# require MTDNUM as the parameter#1
	# it will get mtd size and mtd erase size from /proc/mtd,
	# and calculate the erase block number  
	#
	get_mtdinfo() {

        MTDNUM=$1
        MTDSIZE=0x$(cat /proc/mtd | grep "mtd$MTDNUM" | awk '{ printf $2 }')
        MTDERASESIZE=0x$(cat /proc/mtd | grep "mtd$MTDNUM" | awk '{ printf $3 }')
        
        #calculate block number, it must convert HEX to DEC before div operation
        BLOCKNUM=`expr $(($MTDSIZE)) / $(($MTDERASESIZE))`
	    
	}
	
	check_mountconf()
	{
		if [ -f /conf/AMI ];then
			echo "file exists and conf mounted correctly"
		else		
			#copy all files except FTS folder
			echo "...AMI file not present, so copying all files except FTS/ to /conf..."
			WKDIR=`pwd`
			cd $DEFCONF_DIR
			cp -rf `ls -A | grep -vE "FTS|../../conf/"` ../../conf/
			cd $WKDIR
			touch /conf/AMI	
		fi		
		if [ -f /conf/FTS ];then
			echo "FTS file exists and conf mounted correctly"		
		else
			#copy only FTS folder from /etc/defconfig		
			echo "FTS file not present, so copying FTS/ again to /conf..."
	       		cp /$DEFCONF_DIR/FTS/ /$CONF_DIR/ -rf
			touch /conf/FTS
		fi 
	}	
	#
	# Function do_bkupfiles
	# it copy all file in list to /var/tmp folder before restore to default settings
	#
	do_bkupfiles() {
		cp /$CONF_DIR/preservefiles_list $TMP_DIR/preservefiles_list
		for file in `cat /$CONF_DIR/preservefiles_list`
		do
			cp $file $TMP_DIR/
		done
	}
	#
	# Function do_restorefiles
	# it will restore all file in list to /conf folder after restore to default settings from /var/tmp
	#
	do_restorefiles(){
		for file in `cat $TMP_DIR/preservefiles_list`
		do
			dir_name=`echo $file  | sed "s,/[^/]*$,,"`
			file_name=`echo $file | sed "s,/.*/,,"`
			mv $TMP_DIR/$file_name $dir_name
		done
	}
	#
	# function mountconf,
	# it will check if conf and bkupconf were mounted successfully.
	# And it will invoke mount_conf if one of them mount failed.
	#
	mountconf() {
	    sleep 1
       	    NUM1=`cat /proc/mtd | grep \"$CONF_DIR\" | awk -F: '{ printf $1 }' | sed 's/mtd//g'`

                if [ $NUM1 == 0 ]; then
                    echo "Only one SPI chip available, not mounting conf"
                    return
                fi

		mount /dev/mtdblock$NUM1 /$CONF_DIR -t jffs2
		if [ $? == 0 ] && [ ! -f /conf/restorefactory ]; then
			echo "checking the mounted conf.."   
			#cp /$DEFCONF_DIR/* /$CONF_DIR/ -rf
			check_mountconf
		else
			if [ -f /conf/restorefactory ];then
				echo "Restoring Factory Defaults..."
				if [ -f /$CONF_DIR/preservefiles_list ];then
					do_bkupfiles
				fi
				umount /$CONF_DIR
			else
				echo "mounting conf failed...recovering the conf partition"
			fi
			get_mtdinfo $NUM1
			$FLASH_ERASE /dev/mtd$NUM1 0 $BLOCKNUM
			echo "mounting /conf after erasing..."
			mount /dev/mtdblock$NUM1 /$CONF_DIR -t jffs2
			if [ $? != 0 ]; then
				echo "mounting conf failed"
				return
			fi
			echo "Done."
			echo "mounting done..copying defconf files.."
       			cp /$DEFCONF_DIR/* /$CONF_DIR/ -rf
			touch /conf/AMI		
			touch /conf/FTS
			if [ -f $TMP_DIR/preservefiles_list ];then
				do_restorefiles
			fi
		fi      
	}


	pre_mountall

	if [ "$VERBOSE" = no ]
	then
		log_action_begin_msg "Mounting local filesystems"
		mount_all_local	
		log_action_end_msg $?
	else
#Ractrends 4.0 : Display same messages irespective of the value of VERBOSE.
#		log_daemon_msg "Will now mount local filesystems"
		log_daemon_msg "Mounting local filesystems"	
		mount_all_local
		log_end_msg $?
	fi
	
	echo "Mounting conf.."
	mountconf
	
	post_mountall

	case "$(uname -s)" in
	  *FreeBSD)
		INITCTL=/etc/.initctl
		;;
	  *)
		INITCTL=/dev/initctl
		;;
	esac

	#
	# We might have mounted something over /dev, see if
	# /dev/initctl is there.
	#
	if [ ! -p $INITCTL ]
	then
		rm -f $INITCTL
		mknod -m 600 $INITCTL p
	fi
	kill -USR1 1

	#
	# Execute swapon command again, in case we want to swap to
	# a file on a now mounted filesystem.
	#
	# Ignore 255 status due to swap already being enabled
	#
	if [ "$NOSWAP" = yes ]
	then
#Ractrends 4.0 does not support swap. Remove this warning message
#		[ "$VERBOSE" = no ] || log_warning_msg "Not activating swap as
#requested via bootoption noswap."
		echo -n 
	else
		if [ "$VERBOSE" = no ]
		then
			log_action_begin_msg "Activating swapfile swap"
			swapon -a -e 2>/dev/null || :  # Stifle "Device or resource busy"
			log_action_end_msg 0
		else
			log_daemon_msg "Will now activate swapfile swap"
			swapon -a -e -v
			log_action_end_msg $?
		fi
	fi
}

case "$1" in
  start|"")
	do_start
	;;
  restart|reload|force-reload)
	echo "Error: argument '$1' not supported" >&2
	exit 3
	;;
  stop)
	# No-op
	;;
  *)
	echo "Usage: mountall.sh [start|stop]" >&2
	exit 3
	;;
esac

:
