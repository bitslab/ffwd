#!/bin/sh

set -x

timestamp=`date +%Y.%m.%d.%H.%M.%S`
dir=$0.$timestamp
mkdir $dir

duration=1000

for i in benchmark_list_harris \
         benchmark_list_rcu \
         benchmark_list_rlu \
         benchmark_list_swisstm \
         benchmark_list_vlist \
         benchmark_list_ffwd
do
  for init_size in 1000
    do
    range=$(echo $init_size+$init_size | bc)
    for u in 100 500 1000
      do
      for ncpu in 1 4 8 12 16 20 24 28 32 36 40 44 48
        do
        for j in $(seq 5)
        do
          ./$i -d $duration -u $u -i $init_size -r $range -n $ncpu
        done
      done | tee $dir/$i.size${init_size}.u${u}
    done
  done

done

for i in benchmark_list_move_vlist \
         benchmark_list_move_rlu \
         benchmark_list_move_swisstm \
         benchmark_list_move_ffwd
do
  for init_size in 1000
    do
    range=$(echo $init_size+$init_size | bc)
      for ncpu in 1 4 8 12 16 20 24 28 32 36 40 44 48
        do
        for j in $(seq 5)
        do
          ./$i -d $duration -i $init_size -r $range -n $ncpu
        done
      done | tee $dir/$i.size${init_size}
  done

done

for i in benchmark_tree_vrbtree \
         benchmark_tree_vtree \
         benchmark_tree_rlu \
         benchmark_tree_rcu \
         benchmark_tree_bonsai \
         benchmark_tree_swisstm \
         benchmark_tree_ffwd \
         benchmark_tree_rcl
do
  for init_size in 100000
    do
    range=$(echo $init_size+$init_size | bc)
    for u in 100 500 1000
      do
      for ncpu in 1 4 8 12 16 20 24 28 32 36 40 44 48
        do
        for j in $(seq 5)
        do
          ./$i -d $duration -u $u -i $init_size -r $range -n $ncpu
        done
      done | tee $dir/$i.size${init_size}.u${u}
    done
  done

done
