#!/bin/bash

if [ "$#" -ne 2 ]; then
	echo "Usage: ./plot.sh <hw platform> <type>"
	exit
fi

if [ "$2" == "shared_vars" ];
then
  BENCHMARK_DIR="shared_vars_bench"
	BENCHMARK_NAME="CompWithGlobals"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

	command="set terminal pdf monochrome;"
	command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
	command=$command"set xlabel '# of shared variables (locks)';"
	command=$command"set ylabel 'Throughput (Mops)';"
	command=$command"set yrange [0.0:400.0];"
	command=$command"set logscale x 2;"
	command=$command"set ytics 0, 50, 400;"
	command=$command"set key on horizontal;"
	command=$command"set datafile separator ',';"
elif [ "$2" == "simplelist_threads" ];
then
  BENCHMARK_DIR="simplelist_threads_bench"
  BENCHMARK_NAME="CompWithLinkedLists"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel 'Hardware threads';"
  command=$command"set ylabel 'Throughput (Mops)';"
  command=$command"set yrange [0.0:1.2];"
  command=$command"set xrange [0:130];"
	command=$command"set xtics 0,16,130;"
  command=$command"set ytics 0, 0.2, 1.2;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "lazylist_threads" ];
then
  BENCHMARK_DIR="lazylist_threads_bench"
  BENCHMARK_NAME="CompWithLazyListsOverThreads"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel 'Hardware threads';"
  command=$command"set ylabel 'Throughput (Mops)';"
  command=$command"set yrange [0.0:18.0];"
  command=$command"set xrange [0:130];"
	command=$command"set xtics 0,16,130;"
  command=$command"set ytics 0, 2, 18;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "lazylist_size" ];
then
  BENCHMARK_DIR="lazylist_size_bench"
  BENCHMARK_NAME="CompWithLazyListsOverSize"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
	command=$command"set key horizontal;"
  command=$command"set xlabel '#elements';" # Lazy is important here
  command=$command"set ylabel 'Throughput (Mops)';"
  command=$command"set yrange [0.0:60.0];"
  command=$command"set xrange [1:16500];"
  command=$command"set logscale x 2;"
  command=$command"set ytics 0, 10, 60;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "hash_size" ];
then
  BENCHMARK_DIR="hash_size_bench"
  BENCHMARK_NAME="CompWithHashtablesOverSize"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel '#buckets';"
  command=$command"set ylabel 'Throughput (Mops)';"
  command=$command"set yrange [0.0:225.0];"
  command=$command"set xrange [1:1024];"
  command=$command"set logscale x 2;"
  command=$command"set ytics 0, 25, 225;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "radiosity" ];
then
  BENCHMARK_DIR="radiosity_bench"
  BENCHMARK_NAME="Radiosity"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel '# of threads';"
  command=$command"set ylabel 'Duration (ms)';"
  command=$command"set xrange [0:128];"
  command=$command"set yrange [0:1000];"
  command=$command"set xtics 0,16,128;"
  command=$command"set ytics 0, 150, 1000;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "raytrace_car" ];
then
  BENCHMARK_DIR="raytrace_bench/car"
  BENCHMARK_NAME="RaytraceCar"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel '# of threads';"
  command=$command"set ylabel 'Duration (ms)';"
  command=$command"set xrange [0:128];"
  command=$command"set yrange [0:600];"
  command=$command"set xtics 0,16,128;"
  command=$command"set ytics 0, 100, 600;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "raytrace_balls" ];
then
  BENCHMARK_DIR="raytrace_bench/balls"
  BENCHMARK_NAME="RaytraceBalls"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel '#buckets';"
  command=$command"set ylabel 'Throughput (Mops)';"
  command=$command"set yrange [0.0:225.0];"
  command=$command"set xrange [1:1024];"
  command=$command"set logscale x 2;"
  command=$command"set ytics 0, 25, 225;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "memcached_get" ];
then
  BENCHMARK_DIR="memcached_bench/get"
  BENCHMARK_NAME="MemcachedGet"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel '# of threads';"
  command=$command"set ylabel 'Duration (sec)';"
  command=$command"set xrange [0:128];"
  command=$command"set yrange [0:90];"
  command=$command"set xtics 0,16,128;"
  command=$command"set ytics 0, 15, 90;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "memcached_set" ];
then
  BENCHMARK_DIR="memcached_bench/set"
  BENCHMARK_NAME="MemcachedSet"
	FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
	command=$command"set key horizontal;"
	command=$command"set key center top;"
	command=$command"set key samplen 2;"
  command=$command"set xlabel '# of threads';"
  command=$command"set ylabel 'Duration (sec)';"
  command=$command"set xrange [0:128];"
  command=$command"set yrange [0:300];"
  command=$command"set xtics 0,16,128;"
  command=$command"set ytics 0, 50, 300;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "queue" ];
