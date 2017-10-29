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
#include "pool.h"
#include "ffwd.h"
#include "macro.h"
#include "locks.h"
#include "liblock.h"
#include "liblock-fatal.h"
#include "liblock-config.h"

typedef struct ListNode {
    Object value;		      // initially, there is a sentinel node 
    volatile struct ListNode *next; 	        // in the queue where Head and Tail point to.
} ListNode;	

// CLHLockStruct *lhead CACHE_ALIGN;

#ifdef MUTEX
    struct mutex_lock {
        pthread_mutex_t lock;
        char padding[128-sizeof(pthread_mutex_t)];
    }__attribute__ ((aligned(128), packed)) ;
    struct mutex_lock lhead, ltail;
    #define INIT_LOCK(plock) pthread_mutex_init(&(plock.lock),0);
    #define LOCK(plock) pthread_mutex_lock(&(plock.lock));
    #define UNLOCK(plock) pthread_mutex_unlock(&(plock.lock));
#elif PSPIN
    struct spin_lock {
        pthread_spinlock_t lock;
        char padding[128-sizeof(pthread_spinlock_t)];
    }__attribute__ ((aligned(128), packed)) ;
    struct spin_lock lhead, ltail;
    #define INIT_LOCK(plock) pthread_spin_init(&(plock.lock),PTHREAD_PROCESS_PRIVATE);
    #define LOCK(plock) pthread_spin_lock(&(plock.lock));
    #define UNLOCK(plock) pthread_spin_unlock(&(plock.lock));
#elif SPIN
    typedef int spinlock;

    struct spin_lock
    {
        spinlock status;
        char dummy[124];
    } __attribute__((aligned(128)));
    struct spin_lock *lhead, *ltail;
    #define LOCK(plock) spin_lock(&(plock->status));
    #define UNLOCK(plock) spin_unlock(&(plock->status));
    #define INIT_LOCK(plock) plock = (struct spin_lock *) malloc (sizeof(struct spin_lock));

#elif TICKET
    #define IAF_U32(a) __sync_add_and_fetch(a,1)
    #define TICKET_BASE_WAIT 512
    #define TICKET_WAIT_NEXT 128

    struct ticketlock
    {
        volatile int ticket;
        volatile int turn;
        char padding[120];
    } __attribute__((aligned(128)));

    typedef struct ticketlock ticketlock;
    ticketlock *lhead, *ltail;
    #define LOCK(plock) ticket_lock(plock);
    #define UNLOCK(plock) ticket_unlock(plock);
    #define INIT_LOCK(plock) plock = (struct ticketlock *) malloc (sizeof(struct ticketlock));\
    plock->ticket = 1;\
    plock->turn = 0;

#elif MCS
    typedef struct mcs_lock_t mcs_lock_t;
    typedef volatile struct mcs_lock_t * mcs_lock;
    struct mcs_lock_t
    {   
        volatile mcs_lock_t * next;
        volatile uint8_t spin;
    };
    mcs_lock *lhead, *ltail;
    #define LOCK(plock) lock_mcs(plock, the_lock);
    #define UNLOCK(plock) unlock_mcs(plock, the_lock);
    #define INIT_LOCK(plock) plock = (mcs_lock *) malloc (sizeof(mcs_lock));\
    *(plock) = 0; \
    atomic_barrier(); 
#elif FLAT
    pthread_t thread_id[128];
    // #define TYPE_FLATCOMBINING  "flat"
    #define TYPE_NOINFO TYPE_FLATCOMBINING
    static liblock_lock_t lhead;
    volatile int g_server_started = 0;

    void server_started() {
        g_server_started = 1;
    }
#else
    CLHLockStruct *lhead __attribute__((aligned(128)));
    #define INIT_LOCK(lock) lock = clhLockInit();
    #define LOCK(lock) clhLock(lock, pid);
    #define UNLOCK(lock) clhUnlock(lock, pid);
#endif

