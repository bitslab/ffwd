#!/bin/bash

RUNS=10
THREADS=120
RESULTS_DIR="${RESULTS_DIR:-results}"
DIR="$RESULTS_DIR/raytrace_bench"
INPUT_DIR="../splash2/splash2-ffwd/codes/apps/raytrace/inputs"
IFILE_DIR="inputs"
BIN="../bin"

NPROC=`nproc`

convert() {
  awk 'NR==1' $1 > ./tmp
  awk -F"," 'NR>1 {print $1","$2/1000}' $1 >> ./tmp
  mv ./tmp $1
}

raytrace_balls4() {

  if [ $NPROC -eq 128 ]; then
    THREADS=120
  elif [ $NPROC -eq 64 ]; then
    THREADS=56
  else
    echo "Aborting."
    exit
  fi

  FILE_DIR="$DIR/balls"
  FILE="$FILE_DIR/ffwd.csv"
  IFILE="$IFILE_DIR/balls4.env"
  mkdir -p $FILE_DIR

  echo "FFWD" > $FILE
  for PROC in $(seq 2 $THREADS); 
  do
    echo -n "$PROC," | tee -a $FILE
    echo -n "(" > sum
    for j in `seq 1 $RUNS`; do
      $BIN/ffwd-raytrace -m1024 -p$PROC $IFILE 2>/dev/null| grep "Total time with initialization" | sed -e "s/Total time with initialization//" | sed 's/^ *//g' | tr -d ',\n' >> sum
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

raytrace_car() {

  if [ $NPROC -eq 128 ]; then
    THREADS=120
  elif [ $NPROC -eq 64 ]; then
    THREADS=56
  else
    echo "Aborting."
    exit
  fi

  FILE_DIR="$DIR/car"
  FILE="$FILE_DIR/ffwd.csv"
  IFILE="$IFILE_DIR/car.env"
  mkdir -p $FILE_DIR

  echo "ffwd" > $FILE
  for PROC in $(seq 2 $THREADS); do
  echo -n "$PROC," | tee -a $FILE
  echo -n "(" > sum
  for j in `seq 1 $RUNS`; do
    $BIN/ffwd-raytrace -m1024 -p$PROC $IFILE 2>/dev/null| grep "Total time with initialization" | sed -e "s/Total time with initialization//" | sed 's/^ *//g' | tr -d ',\n' >> sum
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

cp -R $INPUT_DIR .
raytrace_balls4
raytrace_car
rm -f sum list
rm -rf $IFILE_DIR
