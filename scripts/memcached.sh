#!/bin/bash

NUMBER_OF_RUNS=10
APP="memcached_bench"
RESULTS_DIR="${RESULTS_DIR:-results}"
APP_DIR=$RESULTS_DIR/$APP
BIN="../bin"
mkdir -p $APP_DIR

MAX_THREADS=120
NPROC=`nproc`
MID_THREADS=0
if [ $NPROC -eq 128 ]; then
	MIN_THREADS=4
	MAX_THREADS=120
  MID_THREADS=`expr $MAX_THREADS / 2`
elif [ $NPROC -eq 64 ]; then
	MIN_THREADS=4
	MAX_THREADS=56
fi

run_app() {
	DIR=$APP_DIR/$1
	OUT_FILE=$DIR/out
	ERROR_FILE=$DIR/error
	SUM_FILE=$DIR/sum
	LIST_FILE=$DIR/list
  RESULT_FILE=$DIR/ffwd.csv
	THREADS=$2

  mkdir -p $DIR

	echo "Running Memcached-FFWD-$1 with $THREADS threads" | tee $ERROR_FILE
	echo -n "$THREADS," >> $RESULT_FILE
	echo -n "scale=6;(" > $SUM_FILE
	echo -n > $LIST_FILE
	for m in `seq 1 $NUMBER_OF_RUNS`; do
		sudo $BIN/ffwd-memcached -t $THREADS -u root &
		sleep 0.5
		../memcached/libmemcached-1.0.2/clients/memslap --flush --servers=127.0.0.1:11211 --concurrency=460 --test=$1 > $OUT_FILE 2>$ERROR_FILE
		echo "Elapsed time : "
		cat $OUT_FILE
		grep "Took" $OUT_FILE | awk '{ printf "%s",$2; }' | tee -a $SUM_FILE $LIST_FILE
		echo
		if [ $m -lt $NUMBER_OF_RUNS ]; then
			echo -n '+' >> $SUM_FILE
			echo -n ',' >> $LIST_FILE
		fi
		sudo killall -SIGINT -w -v ffwd-memcached
	done
	echo ")/$NUMBER_OF_RUNS" >> $SUM_FILE
	cat $SUM_FILE | bc | tr -d '\n' >> $RESULT_FILE
	echo >> $RESULT_FILE
}

run_memcached() {
	DIR=$APP_DIR/$1
  RESULT_FILE=$DIR/ffwd.csv
	mkdir -p $DIR
	echo "\"FFWD\"" > $RESULT_FILE 

  if [ $MID_THREADS -ne 0 ]; then
	  MID_THREADS2=`expr $MID_THREADS + 8`
    for n in `seq $MIN_THREADS 4 $MID_THREADS`; do
      run_app $1 $n
    done
    for n in `seq $MID_THREADS2 8 $MAX_THREADS`; do
      run_app $1 $n
    done
  else
    for n in `seq $MIN_THREADS 4 $MAX_THREADS`; do
      run_app $1 $n
    done
  fi
}

run_memcached get
run_memcached set
