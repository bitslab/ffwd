#!/bin/bash
 
NUMBER_OF_RUNS=10
MAX_NUM_OF_GLOBAL=1024
START=1
DELAY=25

RESULTS_DIR="${RESULTS_DIR:-results}"
DIR="$RESULTS_DIR/shared_vars_bench"
LOG=$DIR/shared_vars_log.txt
BIN="../bin"

NPROC=`nproc`
NUM_OF_THREADS=$NPROC

function shared_vars_exp {

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

	echo "Experiment with" $1 >> $LOG
  EXECUTABLE="$BIN/$1-shared-vars"
  if [ ! -f $EXECUTABLE ]; then
    echo "$EXECUTABLE not found. Aborting."
  fi

  g=$START;
  shift 3
  while [ $g -le $MAX_NUM_OF_GLOBAL ]; 
  do
	  	echo -n $g
          echo -n ","
          echo -n "scale=6;(" > sum
          echo -n > list
          if [ "$1" = "ffwd" ]; then
            if [ $g -lt 4 ]; then
	  	        echo "sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DELAY -g $g -s $g" >> $LOG
            else
	  	        echo "sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DELAY -g $g -s 4" >> $LOG
            fi
          else
	  	      echo "sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DELAY -g $g" >> $LOG
          fi
          for j in `seq 1 $NUMBER_OF_RUNS`
          do
              if [ "$1" = "ffwd" ]; then
                if [ $g -lt 4 ]; then
                  sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DELAY -g $g -s $g | tr -d ',\n' | tee -a sum list >> /dev/null
                else
                  sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DELAY -g $g -s 4 | tr -d ',\n' | tee -a sum list >> /dev/null
                fi
              else
                sudo $EXECUTABLE -t $NUM_OF_THREADS -d $DELAY -g $g | tr -d ',\n' | tee -a sum list >> /dev/null
              fi
              if [ $j -lt $NUMBER_OF_RUNS ]; then
                  echo -n '+' >> sum
                  echo -n ',' >> list
              fi
          done
      	echo ")/$NUMBER_OF_RUNS" >> sum
          cat sum | bc | tr -d '\n' | tee -a $LOG
          echo
	  g=`expr $g \* 2`
	echo -n "<--" >> $LOG
	cat list >> $LOG
	echo "" >> $LOG
	done
#rm -f $EXECUTABLE 
	rm -f sum list
}

types="ffwd rcl fc mutex mcs ticket hticket ttas tas clh atomic"

mkdir -p $DIR
if [ $# -eq 0 ]; then
	for t in $types; do
		EXPERIMENT_NAME="$DIR/$t"
    header=`echo $t | tr '[:lower:]' '[:upper:]'`
		echo "\"$header\"" > $EXPERIMENT_NAME.csv
		shared_vars_exp $t | tee -a $EXPERIMENT_NAME.csv
	done
else
	for t in "$@"; do
	  EXPERIMENT_NAME="$DIR/$t"
    header=`echo $t | tr '[:lower:]' '[:upper:]'`
	  echo "\"$header\"" > $EXPERIMENT_NAME.csv
	  shared_vars_exp $t | tee -a $EXPERIMENT_NAME.csv
  done
fi
rm -f sum list
