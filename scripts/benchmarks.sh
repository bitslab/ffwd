#!/bin/bash

DIR="results"
if [ "$#" -eq 1 ]; then
  DIR="$1"
fi

rm -rf $DIR

methods="ffwd mutex mcs ticket hticket ttas tas clh"
RESULTS_DIR="$DIR" ./simplelist_threads_exp.sh $methods

methods="ffwd mutex mcs ticket hticket ttas tas clh"
RESULTS_DIR="$DIR" ./hash_size_exp.sh $methods

methods="ffwd rcl fc mutex mcs ticket hticket ttas tas atomic clh"
RESULTS_DIR="$DIR" ./shared_vars_exp.sh $methods

methods="ffwd rcl fc mutex mcs ticket hticket ttas tas clh"
RESULTS_DIR="$DIR" ./lazylist_size_exp.sh $methods

methods="ffwd rcl fc mutex mcs ticket hticket ttas tas clh"
RESULTS_DIR="$DIR" ./lazylist_threads_exp.sh $methods

RESULTS_DIR="$DIR" ./memcached.sh
RESULTS_DIR="$DIR" ./liblock_memcached.sh

RESULTS_DIR="$DIR" ./radiosity.sh

RESULTS_DIR="$DIR" ./raytrace.sh

RESULTS_DIR="$DIR" ./liblock_splash2.sh

cd ../micro-benchmarks/versioning
OUTDIR="$DIR" ./run_all.sh
cd -

cd ../micro-benchmarks/skiplist
OUTDIR="$DIR" ./run.sh
cd -

cd ../micro-benchmarks/sim-synch
OUTDIR="$DIR" ./run_all.sh
cd -



