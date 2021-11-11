#!/bin/sh
### BEGIN INIT INFO
#Runlevel : S = S99
### END INIT INFO


FTS_FORCE_CIM_DISABLE="/conf/fts/force_cim_disable"

force_cim_disable()
{
while true
do
	/usr/local/bin/ipmiraw C8 FA 00
	if [ "$?" -eq 0 ]
	then
		touch $FTS_FORCE_CIM_DISABLE
		exit 0
	else
		sleep 2
	fi
done
exit 0
}

if [ ! -f $FTS_FORCE_CIM_DISABLE ]
then
	force_cim_disable &
fi
exit 0
