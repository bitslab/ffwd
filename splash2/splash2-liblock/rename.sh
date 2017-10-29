#!/bin/bash


for F in $(find splash2 \( -name "*.h" -o -name "*.c" \)); do
		cat $F | sed -e 's/pthread_create/liblock_thread_create/' > $F.tmp
		\mv $F.tmp $F
done

