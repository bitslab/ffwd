#!/bin/bash
 
NUMBER_OF_RUNS=10
SIZE=1024
RANGE=`expr $SIZE \* 2`
MAX_SIZE=20000
UPDATE_RATIO=30
DURATION=15000 #in milliseconds

RESULTS_DIR="${RESULTS_DIR:-results}"
DIR="$RESULTS_DIR/lazylist_threads_bench"
LOG=$DIR/lazylist_threads_log.txt
BIN="../bin"

NPROC=`nproc`

ffwd_threads="2 4 8 16 32 48 56"
rcl_threads="2 4 8 16 32 48 56 63"
other_threads="2 4 8 16 32 48 56 64"
if [ $NPROC -eq 128 ]; then
  ffwd_threads="2 4 8 16 32 48 64 96 120"
  rcl_threads="2 4 8 16 32 48 64 96 120 127"
  other_threads="2 4 8 16 32 48 64 96 120 128"
fi

function list_threads_exp {

  if [ "$1" = "ffwd" ]; then
    benchs=$ffwd_threads
  elif [ "$1" = "rcl" ]; then
    benchs=$rcl_threads
  else
    benchs=$other_threads
  fi

  EXECUTABLE="$BIN/$1-lazylist"
	echo "Experiment with" $1 >> $LOG
  if [ ! -f $EXECUTABLE ]; then
    echo "$EXECUTABLE not found. Aborting."
  fi

#size=$START;
  shift 3
#while [ $size -le $MAX_SIZE ]; 
	for t in $benchs
  do
	    echo -n $t
        echo -n ","
        echo -n "scale=6;(" > sum
        echo -n > list
	    echo "sudo $EXECUTABLE -t $t -d $DURATION -i $SIZE -r $RANGE -u $UPDATE_RATIO" >> $LOG
	    #echo "sudo /usr/local/bin/jemalloc.sh $EXECUTABLE -t $t -d $DURATION -i $SIZE -r $RANGE -u $UPDATE_RATIO" >> $LOG
        for j in `seq 1 $NUMBER_OF_RUNS`
        do
            sudo $EXECUTABLE -t $t -d $DURATION -i $SIZE -r $RANGE -u $UPDATE_RATIO | tr -d ',\n' | tee -a sum list >> /dev/null
            #sudo /usr/local/bin/jemalloc.sh $EXECUTABLE -t $t -d $DURATION -i $SIZE -r $RANGE -u $UPDATE_RATIO | tr -d ',\n' | tee -a sum list >> /dev/null
            if [ $j -lt $NUMBER_OF_RUNS ]; then
                echo -n '+' >> sum
                echo -n ',' >> list
            fi
        done
   	    echo ")/$NUMBER_OF_RUNS" >> sum
        cat sum | bc | tr -d '\n' | tee -a $LOG
        echo
#size=`expr $size \* 2`
		echo -n "<--" >> $LOG
		cat list >> $LOG
		echo "" >> $LOG
	done
#rm -f $EXECUTABLE 
	rm -f sum list
}

types="ffwd rcl fc mutex mcs ticket hticket ttas tas clh"

mkdir -p $DIR
if [ $# -eq 0 ]; then
	for t in $types; do
		EXPERIMENT_NAME="$DIR/$t"
    header=`echo $t | tr '[:lower:]' '[:upper:]'`
		echo "\"$header\"" > $EXPERIMENT_NAME.csv
		list_threads_exp $t | tee -a $EXPERIMENT_NAME.csv
	done
else
	for t in "$@"; do
	  EXPERIMENT_NAME="$DIR/$t"
    header=`echo $t | tr '[:lower:]' '[:upper:]'`
	  echo "\"$header\"" > $EXPERIMENT_NAME.csv
	  list_threads_exp $t | tee -a $EXPERIMENT_NAME.csv
  done
fi
rm -f sum list
