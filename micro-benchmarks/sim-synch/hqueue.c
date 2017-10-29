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
#include "pool.h"
#include "thread.h"
#include "hqueue.h"

int64_t d1 CACHE_ALIGN, d2;

QueueHSynchStruct queue_object;

pthread_barrier_t barr;

inline void Execute(void* Arg) {
    long i, rnum;
    volatile int j;
    long id = (long) Arg;
    HQueueThreadState lqueue_struct;

    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1);
    hqueueThreadStateInit(&queue_object, &lqueue_struct, (int)id);

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
        applyEnqueue(&queue_object, &lqueue_struct, (ArgVal) id, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
        // perform a dequeue operation
        applyDequeue(&queue_object, &lqueue_struct, id);
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

    queueHSynchInit(&queue_object);   

    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    // printf("time: %d\t", (int) (d2 - d1));
    printf("%.3f\t", (double) (RUNS * 2 * N_THREADS) / (double) ((d2 - d1)*1000));
    
    
    printStats();

#ifdef DEBUG
    fprintf(stderr, "enqueue state:    counter: %d rounds: %d\n", queue_object.enqueue_struct.counter, queue_object.enqueue_struct.rounds);
    fprintf(stderr, "dequeue state:    counter: %d rounds: %d\n\n", queue_object.dequeue_struct.counter, queue_object.dequeue_struct.rounds);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
