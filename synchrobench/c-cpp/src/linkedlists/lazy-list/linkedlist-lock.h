/*
 * File:
 *   linkedlist-lock.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Lock-based linked list implementation of an integer set
 *
 * Copyright (c) 2009-2010.
 *
 * linkedlist-lock.c is part of Synchrobench
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

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include "atomic_ops.h"

#ifdef SSYNC
#include "lock_if.h"
#ifdef USE_MUTEX_LOCKS
#include <utils.h>
#endif
#elif LIBLOCK
#include "liblock.h"
#include "liblock-config.h"
#endif

#define DEFAULT_DURATION                10000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   0x7FFFFFFF
#define DEFAULT_SEED                    0
#define DEFAULT_UPDATE                  20
#define DEFAULT_LOCKTYPE	    	2
#define DEFAULT_ALTERNATE	        0
#define DEFAULT_EFFECTIVE	 	1

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define ATOMIC_CAS_MB_FBAR(a, e, v)     (AO_compare_and_swap_full((volatile AO_t *)(a), (AO_t)(e), (AO_t)(v)))

#define ATOMIC_CAS_MB_NOBAR(a, e, v)    (AO_compare_and_swap((volatile AO_t *)(a), (AO_t)(e), (AO_t)(v)))

static volatile AO_t stop;

#define TRANSACTIONAL                   d->unit_tx

typedef intptr_t val_t;
#define VAL_MIN                         INT_MIN
#define VAL_MAX                         INT_MAX

#ifdef MUTEX
typedef pthread_mutex_t ptlock_t;
#  define INIT_LOCK(lock)				pthread_mutex_init((pthread_mutex_t *) lock, NULL);
#  define DESTROY_LOCK(lock)			pthread_mutex_destroy((pthread_mutex_t *) lock)
#  define LOCK(lock)					pthread_mutex_lock((pthread_mutex_t *) lock)
#  define UNLOCK(lock)					pthread_mutex_unlock((pthread_mutex_t *) lock)
#elif FFWD
typedef pthread_mutex_t ptlock_t; // Will not be used for ffwd. Kept for avoiding compilation errors.
#  define INIT_LOCK(lock)
#  define DESTROY_LOCK(lock)
#  define LOCK(lock)
#  define UNLOCK(lock)
#elif LIBLOCK
typedef liblock_lock_t ptlock_t;
#  define INIT_LOCK(lock)   //liblock_lock_init(TYPE_NOINFO, liblock_server_core_1, lock, NULL);
#  define DESTROY_LOCK(lock)
#  define LOCK(lock)
#  define UNLOCK(lock)
#elif SSYNC
#  		define ptlock_t lock_global_data
#  		define INIT_LOCK(lock) init_lock_global(lock)
#		ifndef USE_HTICKET_LOCKS
#  			define DESTROY_LOCK(lock) free_lock_global(*(lock))
#		else
#  			define DESTROY_LOCK(lock)
#		endif
#  		define LOCK(lock, data) acquire_lock(data, lock);
#  		define UNLOCK(lock, data) release_lock(data, lock);
#else
typedef pthread_spinlock_t ptlock_t;
#  define INIT_LOCK(lock)				pthread_spin_init((pthread_spinlock_t *) lock, PTHREAD_PROCESS_PRIVATE);
#  define DESTROY_LOCK(lock)			pthread_spin_destroy((pthread_spinlock_t *) lock)
#  define LOCK(lock)					pthread_spin_lock((pthread_spinlock_t *) lock)
#  define UNLOCK(lock)					pthread_spin_unlock((pthread_spinlock_t *) lock)
#endif

typedef struct node_l {
  val_t val;
  struct node_l *next;
  ptlock_t lock; // removed 'volatile' qualifier because of compiler warnings for ssync integration
#ifdef USE_MCS_LOCKS
  lock_local_data data;
#endif
} node_l_t;

typedef struct intset_l {
  node_l_t *head;
} intset_l_t;

node_l_t *new_node_l(val_t val, node_l_t *next, int transactional);
intset_l_t *set_new_l();
void set_delete_l(intset_l_t *set);
int set_size_l(intset_l_t *set);
void node_delete_l(node_l_t *node);


