==================
| ffwd benchmarks| 
==================
Sepideh Roghanchi, Jakob Eriksson, Nilanjana Basu (SOSP '17)

This file has instuctions on how to use the benchmarks provided in this repository, for more
information, please send an email to srogha2@uic.edu.

The ffwd code and the benchmarks are developed and tested on 4 different machines. To get the best result compatibility the machine specifications and dependencies are provided below:

AMD Opteron 6378
---------------------
uname -a: Linux 3.8.0-29-generic #42~precise1-Ubuntu SMP Wed Aug 14 16:19:23 UTC 2013 x86_64 x86_64 x86_64 GNU/Linux
gcc version: gcc (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3 
2.4 GHz


Intel Xeon E5-4620 
---------------------------
uname -a: Linux 4.0.7 #2 SMP Wed Jul 1 19:50:57 CDT 2015 x86_64 x86_64 x86_64 GNU/Linux
gcc version: gcc (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0
2.20 GHz


Intel Xeon E7-4820
---------------------------
uname -a: Linux 2.6.37.6 #408 SMP Sun Oct 9 22:07:58 PDT 2016 x86_64 x86_64 x86_64 GNU/Linux
gcc version: gcc (Ubuntu/Linaro 4.7.3-1ubuntu1) 4.7.3
2.00 GHz


Intel Xeon E5-4660 v4
---------------------------
uname -a: Linux 4.4.0-62-generic #83-Ubuntu SMP Wed Jan 18 14:10:15 UTC 2017 x86_64 x86_64 x86_64 GNU/Linux
gcc version: gcc (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0
2.20 GHz

============
Benchmarks
============
libnuma and PAPI need to be installed prior to compile the benchmarks.

The include directory contains the ffwd library and a simple sample code called "ffwd_sample.c":
to compile: 	make ffwd_sample
to run: 		./ffwd_sample -t NUM_THREADS -s NUM_SERVERS -d DURATION_IN_MS

For memcached benchmarks, run './configure.sh' in the current directory for once
In order to compile all the benchmarks, run 'make' or run 'make BENCHMARK_NAME' to compile a specific benchmark.

In the scripts directory, you can find scripts to run and plot the results. For each benchmark the results will be stored a directory named "results/" under the benchmarks directory.



