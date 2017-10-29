#!/bin/bash

EXP="radiosity raytrace-car raytrace-balls4"

. ../figures/bench.inc.sh

dest=../papers/usenix12/data

benchs="posix spinlock flat mcs $RCL"

get_name() {
		echo $2
}

for x in $EXP; do
		n=$(echo $x | sed -e 's/.dat//' | sed -e 's/-/\//')
		printf "%-20s" $n
		for b in $benchs; do
				file=results/acceleration-$x-$b.csv
				max=$(cat $file | cut -d',' -f2 | sort -n | tail -n 1)
				cat $file | grep $max | gawk -F',' '{ printf("%10.2f %10d ", $2, $1); }'
		done
		echo
# > $dest/$x.dat
done
