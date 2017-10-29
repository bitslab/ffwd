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
#define DEFAULT_UPDATE                  500

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

#ifdef USE_RBTREE

#include "rbtree.h"
#include "rbtree.c"

typedef rbtree_t intset_t;

intset_t *set_new()
{
  return rbtree_alloc();
}

void set_delete(intset_t *set)
{
  rbtree_free(set);
}

int set_size(intset_t *set)
{
  int size;
  node_t *n;

  if (!rbtree_verify(set, 0)) {
    printf("Validation failed!\n");
    exit(1);
  }

  size = 0;
  for (n = firstEntry(set); n != NULL; n = successor(n))
    size++;

  return size;
}

int set_add_seq(intset_t *set, intptr_t val) {
	return !rbtree_insert(set, val, val);
}

#ifdef MUBENCH_WLPDSTM
int set_add(intset_t *set, intptr_t val, tx_desc *tx)
{
//	wlpdstm_choose_tm_desc(tx, TM_MIXED);
	int res = 0;

	START_ID(0);
	res = !TMrbtree_insert(tx, set, val, val);

	COMMIT;

	return res;
}
#elif defined MUBENCH_TANGER || defined MUBENCH_SEQUENTIAL
int set_add(intset_t *set, intptr_t val)
{
	int res = 0;
	
	START_ID(0);
	res = !rbtree_insert(set, val, val);
	COMMIT;
	
	return res;
}
#endif /* MUBENCH_TANGER || MUBENCH_SEQUENTIAL */

#ifdef MUBENCH_WLPDSTM
int set_remove(intset_t *set, intptr_t val, tx_desc *tx)
{
	int res = 0;

//	wlpdstm_choose_tm_desc(tx, TM_EAGER);
    START_ID(1);
    res = TMrbtree_delete(tx, set, val);

    COMMIT;

	return res;
}
#elif defined MUBENCH_TANGER || defined MUBENCH_SEQUENTIAL
int set_remove(intset_t *set, intptr_t val)
{
	int res = 0;

	START_ID(1);
	res = rbtree_delete(set, val);
	COMMIT;
	
	return res;
}
#endif /* MUBENCH_TANGER || MUBENCH_SEQUENTIAL */

#ifdef MUBENCH_WLPDSTM
int set_contains(intset_t *set, intptr_t val, tx_desc *tx)
{
	int res = 0;

//	wlpdstm_choose_tm_desc(tx, TM_LAZY);
    START_RO_ID(2);
    res = TMrbtree_contains(tx, set, val);
    COMMIT;

	return res;
}
#elif defined MUBENCH_TANGER || defined MUBENCH_SEQUENTIAL
int set_contains(intset_t *set, intptr_t val)
{
	int res = 0;

	START_RO_ID(2);
	res = rbtree_contains(set, val);
	COMMIT;
	
	return res;
}
#endif /* MUBENCH_TANGER || MUBENCH_SEQUENTIAL */

#else /* USE_RBTREE */

/* ################################################################### *
 * INT SET
 * ################################################################### */

typedef struct node {
  int val;
  struct node *next;
} node_t;

typedef struct intset {
  node_t *head;
} intset_t;

#ifdef MUBENCH_WLPDSTM
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
#elif defined MUBENCH_TANGER || defined MUBENCH_SEQUENTIAL
node_t *new_node(int val, node_t *next)
{
	node_t *node;
	
	node = (node_t *)malloc(sizeof(node_t));

	if (node == NULL) {
		perror("malloc");
		exit(1);
	}
	
	node->val = val;
	node->next = next;
	
	return node;
}
#endif /* MUBENCH_TANGER || MUBENCH_SEQUENTIAL */

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
  set->head = min;

  return set;
}

void set_delete(intset_t *set)
{
  node_t *node, *next;

  node = set->head;
  while (node != NULL) {
    next = node->next;
    free(node);
    node = next;
  }
  free(set);
}

