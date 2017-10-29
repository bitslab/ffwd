#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "clh.h"
#include "thread.h"
#include "p.h"
#include "locks.h"
#include "liblock-fatal.h"
#include "liblock-config.h"
#include "htlock.h"

#ifdef FFWD
    #include "ffwd.h"
#endif

volatile int g_number_of_finished_clients = 0;

#ifdef FFWD
    PoolStruct *pool_node_server;
#endif

int counter = 0;
ListNode guard CACHE_ALIGN = {0, null};
volatile ListNode *Head CACHE_ALIGN = &guard;
int64_t d1 CACHE_ALIGN, d2;
    
#ifndef FFWD
    __thread PoolStruct pool_node;
#endif

inline static void push_CS(ListNode *n) {
    n->next = Head;
    Head = n;
}

#ifdef MCS
    inline static void push(PoolStruct *pool_node, int pid, struct ffwd_context *context, mcs_lock_t * the_lock) {
        volatile ListNode *n = alloc_obj(pool_node);
        n->value = (Object)pid;
        LOCK(lhead);   // Critical section
        n->next = Head;
        Head = n;
        UNLOCK(lhead);
    }

    inline static Object pop(PoolStruct *pool_node, int pid, mcs_lock_t * the_lock) {
        Object result;

        LOCK(lhead);
        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }
        UNLOCK(lhead);
        free_obj(pool_node, 0);

        return result;
    }
#elif FLATRCL
    inline static void push(PoolStruct *pool_node, int pid, struct ffwd_context *context) {
        volatile ListNode *n = alloc_obj(pool_node);
        n->value = (Object)pid;
        liblock_exec(&(lhead), &push_CS, (void *)n);
    }

    inline static Object pop(PoolStruct *pool_node) {
        Object result;

        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }

        return result;
    }
#elif FFWD
    inline static void push(int pid) {
        volatile ListNode *n = alloc_obj(pool_node_server);
        n->value = (Object)pid;

        n->next = Head;
        Head = n;
    }

    inline static Object pop(int pid) {
        Object result;
        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }
        free_obj(pool_node_server, 0);

        return result;
    }
#else
    inline static void push(PoolStruct *pool_node, int pid, struct ffwd_context *context) {
        volatile ListNode *n = alloc_obj(pool_node);
        n->value = (Object)pid;
        LOCK(lhead);   // Critical section
        n->next = Head;
        Head = n;
        UNLOCK(lhead);
    }

    inline static Object pop(PoolStruct *pool_node, int pid) {
        Object result;

        LOCK(lhead);
        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }
        UNLOCK(lhead);
        free_obj(pool_node, 0);

        return result;
    }
#endif

pthread_barrier_t barr;


inline static void Execute(void* Arg) {
    long i;
    long rnum;

    #ifdef FFWD
        GET_CONTEXT()
        // volatile struct request* myrequest = context->request;
        long id = context->id;
    #else
        long id = (long) Arg;
    #endif

        uint32_t htlock_node_mine, htlock_id_mine;

    #ifdef MCS
        mcs_lock_t *the_lock;
        the_lock = malloc (sizeof(mcs_lock_t));
        atomic_barrier();
    #endif

    #ifdef FLATRCL
      while(!g_server_started)
        __asm__ __volatile__ ("rep;nop;" ::: "memory");  
    #endif

    volatile int j;

    setThreadId(id);

    #ifdef FFWD
    #elif FLATRCL
    #else
        _thread_pin(id);
    #endif

    simSRandom(id + 1);

    #ifndef FFWD
        init_pool(&pool_node, sizeof(ListNode));
    #endif

    #ifndef FLATRCL
        if (id == N_THREADS - 1)
            d1 = getTimeMillis();
    #endif
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }

    #ifdef FLATRCL
        int randno = rand() % 128;
        id = (rand()+5003) % N_THREADS;
    #endif

    int return_value;

    // start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        #ifdef FFWD
            FFWD_EXEC(0, &push, return_value, 1, id)
        #elif FLATRCL
            push(&pool_node, id, 0);
        #elif MCS
            push(&pool_node, id, 0, the_lock);
        #else
            push(&pool_node, id, 0);
        #endif
            
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        #ifdef FFWD
            FFWD_EXEC(0, &pop, return_value, 1, id)
        #elif FLATRCL
            liblock_exec(&(lhead), &pop, (void *)&pool_node);
            free_obj(&pool_node, 0);
        #elif MCS
            pop(&pool_node, id, the_lock);
        #else
            pop(&pool_node, id);
        #endif
            
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    #ifdef FLATRCL
    if(__sync_add_and_fetch(&g_number_of_finished_clients, 1) == N_THREADS)
    {
          liblock_lock_destroy(&(lhead));
    }
    #endif
    // stop_cpu_counters(id);
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
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

    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    #ifdef FFWD
        pool_node_server = (PoolStruct*)numa_alloc_onnode(sizeof(PoolStruct), 0);
        init_pool(pool_node_server, sizeof(ListNode));
        ffwd_init();
        launch_servers(1);
    #elif FLATRCL
        int j;
        liblock_start_server_threads_by_hand = 1;
        liblock_servers_always_up = 0;
        g_server_started = 0;
        struct core cores[128];
        for (j=0; j<128; j++){
            cores[j].core_id = j;
            cores[j].server_type = 0;
            cores[j].frequency = 2200;
        }
        struct core* liblock_server_core_1;
        liblock_server_core_1 = &topology->cores[0];
        liblock_bind_thread(pthread_self(),
                            liblock_server_core_1,
                            TYPE_NOINFO);

        liblock_define_core(liblock_server_core_1);
        liblock_lock_init(TYPE_NOINFO, liblock_server_core_1, &(lhead), NULL);
    #elif HTICKET

        create_htlock(&lhead);
        create_htlock(&ltail);
    #else
        INIT_LOCK(lhead)
    #endif

    #ifdef FLATRCL
        for (i=1; i<= N_THREADS; i++)
        {
            int err = liblock_thread_create_and_bind(&topology->cores[i], 0, &(thread_id[i]), NULL, &EntryPoint, NULL);

            if (err != 0)
                printf("\ncan't create thread :[%s]", strerror(err));
        }
        d1 = getTimeMillis();
        liblock_lookup(TYPE_NOINFO)->run(server_started);
    #else
        for (i = 0; i < N_THREADS; i++){
            #ifdef FFWD
                ffwd_thread_create(&threads[i],0,EntryPoint,0);
            #else
                threads[i] = StartThread(i);
            #endif
        }
    #endif

    #ifdef FLATRCL
        for (i = 1; i <= N_THREADS; i++){
            pthread_join(thread_id[i], NULL);
        }
    #else
        for (i = 0; i < N_THREADS; i++)
            pthread_join(threads[i], NULL);
    #endif
    d2 = getTimeMillis();
    #ifdef FFWD
        ffwd_shutdown();
    #endif

    // printf("time: %d\t", (int) (d2 - d1));
    printf("%.3f\t", (double) (RUNS * 2 * N_THREADS) / (double) ((d2 - d1)*1000));

    
    printStats();
    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
