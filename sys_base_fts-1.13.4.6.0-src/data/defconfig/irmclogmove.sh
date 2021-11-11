case "$1" in
  logbuffer)
	if [ -f /conf/fts/nvram/iRMClog/iRMC_logbuffer.txt.2.gz ]
	then
	    echo "Moving log buffer files to /var/logs ..."
		mv /var/log/iRMClog/logbuffer.txt.6.gz /var/log/iRMClog/iRMC_logbuffer.txt.7.gz > /dev/null 2>&1
		mv /var/log/iRMClog/logbuffer.txt.5.gz /var/log/iRMClog/iRMC_logbuffer.txt.6.gz > /dev/null 2>&1
		mv /var/log/iRMClog/logbuffer.txt.4.gz /var/log/iRMClog/iRMC_logbuffer.txt.5.gz > /dev/null 2>&1
		mv /var/log/iRMClog/logbuffer.txt.3.gz /var/log/iRMClog/iRMC_logbuffer.txt.4.gz > /dev/null 2>&1
		cp /conf/fts/nvram/iRMClog/iRMC_logbuffer.txt.2.gz /var/log/iRMClog/iRMC_logbuffer.txt.3.gz > /dev/null 2>&1
	fi
    ;;

  dmesg)
	if [ -f /conf/fts/nvram/iRMClog/dmesg.log.2.gz ]
	then
        echo "Moving dmesg log files to /var/logs ..."
		mv /var/log/iRMClog/dmesg.log.4.gz /var/log/iRMClog/dmesg.log.5.gz > /dev/null 2>&1
		mv /var/log/iRMClog/dmesg.log.3.gz /var/log/iRMClog/dmesg.log.4.gz > /dev/null 2>&1
		cp /conf/fts/nvram/iRMClog/dmesg.log.2.gz /var/log/iRMClog/dmesg.log.3.gz > /dev/null 2>&1
	fi
    ;;

  bmcperiodic)
	if [ -f /conf/fts/nvram/iRMClog/bmc_periodic.log.2.gz ]
	then
      	echo "Moving bmc periodic log files to /var/logs ..."
		mv /var/log/iRMClog/bmc_periodic.log.4.gz /var/log/iRMClog/bmc_periodic.log.5.gz > /dev/null 2>&1
		mv /var/log/iRMClog/bmc_periodic.log.3.gz /var/log/iRMClog/bmc_periodic.log.4.gz > /dev/null 2>&1
		cp /conf/fts/nvram/iRMClog/bmc_periodic.log.2.gz /var/log/iRMClog/bmc_periodic.log.3.gz > /dev/null 2>&1
	fi
    ;;

   *)
    echo "Usage: /etc/init.d/irmclogrotated {logbuffer | dmesg | bmcperiodic}"
    exit 1
esac

exit 0
