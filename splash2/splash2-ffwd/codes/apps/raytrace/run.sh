#!/bin/bash

RUNS=10
THREADS=120
SYSTEM="nodes"

raytrace_balls4() {
  FILE=splash2-raytrace-balls4-ffwd-$SYSTEM.csv
  IFILE=inputs/balls4.env
  rm -f $FILE

  echo "ffwd-1" > $FILE
  for PROC in $(seq 2 $THREADS); do
    echo -n "$PROC," | tee -a $FILE
    echo -n "(" > sum
    for j in `seq 1 $RUNS`; do
      ./RAYTRACE -m1024 -p$PROC $IFILE 2>/dev/null| grep "Total time with initialization" | sed -e "s/Total time with initialization//" | sed 's/^ *//g' | tr -d ',\n' >> sum
      if [ $j -lt $RUNS ]; then
        echo -n '+' >> sum
        echo -n ',' >> list
      fi
    done
    echo ")/$RUNS" >> sum
    cat sum | bc | tee -a $FILE
  done
}

raytrace_car() {
  FILE=splash2-raytrace-car-ffwd-$SYSTEM.csv
  IFILE=inputs/car.env
  rm -f $FILE

  echo "ffwd-1" > $FILE
  for PROC in $(seq 2 $THREADS); do
    echo -n "$PROC," | tee -a $FILE
    echo -n "(" > sum
    for j in `seq 1 $RUNS`; do
      ./RAYTRACE -m1024 -p$PROC $IFILE 2>/dev/null| grep "Total time with initialization" | sed -e "s/Total time with initialization//" | sed 's/^ *//g' | tr -d ',\n' >> sum
      if [ $j -lt $RUNS ]; then
        echo -n '+' >> sum
        echo -n ',' >> list
      fi
    done
    echo ")/$RUNS" >> sum
    cat sum | bc | tee -a $FILE
  done
}

raytrace_balls4
raytrace_car
