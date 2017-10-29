/*
 *  linkedlist.h
 *  
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
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
#ifdef SSYNC
#include "lock_if.h"
#ifdef USE_MUTEX_LOCKS
#include <utils.h>
#endif
#endif
#include <atomic_ops.h>

#include "tm.h"

#ifdef DEBUG
#define IO_FLUSH                        fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define DEFAULT_DURATION                10000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   0x7FFFFFFF
#define DEFAULT_SEED                    0
#define DEFAULT_UPDATE                  20
#define DEFAULT_ALTERNATE								0
#define DEFAULT_EFFECTIVE								1
#define DEFAULT_ELASTICITY							4
#define DEFAULT_LOCKTYPE	    	2

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

extern lock_global_data g_lock;

/* SSYNC Functions */
#  		define ptlock_t lock_global_data
#  		define INIT_LOCK(lock) init_lock_global(lock)
#		ifndef USE_HTICKET_LOCKS
#  			define DESTROY_LOCK(lock) free_lock_global(*(lock))
#		else
#  			define DESTROY_LOCK(lock)
#		endif
#  		define LOCK(data, lock) acquire_lock(data, &(lock));
#  		define UNLOCK(data, lock) release_lock(data, &(lock));

#define ATOMIC_CAS_MB(a, e, v)          (AO_compare_and_swap_full((volatile AO_t *)(a), (AO_t)(e), (AO_t)(v)))

static volatile AO_t stop;

#define TRANSACTIONAL                   d->unit_tx

typedef intptr_t val_t;
#define VAL_MIN                         INT_MIN
#define VAL_MAX                         INT_MAX

typedef struct node {
	val_t val;
	struct node *next;
} node_t;

typedef struct intset {
	node_t *head;
	lock_global_data lock;
} intset_t;

typedef intset_t intset_l_t;

node_t *new_node(val_t val, node_t *next, int transactional);
intset_t *set_new();
void set_delete(intset_t *set);
int set_size(intset_t *set);
