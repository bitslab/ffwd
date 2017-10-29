#!/bin/bash

GDB="$(pwd)/../lock-profiler/lock-profiler -a"

#. .root

#. ../figures/bench.inc.sh

DEFAULT_NPROCS=$(cat /proc/cpuinfo | grep processor | wc -l)

NPROCS=`nproc`
RESULTS_DIR="${RESULTS_DIR:-results}"
OUT_DIR=$RESULTS_DIR/liblock-splash2
SPLASH2_DIR=$2
RUNS=15
BIN="../bin"
IFILE_DIR="inputs"

if [ -z "$NPROCS" ]; then NPROCS=$DEFAULT_NPROCS; fi
if [ -z "$SPLASH2_DIR" ];    then SPLASH2_DIR=../splash2/splash2-liblock/splash2; fi
if [ -z "$RUNS" ];   then RUNS=20; fi

CODEDIR=$SPLASH2_DIR/codes
INPUT_DIR="$CODEDIR/apps/raytrace/inputs"

export NPROCS

run_barnes() {
	if [ -f $CODEDIR/apps/barnes/BARNES ]; then
		cat $CODEDIR/apps/barnes/input | sed -e "s/46/$NPROCS/" | $GDB $CODEDIR/apps/barnes/BARNES
  fi
}

run_ocean_non() {
	if [ -f $CODEDIR/apps/ocean/non_contiguous_partitions/OCEAN ]; then
			$GDB $CODEDIR/apps/ocean/non_contiguous_partitions/OCEAN -p$NPROCS
	fi
}

run_ocean_cont() {
	if [ -f $CODEDIR/apps/ocean/contiguous_partitions/OCEAN ]; then
			$GDB $CODEDIR/apps/ocean/contiguous_partitions/OCEAN -p $NPROCS
	fi
}

run_water_nsquared() {
	if [ -f $CODEDIR/apps/water-nsquared/WATER-NSQUARED ]; then
			cd $CODEDIR/apps/water-nsquared
			cat input | sed -e "s/46/$NPROCS/" | $GDB ./WATER-NSQUARED
			cd $CODEDIR
	fi		
}

run_water_spatial() {
	if [ -f $CODEDIR/apps/water-spatial/WATER-SPATIAL ]; then
			cd $CODEDIR/apps/water-spatial
			cat input | sed -e "s/46/$NPROCS/" | $GDB ./WATER-SPATIAL
			cd $CODEDIR
	fi		
}

run_fmm() {
	if [ -f $CODEDIR/apps/fmm/FMM ]; then
			cd $CODEDIR/apps/fmm
			cat inputs/input.16384 | sed -e "s/46/$NPROCS/" | $GDB ./FMM
			cd $CODEDIR
	fi		
}

run_kernels_fft() {
	if [ -f $CODEDIR/kernels/fft/FFT ]; then
			cd $CODEDIR/kernels/fft
			$GDB ./FFT -p$NPROCS -l6 -n64
			cd $CODEDIR
	fi
}

run_kernels_lu_contiguous() {
	if [ -f $CODEDIR/kernels/lu/contiguous_blocks/LU ]; then
			cd $CODEDIR/kernels/lu/contiguous_blocks/
			$GDB ./LU -p$NPROCS -b8
			cd $CODEDIR
	fi
}

run_volrend() {
	if [ -f $CODEDIR/apps/volrend/VOLREND ]; then
			cd $CODEDIR/apps/volrend
			$GDB ./VOLREND $NPROCS inputs/head
			cd $CODEDIR
	fi
}

run_cholesky() {
	if [ -f $CODEDIR/kernels/cholesky/CHOLESKY ]; then
			cd $CODEDIR/kernels/cholesky
			$GDB ./CHOLESKY -p$NPROCS -C64 inputs/tk29.O
			cd $CODEDIR
	fi
}

run_radiosity() {
#		echo "-- run-radiosity: '$LIBLOCK_LOCK_NAME' lock, $LIBLOCK_CORES cores and $NPROCS threads --" 1>&2
	if [ -f $BIN/liblock-radiosity ]; then
			#$GDB ./RADIOSITY -p $NPROCS -batch -ae 500 -bf 0.005
			$BIN/liblock-radiosity -p $NPROCS -batch -ae 500 -bf 0.005
	fi
}

