/*
 * File:
 *   test.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Concurrent accesses to the linked list integer set
 *
 * Copyright (c) 2009-2010.
 *
 * test.c is part of Synchrobench
 * 
 * Synchrobench is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "pthread.h"
#ifdef SIMPLE
#include "intset_simple.h"
#else
#include "intset.h"
#endif
#ifdef LIBLOCK
#include "liblock.h"
//#include "liblock-config.h"
ptlock_t g_liblock;
#endif

#ifdef SIMPLE
//lock_global_data g_lock;
#endif

#define ITERATIONS 100000
#define MAX_THREADS 128

#ifdef SSYNC
#ifdef CORES_128
#define CORES	128
#else
#define CORES 	64
#endif
#endif

int num_of_threads = 0;
volatile int g_server_started = 0;
volatile int g_number_of_finished_clients = 0;

void server_started() {
    g_server_started = 1;
}

typedef struct barrier {
  pthread_cond_t complete;
  pthread_mutex_t mutex;
  int count;
  int crossing;
} barrier_t;

void barrier_init(barrier_t *b, int n)
{
  pthread_cond_init(&b->complete, NULL);
  pthread_mutex_init(&b->mutex, NULL);
  b->count = n;
  b->crossing = 0;
}

void barrier_cross(barrier_t *b)
{
  pthread_mutex_lock(&b->mutex);
  /* One more thread through */
  b->crossing++;
  /* If not all here, wait */
  if (b->crossing < b->count) {
    pthread_cond_wait(&b->complete, &b->mutex);
  } else {
    pthread_cond_broadcast(&b->complete);
    /* Reset for next time */
    b->crossing = 0;
  }
  pthread_mutex_unlock(&b->mutex);
}

/* 
 * Returns a pseudo-random value in [1; range].
 * Depending on the symbolic constant RAND_MAX>=32767 defined in stdlib.h,
 * the granularity of rand() could be lower-bounded by the 32767^th which might 
 * be too high for given program options [r]ange and [i]nitial.
 *
 * Note: this is not thread-safe and will introduce futex locks
 */
#ifndef RAND_RANGE
#define RAND_RANGE
inline long rand_range(long r) {
  int m = RAND_MAX;
  int d, v = 0;
 
  do {
    d = (m > r ? r : m);
    v += 1 + (int)(d * ((double)rand()/((double)(m)+1.0)));
    r -= m;
  } while (r > 0);
  return v;
}
long rand_range(long r);
#endif

/* Thread-safe, re-entrant version of rand_range(r) */
inline long rand_range_re(unsigned int *seed, long r) {
  int m = RAND_MAX;
  int d, v = 0;
 
  do {
    d = (m > r ? r : m);		
    v += 1 + (int)(d * ((double)rand_r(seed)/((double)(m)+1.0)));
    r -= m;
  } while (r > 0);
  return v;
}
long rand_range_re(unsigned int *seed, long r);

typedef struct thread_data {
  val_t first;
  long range;
  int update;
  int unit_tx;
  int alternate;
  int effective;
  unsigned long nb_add;
  unsigned long nb_added;
  unsigned long nb_remove;
  unsigned long nb_removed;
  unsigned long nb_contains;
  unsigned long nb_found;
  unsigned long nb_aborts;
  unsigned long nb_aborts_locked_read;
  unsigned long nb_aborts_locked_write;
  unsigned long nb_aborts_validate_read;
  unsigned long nb_aborts_validate_write;
  unsigned long nb_aborts_validate_commit;
  unsigned long nb_aborts_invalid_memory;
  unsigned long max_retries;
  unsigned int seed;
#ifdef SSYNC
  int cpu;
#endif
  intset_l_t *set;
  barrier_t *barrier;
} thread_data_t;


