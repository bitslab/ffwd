#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "dsmsynch.h"
#include "pool.h"
#include "thread.h"

#define  GUARD          INT_MIN


typedef struct Node {
    Object val;
    volatile struct Node *next;
} Node;

Node guard = {GUARD, null};
volatile Node *head CACHE_ALIGN;
LockStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;

__thread PoolStruct pool_node;

void SHARED_OBJECT_INIT(void) {
    head = &guard;
    lock_init(&object_lock);   
}

inline static RetVal push(ArgVal arg, int pid) {
    Node *node = alloc_obj(&pool_node);
    node->next = head;
    node->val = arg;
    head = node;
    return -1;
}

inline static RetVal pop(ArgVal arg, int pid) {
     Node *node = (Node *)head;
     if (head->next == null)
         return -1;
     else { 
        head = head->next;
        return node->val;
     }
}

// Global Variables
pthread_barrier_t barr;

inline static void Execute(void* Arg) {
    ThreadState lobject_lock;
    long i, rnum;
    volatile long j;
    long id = (long) Arg;

    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1);
    threadStateInit(&lobject_lock, &object_lock, (int)id);
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
        applyOp(&object_lock, &lobject_lock, push, (ArgVal) 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
        // perform a dequeue operation
        applyOp(&object_lock, &lobject_lock, pop, (ArgVal) 1, id);
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

inline static pthread_t StartThread(int arg) {
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
    fprintf(stderr, "object state:    counter: %d\n", object_lock.counter);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
