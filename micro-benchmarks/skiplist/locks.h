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


inline void *xchg_64(void *ptr, void *x)
    {
        __asm__ __volatile__("xchgq %0,%1"
                    :"=r" ((unsigned long long) x)
                    :"m" (*(volatile long long *)ptr), "0" ((unsigned long long) x)
                    :"memory");
        return x;
    }

#ifdef MUTEX
    pthread_mutex_t *lock __attribute__((aligned(128)));
    #define LOCK(l) pthread_mutex_lock(l);
    #define UNLOCK(l) pthread_mutex_unlock(l);
    #define INIT_LOCK(l) l = (pthread_mutex_t *) malloc (sizeof(pthread_mutex_t));\
                         pthread_mutex_init(l, 0);
#elif MCS
    typedef struct mcs_lock_t mcs_lock_t;
   typedef volatile struct mcs_lock_t * mcs_lock;
    struct mcs_lock_t
    {   
        volatile mcs_lock_t * next;
        volatile uint8_t spin;
    };
    
    mcs_lock *lock __attribute__((aligned(128)));
    #define LOCK(l) lock_mcs(l, the_lock);
    #define UNLOCK(l) unlock_mcs(l, the_lock);
    #define INIT_LOCK(l) l = (mcs_lock *) malloc (sizeof(mcs_lock));\
                        *(l) = 0; \
                        atomic_barrier();
    #define atomic_barrier() __sync_synchronize()

    void lock_mcs(mcs_lock *m, mcs_lock_t *me)
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

    void unlock_mcs(mcs_lock *m, mcs_lock_t *me)
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
#elif FFWD
    int lock;
    #define LOCK(l)
    #define UNLOCK(l)
    #define INIT_LOCK(l)
#endif