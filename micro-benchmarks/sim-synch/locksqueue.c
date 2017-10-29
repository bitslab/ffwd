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

#define POOL_SIZE                  1024

volatile int g_number_of_finished_clients = 0;

#ifdef FFWD
    PoolStruct *pool_node_server;
#endif

ListNode guard CACHE_ALIGN = {null, 0};

volatile ListNode *Head CACHE_ALIGN = &guard;
volatile ListNode *Tail CACHE_ALIGN = &guard;
int64_t d1 CACHE_ALIGN, d2;


inline static void enqueue_cs(ListNode *n){
    Tail->next = n;
    Tail = n;
}

#ifdef MCS
    inline static void enqueue(PoolStruct *pool_node, int32_t arg, int pid, struct ffwd_context *context, mcs_lock_t * the_lock) {
        ListNode *n = alloc_obj(pool_node);

        n->value = (int32_t)arg;
        n->next = null;
        LOCK(ltail)
        Tail->next = n;
        Tail = n;
        UNLOCK(ltail)
    }

    inline static int32_t dequeue(int pid, mcs_lock_t * the_lock) {
        int32_t result;

        LOCK(lhead)
        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }
        UNLOCK(lhead)

        return result;
    }
#elif FLATRCL
        inline static void enqueue(PoolStruct *pool_node, int32_t arg, int pid, struct ffwd_context *context) {
            ListNode *n = alloc_obj(pool_node);

            n->value = (int32_t)arg;
            n->next = null;

            liblock_exec(&(ltail), &enqueue_cs, (void *)n);
           
        }
        inline static int32_t dequeue_flat(void* pid) {
            int32_t result;

            if (Head->next == null) 
                result = -1;
            else {
                result = Head->next->value;
                Head = Head->next;
            }
            return result;
        }
#elif FFWD
        inline static void enqueue(int pid) {
        ListNode *n = alloc_obj(pool_node_server);

        n->value = (int32_t)1;
        n->next = null;
        Tail->next = n;
        Tail = n;
    }

    inline static int32_t dequeue(int pid) {
        int32_t result;

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
    inline static void enqueue(PoolStruct *pool_node, int32_t arg, int pid, struct ffwd_context *context) {
        ListNode *n = alloc_obj(pool_node);

        n->value = (int32_t)arg;
        n->next = null;
        LOCK(ltail)
        Tail->next = n;
        Tail = n;
        UNLOCK(ltail)
    }

    inline static int32_t dequeue(int pid) {
        int32_t result;

        LOCK(lhead)
        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }
        UNLOCK(lhead)
        return result;
    }
#endif

pthread_barrier_t barr;

inline void Execute(void* Arg) {

    #ifdef FFWD
        GET_CONTEXT()
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

    long i;
    long rnum;
    volatile int j;

    #ifndef FFWD
       PoolStruct pool_node;
    #endif

    setThreadId(id);

    #ifdef FLATRCL

    #elif FFWD

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
    // printf("4\n");
    #ifdef FLATRCL
        int randno = rand() % 128;
        id = (rand()+5003) % N_THREADS;
    #endif

        int ret;
        
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }

    for (i = 0; i < RUNS; i++) {
        #ifdef FFWD
            FFWD_EXEC(0, &enqueue, ret, 1, id)
        #elif FLATRCL
            enqueue(&pool_node, (int32_t)1, id, 0);
        #elif MCS
            enqueue(&pool_node, (int32_t)1, id, 0, the_lock);
        #else
            enqueue(&pool_node, (int32_t)1, id, 0);
        #endif
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        #ifdef FFWD
            FFWD_EXEC(0, &dequeue, ret, 1, id)
        #elif FLATRCL
            liblock_exec(&(lhead), &dequeue_flat, (void *)&randno);
        #elif MCS
            dequeue(id, the_lock);
        #else
            dequeue(id);
        #endif
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    #ifdef FLATRCL
     if(__sync_add_and_fetch(&g_number_of_finished_clients, 1) == N_THREADS)
    {
          liblock_lock_destroy(&(lhead));
          liblock_lock_destroy(&(ltail));
    }
    #endif
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
        
        liblock_bind_thread(pthread_self(), liblock_server_core_1, TYPE_NOINFO);

        liblock_define_core(liblock_server_core_1);
        liblock_lock_init(TYPE_NOINFO, liblock_server_core_1, &(ltail), NULL);
        liblock_lock_init(TYPE_NOINFO, liblock_server_core_1, &(lhead), NULL);
    #elif HTICKET
        create_htlock(&lhead);
        create_htlock(&ltail);
    #else
        INIT_LOCK(ltail)
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
        for (i = 0; i < N_THREADS; i++){
            pthread_join(threads[i], NULL);
        }
    #endif
    d2 = getTimeMillis();

    // printf("time: %d\t", (int) (d2 - d1));
    printf("%.3f\t", (double) (RUNS * 2 * N_THREADS) / (double) ((d2 - d1)*1000));

    #ifdef FFWD
        ffwd_shutdown();
    #endif
    
    printStats();

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
