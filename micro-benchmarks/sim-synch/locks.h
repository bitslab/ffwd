#include "liblock.h"

/* Compile read-write barrier */
#define barrier() __asm__ __volatile__("": : :"memory")

/* Pause instruction to prevent excess processor bus usage */ 
#define cpu_relax() __asm__ __volatile__("pause\n": : :"memory")

#define EBUSY 1

#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))
#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define atomic_inc(P) __sync_add_and_fetch((P), 1)
#define atomic_dec(P) __sync_add_and_fetch((P), -1) 
#define atomic_add(P, V) __sync_add_and_fetch((P), (V))
#define atomic_set_bit(P, V) __sync_or_and_fetch((P), 1<<(V))
#define atomic_clear_bit(P, V) __sync_and_and_fetch((P), ~(1<<(V)))


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
#elif HTICKET
    // __thread uint32_t htlock_node_mine, htlock_id_mine;
    typedef htlock_t lock_global_data;

    htlock_t lhead, ltail;

    #define LOCK(plock) htlock_lock(&plock);
    #define UNLOCK(plock) htlock_release(&plock);
#elif FLAT
    pthread_t thread_id[128];
    #define TYPE_NOINFO TYPE_FLATCOMBINING
    static liblock_lock_t lhead, ltail;
    volatile int g_server_started = 0;

    void server_started() {
        g_server_started = 1;
    }
#elif RCL
    pthread_t thread_id[128];
    #define TYPE_NOINFO TYPE_RCL
    static liblock_lock_t lhead, ltail;
    volatile int g_server_started = 0;

    void server_started() {
        g_server_started = 1;
    }
#else
    CLHLockStruct *lhead, *ltail;
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