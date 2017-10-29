#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// ../splash2-work/codes/apps/barnes/code.c:352
// mutex 0 	locked 230 	cont 101 	l2 5.067

// *** Global->io_lock
#define TYPE_BARNES_1 TYPE_POSIX
#define ARG_BARNES_1 DEFAULT_ARG

// *** TODO: CellLock->CL
// splash2/codes/apps/barnes/code.c:430
#define TYPE_BARNES_17 TYPE_EXPERIENCE
#define ARG_BARNES_17 DEFAULT_ARG

// ../splash2-work/codes/apps/barnes/code.c:428
// mutex 2 	locked 545 	cont 253 	l2 5.439
// mutex 1516 	locked 317 	cont 29 	l2 1.861
// mutex 872 	locked 176 	cont 28 	l2 6.040
// mutex 1850 	locked 240 	cont 28 	l2 5.593
// mutex 1406 	locked 314 	cont 28 	l2 4.312
// mutex 1223 	locked 241 	cont 27 	l2 6.636
// mutex 169 	locked 310 	cont 26 	l2 2.333
// mutex 1222 	locked 298 	cont 23 	l2 3.081
// mutex 1866 	locked 323 	cont 22 	l2 -nan

#define TYPE_BARNES_2 TYPE_POSIX
#define ARG_BARNES_2 DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// ../splash2-work/codes/apps/fmm/memory.c:70
// mutex 12 	locked 1604 	cont 490 	l2 0.916
// mutex 11 	locked 1635 	cont 426 	l2 0.707
// mutex 13 	locked 1380 	cont 352 	l2 1.017
// mutex 14 	locked 1230 	cont 307 	l2 0.822
// mutex 9 	locked 1293 	cont 300 	l2 0.482
// mutex 8 	locked 1048 	cont 269 	l2 0.957
// mutex 21 	locked 1263 	cont 232 	l2 1.072
// mutex 27 	locked 1123 	cont 197 	l2 0.714
// mutex 17 	locked 1274 	cont 197 	l2 0.921
// mutex 22 	locked 1221 	cont 196 	l2 1.151

#define TYPE_FMM_1 TYPE_POSIX
#define ARG_FMM_1 DEFAULT_ARG

// *** G_Memory->lock_array
// TODO: splash2/codes/apps/fmm/memory.c:73
#define TYPE_FMM_17 TYPE_EXPERIENCE
#define ARG_FMM_17 DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// ../splash2-work/codes/apps/ocean/contiguous_partitions/main.c:430
// mutex 0 	locked 32 	cont 12 	l2 6.857

#define TYPE_OCEAN_1 TYPE_POSIX
#define ARG_OCEAN_1 DEFAULT_ARG

// ../splash2-work/codes/apps/ocean/contiguous_partitions/main.c:431
// mutex 1 	locked 192 	cont 126 	l2 7.024

#define TYPE_OCEAN_2 TYPE_POSIX
#define ARG_OCEAN_2 DEFAULT_ARG

// ../splash2-work/codes/apps/ocean/contiguous_partitions/main.c:432
// mutex 2 	locked 32 	cont 22 	l2 6.714

#define TYPE_OCEAN_3 TYPE_POSIX
#define ARG_OCEAN_3 DEFAULT_ARG

// ../splash2-work/codes/apps/ocean/contiguous_partitions/main.c:434
// mutex 4 	locked 6400 	cont 4154 	l2 6.715
// error_lock
#define TYPE_OCEAN_4 TYPE_POSIX
#define ARG_OCEAN_4 DEFAULT_ARG

// *** bar_lock
// TODO: splash2/codes/apps/ocean/contiguous_partitions/main.c:438
#define TYPE_OCEAN_CONT_17 TYPE_EXPERIENCE
#define ARG_OCEAN_CONT_17  DEFAULT_ARG

// ../splash2-work/codes/apps/ocean/non_contiguous_partitions/main.c:245
// mutex 0 	locked 32 	cont 11 	l2 5.857

