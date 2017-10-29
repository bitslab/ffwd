#!/bin/bash
 
NUMBER_OF_RUNS=10
START=1
MAX_SIZE=2000
UPDATE_RATIO=30
LOAD=1
DURATION=15000 #in milliseconds

RESULTS_DIR="${RESULTS_DIR:-results}"
DIR="$RESULTS_DIR/hash_size_bench"
LOG=$DIR/hash_size_log.txt
BIN="../bin"

NPROC=`nproc`
NUM_OF_THREADS=$NPROC

function hash_size_exp {

  if [ "$1" = "ffwd" ]; then
    if [ $NPROC -eq 128 ]; then
      NUM_OF_THREADS=120
    elif [ $NPROC -eq 64 ]; then
      NUM_OF_THREADS=56
    else
      echo "Aborting."
      exit
    fi
  elif [ "$1" = "rcl" ]; then
    NUM_OF_THREADS=`expr $NUM_OF_THREADS - 1`
  fi

  EXECUTABLE="$BIN/$1-hashtable"
	echo "Experiment with" $1 >> $LOG
  if [ ! -f $EXECUTABLE ]; then
    echo "$EXECUTABLE not found. Aborting."
  fi

  size=$START;
	range=`expr $size \* 2`
  shift 3
  while [ $size -le $MAX_SIZE ]; 
  do
	  echo -n $size
    echo -n ","
    echo -n "scale=6;(" > sum
    echo -n > list
	  echo "sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DURATION -i $size -r $range -u $UPDATE_RATIO -l $LOAD" >> $LOG
    for j in `seq 1 $NUMBER_OF_RUNS`
    do
        sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DURATION -i $size -r $range -u $UPDATE_RATIO -l $LOAD| tr -d ',\n' | tee -a sum list > /dev/null
        if [ $j -lt $NUMBER_OF_RUNS ]; then
            echo -n '+' >> sum
            echo -n ',' >> list
        fi
    done
    echo ")/$NUMBER_OF_RUNS" >> sum
    cat sum | bc | tr -d '\n' | tee -a $LOG
    echo
    size=`expr $size \* 2`
    range=`expr $size \* 2`
    echo -n "<--" >> $LOG
    cat list >> $LOG
    echo "" >> $LOG
	done
	rm -f sum list
}

types="ffwd mutex mcs ticket hticket ttas tas clh"

mkdir -p $DIR
if [ $# -eq 0 ]; then
	for t in $types; do
		EXPERIMENT_NAME="$DIR/$t"
    header=`echo $t | tr '[:lower:]' '[:upper:]'`
		echo "\"$header\"" > $EXPERIMENT_NAME.csv
		hash_size_exp $t | tee -a $EXPERIMENT_NAME.csv
	done
else
	for t in "$@"; do
	  EXPERIMENT_NAME="$DIR/$t"
    header=`echo $t | tr '[:lower:]' '[:upper:]'`
	  echo "\"$header\"" > $EXPERIMENT_NAME.csv
	  hash_size_exp $t | tee -a $EXPERIMENT_NAME.csv
  done
fi
rm -f sum list
