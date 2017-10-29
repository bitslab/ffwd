#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "oyama.h"
#include "pool.h"
#include "thread.h"

#define  GUARD          LONG_MIN


volatile Object object CACHE_ALIGN;
volatile OyamaLockStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;


void SHARED_OBJECT_INIT(void) {
    object = 1;
    oyamaLockInit((OyamaLockStruct *)&object_lock);   
}

typedef union CRStruct {
    volatile Object obj;
    char pad[CACHE_LINE_SIZE];
} CRStruct;

CRStruct Critical[OBJECT_SIZE] CACHE_ALIGN;

inline RetVal fetchAndMultiply(ArgVal arg, int pid) {
    int i;

    for (i = 0; i < OBJECT_SIZE; i++)
        Critical[i].obj += 1;
    return Critical[0].obj;
}

// Global Variables
pthread_barrier_t barr;

inline static void Execute(void* Arg) {
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1);
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
        // perform a fetchAndMultiply operation
        applyOp((OyamaLockStruct *)&object_lock, fetchAndMultiply, (ArgVal) id, id);
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
    fprintf(stderr, "object counter: %d\n", object_lock.counter);
    fprintf(stderr, "rounds: %d\n", object_lock.rounds);
    fprintf(stderr, "Average helping: %f\n", (float)object_lock.counter/object_lock.rounds);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
