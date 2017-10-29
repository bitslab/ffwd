#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "tvec.h"
#include "rand.h"
#include "backoff.h"
#include "thread.h"


pthread_barrier_t barr;
int64_t d1 CACHE_ALIGN, d2;
volatile ToggleVector active_set;


void SHARED_OBJECT_INIT(void) {

    TVEC_SET_ZERO((ToggleVector *)&active_set);
}


inline static void Execute(void* Arg) {
    long i, rnum, mybank;
    volatile long j;
    long id = (long) Arg;
    ToggleVector lactive_set;
    ToggleVector mystate;

    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1);
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }
    TVEC_SET_ZERO(&mystate);
    TVEC_SET_BIT(&mystate, id);
    mybank = TVEC_GET_BANK_OF_BIT(id);
    for (i = 0; i < RUNS; i++) {
        mystate = TVEC_NEGATIVE(mystate);
        TVEC_ATOMIC_ADD_BANK(&active_set, &mystate, mybank);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        lactive_set = active_set;
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
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

    printf("time: %d\t", (int) (d2 - d1));
    printStats();

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
