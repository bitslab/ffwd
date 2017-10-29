/*
 * File:
 *   intset.c
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *   Aleksandar Dragojevic <aleksandar.dragojevic@epfl.ch> (added support for wlpdstm)
 * Description:
 *   Integer set stress test.
 *
 * Copyright (c) 2007-2008.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#ifdef MUBENCH_WLPDSTM
#include <atomic_ops.h>
#endif /* MUBENCH_WLPDSTM */

#include "tm_spec.h"

#ifdef DEBUG
#define IO_FLUSH                        fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define DEFAULT_DURATION                1000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   512
#define DEFAULT_SEED                    0
//#define DEFAULT_UPDATE                  20

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

#ifdef MUBENCH_WLPDSTM
static volatile AO_t stop;
#else
static volatile int stop;
#endif /* MUBENCH_WLPDSTM */

/* ################################################################### *
 * INT SET
 * ################################################################### */

typedef struct node {
  int val;
  struct node *next;
} node_t;

typedef struct intset {
  node_t *head[2];
} intset_t;

node_t *new_node(int val, node_t *next, tx_desc *tx)
{
	node_t *node;

	if (tx == NULL)
	  node = (node_t *)malloc(sizeof(node_t));
	else
	  node = (node_t *)MALLOC(sizeof(node_t));

	if (node == NULL) {
		perror("malloc");
		exit(1);
	}

	node->val = val;
	node->next = next;

	return node;
}

