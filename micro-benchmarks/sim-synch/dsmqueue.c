#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>
#include <sched.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "dsmsynch.h"
#include "pool.h"
#include "thread.h"

#define  GUARD          INT_MIN


typedef struct Node {
    Object val;
    struct Node *next;
} Node;

typedef struct ObjectState {
    volatile Node *last CACHE_ALIGN;
    volatile Node *first CACHE_ALIGN;
} ObjectState;

int64_t d1 CACHE_ALIGN, d2;

volatile ObjectState sp CACHE_ALIGN;
Node guard_node CACHE_ALIGN = {GUARD, null};

LockStruct enqueue_lock CACHE_ALIGN;
LockStruct dequeue_lock CACHE_ALIGN;

__thread ThreadState lenqueue_lock;
__thread ThreadState ldequeue_lock;
__thread PoolStruct pool_node;

void SHARED_OBJECT_INIT(void) {
    sp.first = sp.last = &guard_node;
    lock_init(&enqueue_lock);   
    lock_init(&dequeue_lock);   
}

inline static RetVal enqueue(ArgVal arg, int pid) {
     Node *node = alloc_obj(&pool_node);
     node->next = null;
     node->val = arg;
     sp.last->next = node;
     sp.last = node;
     return -1;
}

inline static RetVal dequeue(ArgVal arg, int pid) {
        Node *node = (Node *)sp.first;
	if (sp.first->next != null){
            sp.first = sp.first->next;
            return node->val;
	} else {
            return -1;
     }
}

// Global Variables
pthread_barrier_t barr;

inline void Execute(void* Arg) {
    long i, rnum;
    volatile long j;
    long id = (long) Arg;

    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1);
    threadStateInit(&lenqueue_lock, &enqueue_lock, (int)id);
    threadStateInit(&ldequeue_lock, &dequeue_lock, (int)id);
    init_pool(&pool_node, sizeof(Node));

    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        // perform an enqueue operation
        applyOp(&enqueue_lock, &lenqueue_lock, enqueue, (ArgVal) 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
        // perform a dequeue operation
        applyOp(&dequeue_lock, &ldequeue_lock, dequeue, (ArgVal) 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
    }
    stop_cpu_counters(id);
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return NULL;
}

inline pthread_t StartThread(int arg) {
    long id = (long) arg;
    void *Arg = (void*) id;
    pthread_t thread_p;
    int thread_id;

    pthread_attr_t my_attr;
    pthread_attr_init(&my_attr);
    thread_id = pthread_create(&thread_p, &my_attr, EntryPoint, Arg);

    return thread_p;
}

int main(void) {
    pthread_t threads[N_THREADS];
    int i;

    init_cpu_counters();
    // Barrier initialization
    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    SHARED_OBJECT_INIT();
    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    // printf("time: %d\t", (int) (d2 - d1));
    printf("%.3f\t", (double) (RUNS * 2 * N_THREADS) / (double) ((d2 - d1)*1000));
    
    printStats();

#ifdef DEBUG
    fprintf(stderr, "enqueue state:    counter: %d\n", enqueue_lock.counter);
    fprintf(stderr, "dequeue state:    counter: %d\n", dequeue_lock.counter);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