void *test(void *data) 
{
  int unext, last = -1; 
  val_t val = 0;
  int ret = 0;

#ifdef LIBLOCK
  while(!g_server_started)
      PAUSE();
#endif

  thread_data_t *d = (thread_data_t *)data;
	
#if defined(SSYNC)
#ifdef USE_MCS_LOCKS
  set_cpu(d->cpu);
#endif
  lock_global_data l; // Just for avoiding compilation error. Important for HCLH & ARRAY locks, which we won't use.
  lock_local_data client_data1;
  lock_local_data client_data2;
  init_lock_local(d->cpu, &l, &client_data1);
  init_lock_local(d->cpu, &l, &client_data2);
#endif
  /* Wait on barrier */
  //barrier_cross(d->barrier);
	
  /* Is the first op an update? */
  unext = (rand_range_re(&d->seed, 100) - 1 < d->update);
		
  int iter = 0;
  while (iter++ < ITERATIONS) {
		
    //printf("Inside iter\n");
    if (unext) { // update
				
      if (last < 0) { // add
					
	val = rand_range_re(&d->seed, d->range);

#if defined(SSYNC)
	ret = set_add_l(d->set, val, TRANSACTIONAL, &client_data1, &client_data2);
#else
	ret = set_add_l(d->set, val, TRANSACTIONAL);
#endif

	if (ret == 1) 
	{
	  d->nb_added++;
	  last = val;
	} 				
#ifdef SIMPLE
	d->nb_add++;
#else
	if (ret != -2)
	{
		d->nb_add++;
	}
#endif
					
      } else { // remove
					
	if (d->alternate) { // alternate mode
						
#if defined(SSYNC)
	  ret = set_remove_l(d->set, last, TRANSACTIONAL, &client_data1, &client_data2);
#else
	  ret = set_remove_l(d->set, last, TRANSACTIONAL);
#endif
	  if (ret == 1)
	  {
	    d->nb_removed++;
	  }
	  last = -1;
						
	} else {
					
	  val = rand_range_re(&d->seed, d->range);
#if defined(SSYNC)
	  ret = set_remove_l(d->set, val, TRANSACTIONAL, &client_data1, &client_data2);
#else
	  ret = set_remove_l(d->set, val, TRANSACTIONAL);
#endif
	  if (ret == 1)
	  {
	    d->nb_removed++;
	    last = -1;
	  } 
	}
#ifdef SIMPLE
	d->nb_remove++;
#else
	if (ret != -2)
	{
		d->nb_remove++;
	}
#endif
      }
				
    } else { // read
				
      if (d->alternate) {
	if (d->update == 0) {
	  if (last < 0) {
	    val = d->first;
	    last = val;
	  } else { // last >= 0
	    val = rand_range_re(&d->seed, d->range);
	    last = -1;
	  }
	} else { // update != 0
	  if (last < 0) {
	    val = rand_range_re(&d->seed, d->range);
	    //last = val;
	  } else {
	    val = last;
	  }
	}
      }	else val = rand_range_re(&d->seed, d->range);
				
#if defined(SSYNC)
      if (set_contains_l(d->set, val, TRANSACTIONAL, &client_data1, &client_data2)) 
#else
      if (set_contains_l(d->set, val, TRANSACTIONAL)) 
#endif
		d->nb_found++;
      d->nb_contains++;			
    }
			
    /* Is the next op an update? */
    if (d->effective) { // a failed remove/add is a read-only tx
      unext = ((100 * (d->nb_added + d->nb_removed))
	       < (d->update * (d->nb_add + d->nb_remove + d->nb_contains)));
    } else { // remove/add (even failed) is considered an update
      unext = (rand_range_re(&d->seed, 100) - 1 < d->update);
    }
			
  }	
  
  if(__sync_add_and_fetch(&g_number_of_finished_clients, 1) == num_of_threads)
  {
    liblock_lock_destroy(&g_liblock);
  }

#ifdef SSYNC
  free_lock_local(client_data1);
  free_lock_local(client_data2);
#endif
  return NULL;
}

