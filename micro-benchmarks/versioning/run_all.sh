#!/bin/bash

NUMBER_OF_RUNS=10
INIT_SIZE=1024
DURATION=10000
NPROC=`nproc`
OUTDIR=results

ffwd_max_threads="56"
rcl_max_threads="63"
max_threads="64"
ffwd_threads="1 4 8 16 32 48 56"
rcl_threads="1 4 8 16 32 48 63"
other_threads="1 4 8 16 32 48 64"
single_threaded="1"
if [ $NPROC -eq 128 ]; then
	ffwd_max_threads="120"
	rcl_max_threads="127"
	max_threads="128"
	ffwd_threads="1 4 8 16 32 48 64 96 120"
	rcl_threads="1 4 8 16 32 48 64 96 127"
	other_threads="1 4 8 16 32 48 64 96 128"
fi

tree=("ffwd" "bonsai" "vtree" "vrbtree" "rlu" "rcl" "swisstm")
list=("swisstm" "harris")

function run_experiment {

	size=$5
	for t in $3; do 
		echo with $t threads
		for i in $NUMBER_OF_RUNS; do
			sudo ./benchmark_$2_$1 -n $t -d $DURATION -i $size -r $((size*2)) > tmp
			if [ "$1" = "swisstm" ]; then
				awk '{if($1=="#txs") print $4}'  tmp | sed 's/(//g' | sed 's/\/s)//g' >> avg
			else
				awk '{if($1=="ops:") print $3}' tmp | sed 's/(//g' | sed 's/\/s)//g' >> avg
			fi
		done
		if [ "$4" = "over_threads" ]; then
			echo -n $t ,\ >> $OUTDIR/$2/$4/$1.csv
		else
			echo -n $size ,\ >> $OUTDIR/$2/$4/$1.csv
		fi
		awk '{ sum += $1; n++ } END { if (n > 0) print sum/n; }' avg >> $OUTDIR/$2/$4/$1.csv
		rm tmp avg
	done
}

mkdir -p $OUTDIR
mkdir -p $OUTDIR/tree
mkdir -p $OUTDIR/list
mkdir -p $OUTDIR/tree/over_threads
mkdir -p $OUTDIR/tree/over_size
mkdir -p $OUTDIR/list/over_threads
mkdir -p $OUTDIR/list/over_size

rm -f tmp avg
for b in ${tree[@]}; do
	echo running $b
	echo `echo $b | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/tree/over_threads/$b.csv
	if [ "$b" = "ffwd" ]; then
		run_experiment $b tree "$ffwd_threads" over_threads $INIT_SIZE
	elif [ "$b" = "rcl" ]; then
		run_experiment $b tree "$rcl_threads" over_threads $INIT_SIZE
	else
		run_experiment $b tree "$other_threads" over_threads $INIT_SIZE
	fi
done
for b in ${list[@]}; do
	echo `echo $b | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/list/over_threads/list_$b.csv
	run_experiment $b list "$other_threads" over_threads $INIT_SIZE
done

for ((s = 128; s <= 131072; s*=2)); do
	for b in ${tree[@]}; do
		echo `echo $b | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/tree/over_size/$b.csv
		if [ "$b" = "ffwd" ]; then
			run_experiment $b tree "$ffwd_max_threads" over_size $s
		elif [ "$b" = "rcl" ]; then
			run_experiment $b tree "$rcl_max_threads" over_size $s
		else
			run_experiment $b tree "$max_threads" over_size $s
		fi
	done
	for b in ${list[@]}; do
		echo `echo $b | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/list/over_size/$b.csv
		run_experiment $b list "$max_threads" over_size $s
	done
	echo single_threaded  >> $OUTDIR/tree/over_size/single_threaded.csv
	run_experiment single_threaded tree "$single_threaded" over_size $s
	echo FFWD-S4  >> $OUTDIR/tree/over_size/ffwd-s4.csv
	run_experiment ffwd_4s tree "$ffwd_max_threads" over_size $s
done




