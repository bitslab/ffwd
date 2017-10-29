#!/bin/bash

NUMBER_OF_RUNS=10
INIT_SIZE=1024
DURATION=10000
NPROC=`nproc`
OUTDIR=results

ffwd_threads="1 4 8 16 32 48 56"
rcl_threads="1 4 8 16 32 48 63"
other_threads="1 4 8 16 32 48 64"
if [ $NPROC -eq 128 ]; then
	ffwd_threads="1 4 8 16 32 48 64 96 120"
	rcl_threads="1 4 8 16 32 48 64 96 127"
	other_threads="1 4 8 16 32 48 64 96 128"
fi

queue_methods=("ccQueue.run" "dsmQueue.run" "LocksQueue.run.flat" "LocksQueue.run.spin" "LocksQueue.run.mcs" "LocksQueue.run.ticket"  
			 "LocksQueue.run" "LocksQueue.run.mutex" "SimQueue.run" "MSQueue.run" "hQueue.run" )

stack_methods=("ccStack.run" "dsmStack.run" "LocksStack.run.flat" "LocksStack.run.spin" "LocksStack.run.mcs" 
				"LocksStack.run.ticket" "LocksStack.run" "LocksStack.run.mutex" "SimStack.run" "LFStack.run" "hStack.run" )

queue_methods_out=("cc" "dsm" "fc" "ttas" "mcs" "ticket" "clh" "mutex" "sim" "ms" "h")
stack_methods_out=("cc" "dsm" "fc" "ttas" "mcs" "ticket" "clh" "mutex" "sim" "lf" "h")

ffwd=("LocksQueue.run.ffwd" "LocksStack.run.ffwd" )
rcl=("LocksQueue.run.rcl""LocksStack.run.rcl" )


function run_experiment {
	
	if [ "$3" = "queue" ]; then
		DIR=queue
	else
		DIR=stack
	fi
	for i in $NUMBER_OF_RUNS; do
		if [ "$1" = "SimQueue.run" ]; then
			sudo ./$1 0 > avg
		elif [ "$1" = "SimStack.run" ]; then
			sudo ./$1 0 > avg
		elif [ "$1" = "LFStack.run" ]; then
			sudo ./$1 0 0 > avg
		elif [ "$1" = "MSQueue.run" ]; then
			sudo ./$1 0 0 > avg
		else
			sudo ./$1 > avg
		fi
	done
	echo $OUTDIR/$DIR/$4.csv
	echo -n $2 ,\ >> $OUTDIR/$DIR/$4.csv
	awk '{ sum += $1; n++ } END { if (n > 0) print sum/n; }' avg >> $OUTDIR/$DIR/$4.csv
	rm avg
	done
}

mkdir -p $OUTDIR
mkdir -p $OUTDIR/stack
mkdir -p $OUTDIR/queue

for ((i=0; i<11; i++)); do
	echo `echo ${queue_methods_out[$i]} | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/queue/${queue_methods_out[$i]}.csv
done

for ((i=0; i<11; i++)); do
	echo `echo ${stack_methods_out[$i]} | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/stack/${stack_methods_out[$i]}.csv
done

echo FFWD >> $OUTDIR/stack/ffwd.csv
echo FFWD >> $OUTDIR/queue/ffwd.csv

echo RCL >> $OUTDIR/stack/rcl.csv
echo RCL >> $OUTDIR/queue/rcl.csv

for i in $other_threads; do
	echo Running with $i threads
	make nthreads=$i &> /dev/null
	for ((b=0; b < 11 ; b++)); do
		run_experiment ${queue_methods[$b]} $i queue ${queue_methods_out[$b]}
		run_experiment ${stack_methods[$b]} $i stack ${stack_methods_out[$b]}
	done
done

echo FFWD
for i in $ffwd_threads; do
	echo Running with $i threads
	make nthreads=$i &> /dev/null
	run_experiment ${ffwd[0]} $i queue ffwd
	run_experiment ${ffwd[1]} $i stack ffwd
done

echo RCL
for i in $rcl_threads; do
	echo Running with $i threads
	make nthreads=$i &> /dev/null
	run_experiment ${rcl[0]} $i queue rcl
	run_experiment ${rcl[1]} $i stack rcl
done


