#!/bin/bash

START_THREADS=$1
END_THREADS=$2
OUTDIR=$3

if [ -z "$START_THREADS" ]; then START_THREADS=1; fi
if [ -z "$END_THREADS" ]; then END_THREADS=128; fi
if [ -z "$OUTDIR" ]; then OUTDIR=results; fi

INTERVAL=`seq $START_THREADS $END_THREADS`

methods="mutex tas fc mcs rcl ffwd"

TESTDIR=./tests
BENCHMARKS=("linear_regression" "string_match" "matrix_multiply")

for f in string_match.tar.gz linear_regression.tar.gz; do
	wget rclrepository.gforge.inria.fr/phoenix_datasets/$f
	tar -xvzf $f
	rm $f
done

mkdir $OUTDIR
mkdir $OUTDIR/linear_regression
mkdir $OUTDIR/string_match
mkdir $OUTDIR/matrix_multiply
rm tmp avg

for b in ${BENCHMARKS[@]}; do
	if [ "$b" = "matrix_multiply" ]; then
		for i in 500 1000 2000; do
			mkdir $OUTDIR/matrix_multiply/$i
			for m in methods; do
				echo `echo $m | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/matrix_multiply/$i/$m.csv
			done
		done
	else
		for i in 50 100 500; do
			mkdir $OUTDIR/$b/$i
			for m in methods; do
				echo `echo $m | tr '[:lower:]' '[:upper:]'` >> $OUTDIR/$b/$i/$m.csv
			done
		done
	fi
done

for x in posix flat spinlock mcs rcl; do
	for b in ${BENCHMARKS[@]}; do
		echo Running $b ...
		for y in $INTERVAL; do
			if [ "$b" = "matrix_multiply" ]; then
				for mside in 500 1000 2000; do
					for((i=0; i<10; i++)); do
						sudo LD_LIBRARY_PATH=../../liblock/ MR_NUMTHREADS=$y LIBLOCK_LOCK_NAME=$x $TESTDIR/$b/$b-rcl $mside &> tmp
						awk '{if ($2=="duration") print $4}' tmp >> avg;
						rm tmp
					done
					awk '{ sum += $1; n++ } END { if (n > 0) print sum/n; }' avg >> $OUTDIR/matrix_multiply/$mside/$x.csv;
					rm avg
				done	
			else
				for size in 50 100 500; do
					for((i=0; i<10; i++)); do
						sudo LD_LIBRARY_PATH=../../liblock/ MR_NUMTHREADS=$y LIBLOCK_LOCK_NAME=$x $TESTDIR/$b/$b-rcl $b\_datafiles/key_file_$size\MB.txt &> tmp
						awk '{if ($2=="duration") print $4}' tmp >> avg;
						rm tmp
					done
					awk '{ sum += $1; n++ } END { if (n > 0) print sum/n; }' avg >> $OUTDIR/$b/$size/$x.csv;
					rm avg
				done
			fi
		done
	done
done

for x in ffwd; do
	for b in ${BENCHMARKS[@]}; do
		echo Running $b ...
		for y in $INTERVAL; do
			if [ "$b" = "matrix_multiply" ]; then
				for mside in 500 1000 2000; do
					for((i=0; i<10; i++)); do
						sudo LD_LIBRARY_PATH=../../liblock/ MR_NUMTHREADS=$y $TESTDIR/$b/$b-ffwd $mside &> tmp
						awk '{if ($2=="duration") print $4}' tmp >> avg;
						rm tmp
					done
					awk '{ sum += $1; n++ } END { if (n > 0) print sum/n; }' avg >> $OUTDIR/matrix_multiply/$mside/$x.csv;
					rm avg
				done	
			else
				for size in 50 100 500; do
					for((i=0; i<10; i++)); do
						sudo LD_LIBRARY_PATH=../../liblock/ MR_NUMTHREADS=$y $TESTDIR/$b/$b-ffwd $b\_datafiles/key_file_$size\MB.txt &> tmp
						awk '{if ($2=="duration") print $4}' tmp >> avg;
						rm tmp
					done
					awk '{ sum += $1; n++ } END { if (n > 0) print sum/n; }' avg >> $OUTDIR/$b/$size/$x.csv;
					rm avg
				done
			fi
		done
	done
done