int main(int argc, char **argv)
{
  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"duration",                  required_argument, NULL, 'd'},
    {"initial-size",              required_argument, NULL, 'i'},
    {"thread-num",                required_argument, NULL, 't'},
    {"range",                     required_argument, NULL, 'r'},
    {"seed",                      required_argument, NULL, 'S'},
    {"update-rate",               required_argument, NULL, 'u'},
    {"unit-tx",                   required_argument, NULL, 'x'},
    {NULL, 0, NULL, 0}
  };
	
  intset_l_t *set;
  int i, c, size;
  val_t last = 0; 
  val_t val = 0;
  unsigned long reads, effreads, updates, effupds, aborts, aborts_locked_read, aborts_locked_write,
    aborts_validate_read, aborts_validate_write, aborts_validate_commit,
    aborts_invalid_memory, max_retries;
  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  barrier_t barrier;
  struct timeval start, end;
  struct timespec timeout;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = DEFAULT_NB_THREADS;
  long range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  int update = DEFAULT_UPDATE;
  int unit_tx = DEFAULT_LOCKTYPE;
  int alternate = DEFAULT_ALTERNATE;
  int effective = DEFAULT_EFFECTIVE;
  sigset_t block_set;
	
#ifdef LIBLOCK
  int j;
  liblock_start_server_threads_by_hand = 1;
  liblock_servers_always_up = 0;
  g_server_started = 0;

  struct core cores[MAX_THREADS];
  for (j=0; j<MAX_THREADS; j++){
      cores[j].core_id = j;
      cores[j].server_type = 0;
      cores[j].frequency = 2200;
  }

  /* Check liblock-config files */
  struct core* liblock_server_core_1;
  liblock_server_core_1 = &topology->cores[0];

  liblock_bind_thread(pthread_self(), liblock_server_core_1, TYPE_NOINFO);
  liblock_define_core(liblock_server_core_1);
  liblock_lock_init(TYPE_NOINFO, liblock_server_core_1, &g_liblock, NULL);
#endif

#ifdef SIMPLE
  //INIT_LOCK(&g_lock);
#endif
	
  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hAf:d:i:t:r:S:u:x:", long_options, &i);
		
    if(c == -1)
      break;
		
    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;
		
    switch(c) {
    case 0:
      /* Flag is automatically set */
      break;
    case 'h':
      printf("intset -- STM stress test "
	     "(linked list)\n"
	     "\n"
	     "Usage:\n"
	     "  intset [options...]\n"
	     "\n"
	     "Options:\n"
	     "  -h, --help\n"
	     "        Print this message\n"
	     "  -A, --alternate (default="XSTR(DEFAULT_ALTERNATE)")\n"
	     "        Consecutive insert/remove target the same value\n"
	     "  -f, --effective <int>\n"
	     "        update txs must effectively write (0=trial, 1=effective, default=" XSTR(DEFAULT_EFFECTIVE) ")\n"
	     "  -d, --duration <int>\n"
	     "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
	     "  -i, --initial-size <int>\n"
	     "        Number of elements to insert before test (default=" XSTR(DEFAULT_INITIAL) ")\n"
	     "  -t, --thread-num <int>\n"
	     "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
	     "  -r, --range <int>\n"
	     "        Range of integer values inserted in set (default=" XSTR(DEFAULT_RANGE) ")\n"
	     "  -S, --seed <int>\n"
	     "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
	     "  -u, --update-rate <int>\n"
	     "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
	     "  -x, --lock-based algorithm (default=1)\n"
	     "        Use lock-based algorithm\n"
	     "        1 = lock-coupling,\n"
	     "        2 = lazy algorithm\n"
	     );
      exit(0);
    case 'A':
      alternate = 1;
      break;
    case 'f':
      effective = atoi(optarg);
      break;			
    case 'd':
      duration = atoi(optarg);
      break;
    case 'i':
      initial = atoi(optarg);
      break;
    case 't':
      nb_threads = atoi(optarg);
      break;
    case 'r':
      range = atol(optarg);
      break;
    case 'S':
      seed = atoi(optarg);
      break;
    case 'u':
      update = atoi(optarg);
      break;
    case 'x':
      printf("The parameter x is not valid for this benchmark.\n");
      exit(0);
    case 'a':
      printf("The parameter a is not valid for this benchmark.\n");
      exit(0);
    case 's':
      printf("The parameter s is not valid for this benchmark.\n");
      exit(0);
    case '?':
      printf("Use -h or --help for help.\n");
      exit(0);
    default:
      exit(1);
    }
  }
	
  assert(duration >= 0);
  assert(initial >= 0);
  assert(nb_threads > 0);
  assert(range > 0 && range >= initial);
  assert(update >= 0 && update <= 100);
  num_of_threads = nb_threads;
	