#define TYPE_OCEAN_5 TYPE_POSIX
#define ARG_OCEAN_5 DEFAULT_ARG

// ../splash2-work/codes/apps/ocean/non_contiguous_partitions/main.c:247
// mutex 2 	locked 224 	cont 153 	l2 4.980

#define TYPE_OCEAN_6 TYPE_POSIX
#define ARG_OCEAN_6 DEFAULT_ARG

// ../splash2-work/codes/apps/ocean/non_contiguous_partitions/main.c:249
// mutex 4 	locked 6400 	cont 4234 	l2 6.221

#define TYPE_OCEAN_7 TYPE_POSIX
#define ARG_OCEAN_7 DEFAULT_ARG

// *** bar_lock
// TODO: splash2/codes/apps/ocean/contiguous_partitions/main.c:253
#define TYPE_OCEAN_NON_17 TYPE_EXPERIENCE
#define ARG_OCEAN_NON_17  DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// ../splash2-work/codes/apps/radiosity/elemman.c:2533
// mutex 0 	locked 261277 	cont 74138 	l2 1.546

/* free_interaction_lock (elemman.c) */
#define TYPE_RADIOSITY_1 TYPE_EXPERIENCE
#define ARG_RADIOSITY_1  get_server_core_1()

// *** q\_lock 
// splash2/codes/apps/radiosity/taskman.c:1903
#define TYPE_RADIOSITY_4 TYPE_POSIX
#define ARG_RADIOSITY_4  get_server_core_2()

// *** f\_lock
// splash2/codes/apps/radiosity/taskman.c:1903
#define TYPE_RADIOSITY_19 TYPE_POSIX
#define ARG_RADIOSITY_19  DEFAULT_ARG

// ../splash2-work/codes/apps/radiosity/rad_main.c:2287
// mutex 3996 	locked 2916 	cont 580 	l2 1.981

#define TYPE_RADIOSITY_2 TYPE_POSIX
#define ARG_RADIOSITY_2 DEFAULT_ARG

// ../splash2-work/codes/apps/radiosity/rad_main.c:2305
// mutex 3998 	locked 39820 	cont 2101 	l2 0.061

#define TYPE_RADIOSITY_3 TYPE_POSIX
#define ARG_RADIOSITY_3 DEFAULT_ARG

// ../splash2-work/codes/apps/radiosity/taskman.c:1783
// mutex 3 	locked 279844 	cont 244914 	l2 0.640
// mutex 33 	locked 6145 	cont 1488 	l2 0.117
// mutex 61 	locked 4824 	cont 622 	l2 0.354
// mutex 47 	locked 4815 	cont 488 	l2 0.684
// mutex 21 	locked 7425 	cont 409 	l2 1.032
// mutex 27 	locked 6887 	cont 406 	l2 0.386
// mutex 83 	locked 4456 	cont 388 	l2 0.428

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// *** raytrace -> ridlock
// splash2/codes/apps/raytrace/main.c:374
#define TYPE_RAYTRACE_17 TYPE_EXPERIENCE
#define ARG_RAYTRACE_17 get_server_core_1()

// *** raytrace -> memlock
// splash2/codes/apps/raytrace/main.c:375
#define TYPE_RAYTRACE_18 TYPE_POSIX
#define ARG_RAYTRACE_18  DEFAULT_ARG

// *** raytrace -> pidlock
// splash2/codes/apps/raytrace/main.c:373
#define TYPE_RAYTRACE_19 TYPE_POSIX
#define ARG_RAYTRACE_19  DEFAULT_ARG

// ../splash2-work/codes/apps/volrend/main.c:151
// mutex 1 	locked 736 	cont 533 	l2 24.972

#define TYPE_VOLREND_1 TYPE_POSIX
#define ARG_VOLREND_1 DEFAULT_ARG

