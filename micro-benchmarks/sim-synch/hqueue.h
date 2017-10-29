#ifndef _HSTACK_H_
#define _HSTACK_H_

#include "hsynch.h"
#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "pool.h"

#define  GUARD          INT_MIN


typedef struct Node {
    Object val;
    volatile struct Node *next;
} Node;

typedef struct QueueHSynchStruct {
    HSynchStruct enqueue_struct CACHE_ALIGN;
    HSynchStruct dequeue_struct CACHE_ALIGN;
    volatile Node *last CACHE_ALIGN;
    volatile Node *first CACHE_ALIGN;
    Node guard CACHE_ALIGN;
    PoolStruct pool_node[N_THREADS] CACHE_ALIGN;
} QueueHSynchStruct;


typedef struct HQueueThreadState {
    HSynchThreadState enqueue_thread_state;
    HSynchThreadState dequeue_thread_state;
} HQueueThreadState;


inline static void queueHSynchInit(QueueHSynchStruct *queue_object_struct) {
    HSynchStructInit(&queue_object_struct->enqueue_struct);
    HSynchStructInit(&queue_object_struct->dequeue_struct);
    queue_object_struct->guard.val = GUARD;
    queue_object_struct->guard.next = null;
    queue_object_struct->first = &queue_object_struct->guard;
    queue_object_struct->last = &queue_object_struct->guard;
}

inline static void hqueueThreadStateInit(QueueHSynchStruct *object_struct, HQueueThreadState *lobject_struct, int pid) {
    HSynchThreadStateInit(&lobject_struct->enqueue_thread_state, (int)pid);
    HSynchThreadStateInit(&lobject_struct->dequeue_thread_state, (int)pid);
    init_pool(&object_struct->pool_node[pid], sizeof(Node));
}


inline static RetVal serialEnqueue(void *state, ArgVal arg, int pid) {
    QueueHSynchStruct *st = (QueueHSynchStruct *)state;
    Node *node;
    
    node = alloc_obj(&st->pool_node[getThreadId()]);
    node->next = null;
    node->val = arg;
    st->last->next = node;
    st->last = node;
    return -1;
}

inline static RetVal serialDequeue(void *state, ArgVal arg, int pid) {
    QueueHSynchStruct *st = (QueueHSynchStruct *)state;
    Node *node = (Node *)st->first;
    
    if (st->first->next != null){
        st->first = st->first->next;
        return node->val;
    } else {
        return -1;
    }
}


inline static void applyEnqueue(QueueHSynchStruct *object_struct, HQueueThreadState *lobject_struct, ArgVal arg, int pid) {
    applyOp(&object_struct->enqueue_struct, &lobject_struct->enqueue_thread_state, serialEnqueue, object_struct, (ArgVal) pid, pid);
}

inline static RetVal applyDequeue(QueueHSynchStruct *object_struct, HQueueThreadState *lobject_struct, int pid) {
    return applyOp(&object_struct->dequeue_struct, &lobject_struct->dequeue_thread_state, serialDequeue, object_struct, (ArgVal) pid, pid);
}
#endif