then
  BENCHMARK_DIR="micro-benchmarks/sim-synch/results/queue"
  BENCHMARK_NAME="Queue"
  FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
  command=$command"set key horizontal;"
  command=$command"set key center top;"
  command=$command"set key samplen 2;"
  command=$command"set xlabel 'Hardware threads';"
  command=$command"set ylabel 'Throughput (Mops)';"
  # command=$command"set yrange [0.0:1.2];"
  # command=$command"set xrange [0:130];"
  # command=$command"set xtics 0,16,130;"
  # command=$command"set ytics 0, 0.2, 1.2;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "stack" ];
then
  BENCHMARK_DIR="micro-benchmarks/sim-synch/results/stack"
  BENCHMARK_NAME="Stack"
  FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
  command=$command"set key horizontal;"
  command=$command"set key center top;"
  command=$command"set key samplen 2;"
  command=$command"set xlabel 'Hardware threads';"
  command=$command"set ylabel 'Throughput (Mops)';"
  # command=$command"set yrange [0.0:1.2];"
  # command=$command"set xrange [0:130];"
  # command=$command"set xtics 0,16,130;"
  # command=$command"set ytics 0, 0.2, 1.2;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "tree_over_size" ];
then
  BENCHMARK_DIR="micro-benchmarks/versioning/results/tree/over_threads"
  BENCHMARK_NAME="tree_over_threads"
  FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
  command=$command"set key horizontal;"
  command=$command"set key center top;"
  command=$command"set key samplen 2;"
  command=$command"set xlabel 'Hardware threads';"
  command=$command"set ylabel 'Throughput (Mops)';"
  # command=$command"set yrange [0.0:1.2];"
  # command=$command"set xrange [0:130];"
  # command=$command"set xtics 0,16,130;"
  # command=$command"set ytics 0, 0.2, 1.2;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
elif [ "$2" == "tree_over_threads" ];
then
  BENCHMARK_DIR="micro-benchmarks/versioning/results/tree/over_size"
  BENCHMARK_NAME="tree_over_size"
  FILE_NAME="$1$BENCHMARK_NAME.pdf"

  command="set terminal pdf monochrome;"
  command=$command"set output '$FILE_NAME';"
  command=$command"set key horizontal;"
  command=$command"set key center top;"
  command=$command"set key samplen 2;"
  command=$command"set xlabel 'Tree size';"
  command=$command"set ylabel 'Throughput (Mops)';"
  # command=$command"set yrange [0.0:1.2];"
  # command=$command"set xrange [0:130];"
  # command=$command"set xtics 0,16,130;"
  # command=$command"set ytics 0, 0.2, 1.2;"
  command=$command"set key on horizontal;"
  command=$command"set datafile separator ',';"
else
  echo "Unknown option. Aborting."
  exit
fi

DIR="$1/$BENCHMARK_DIR/"
first=1
files="ffwd.csv ffwdx2.csv ffwd-skiplist.csv mcs.csv mcs-skiplist.csv mutex.csv ttas.csv ticket.csv clh.csv tas.csv hticket.csv harris.csv swisstm.csv fc.csv rcl.csv atomic.csv cc.csv dsm.csv sim.csv ms.csv lf.csv h.csv ffwd-s4.csv single_threaded.csv"
i=1
for f in $files
do
	if [ -f "$DIR/$f" ]; then
    if [ "$f" = "ffwd-skiplist.csv" ]; then
      pt="pt 12"
    elif [ "$f" = "swisstm.csv" ]; then
      pt="pt 12"
    elif [ "$f" = "ffwdx2.csv" ]; then
      pt="pt 0"
    elif [ "$f" = "mcs-skiplist.csv" ]; then
      pt="pt 13"
    elif [ "$f" = "harris.csv" ]; then
      pt="pt 11"
    elif [ "$f" = "mcs.csv" ]; then
      pt="pt 4"
    elif [ "$f" = "cc.csv" ]; then
      pt="pt 11"
    elif [ "$f" = "dsm.csv" ]; then
      pt="pt 12"
    elif [ "$f" = "h.csv" ]; then
      pt="pt 13"
    elif [ "$f" = "ffwd-s4.csv" ]; then
      pt="pt 9"
    elif [ "$f" = "single_threaded.csv" ]; then
      pt="pt 8"
    else
      pt="pt $i"
      if [ $i -eq 3 ]; then
        i=`expr $i + 2`
      else 
        i=`expr $i + 1`
      fi
    fi
		if [ $first -eq 1 ]; then
			command=$command"plot '$DIR/$f' using 1:2 title columnheader(1) with lp $pt lw 2 ps .75"
			first=0
		else
			command=$command", '$DIR/$f' using 1:2 title columnheader(1) with lp $pt lw 2 ps .75"
		fi
	fi
done
echo $command | gnuplot
echo "Plot has been generated in $FILE_NAME"
