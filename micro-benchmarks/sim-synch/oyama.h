#ifndef _OYAMA_H_
#define _OYAMA_H_

#include <time.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"

#define LOCKED                          1
#define UNLOCKED                        0
#define OYAMA_HELP_BOUND                (10 * N_THREADS)

typedef struct HalfOyamaAnnounceNode {
    volatile struct OyamaAnnounceNode *next;
    volatile ArgVal arg_ret;
    int32_t pid;
    volatile bool completed;
} HalfOyamaAnnounceNode;

typedef struct OyamaAnnounceNode {
    volatile struct OyamaAnnounceNode *next;
    volatile ArgVal arg_ret;
     int32_t pid;
    volatile bool completed;
    int32_t align[PAD_CACHE(sizeof(HalfOyamaAnnounceNode))];
} OyamaAnnounceNode;


typedef struct OyamaLockStruct {
    volatile int lock CACHE_ALIGN;
    volatile OyamaAnnounceNode *tail CACHE_ALIGN;
    volatile OyamaAnnounceNode pool[N_THREADS] CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} OyamaLockStruct;



inline static void OYAMA_wait(void) {
#if N_THREADS > USE_CPUS
    ;
//    sched_yield();
#elif defined(sparc)
    sched_yield();
    //sched_yield();
    //sched_yield();
    //sched_yield();
    //sched_yield();
    //sched_yield();
#else
        ;
#endif
}


inline static RetVal applyOp(volatile OyamaLockStruct *l, RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid) {
    volatile OyamaAnnounceNode *mynode = &l->pool[pid];
    volatile OyamaAnnounceNode *p;
    register OyamaAnnounceNode *tmp_next;
  
    // Initializing node
    mynode->arg_ret = arg;
    mynode->pid = pid;
    mynode->completed = false;
    mynode->next = l->tail;
    while (!CASPTR(&l->tail, mynode->next, mynode)) {  // try to insert node to the announce list
        OYAMA_wait();
        mynode->next = l->tail;
    } 

    do {
        if (l->lock == UNLOCKED && CASPTR(&l->lock, UNLOCKED, LOCKED)) {
            int counter =0;
#ifdef DEBUG
            l->rounds++;
#endif
            while (counter < OYAMA_HELP_BOUND &&
                   (p = (OyamaAnnounceNode *) SWAP(&l->tail, null)) != null) {
                // Start helping all the active processes
                while (p != null) {
                    counter++;
#ifdef DEBUG
                    l->counter++;
#endif
                    tmp_next = (OyamaAnnounceNode *)p->next;
                    p->arg_ret = sfunc(p->arg_ret, p->pid);
                    p->completed = true;
                    p = (OyamaAnnounceNode *)tmp_next;
                }
            }
            // Release the lock
            l->lock = UNLOCKED;
            StoreFence();
            return mynode->arg_ret;
        } else {
            while (*((volatile int32_t *)&mynode->completed) == false && 
			       *((volatile int32_t *)&l->lock) == LOCKED) {
                ;
            }
            if (mynode->completed)
                return mynode->arg_ret;
        }
    } while (true); 
}

void oyamaLockInit(OyamaLockStruct *l) {
    l->lock = UNLOCKED;
    l->tail = null;
#ifdef DEBUG
    l->rounds = l->counter = 0;
#endif
    FullFence();
}

#endif