intset_t *set_new()
{
  intset_t *set;
  node_t *min, *max;

  if ((set = (intset_t *)malloc(sizeof(intset_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  max = new_node(INT_MAX, NULL, NULL);
  min = new_node(INT_MIN, max, NULL);
  set->head[0] = min;
  max = new_node(INT_MAX, NULL, NULL);
  min = new_node(INT_MIN, max, NULL);
  set->head[1] = min;

  return set;
}

int set_add_seq(intset_t *set, int val, int head)
{
	int result;
	node_t *prev, *next;
	int v;

    prev = (node_t *)(set->head[head]);
    next = (node_t *)(prev->next);
    while (1) {
		v = (int)(next->val);
		if (v >= val)
			break;
		prev = next;
		next = (node_t *)(prev->next);
    }
    result = (v != val);
    if (result) {
      prev->next = new_node(val, next, NULL);
    }

	return result;
}

int set_move(intset_t *set, int val, int from, tx_desc *tx)
{
	int result = 0;
	node_t *prev1, *next1;
	node_t *prev2, *next2;
	int v;
	node_t *n;

	START_ID(6);
	prev1 = (node_t *)LOAD(&set->head[from]);
	next1 = (node_t *)LOAD(&prev1->next);
	while (1) {
		v = (int)LOAD(&next1->val);
		if (v >= val)
			break;
		prev1 = next1;
		next1 = (node_t *)LOAD(&prev1->next);
	}
	if (v == val) {
		prev2 = (node_t *)LOAD(&set->head[1 - from]);
		next2 = (node_t *)LOAD(&prev2->next);
		while (1) {
			v = (int)LOAD(&next2->val);
			if (v >= val)
				break;
			prev2 = next2;
			next2 = (node_t *)LOAD(&prev2->next);
		}
		if (v != val) {
			n = (node_t *)LOAD(&next1->next);
			STORE(&prev1->next, n);
			STORE(&next1->next, next2);
			STORE(&prev2->next, next1);
			result = 1;
		}
	}

	COMMIT;

	return result;
}

/* ################################################################### *
 * BARRIER
 * ################################################################### */

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

/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data {
  int range;
  //int update;
  //unsigned long nb_add;
  //unsigned long nb_remove;
  //unsigned long nb_contains;
  //unsigned long nb_found;
  unsigned long nb_move;
  unsigned long nb_aborts;
  unsigned int seed;
  intset_t *set;
  barrier_t *barrier;
} thread_data_t;

void *test(void *data)
{
  int val, from;
  thread_data_t *d = (thread_data_t *)data;

  /* init thread */
  TM_THREAD_ENTER();
  /* Wait on barrier */
  barrier_cross(d->barrier);

  unsigned long aborts = 0;

#ifdef MUBENCH_WLPDSTM
  while (AO_load_full(&stop) == 0) {
#else
  while (stop == 0) {
#endif /* MUBENCH_WLPDSTM */
#if 0
    val = rand_r(&d->seed) % 100;
    if (val < d->update) {
      if (val < d->update / 2) {
        /* Add random value */
        val = (rand_r(&d->seed) % d->range) + 1;
        if (set_add(d->set, val TM_ARG_LAST)) {
          d->diff++;
        }
        d->nb_add++;
      } else {
        /* Remove random value */
        val = (rand_r(&d->seed) % d->range) + 1;
        if (set_remove(d->set, val TM_ARG_LAST)) {
          d->diff--;
        }
        d->nb_remove++;
      }
    } else {
      /* Look for random value */
      val = (rand_r(&d->seed) % d->range) + 1;
      if (set_contains(d->set, val TM_ARG_LAST))
        d->nb_found++;
      d->nb_contains++;
    }
#else
    from = rand_r(&d->seed) % 2;
    val = rand_r(&d->seed) % d->range;
    set_move(d->set, val, from TM_ARG_LAST);
    d->nb_move++;
#endif
  }

  d->nb_aborts = aborts;

  TM_THREAD_EXIT();

  return NULL;
}

int main(int argc, char **argv)
{
  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"duration",                  required_argument, NULL, 'd'},
    {"initial-size",              required_argument, NULL, 'i'},
    {"num-threads",               required_argument, NULL, 'n'},
    {"range",                     required_argument, NULL, 'r'},
    {"seed",                      required_argument, NULL, 's'},
    //{"update-rate",               required_argument, NULL, 'u'},
    {NULL, 0, NULL, 0}
  };

  intset_t *set;
  //int i, c, val, size;
  int i, c;
  //unsigned long reads, updates;
  unsigned long moves;
  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  barrier_t barrier;
  struct timeval start, end;
  struct timespec timeout;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = DEFAULT_NB_THREADS;
  int range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  //int update = DEFAULT_UPDATE;

  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hd:i:n:r:s:u:", long_options, &i);

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
#ifdef USE_RBTREE
              "(red-black tree)\n"
#else
              "(linked list)\n"
#endif
              "\n"
              "Usage:\n"
              "  intset [options...]\n"
              "\n"
              "Options:\n"
              "  -h, --help\n"
              "        Print this message\n"
              "  -d, --duration <int>\n"
              "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
              "  -i, --initial-size <int>\n"
              "        Number of elements to insert before test (default=" XSTR(DEFAULT_INITIAL) ")\n"
              "  -n, --num-threads <int>\n"
              "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
              "  -r, --range <int>\n"
              "        Range of integer values inserted in set (default=" XSTR(DEFAULT_RANGE) ")\n"
              "  -s, --seed <int>\n"
              "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
              //"  -u, --update-rate <int>\n"
              //"        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
         );
       exit(0);
     case 'd':
       duration = atoi(optarg);
       break;
     case 'i':
       initial = atoi(optarg);
       break;
     case 'n':
       nb_threads = atoi(optarg);
       break;
     case 'r':
       range = atoi(optarg);
       break;
     case 's':
       seed = atoi(optarg);
       break;
     //case 'u':
       //update = atoi(optarg);
       //break;
     case '?':
       printf("Use -h or --help for help\n");
       exit(0);
     default:
       exit(1);
    }
  }

  assert(duration >= 0);
  assert(initial >= 0);
  assert(nb_threads > 0);
  assert(range > 0);
  //assert(update >= 0 && update <= 100);

#ifdef USE_RBTREE
  printf("Set type     : red-black tree\n");
#else
  printf("Set type     : linked list\n");
#endif
  printf("Duration     : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Nb threads   : %d\n", nb_threads);
  printf("Value range  : %d\n", range);
  printf("Seed         : %d\n", seed);
  //printf("Update rate  : %d\n", update);

  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;

  if ((data = (thread_data_t *)malloc(nb_threads * sizeof(thread_data_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  if ((threads = (pthread_t *)malloc(nb_threads * sizeof(pthread_t))) == NULL) {
    perror("malloc");
    exit(1);
  }

  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);

  set = set_new();

  stop = 0;

  /* Init STM */
  printf("Initializing STM\n");
  TM_STARTUP();

  /* Populate set */
	for ( i =0; i < range; i += range / initial) {
		set_add_seq(set, i, 0);
		set_add_seq(set, i + 1, 1);
	}
  //size = set_size(set);
  //printf("Set size     : %d\n", size);

  /* Access set from all threads */
  barrier_init(&barrier, nb_threads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread %d\n", i);
    data[i].range = range;
    //data[i].update = update;
    //data[i].nb_add = 0;
    //data[i].nb_remove = 0;
    //data[i].nb_contains = 0;
    //data[i].nb_found = 0;
    data[i].nb_move = 0;
    //data[i].diff = 0;
    data[i].seed = rand();
    data[i].set = set;
    data[i].barrier = &barrier;
    if (pthread_create(&threads[i], &attr, test, (void *)(&data[i])) != 0) {
      fprintf(stderr, "Error creating thread\n");
      exit(1);
    }
  }
  pthread_attr_destroy(&attr);

  /* Start threads */
  barrier_cross(&barrier);

  printf("STARTING...\n");
  gettimeofday(&start, NULL);
  if (duration > 0) {
    nanosleep(&timeout, NULL);
  }
#ifdef MUBENCH_WLPDSTM
  AO_store_full(&stop, 1);
#else
  stop = 1;
#endif /* MUBENCH_WLPDSTM */
  gettimeofday(&end, NULL);
  printf("STOPPING...\n");

  /* Wait for thread completion */
  for (i = 0; i < nb_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
  }

  duration = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
  //reads = 0;
  //updates = 0;
  moves = 0;
  for (i = 0; i < nb_threads; i++) {
    printf("Thread %d\n", i);
    //printf("  #add        : %lu\n", data[i].nb_add);
    //printf("  #remove     : %lu\n", data[i].nb_remove);
    //printf("  #contains   : %lu\n", data[i].nb_contains);
    //printf("  #found      : %lu\n", data[i].nb_found);
    printf("  #move  : %lu\n", data[i].nb_move);
    //reads += data[i].nb_contains;
    //updates += (data[i].nb_add + data[i].nb_remove);
    moves += data[i].nb_move;
    //size += data[i].diff;
  }
  //printf("Set size      : %d (expected: %d)\n", set_size(set), size);
  printf("Duration      : %d (us)\n", duration);
  printf("#txs          : %lu (%f / s)\n", moves, (moves) * 1000000.0 / duration);
  //printf("#txs          : %lu (%f / s)\n", reads + updates, (reads + updates) * 1000000.0 / duration);
  //printf("#read txs     : %lu (%f / s)\n", reads, reads * 1000000.0 / duration);
  //printf("#update txs   : %lu (%f / s)\n", updates, updates * 1000000.0 / duration);
  //printf("Avg tx time (ns) : %f (%f / %d) \n", (duration * 1000.0) / (reads + updates), (duration * 1000.0), (reads + updates));
  //printf("Avg ops per us : %f\n", ((double)reads + updates) / duration);
  printf("Avg ops per us : %f\n", ((double)moves) / duration);

  TM_SHUTDOWN();

  free(threads);
  free(data);

  return 0;
}