run_radiosity-orig() {
#		echo "-- run-radiosity-orig: '$LIBLOCK_LOCK_NAME' lock, $LIBLOCK_CORES cores and $NPROCS threads --" 1>&2
	if [ -f $BIN/liblock-radiosity ]; then
			#$GDB ./liblock-radiosity -p $NPROCS -batch
			$BIN/liblock-radiosity -p $NPROCS -batch
	fi
}

run_raytrace() {
	echo "-- run-raytrace: '$LIBLOCK_LOCK_NAME' lock, $LIBLOCK_CORES cores and $NPROCS threads in $CODEDIR --" 1>&2
	if [ -f $BIN/liblock-raytrace ]; then
			#$GDB ./liblock-raytrace -m1024 -p$NPROCS inputs/$1.env
			#echo "sudo $BIN/liblock-raytrace -m1024 -p$NPROCS $IFILE_DIR/$1.env" 1>&2
			$BIN/liblock-raytrace -m1024 -p$NPROCS $IFILE_DIR/$1.env
	fi
}

eval_exp() {
		RUNS=$1
		shift;
		
		#echo "-- $RUNS runs of $1 with '$LIBLOCK_LOCK_NAME' lock, $LIBLOCK_CORES cores and $NPROCS threads --" 1>&2
		for F in $(seq 1 $RUNS); do
				eval $@
		done
}

moyenne() {
		let TOT=0
		let N=0
		while read line; do
				let TOT=$TOT+$line
				let N=$N+1
		done
		let TOT=$TOT/$N
		echo "$TOT"
}

eval_all_of() {
		NAME=$1
		#RUNS=$2
		RUNS=10
		#RRR=$3
		RRR=1
		BBB=$4
		START=2
		END=$NPROCS
    END_DUMMY=$END
		shift; shift; shift; shift

		echo "eval: $NAME - $RUNS - $BBB - $START/$END/$RRR"

		for x in $BBB; do
        echo "Evaluating for Benchmark $x"
        echo "\"$x\"" > $OUT_DIR/splash2-$NAME-$x.csv
				END=$END_DUMMY
        DEC=0
        START=`expr $START - 1`
				if [ $x = "rcl" ]; then
						DEC=$RRR;
						END=$(echo "$END-$RRR-1" | bc -l)
				fi
				for y in `seq $START $END`; do
						v=$(echo $y+$DEC | bc)
						echo -n "$v, "
						#if [ $x = "posix" ]; then
						#		rep=./../splash2-base
						#else
								rep=splash2
						#fi
            #echo "CODEDIR=$SPLASH2_DIR/codes LIBLOCK_LOCK_NAME=$x NPROCS=$y eval_exp $RUNS "$@" | tee -a $OUT_DIR/splash2-$NAME-$x-$v.csv"
						CODEDIR=$SPLASH2_DIR/codes LIBLOCK_LOCK_NAME=$x NPROCS=$y eval_exp $RUNS "$@" | tee -a $OUT_DIR/splash2-$NAME-$x-$v.csv | moyenne
				done | tee -a $OUT_DIR/splash2-$NAME-$x.csv
		done
}

#LIBLOCK_LOCK_NAME=mcs run_raytrace car
#exit

convert() {
  if [ -f "$1" ];then
    awk 'NR==1' $1 > ./tmp
    awk -F"," 'NR>1 {print $1","$2/1000}' $1 >> ./tmp
    mv ./tmp $1
  fi
}

copy()
{
  mkdir -p $RESULTS_DIR/raytrace_bench/car/
  mkdir -p $RESULTS_DIR/raytrace_bench/balls/
  mkdir -p $RESULTS_DIR/radiosity_bench/
  for M in posix mcs spinlock flat rcl; do
    N=$M
    if [ "$M" = "posix" ]; then
      N="mutex"
    elif [ "$M" = "spinlock" ]; then
      N="tas"
    elif [ "$M" = "flat" ]; then
      N="fc"
    fi
    cp $OUT_DIR/splash2-raytrace-car-$M.csv $RESULTS_DIR/raytrace_bench/car/$N.csv 2>&1
    convert $RESULTS_DIR/raytrace_bench/car/$N.csv
    cp $OUT_DIR/splash2-raytrace-balls-$M.csv $RESULTS_DIR/raytrace_bench/balls/$N.csv 2>&1
    convert $RESULTS_DIR/raytrace_bench/balls/$N.csv
    cp $OUT_DIR/splash2-radiosity-$M.csv $RESULTS_DIR/radiosity_bench/$N.csv 2>&1
    convert $RESULTS_DIR/radiosity_bench/$N.csv
  done
}

