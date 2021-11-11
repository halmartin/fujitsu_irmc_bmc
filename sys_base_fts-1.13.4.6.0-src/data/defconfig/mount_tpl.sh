#!/bin/sh
# /etc/init.d/mount_tpl.sh: Mount squash TPL and link to /usr/local/www/ThirdPartyLicenseReadme.txt
#
#Runlevel : S = S38


TPL_TX="/usr/local/www/ThirdPartyLicenseReadme.txt"
TPL_SQ="/usr/local/www/ThirdPartyLicenseReadme.squashfs"
MNT_SQ="/mnt/squashfs"


if [ ! -f "$TPL_TX" ]
then
	echo "No $TPL_TX found. Looking for $TPL_SQ"
	if [ -f "$TPL_SQ" ]
	then
	    echo "$TPL_SQ found. Mounting the SquashFS and linking."
	    mount $TPL_SQ $MNT_SQ -t squashfs
	fi
fi
