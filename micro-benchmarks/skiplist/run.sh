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

function run_experiment {
	size=$4
	for t in $2; do 
		echo with $t threads
		for i in $NUMBER_OF_RUNS; do
			./$1 -t $t -d $DURATION -i $size -r $((size*2)) > tmp
			if [ "$1" = "swisstm" ]; then
				awk '{if($1=="#txs") print $4}'  tmp | sed 's/(//g' | sed 's/\/s)//g' >> avg
			else
				awk '{if($1=="ops:") print $3}' tmp | sed 's/(//g' | sed 's/\/s)//g' >> avg
			fi
		done
		if [ "$3" = "over_threads" ]; then
			echo -n $t ,\ >> $OUTDIR/$3/$1.csv
		else
			echo -n $size ,\ >> $OUTDIR/$3/$1.csv
		fi
		awk '{ sum += $1; n++ } END { if (n > 0) print sum/n; }' avg >> $OUTDIR/$3/$1.csv
		rm tmp avg
	done
}

mkdir -p $OUTDIR
mkdir -p $OUTDIR/over_threads
mkdir -p $OUTDIR/over_size
rm -f tmp avg

for b in ffwd_skiplist mcs_skiplist; do
	echo running $b
	if [ "$b" = "ffwd_skiplist" ]; then
		echo FFWD-SK >> $OUTDIR/ffwd-skiplist.csv
		run_experiment $b "$ffwd_threads" over_threads $INIT_SIZE
	else
		echo MCS-SK >> $OUTDIR/mcs-skiplist.csv
		run_experiment $b "$other_threads" over_threads $INIT_SIZE
	fi
done

for ((s = 1; s <= 16384; s*=2)); do
	for b in ffwd_skiplist mcs_skiplist; do
		if [ "$b" = "ffwd_skiplist" ]; then
			run_experiment $b "$ffwd_max_threads" over_size $s
		else
			run_experiment $b "$max_threads" over_size $s
		fi
	done
done

