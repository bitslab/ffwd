#!/bin/bash

EXP="radiosity raytrace-car raytrace-balls4"

. ../figures/bench.inc.sh

benchs="$RCL mcs spinlock flat posix"

compute_var() {
		BASE=$1
		M=$2
		S=0
		N=0
		while read line; do
				let N=N+1
				S=$(echo "$S + ($BASE/$line - $M)^2" | bc -l)
		done
		echo "sqrt($S/$N)" | bc -l
}

compute_acceleration() {
		BASE=$1
		BENCH=$2
		LOCK=$3
		while read line; do
				N=`echo $line | cut -d',' -f1`
				V=`echo $line | cut -d',' -f2`
				A=`echo $BASE/$V | bc -l`
				E=$(compute_var $BASE $A < results/splash2-$BENCH-$LOCK-$N.csv)
				echo $N, $A, $E
		done
}

build_data() {
for n in $EXP; do #
 		BASE=$(cat results/splash2-$n-posix.csv | head -n 1 | cut -d ',' -f2)
  	for t in $benchs; do
				real_n=$n
				if [ $n = "radiosity" ]; then
						if [ $t = "rcl" ]; then
								real_n=$n
						fi
				fi
  			compute_acceleration $BASE $real_n $t < results/splash2-$real_n-$t.csv > results/acceleration-$n-$t.csv
  	done
done
}

build_data

plot() {
 		exp=$1
 		first=$2
		has_title=$3
		id=$4
 		name=$5
		aspect=$8

		if [ $first = 1 ]; then
				echo -n "plot "
		else
				echo ",\\"
 				echo -n "     "
 		fi

 		file="results/acceleration-$exp-$id.csv"

		echo -n \"$file\" using 1:2:3 axes x1y1 title \"$name\" with yerrorlines $aspect
}

header() {
		cat ../figures/model.plot

		echo set ytics auto;
		echo set xtics auto;

#set size ratio 0.5

		echo set xrange[1:48];

		echo set xlabel '"Number of cores"';
		echo set ylabel '"Speedup"';
		echo set y2label "";
}

generate_figure() {
		exp=$1
		has_title=$2
		first=1

		header

		if [ $has_title = 1 ]; then
#				echo set key default;
				echo set key width 2;
				echo set key samplen 3;
				echo set key font "\"Helvetica,8\""
				echo set key outside;
				echo set key center;
				echo set key top;
				echo set key horizontal;
#				echo set key maxcols 10;
		else
				echo set key off
		fi

		for b in $benchs; do 
				on_bench $b plot $exp $first $has_title
				first=0
		done
		echo
}

for i in $EXP; do
		generate_figure $i 0 | gnuplot > splash2-$i.pdf
done

generate_figure $i 1 | gnuplot > splash2-labels.pdf