// *** CountLock
// ../splash2-work/codes/apps/volrend/main.c:155
#define TYPE_VOLREND_17 TYPE_EXPERIENCE
#define ARG_VOLREND_17 DEFAULT_ARG

// ../splash2-work/codes/apps/volrend/main.c:158
// mutex 20 	locked 2201 	cont 490 	l2 0.768
// mutex 21 	locked 2227 	cont 486 	l2 0.827
// mutex 15 	locked 2111 	cont 484 	l2 0.825
// mutex 16 	locked 2122 	cont 461 	l2 0.851
// mutex 18 	locked 2152 	cont 459 	l2 0.858
// mutex 22 	locked 2246 	cont 435 	l2 0.737
// mutex 19 	locked 2176 	cont 413 	l2 0.776
// mutex 14 	locked 2096 	cont 395 	l2 0.885
// mutex 17 	locked 2140 	cont 383 	l2 0.826

#define TYPE_VOLREND_2 TYPE_POSIX
#define ARG_VOLREND_2 DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// *** gl->PotengSumLock
// splash2/codes/apps/water-nsquared/water.c:243
#define TYPE_WATER_NSQUARED_17 TYPE_EXPERIENCE
#define ARG_WATER_NSQUARED_17  DEFAULT_ARG

// ../splash2-work/codes/apps/water-nsquared/water.c:201

#define TYPE_WATER_NSQUARED_1 TYPE_POSIX
#define ARG_WATER_NSQUARED_1 DEFAULT_ARG

// ../splash2-work/codes/apps/water-nsquared/water.c:202
// mutex 2 	locked 184 	cont 92 	l2 4.361
// mutex 2 	locked 184 	cont 96 	l2 4.167

#define TYPE_WATER_NSQUARED_2 TYPE_POSIX
#define ARG_WATER_NSQUARED_2 DEFAULT_ARG

// ../splash2-work/codes/apps/water-nsquared/water.c:203
// mutex 3 	locked 184 	cont 78 	l2 4.389
// mutex 3 	locked 184 	cont 82 	l2 4.361

#define TYPE_WATER_NSQUARED_3 TYPE_POSIX
#define ARG_WATER_NSQUARED_3 DEFAULT_ARG

// ../splash2-work/codes/apps/water-nsquared/water.c:213
// mutex 114 	locked 96 	cont 10 	l2 6.028
// mutex 104 	locked 96 	cont 10 	l2 6.028
// mutex 90 	locked 96 	cont 9 	l2 6.111
// mutex 77 	locked 96 	cont 9 	l2 6.278
// mutex 69 	locked 96 	cont 9 	l2 6.028
// mutex 34 	locked 96 	cont 12 	l2 5.958
// mutex 100 	locked 96 	cont 12 	l2 5.958
// mutex 82 	locked 96 	cont 11 	l2 6.250
// mutex 58 	locked 96 	cont 11 	l2 6.250
// mutex 349 	locked 96 	cont 10 	l2 6.083

#define TYPE_WATER_NSQUARED_4 TYPE_POSIX
#define ARG_WATER_NSQUARED_4 DEFAULT_ARG

// ../splash2-work/codes/apps/water-nsquared/water.c:237
// mutex 7 	locked 414 	cont 160 	l2 1.815
// mutex 7 	locked 414 	cont 162 	l2 1.963

#define TYPE_WATER_NSQUARED_5 TYPE_POSIX
#define ARG_WATER_NSQUARED_5 DEFAULT_ARG

// ../splash2-work/codes/apps/water-nsquared/water.c:241
// mutex 8 	locked 46 	cont 14 	l2 5.556
// mutex 8 	locked 46 	cont 17 	l2 6.333

#define TYPE_WATER_NSQUARED_6 TYPE_POSIX
#define ARG_WATER_NSQUARED_6 DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// *** gl->PotengSumLock
// splash2/codes/apps/water-spatial/water.c:332
#define TYPE_WATER_SPATIAL_17 TYPE_EXPERIENCE
#define ARG_WATER_SPATIAL_17  DEFAULT_ARG