eval_all() {
		#eval_all_of fmm $RUNS 'run_fmm | grep "Total time with initialization" | cut -d":" -f2'

#		eval_all_of radiosity-0-1 $RUNS 2 "rcl" 'LIBLOCK_CORES=0:1,1:1 run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
#		eval_all_of radiosity-0-1-0 $RUNS 2 "rcl" 'LIBLOCK_CORES=0:1,1:1,0:1 run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
#		eval_all_of radiosity-0-1-2 $RUNS 3 "rcl" 'LIBLOCK_CORES=0:1,1:1,2:1 run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
#		eval_all_of radiosity-0-1-2-0 $RUNS 3 "rcl" 'LIBLOCK_CORES=0:1,1:1,2:1,0:1 run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
		eval_all_of raytrace-car $RUNS 1 "posix" 'run_raytrace car 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
		#eval_all_of radiosity $RUNS 1 "posix" 'LIBLOCK_CORES=0:1 run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
    copy

exit
		eval_all_of raytrace-car $RUNS 1 "posix mcs spinlock flat" 'run_raytrace car 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
		eval_all_of raytrace-balls4 $RUNS 1 "posix mcs spinlock flat" 'run_raytrace balls4 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
		eval_all_of radiosity $RUNS 1 "posix spinlock flat" 'LIBLOCK_CORES=0:1 run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'

		eval_all_of raytrace-car $RUNS 2 "rcl" 'run_raytrace car 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
		eval_all_of raytrace-balls4 $RUNS 2 "rcl" 'run_raytrace balls4 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
		eval_all_of radiosity $RUNS 2 "rcl" 'LIBLOCK_CORES=0:1 run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
#		eval_all_of radiosity-orig $RUNS 1 "$benchs" 'LIBLOCK_CORES=0:1 run_radiosity-orig | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'

#		eval_all_of raytrace-car $RUNS 1 "posix" 'run_raytrace car 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
#		eval_all_of raytrace-balls4 $RUNS 1 "posix" 'run_raytrace balls4 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
}

 # for F in posix multircl spinlock mcs flat; do
 # 		echo -n $F:
 # 		NPROCS=1 LIBLOCK_LOCK_NAME=$F    run_water_spatial | grep COMPUTETIME
 # done

#NPROCS=1 LIBLOCK_LOCK_NAME=posix run_raytrace

mkdir -p $OUT_DIR
cp -R $INPUT_DIR .
eval_all
copy
rm -rf $IFILE_DIR

#run_raytrace car
#run_radiosity
#LIBLOCK_CORES="0:1,1:1,2:1,0:1" NPROCS=45 run_radiosity

#eval_exp 30 'run_raytrace balls4 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'
#eval_exp 30 'run_raytrace car 2>/dev/null | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'

#eval_exp 30 'run_radiosity | grep  "Total time with initialization" | sed -e "s/Total time with initialization//"'

#eval_exp "Ray Trace car" 30 'run_raytrace car 2>/dev/null | grep "Total time with initialization" | sed -e "s/Total time with initialization//"'
#eval_exp Barnes 30 'run_barnes | grep COMPUTETIME | cut -d"=" -f2'

# eval_exp "Ocean contigous" 30 'run_ocean_cont | grep "Total time with initialization" | sed -e "s/Total time with initialization//" | sed -e "s/://"'

# eval_exp "Ocean non contigous" 30 'run_ocean_non | grep "Total time with initialization" | sed -e "s/Total time with initialization//" | sed -e "s/://"'


# run_barnes

# run_ocean_non
# eval_exp "Ocean Non Contigous" 30 'run_ocean_non | grep "Total time with initialization" | cut -d":" -f2'

# eval_exp "Ocean Contigous" 30 'run_ocean_cont | grep "Total time with initialization" | cut -d":" -f2'

# eval_exp "Water nsquared" 30 'run_water_nsquared' | grep "COMPUTETIME (after initialization) =" | sed -e "s/COMPUTETIME (after initialization) =//"'

#eval_exp "Water spatial" 30 'run_water_spatial | grep "COMPUTETIME (after initialization) =" | sed -e "s/COMPUTETIME (after initialization) =//"'

# run_water_nsquared
# run_water_spatial
# run_ocean_cont

# run_kernels_fft
# run_kernels_lu_contiguous

# run not ok, compile ok (3)
# run_radiosity
# run_volrend
# run_cholesky

# cocci not ok (2)
#	apps/fmm
#	kernels/radix

