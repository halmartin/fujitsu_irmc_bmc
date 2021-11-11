#!/bin/sh
#
#Runlevel : 3 = S98
if [ -e /etc/crontab ] && [ -e /conf/crontab ]
then
	MONITOR_ENTRY="*/2 * * * * sysadmin [ 0 -eq \$(ps -ef | grep /etc/init.d/ipmi_monitor.sh | grep -vc grep) ] && /etc/init.d/ipmi_monitor.sh"
	MONITOR_FOUND=$(grep "ipmi_monitor.sh" /conf/crontab)
	if [ $? != 0 ]
	then 
		# no crontab entry found for monitoring
		echo "$MONITOR_ENTRY" >> /conf/crontab
	else
		# crontab entry found for monitoring
		if [ "$MONITOR_FOUND" != "$MONITOR_ENTRY" ]
		then
			# monitor string has not the expected value -> exchange it
			sed -e "/ipmi_monitor.sh/d" /conf/crontab > /tmp/crontab
			echo "$MONITOR_ENTRY" >> /tmp/crontab
			mv /tmp/crontab /conf/crontab
		fi
	fi
fi

