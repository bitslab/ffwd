#!/bin/bash

RUNS=10
THREADS=120
RESULTS_DIR="${RESULTS_DIR:-results}"
DIR="$RESULTS_DIR/radiosity_bench"
BIN="../bin"

NPROC=`nproc`

convert() {
  awk 'NR==1' $1 > ./tmp
  awk -F"," 'NR>1 {print $1","$2/1000}' $1 >> ./tmp
  mv ./tmp $1
}

radiosity() {

  if [ $NPROC -eq 128 ]; then
    THREADS=120
  elif [ $NPROC -eq 64 ]; then
    THREADS=56
  else
    echo "Aborting."
    exit
  fi

  mkdir -p $DIR
  FILE="$DIR/ffwd.csv"
  echo "FFWD" > $FILE
  for PROC in $(seq 2 $THREADS); do
    echo -n "$PROC," | tee -a $FILE
    echo -n "(" > sum
    for j in `seq 1 $RUNS`; do
      $BIN/ffwd-radiosity -p $PROC -batch -ae 500 -bf 0.005 2>/dev/null| grep "Total time with initialization" | sed -e "s/Total time with initialization//" | sed 's/^ *//g' | tr -d ',\n' >> sum
      if [ $j -lt $RUNS ]; then
        echo -n '+' >> sum
        echo -n ',' >> list
      fi
    done
    echo ")/$RUNS" >> sum
    cat sum | bc | tee -a $FILE
  done
  convert $FILE
}

radiosity
rm -f sum list
