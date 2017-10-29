NODES=/sys/devices/system/node
NUM_SOCK=0; 
for F in `ls -d $NODES/node*`; do 
	for i in $(cat $F/cpulist | sed 's/,/ /g'); do 
		NUM_SOCK=$((NUM_SOCK+1)); 
	done; 
done;
echo $NUM_SOCK
