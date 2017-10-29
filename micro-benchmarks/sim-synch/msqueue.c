#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "backoff.h"
#include "rand.h"
#include "thread.h"

#define POOL_SIZE                  1024

int MIN_BAK;
int MAX_BAK;


typedef struct Node {
  Object value;
  struct Node *next;
} Node;

typedef union pointer_t {
    struct{
        Node *ptr;
        long seq;
    } sep;
    long double con;
} pointer_t;


volatile Node *head CACHE_ALIGN;
volatile Node *tail CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;

__thread Node *pool_node = null;
__thread int_fast32_t pool_node_index = 0;
__thread BackoffStruct backoff;


void SHARED_OBJECT_INIT(void) {
    Node *p = getMemory(sizeof(Node));
    p->next = null;
    head = p;
    tail = p;
    return;
}


inline void enqueue(Object arg, int pid) {
    Node *p;
    Node *next, *last;

    if ( (pool_node_index - 1) < 0) {
        pool_node = getMemory(POOL_SIZE * sizeof(Node));
        pool_node_index = POOL_SIZE;
    }
    pool_node_index -= 1;
    p = &pool_node[pool_node_index];

    p->value = arg;                       
    p->next = null;  
    reset_backoff(&backoff);
    while (true) {
        last = (Node *)tail;
        next = last->next;
        if (last == tail) {
            if (next == null) { 
                reset_backoff(&backoff);
                if (CASPTR(&last->next, next, p))
                    break;
            }
            else {
                CASPTR(&tail, last, next);
                backoff_delay(&backoff);
            }
        }
    }
    CASPTR(&tail, last, p);

    return;
}

inline Object dequeue(int pid) {
    Node *first, *last, *next;
    Object value;

    reset_backoff(&backoff);
    while (true) {
        first = (Node *)head;
        last = (Node *)tail;
        next = first->next;
        if (first == head) {
             if (first == last) {
                 if (next == null) return -1;
                 CASPTR(&tail, last, next);
                 backoff_delay(&backoff);
             }
             else {
                  value = next->value;
                  if (CASPTR(&head, first, next))
                      break;
                  backoff_delay(&backoff);
             } 
        } 
     }
     return value;
}


pthread_barrier_t barr;


inline void Execute(void* Arg) {
    long i;
    long id = (long) Arg;
    long rnum;
    volatile long j;

    setThreadId(id);
    _thread_pin(id);
    init_backoff(&backoff, MIN_BAK, MAX_BAK, 1);
    simSRandom(id + 1);
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
        enqueue(id, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        dequeue(id);
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
    int counter = 0;

    while(head != null) {
        head = head->next;
        counter++;
    }
    fprintf(stderr, "%d nodes were left in the queue!\n", counter - 1);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