#ifdef TICKET
    static inline void
        nop_rep(uint32_t num_reps)
        {
            uint32_t i;
            for (i = 0; i < num_reps; i++)
            {
                __asm__ __volatile__ ("NOP");
            }
        }

    void ticket_lock(ticketlock *lock)
    {
      uint32_t my_ticket = IAF_U32(&(lock->turn));


      uint32_t wait = TICKET_BASE_WAIT;
      uint32_t distance_prev = 1;

      while (1)
        {
            uint32_t cur = lock->ticket;
            if (cur == my_ticket)
                break;
          
            uint32_t distance = my_ticket - cur;

            if (distance > 1)
            {
                if (distance != distance_prev)
                {
                    distance_prev = distance;
                    wait = TICKET_BASE_WAIT;
                }

                nop_rep(distance * wait);
            }
            else
            {
                nop_rep(TICKET_WAIT_NEXT);
            }

            if (distance > 20)
            {
                sched_yield();
            }
        }

    }

    static void ticket_unlock(ticketlock *t)
    {
        __asm__ __volatile__("" ::: "memory"); 
        t->ticket++;
    }

#elif MCS
    #define atomic_barrier() __sync_synchronize()
    static inline void *xchg_64(void *ptr, void *x)
    {
        __asm__ __volatile__("xchgq %0,%1"
                    :"=r" ((unsigned long long) x)
                    :"m" (*(volatile long long *)ptr), "0" ((unsigned long long) x)
                    :"memory");

        return x;
    }

    static void lock_mcs(mcs_lock *m, mcs_lock_t *me)
    {

        mcs_lock_t *tail;
        
        me->next = NULL;
        atomic_barrier();

        tail = (mcs_lock_t *) xchg_64(m, me);
        
        /* No one there? */
        if (!tail) return;

        /* Someone there, need to link in */
        me->spin = 0;
        atomic_barrier();

        tail->next = me;    
        
        /* Spin on my spin variable */
        while (!me->spin) cpu_relax();
        
        return;
    }

    static void unlock_mcs(mcs_lock *m, mcs_lock_t *me)
    {
        /* No successor yet? */
        volatile mcs_lock_t *succ;
        // if (!(me->next))
        if (!(succ = me->next))
        {
            /* Try to atomically unlock */
            if (__sync_val_compare_and_swap(m, me, NULL) == me) return;
        
            /* Wait for successor to appear */
            do {
                succ = me->next;
                cpu_relax();
            } while (!succ);
                // cpu_relax();
        }

        /* Unlock next one */
        succ->spin = 1; 
    }
#elif SPIN
    static inline unsigned xchg_32(void *ptr, unsigned x)
    {
        __asm__ __volatile__("xchgl %0,%1"
                    :"=r" ((unsigned) x)
                    :"m" (*(volatile unsigned *)ptr), "0" (x)
                    :"memory");

        return x;
    }

    static void spin_lock(spinlock *lock)
    {   
        int delay = 1;
        while (1){

            if (!xchg_32(lock, EBUSY)) return;
            
            while (*lock) {
                // for (int i=0; i < wait; i++){
                    cpu_relax();
                // }
                // delay *= 2;
            }
        }
    }

    static void spin_unlock(spinlock *lock)
    {
        barrier();
        *lock = 0;
    }
#endif

int counter = 0;
ListNode guard CACHE_ALIGN = {0, null};
volatile ListNode *Head CACHE_ALIGN = &guard;
int64_t d1 CACHE_ALIGN, d2;

__thread PoolStruct pool_node;


inline static void push_CS(ListNode *n) {
    n->next = Head;
    Head = n;
}

#ifdef MCS
    inline static void push(PoolStruct *pool_node, Object arg, int pid, struct ffwd_context *context, mcs_lock_t * the_lock) {
        volatile ListNode *n = alloc_obj(&pool_node);
        n->value = (Object)arg;
        LOCK(lhead);   // Critical section
        n->next = Head;
        Head = n;
        UNLOCK(lhead);
    }

    inline static Object pop(int pid, mcs_lock_t * the_lock) {
        Object result;

        LOCK(lhead);
        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }
        UNLOCK(lhead);

        return result;
    }
