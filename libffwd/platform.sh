if cat /proc/cpuinfo | grep -q "Opteron" ; then 
	echo Opteron; 
else 
	if cat /proc/cpuinfo | grep -q "Xeon" ; then 
		echo XEON;
	else 
		echo NS;
	fi 
fi
