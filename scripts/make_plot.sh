#!/bin/bash

if [ "$#" -ne 1 ]; then
	echo "Usage: ./plot.sh <results directory name>"
	exit
fi

./plot.sh $1 "shared_vars"
./plot.sh $1 "simplelist_threads"
./plot.sh $1 "lazylist_threads"
./plot.sh $1 "lazylist_size"
./plot.sh $1 "hash_size"
./plot.sh $1 "radiosity"
./plot.sh $1 "raytrace_car"
./plot.sh $1 "raytrace_balls"
./plot.sh $1 "memcached_get"
./plot.sh $1 "memcached_set"