int set_size(intset_t *set)
{
  int size = 0;
  node_t *node;

  /* We have at least 2 elements */
  node = set->head->next;
  while (node->next != NULL) {
    size++;
    node = node->next;
  }

  return size;
}

int set_add_seq(intset_t *set, int val)
{
	int result;
	node_t *prev, *next;
	int v;
	
#ifdef DEBUG
	printf("++> set_add(%d)\n", val);
	IO_FLUSH;
#endif

    prev = (node_t *)(set->head);
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

#ifdef MUBENCH_WLPDSTM
int set_add(intset_t *set, int val, tx_desc *tx)
{
	int result;
	node_t *prev, *next;
	int v;

#ifdef DEBUG
	printf("++> set_add(%d)\n", val);
	IO_FLUSH;
#endif

    START_ID(3);
    prev = (node_t *)LOAD(&set->head);
    next = (node_t *)LOAD(&prev->next);
    while (1) {
      v = (int)LOAD(&next->val);
      if (v >= val)
        break;
      prev = next;
      next = (node_t *)LOAD(&prev->next);
    }
    result = (v != val);
    if (result) {
      STORE(&prev->next, new_node(val, next, tx));
    }
    COMMIT;

	return result;
}
#elif defined MUBENCH_TANGER || defined MUBENCH_SEQUENTIAL
int set_add(intset_t *set, int val)
{
	int result;
	node_t *prev, *next;
	int v;
	
#ifdef DEBUG
	printf("++> set_add(%d)\n", val);
	IO_FLUSH;
#endif
	
	START_ID(3);
	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val != val);
	if (result) {
		prev->next = new_node(val, next, tx);
	}
	COMMIT;
	
	return result;
}
#endif /* MUBENCH_TANGER || MUBENCH_SEQUENTIAL */

#ifdef MUBENCH_WLPDSTM
int set_remove(intset_t *set, int val, tx_desc *tx)
{
	int result;
	node_t *prev, *next;
	int v;
	node_t *n;

#ifdef DEBUG
	printf("++> set_remove(%d)\n", val);
	IO_FLUSH;
#endif

    START_ID(4);
    prev = (node_t *)LOAD(&set->head);
    next = (node_t *)LOAD(&prev->next);
    while (1) {
      v = (int)LOAD(&next->val);
      if (v >= val)
        break;
      prev = next;
      next = (node_t *)LOAD(&prev->next);
    }
    result = (v == val);
    if (result) {
      n = (node_t *)LOAD(&next->next);
      STORE(&prev->next, n);
      /* Free memory (delayed until commit) */
      FREE(next, sizeof(node_t));
    }
    COMMIT;

	return result;
}
#elif defined MUBENCH_TANGER || defined MUBENCH_SEQUENTIAL
int set_remove(intset_t *set, int val)
{
	int result;
	node_t *prev, *next;
	int v;
	node_t *n;
	
#ifdef DEBUG
	printf("++> set_remove(%d)\n", val);
	IO_FLUSH;
#endif
	
	START_ID(4)
	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val == val);
	if (result) {
		prev->next = next->next;
		free(next);
	}
	COMMIT;
	
	return result;
}
#endif /* MUBENCH_TANGER || MUBENCH_SEQUENTIAL */

#ifdef MUBENCH_WLPDSTM
int set_contains(intset_t *set, int val, tx_desc *tx)
{
	int result;
	node_t *prev, *next;
	int v;

#ifdef DEBUG
	printf("++> set_contains(%d)\n", val);
	IO_FLUSH;
#endif

    START_RO_ID(5);
    prev = (node_t *)LOAD(&set->head);
    next = (node_t *)LOAD(&prev->next);
    while (1) {
      v = (int)LOAD(&next->val);
      if (v >= val)
        break;
      prev = next;
      next = (node_t *)LOAD(&prev->next);
    }
    result = (v == val);
    COMMIT;

	return result;
}
#elif defined MUBENCH_TANGER || defined MUBENCH_SEQUENTIAL
int set_contains(intset_t *set, int val)
{
	int result;
	node_t *prev, *next;
	int v;
	
#ifdef DEBUG
	printf("++> set_contains(%d)\n", val);
	IO_FLUSH;
#endif
	
	START_RO_ID(5);
	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val == val);
	COMMIT;
	
	return result;
}
#endif /* MUBENCH_TANGER || MUBENCH_SEQUENTIAL */