#elif FLAT
    inline static void push(PoolStruct *pool_node, Object arg, int pid, struct ffwd_context *context) {
        volatile ListNode *n = alloc_obj(pool_node);
        n->value = (Object)arg;
        #ifdef FLAT
            liblock_exec(&(lhead), &push_CS, (void *)n);
        #else 
            LOCK(lhead);   // Critical section
            n->next = Head;
            Head = n;
            UNLOCK(lhead);
        #endif
    }

    inline static Object pop(int* pid) {
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
    inline static void push(PoolStruct *pool_node) {
        volatile ListNode *n = alloc_obj(pool_node);
        n->value = (Object)1;

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

        return result;
    }
#else
    inline static void push(PoolStruct *pool_node, Object arg, int pid, struct ffwd_context *context) {
        volatile ListNode *n = alloc_obj(pool_node);
        n->value = (Object)arg;
        LOCK(lhead);   // Critical section
        n->next = Head;
        Head = n;
        UNLOCK(lhead);
    }

    inline static Object pop(int pid) {
        Object result;

        LOCK(lhead);
        if (Head->next == null) 
            result = -1;
        else {
            result = Head->next->value;
            Head = Head->next;
        }
        UNLOCK(lhead);

        return result;
    }
#endif

pthread_barrier_t barr;


inline static void Execute(void* Arg) {
    long i;
    long rnum;

    #ifdef FFWD
        GET_CONTEXT()
        volatile struct request* myrequest = context->request;
        long id = context->id;
    #else
        long id = (long) Arg;
    #endif

    #ifdef MCS
        mcs_lock_t *the_lock;
        the_lock = malloc (sizeof(mcs_lock_t));
        atomic_barrier();
    #endif

    #ifdef FLAT
      while(!g_server_started)
        __asm__ __volatile__ ("rep;nop;" ::: "memory");  
    #endif

    volatile int j;

    setThreadId(id);

    #ifdef FFWD

    #elif FLAT

    #else
        _thread_pin(id);
    #endif
 //   PoolStruct pool_node;

    simSRandom(id + 1);
    init_pool(&pool_node, sizeof(ListNode));

    #ifndef FLAT
        if (id == N_THREADS - 1)
            d1 = getTimeMillis();
    #endif
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }

    #ifdef FLAT
        int randno = rand() % 128;
        id = (rand()+5003) % N_THREADS;
    #endif

    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        #ifdef FFWD
            FFWD_EXEC(&push, 1, &pool_node)
        #elif FLAT
            push(&pool_node, (Object)1, id, 0);
        #elif MCS
            push(&pool_node, (Object)1, id, 0, the_lock);
        #else
            push(&pool_node, (Object)1, id, 0);
        #endif
            
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        #ifdef FFWD
            FFWD_EXEC(&pop, 1, id)
        #elif FLAT
            liblock_exec(&(lhead), &pop, (void *)&randno);
        #elif MCS
            pop(id, the_lock);
        #else
            pop(id);
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

    // lhead = clhLockInit();
    #ifdef FFWD
        ffwd_init();
    #elif FLAT
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
    #else
        INIT_LOCK(lhead)
    #endif

    #ifdef FLAT
        for (i=1; i<= N_THREADS; i++)
        {
            int err = liblock_thread_create_and_bind(&topology->cores[i], 0, &(thread_id[i]), NULL, &EntryPoint, NULL);

            if (err != 0)
                printf("\ncan't create thread :[%s]", strerror(err));
        }
        // printf("done creating\n");
        // d1 = getTimeMillis();
        liblock_lookup(TYPE_NOINFO)->run(server_started);
        d1 = getTimeMillis();
    #else
        for (i = 0; i < N_THREADS; i++){
            #ifdef FFWD
                ffwd_thread_create(&threads[i],EntryPoint,0);
            #else
                threads[i] = StartThread(i);
            #endif
        }
    #endif

    #ifdef FLAT
        for (i = 1; i <= N_THREADS; i++){
            pthread_join(thread_id[i], NULL);
        }
    #else
        for (i = 0; i < N_THREADS; i++)
            pthread_join(threads[i], NULL);
    #endif
    d2 = getTimeMillis();

    printf("time: %d\t", (int) (d2 - d1));
    //printf("%.3f\t", (double) (N_THREADS*RUNS*2) / (d2 - d1));
    
    printStats();
    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