// ../splash2-work/codes/apps/water-spatial/stdio2.h:98
// mutex 69 	locked 46 	cont 24 	l2 10.222

#define TYPE_WATER_SPATIAL_1 TYPE_POSIX
#define ARG_WATER_SPATIAL_1 DEFAULT_ARG

// ../splash2-work/codes/apps/water-spatial/water.c:324
// mutex 65 	locked 46 	cont 16 	l2 4.889

#define TYPE_WATER_SPATIAL_2 TYPE_POSIX
#define ARG_WATER_SPATIAL_2 DEFAULT_ARG

// ../splash2-work/codes/apps/water-spatial/water.c:325
// mutex 66 	locked 184 	cont 86 	l2 6.722

#define TYPE_WATER_SPATIAL_3 TYPE_POSIX
#define ARG_WATER_SPATIAL_3 DEFAULT_ARG

// ../splash2-work/codes/apps/water-spatial/water.c:326
// mutex 67 	locked 184 	cont 94 	l2 6.917

#define TYPE_WATER_SPATIAL_4 TYPE_POSIX
#define ARG_WATER_SPATIAL_4 DEFAULT_ARG

// ../splash2-work/codes/apps/water-spatial/water.c:327
// mutex 68 	locked 414 	cont 163 	l2 2.198

#define TYPE_WATER_SPATIAL_5 TYPE_POSIX
#define ARG_WATER_SPATIAL_5 DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// ../splash2-work/codes/kernels/cholesky/mf.c:53
// mutex 34 	locked 1886 	cont 577 	l2 1.150
// mutex 14 	locked 1816 	cont 570 	l2 0.581
// mutex 13 	locked 1818 	cont 569 	l2 0.422
// mutex 36 	locked 1910 	cont 561 	l2 0.709
// mutex 37 	locked 1918 	cont 558 	l2 0.565
// mutex 35 	locked 1940 	cont 551 	l2 1.179
// mutex 29 	locked 1982 	cont 524 	l2 0.618
// mutex 12 	locked 1840 	cont 522 	l2 0.554
// mutex 30 	locked 1926 	cont 521 	l2 0.552
// mutex 21 	locked 1884 	cont 483 	l2 0.687

#define TYPE_CHOLESKY_1 TYPE_POSIX
#define ARG_CHOLESKY_1 DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// ../splash2-work/codes/kernels/fft/fft.c:325
// mutex 0 	locked 32 	cont 9 	l2 4.571

#define TYPE_FFT_1 TYPE_POSIX
#define ARG_FFT_1 DEFAULT_ARG

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

#ifndef TYPE_NOINFO
#define TYPE_NOINFO TYPE_POSIX
#define ARG_NOINFO DEFAULT_ARG
#endif

// ../splash2-work/codes/kernels/radix/radix.c:256
// mutex 0 	locked 32 	cont 10 	l2 4.143

#define TYPE_RADIX_1 TYPE_POSIX
#define ARG_RADIX_1 DEFAULT_ARG

// ../splash2-work/codes/kernels/radix/radix.c:276
// mutex 55 	locked 5 	cont 1 	l2 -nan
// mutex 49 	locked 5 	cont 1 	l2 4.200
// mutex 43 	locked 5 	cont 1 	l2 -nan
// mutex 42 	locked 12 	cont 1 	l2 -nan
// mutex 35 	locked 5 	cont 1 	l2 -nan
// mutex 34 	locked 10 	cont 1 	l2 5.000
// mutex 52 	locked 12 	cont 0 	l2 -nan
// mutex 46 	locked 12 	cont 0 	l2 1.000
// mutex 38 	locked 12 	cont 0 	l2 -nan

#define TYPE_RADIX_2 TYPE_POSIX
#define ARG_RADIX_2 DEFAULT_ARG