#if 0
  printf("Set type     : lazy linked list\n");
  printf("Length       : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Thread num   : %d\n", nb_threads);
  printf("Value range  : %ld\n", range);
  printf("Seed         : %d\n", seed);
  printf("Update rate  : %d\n", update);
  printf("Lock alg     : %d\n", unit_tx);
  printf("Alternate    : %d\n", alternate);
  printf("Effective    : %d\n", effective);
  printf("Type sizes   : int=%d/long=%d/ptr=%d/word=%d\n",
	 (int)sizeof(int),
	 (int)sizeof(long),
	 (int)sizeof(void *),
	 (int)sizeof(uintptr_t));
#endif
  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;
	
  if ((data = (thread_data_t *)malloc((nb_threads+1) * sizeof(thread_data_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  if ((threads = (pthread_t *)malloc((nb_threads+1) * sizeof(pthread_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
	
  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);
	
	
  //stop = 0;
	
  /* Init STM */
  //printf("Initializing STM\n");
	
  /* Populate set */
  //printf("Adding %d entries to set\n", initial);
  i = 0;
#if defined(SSYNC) || defined(FFWD) || defined(LIBLOCK)
#ifdef USE_MCS_LOCKS
  set_cpu(0);
#endif
  //lock_local_data client_data;
  //lock_global_data l; // Just for avoiding compilation error. Important for HCLH & ARRAY locks, which we won't use.
  //init_lock_local(0, &l, &client_data);
  set = set_new_l();
  while (i < initial) {
    val = (rand() % range) + 1;
    if (set_add_l_2(set, val, 0)) {
      last = val;
      i++;
    }
  }
#else
  set = set_new_l();
  while (i < initial) {
    val = (rand() % range) + 1;
    if (set_add_l(set, val, 0)) {
      last = val;
      i++;
    }
  }
#endif
  size = set_size_l(set);
  //printf("Set size     : %d\n", size);
	
  /* Access set from all threads */
  //barrier_init(&barrier, nb_threads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (i = 1; i <= nb_threads; i++) {
    data[i].first = last;
    data[i].range = range;
    data[i].update = update;
    data[i].alternate = alternate;
    data[i].unit_tx = unit_tx;
    data[i].alternate = alternate;
    data[i].effective = effective;
    data[i].nb_add = 0;
    data[i].nb_added = 0;
    data[i].nb_remove = 0;
    data[i].nb_removed = 0;
    data[i].nb_contains = 0;
    data[i].nb_found = 0;
    data[i].nb_aborts = 0;
    data[i].nb_aborts_locked_read = 0;
    data[i].nb_aborts_locked_write = 0;
    data[i].nb_aborts_validate_read = 0;
    data[i].nb_aborts_validate_write = 0;
    data[i].nb_aborts_validate_commit = 0;
    data[i].nb_aborts_invalid_memory = 0;
    data[i].max_retries = 0;
    data[i].seed = rand();
    data[i].set = set;
    data[i].barrier = &barrier;
#ifdef SSYNC
	data[i].cpu = i%CORES;
#endif

#ifdef LIBLOCK
    //INIT_LOCK(&g_liblock);
    if ((liblock_thread_create_and_bind(&topology->cores[i], 0, &threads[i], &attr, test, (void *)(&data[i]))) != 0) {
      fprintf(stderr, "Error creating liblock thread\n");
      exit(1);
    }
#endif
  }

  pthread_attr_destroy(&attr);
  gettimeofday(&start, NULL);
  liblock_lookup(TYPE_NOINFO)->run(server_started);
	
  /* Wait for thread completion */
  for (i = 1; i <= nb_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
  }
  gettimeofday(&end, NULL);
	
  aborts = 0;
  aborts_locked_read = 0;
  aborts_locked_write = 0;
  aborts_validate_read = 0;
  aborts_validate_write = 0;
  aborts_validate_commit = 0;
  aborts_invalid_memory = 0;
  reads = 0;
  effreads = 0;
  updates = 0;
  effupds = 0;
  max_retries = 0;
  for (i = 1; i <= nb_threads; i++) {
#if 0
    printf("Thread %d\n", i);
    printf("  #add        : %lu\n", data[i].nb_add);
    printf("    #added    : %lu\n", data[i].nb_added);
    printf("  #remove     : %lu\n", data[i].nb_remove);
    printf("    #removed  : %lu\n", data[i].nb_removed);
    printf("  #contains   : %lu\n", data[i].nb_contains);
    printf("  #found      : %lu\n", data[i].nb_found);
    printf("  #aborts     : %lu\n", data[i].nb_aborts);
    printf("    #lock-r   : %lu\n", data[i].nb_aborts_locked_read);
    printf("    #lock-w   : %lu\n", data[i].nb_aborts_locked_write);
    printf("    #val-r    : %lu\n", data[i].nb_aborts_validate_read);
    printf("    #val-w    : %lu\n", data[i].nb_aborts_validate_write);
    printf("    #val-c    : %lu\n", data[i].nb_aborts_validate_commit);
    printf("    #inv-mem  : %lu\n", data[i].nb_aborts_invalid_memory);
    printf("  Max retries : %lu\n", data[i].max_retries);
#endif
    aborts += data[i].nb_aborts;
    aborts_locked_read += data[i].nb_aborts_locked_read;
    aborts_locked_write += data[i].nb_aborts_locked_write;
    aborts_validate_read += data[i].nb_aborts_validate_read;
    aborts_validate_write += data[i].nb_aborts_validate_write;
    aborts_validate_commit += data[i].nb_aborts_validate_commit;
    aborts_invalid_memory += data[i].nb_aborts_invalid_memory;

    reads += data[i].nb_contains;
    effreads += data[i].nb_contains + 
      (data[i].nb_add - data[i].nb_added) + 
      (data[i].nb_remove - data[i].nb_removed); 
    updates += (data[i].nb_add + data[i].nb_remove);
    effupds += data[i].nb_removed + data[i].nb_added; 
		
    //size += data[i].diff;
    size += data[i].nb_added - data[i].nb_removed;
    if (max_retries < data[i].max_retries)
      max_retries = data[i].max_retries;
  }
  //printf("Set size      : %d (expected: %d)\n", set_size_l(set), size);
  //printf("Duration      : %d (ms)\n", duration);
  /* Converting output to Mops */
	double trans = (double)(reads + updates);
  duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
	double ops = (double)(trans * 1000) / (double)duration;
	double mops = ops / 1000000LL;
	printf("%.3f\n", mops);
#if 0
  printf("#txs          : %lu (%f Mop/s)\n", reads + updates, (reads + updates) * 1000.0 / duration);
	
  printf("#read txs     : ");
  if (effective) {
    printf("%lu (%f Mop/s)\n", effreads, effreads * 1000.0 / duration);
    printf("  #contains   : %lu (%f Mop/s)\n", reads, reads * 1000.0 / duration);
  } else printf("%lu (%f Mop/s)\n", reads, reads * 1000.0 / duration);
	
  printf("#eff. upd rate: %f \n", 100.0 * effupds / (effupds + effreads));
	
  printf("#update txs   : ");
  if (effective) {
    printf("%lu (%f Mop/s)\n", effupds, effupds * 1000.0 / duration);
    printf("  #upd trials : %lu (%f Mop/s)\n", updates, updates * 1000.0 / 
	   duration);
  } else printf("%lu (%f Mop/s)\n", updates, updates * 1000.0 / duration);
#endif
  /* Delete set */
  //set_delete_l(set);
  //free_lock_local(client_data);
#ifdef SIMPLE
  //DESTROY_LOCK(&g_lock);
#endif
	
  free(threads);
  free(data);
	
  return 0;
}