#endif /* USE_RBTREE */

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
  int update;
  unsigned long nb_add;
  unsigned long nb_remove;
  unsigned long nb_contains;
  unsigned long nb_found;
  unsigned long nb_aborts;
  int diff;
  unsigned int seed;
  intset_t *set;
  barrier_t *barrier;
} thread_data_t;

void *test(void *data)
{
  int val;
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
    val = rand_r(&d->seed) % 1000;
    if (val < d->update) {
      if (val < d->update / 2) {
        /* Add random value */
        val = (rand_r(&d->seed) % d->range);
        if (set_add(d->set, val TM_ARG_LAST)) {
          d->diff++;
        }
        d->nb_add++;
      } else {
        /* Remove random value */
        val = (rand_r(&d->seed) % d->range);
        if (set_remove(d->set, val TM_ARG_LAST)) {
          d->diff--;
        }
        d->nb_remove++;
      }
    } else {
      /* Look for random value */
      val = (rand_r(&d->seed) % d->range);
      if (set_contains(d->set, val TM_ARG_LAST))
        d->nb_found++;
      d->nb_contains++;
    }
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
    {"update-rate",               required_argument, NULL, 'u'},
    {NULL, 0, NULL, 0}
  };

  intset_t *set;
  int i, c, val, size;
  unsigned long reads, updates;
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
  int update = DEFAULT_UPDATE;

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
              "  -u, --update-rate <int>\n"
              "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
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
     case 'u':
       update = atoi(optarg);
       break;
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
  assert(update >= 0 && update <= 1000);

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
  printf("Update rate  : %d\n", update);

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
/*
  printf("Adding %d entries to set\n", initial);
  for (i = 0; i < initial; i++) {
    val = (rand() % range) + 1;
    set_add_seq(set, val);
  }
  size = set_size(set);
  printf("Set size     : %d\n", size);
*/

#ifdef USE_RBTREE
	for (i = 0; i < initial;) {
		val = rand() % range;
		if (set_add_seq(set, val))
			i++;
	}
	size = i;
#else
	for (i = 0; i < range; i += range / initial)
		set_add_seq(set, i);
	size = set_size(set);
  printf("Set size     : %d\n", size);
#endif

  /* Access set from all threads */
  barrier_init(&barrier, nb_threads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread %d\n", i);
    data[i].range = range;
    data[i].update = update;
    data[i].nb_add = 0;
    data[i].nb_remove = 0;
    data[i].nb_contains = 0;
    data[i].nb_found = 0;
    data[i].diff = 0;
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
  reads = 0;
  updates = 0;
  for (i = 0; i < nb_threads; i++) {
    printf("Thread %d\n", i);
    printf("  #add        : %lu\n", data[i].nb_add);
    printf("  #remove     : %lu\n", data[i].nb_remove);
    printf("  #contains   : %lu\n", data[i].nb_contains);
    printf("  #found      : %lu\n", data[i].nb_found);
    reads += data[i].nb_contains;
    updates += (data[i].nb_add + data[i].nb_remove);
    size += data[i].diff;
  }
  printf("Set size      : %d (expected: %d)\n", set_size(set), size);
  printf("Duration      : %d (us)\n", duration);
  printf("#txs          : %lu (%f / s)\n", reads + updates, (reads + updates) * 1000000.0 / duration);
  printf("#read txs     : %lu (%f / s)\n", reads, reads * 1000000.0 / duration);
  printf("#update txs   : %lu (%f / s)\n", updates, updates * 1000000.0 / duration);
  printf("Avg tx time (ns) : %f (%f / %d) \n", (duration * 1000.0) / (reads + updates), (duration * 1000.0), (reads + updates));


  TM_SHUTDOWN();

  free(threads);
  free(data);

  return 0;
}
