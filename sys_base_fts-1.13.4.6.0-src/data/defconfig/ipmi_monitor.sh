#!/bin/sh

#This script checks if ipmi is working properly and if not then try 3 times,even then if it fails kill ipmimain

SCRIPT_IS_STARTED_FILE=/tmp/ipmi_monitor_started
CHECK_OUTPUT_FILENAME=/tmp/ipmi_monitor_check
IPMI_MONITOR_RESTART_FILENAME=/tmp/ipmi_monitor_restart

echo "PID: $$" > ${SCRIPT_IS_STARTED_FILE}

#Check if an ipmi_monitor script is already running

count=0;
while [ $count -le 2 ]
do
	#If IPMIMain hungs then ipmiraw will not come back
	echo "" > ${CHECK_OUTPUT_FILENAME}
	ipmiraw 18 01 > ${CHECK_OUTPUT_FILENAME} &
	IPMIRAW_PID=$!

	sleep 10
	CHECK_OUTPUT_FILENSIZE=$(stat -c%s "${CHECK_OUTPUT_FILENAME}")

	USED_VAR_SPACE=$(df | grep /var | sed -e "s/^.* \([^ ]*\)\%.*/\1/")
	if [ ${USED_VAR_SPACE} -gt 99 ]
	then
		#If there is no space left in /var the result of ipmiraw 18 01 can't be stored. Then do not kill IPMIMain.
		exit
	fi
	
	if [ ${CHECK_OUTPUT_FILENSIZE} -gt 60 ]
	then
		echo "IPMIMain ok" >> ${SCRIPT_IS_STARTED_FILE}
		exit
	fi
	
	echo "IPMIMain do not respond" >> ${SCRIPT_IS_STARTED_FILE}
	/usr/local/bin/FTS_CoreDump -w >/dev/null 2>&1
	sleep 20
	
	ps -p ${IPMIRAW_PID} > /dev/null
	RESULT=$?
	if [ "${RESULT}" -eq "0" ]
	then
	    touch ${IPMI_MONITOR_RESTART_FILENAME}
		kill -9 ${IPMIRAW_PID}
	fi
	count=$((count+1))

done
/usr/local/bin/FTS_CoreDump -w >/dev/null 2>&1
echo "Max retry count reached. Killing IPMIMain" >> ${SCRIPT_IS_STARTED_FILE}
touch ${IPMI_MONITOR_RESTART_FILENAME}
killall -9 IPMIMain
