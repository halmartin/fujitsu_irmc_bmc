#!/bin/busybox ash

###########################
# modify rsyslog template #
###########################

templtyp=${1:-"def"}
conf=/etc/rsyslog.conf

# def # default template
default='\$ActionFileDefaultTemplate RSYSLOG_TraditionalFileFormat'
# num # template with numeric severity
numeric='\$template TraditionalFormatWithSeverity, \"%timegenerated% %HOSTNAME% <%syslogseverity%> %syslogtag%%msg:::drop-last-lf%\\n\"'
numact='\$ActionFileDefaultTemplate TraditionalFormatWithSeverity'
# txt # template with textual severity
text='\$template TraditionalFormatWithSeverityText, \"%timegenerated% %HOSTNAME% <%syslogseverity-text%> %syslogtag%%msg:::drop-last-lf%\\n\"'
txtact='\$ActionFileDefaultTemplate TraditionalFormatWithSeverityText'

case $templtyp in
	0|def)
		sed -i -e '/^\$template/d' -e "s/^\$ActionFileDefaultTemplate.*$/$default/" $conf
		;;
	1|num)
		sed -i -e '/^\$template/d' -e "/^\$ActionFileDefaultTemplate.*$/i$numeric" \
		       -e "s/^\$ActionFileDefaultTemplate.*$/$numact/" $conf
		;;
	2|txt)
		sed -i -e '/^\$template/d' -e "/^\$ActionFileDefaultTemplate.*$/i$text" \
		       -e "s/^\$ActionFileDefaultTemplate.*$/$txtact/" $conf
		;;
	*)
		echo "Usage: ${0##*/} {[0|def] | [1|num] | [2|txt]}"
		exit 1
		;;
esac

exit 0