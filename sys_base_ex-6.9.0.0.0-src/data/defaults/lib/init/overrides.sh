
if [ -f /lib/init/overrides_file ]
then
        export LD_LIBRARY_PATH="/usr/local/lib/overrides/"
        export LD_PRELOAD=`cat /lib/init/overrides_file`
fi

