#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "backoff.h"
#include "thread.h"


int MIN_BAK;
int MAX_BAK;

typedef int64_t ObjectState;

volatile ObjectState sp CACHE_ALIGN;

int64_t d1 CACHE_ALIGN, d2;
BackoffStruct backoff;
__thread long long max_latency = 0;

void SHARED_OBJECT_INIT(void) {
    sp = 1L;
    FullFence();
}


inline static long fetchAndMultiply(ObjectState *mod_sp, Object arg, int pid) {
     *mod_sp +=1;//*= arg;
     return (long)*mod_sp;
}

inline static void apply_op(long (*sfunc)(ObjectState *, Object, int), Object arg, int pid) {
    ObjectState mod_sp, lsp;


    //reset_backoff(&backoff);
    lsp = *((volatile ObjectState *)&sp);
    mod_sp = lsp;
    sfunc(&mod_sp, arg, pid);
    while(CAS64((int64_t *)&sp, (int64_t)lsp, (int64_t) mod_sp) == false) {
        //backoff_delay(&backoff);
        lsp = *((volatile ObjectState *)&sp);
        mod_sp = lsp;
        sfunc(&mod_sp, arg, pid);
    }
}

// Global Variables
pthread_barrier_t barr;


inline static void Execute(void* Arg) {
    long i, rnum;
    volatile long j;
    long id = (long) Arg;

    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1L);
    init_backoff(&backoff, MIN_BAK, MAX_BAK, 1);
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
        apply_op(fetchAndMultiply, (Object) (id + 1L), id);
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


int main(int argc, char *argv[]) {
    pthread_t threads[N_THREADS];
    int i;

    init_cpu_counters();
    if (argc != 3) {
        fprintf(stderr, "Please set upper and lower bound for backoff!\n");
        exit(EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &MIN_BAK);
        sscanf(argv[2], "%d", &MAX_BAK);
    }

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
    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}


