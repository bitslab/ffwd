#ifndef _DSMSYNCH_H_
#define _DSMSYNCH_H_

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif
#include "config.h"
#include "primitives.h"
#include "rand.h"

const int DSMSIM_HELP_BOUND = 3 * N_THREADS;

typedef struct HalfLockNode {
    volatile struct LockNode *next;
    volatile ArgVal arg;
    volatile RetVal ret;
    volatile int32_t locked;
    volatile int32_t completed;
    volatile int32_t pid;
} HalfLockNode;

typedef struct LockNode {
    volatile struct LockNode *next;
    volatile ArgVal arg;
    volatile RetVal ret;
    volatile int32_t locked;
    volatile int32_t completed;
    volatile int32_t pid;
    int32_t align[PAD_CACHE(sizeof(HalfLockNode))];
} LockNode;

typedef struct ThreadState {
    int toggle;
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif
} ThreadState;

typedef struct LockStruct {
    volatile LockNode *Tail CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
    volatile LockNode MyNodes[2 * N_THREADS] CACHE_ALIGN;
    ThreadState th_state[N_THREADS];
} LockStruct;


inline static RetVal applyOp(LockStruct *l, ThreadState *st_thread, RetVal (*sfunc)(ArgVal, int), ArgVal arg, int pid) {
    volatile LockNode *mynode;
    volatile LockNode *mypred;
    volatile LockNode *p;
    register int toggle, counter;

    toggle = st_thread->toggle;
    toggle = 1 - toggle;
    mynode = (LockNode *)&l->MyNodes[2 * pid + toggle];
    st_thread->toggle = toggle;
    mynode->locked = true;
    mynode->completed = false;
    mynode->pid = pid;
    mynode->next = null;
    mynode->arg = arg;
#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
    mypred = (LockNode *)SWAP(&l->Tail, mynode);
    if (mypred != null) {
        mypred->next = mynode;
        StoreFence();
#if defined(__sun) || defined(sun)
        schedctl_stop(st_thread->schedule_control);
#endif
        while (mynode->locked == true) {
#if N_THREADS > USE_CPUS
            sched_yield();
#elif defined(sparc)
            Pause();
            Pause();
            Pause();
            Pause();
#else
            Pause();
#endif
        }
        if (mynode->completed == true) // operation has already applied
            return mynode->ret;
    }
#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
#ifdef DEBUG
    l->rounds += 1;
#endif
    counter = 0;
    p = mynode;
    do {  // I surely do it for myself
        //ReadPrefetch(&p->next);
        counter++;
#ifdef DEBUG
        l->counter += 1;
#endif
        p->ret = sfunc(p->arg, p->pid);
        p->completed = true;
        p->locked = false;
        if (p->next == null || p->next->next == null  || counter >= DSMSIM_HELP_BOUND)
            break;
        p = p->next;
    } while(true);
    // End critical section
    if (p->next == null) {
        if (l->Tail == p && CASPTR(&l->Tail, p, null) == true)
            return mynode->ret;
        while (*((volatile LockNode **)&p->next) == null) {
            sched_yield();
        }
    }
    p->next->locked = false;
    StoreFence();
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif
    return mynode->ret;
}

void lock_init(LockStruct *l) {
    l->Tail = NULL;
#ifdef DEBUG
    l->counter = 0;
#endif
    StoreFence();
}

inline static void threadStateInit(ThreadState *st_thread, LockStruct *l, int pid) {
    st_thread->toggle = 0;
#ifdef sun
    st_thread->schedule_control = schedctl_init();
#endif

}

#endif
