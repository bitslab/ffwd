#!/bin/bash
cd memcached/libmemcached-1.0.2/
./configure
cd -

NPROC=`nproc`
cd memcached/memcached-1.4.6-ffwd/
./configure 'CFLAGS=-g -O3 -mcmodel=large -DT$(NPROC) -DFFWD -lrt -lnuma -lpthread -I../../include -Wno-error'
cd -

cd memcached/memcached-1.4.6-patched/
./configure 'CFLAGS=-g -O3'
cd -
