#!/bin/bash

RESULTS_DIR="${RESULTS_DIR:-results}"
RESULTS_DIR=$RESULTS_DIR/memcached

NUMBER_OF_RUNS=10
MEMCACHED_VERSION=1.4.6
#LIBMEMCACHED_VERSION=0.49
LIBMEMCACHED_VERSION=1.0.2
MEMCACHED_PATH=../memcached/
BIN_PATH=../bin

NPROC=`nproc`
MIN_THREADS=0
MAX_THREADS=$NPROC

MID_THREADS=0
if [ $NPROC -eq 128 ]; then
  MID_THREADS=`expr $MAX_THREADS / 2`
fi

trap "cleanup; exit" SIGHUP SIGINT SIGTERM

function cleanup {
    rm -f $RESULTS_DIR/sum
    rm -f $RESULTS_DIR/list
    rm -f $RESULTS_DIR/out
}

function benchmark {
    APP_DIR=$RESULTS_DIR/$3/
    mkdir -p $APP_DIR
    sudo killall -9 -q memcached
   
    cleanup 
    rm -f $APP_DIR/$2.csv

    echo "Starting generating file $APP_DIR/"$2".csv..."
    date

    if [[ "$1" == "rcl" ]]; then
      MAX_THREADS=`expr $MAX_THREADS - 1`
    fi

    for n in `seq $MIN_THREADS 4 $MAX_THREADS`; do
        if [ $n -eq 0 ]; then
          n=1
        fi
        echo -n "$n," >> $APP_DIR/$2.csv
           
        echo -n "scale=6;(" > $RESULTS_DIR/sum
        echo -n > $RESULTS_DIR/list

        for m in `seq 1 $NUMBER_OF_RUNS`; do
            echo "Lock: "$1", experiment: "$3", cores : "$n", iteration : "$m"."

            sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"../liblock/" \
                 LIBLOCK_LOCK_NAME=$1 \
                 $BIN_PATH/liblock-memcached -t $n -u root &

            sleep 0.5
                
            $MEMCACHED_PATH/libmemcached-$LIBMEMCACHED_VERSION/clients/memslap \
                --flush --servers=127.0.0.1:11211 --concurrency=460 \
                --test=$3 $4 > $RESULTS_DIR/out #2> /dev/null
            
            echo "Elapsed time : "
            cat $RESULTS_DIR/out
            grep "Took" $RESULTS_DIR/out | awk '{ printf "%s",$2; }' \
                                    | tee -a $RESULTS_DIR/sum $RESULTS_DIR/list
            echo
            
            if [ $m -lt $NUMBER_OF_RUNS ]; then
                echo -n '+' >> $RESULTS_DIR/sum
                echo -n ',' >> $RESULTS_DIR/list
            fi

            #sudo killall -9 -w -v memcached
            sudo killall -SIGINT -w -v memcached
        done

        echo ")/$NUMBER_OF_RUNS" >> $RESULTS_DIR/sum
        cat $RESULTS_DIR/sum | bc | tr -d '\n' >> $APP_DIR/$2.csv
        echo -n "," >> $APP_DIR/$2.csv
        cat $RESULTS_DIR/list >> $APP_DIR/$2.csv
        echo >> $APP_DIR/$2.csv
    done

    echo "File $APP_DIR/"$2".csv contents:"
    cat $APP_DIR/$2.csv

    cleanup

    echo "Done."
    date
}

mkdir -p $RESULTS_DIR

benchmark posix mutex get
benchmark posix mutex set

benchmark spinlock tas get
benchmark spinlock tas set

benchmark mcs mcs get
benchmark mcs mcs set

benchmark rcl rcl get
benchmark rcl rcl set

