#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "clh.h"
#include "thread.h"


typedef struct ObjectState {
    long long state;
} ObjectState;

CLHLockStruct *object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;

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

inline void apply_op(RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid) {
    clhLock(object_lock, pid);
    sfunc(arg, pid);
    clhUnlock(object_lock, pid);
}

// Global Variables
pthread_barrier_t barr;


inline void Execute(void* Arg) {
    long i, rnum;
    volatile long j;
    long id = (long) Arg;
    
    setThreadId(id);
    _thread_pin(id);
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif

#ifdef sun
    schedule_control = schedctl_init();
#endif
    simSRandom(id + 1L);
    if (id == 0)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
#if defined(__sun) || defined(sun)
        schedctl_start(schedule_control);
#endif
        apply_op(fetchAndMultiply, (ArgVal) i, (int)id);
#if defined(__sun) || defined(sun)
        schedctl_stop(schedule_control);
#endif
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
    }
    stop_cpu_counters(id);
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
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

    // Barrier initialization
    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    object_lock = clhLockInit();
    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    // printf("time: %d\t", (int) (d2 - d1));
    printf("%.3f\t", (double) (RUNS * 2 * N_THREADS) / (double) ((d2 - d1)*1000));

    printStats();

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
