#ifndef _HSYNCH_H_
#define _HSYNCH_H_

#if defined(sun) || defined(_sun)
#    include <schedctl.h>
#endif

#include "config.h"
#include "primitives.h"
#include "clh.h"
#include "rand.h"

#define HSYNCH_NCLUSTERS                2
const int HSYNCH_HELP_BOUND =           3 * N_THREADS;

typedef struct HalfHSynchNode {
    struct HalfHSynchNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
} HalfHSynchNode;

typedef struct HSynchNode {
    struct HSynchNode *next;
    ArgVal arg_ret;
    int32_t pid;
    int32_t locked;
    int32_t completed;
    int32_t align[PAD_CACHE(sizeof(HalfHSynchNode))];
} HSynchNode;

typedef union HSynchNodePtr {
    struct HSynchNode *ptr;
    char pad[CACHE_LINE_SIZE];
} HSynchNodePtr;

typedef struct HSynchThreadState {
    HSynchNode *next_node;
#if defined(__sun) || defined(sun)
    schedctl_t *schedule_control;    
#endif
} HSynchThreadState;

typedef struct HSynchStruct {
    CLHLockStruct *central_lock CACHE_ALIGN;
    volatile HSynchNodePtr Tail[HSYNCH_NCLUSTERS] CACHE_ALIGN;
#ifdef DEBUG
    volatile int rounds CACHE_ALIGN;
    volatile int counter;
#endif
} HSynchStruct;


inline static RetVal applyOp(HSynchStruct *l, HSynchThreadState *st_thread, RetVal (*sfunc)(void *, ArgVal, int), void *state, ArgVal arg, int pid) {
    volatile HSynchNode *p;
    volatile HSynchNode *cur;
    register HSynchNode *next_node, *tmp_next;
    register int counter = 0;

    next_node = st_thread->next_node;
    next_node->next = null;
    next_node->locked = true;
    next_node->completed = false;

#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
    cur = (volatile HSynchNode *)SWAP(&l->Tail[pid % HSYNCH_NCLUSTERS].ptr, next_node);
    cur->arg_ret = arg;
    cur->next = (HSynchNode *)next_node;
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif
    st_thread->next_node = (HSynchNode *)cur;

    while (cur->locked) {                   // spinning
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
#if defined(__sun) || defined(sun)
    schedctl_start(st_thread->schedule_control);
#endif
    p = cur;                                // I am not been helped
    if (cur->completed)                     // I have been helped
        return cur->arg_ret;
    clhLock(l->central_lock, pid);
#ifdef DEBUG
    l->rounds++;
#endif
    while (p->next != null && counter < HSYNCH_HELP_BOUND) {
        ReadPrefetch(p->next);
        counter++;
#ifdef DEBUG
        l->counter++;
#endif
        tmp_next = p->next;
        p->arg_ret = sfunc(state, p->arg_ret, p->pid);
        p->completed = true;
        p->locked = false;
        WeakFence();
        p = tmp_next;
    }
    p->locked = false;                      // Unlock the next one
    clhUnlock(l->central_lock, pid);
#if defined(__sun) || defined(sun)
    schedctl_stop(st_thread->schedule_control);
#endif

    return cur->arg_ret;
}

inline static void HSynchThreadStateInit(HSynchThreadState *st_thread, int pid) {
    st_thread->next_node = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HSynchNode));
#ifdef sun
    st_thread->schedule_control = schedctl_init();
#endif
}

void HSynchStructInit(HSynchStruct *l) {
    int i;

    l->central_lock = clhLockInit();
    for (i = 0; i < HSYNCH_NCLUSTERS; i++) {
        l->Tail[i].ptr = getAlignedMemory(CACHE_LINE_SIZE, sizeof(HSynchNode));
        l->Tail[i].ptr->next = null;
        l->Tail[i].ptr->locked = false;
        l->Tail[i].ptr->completed = false;
    }
#ifdef DEBUG
    l->rounds = l->counter =0;
#endif
    StoreFence();
}

#endif
