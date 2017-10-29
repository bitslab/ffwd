#ifndef _CCSTACK_H_
#define _CCSTACK_H_

#include "ccsynch.h"
#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "pool.h"

#define  GUARD                    INT_MIN
#define  PUSH_OP                  
#define  POP_OP                   INT_MIN


typedef struct Node {
    Object val;
    volatile struct Node *next;
} Node;

typedef struct StackCCSynchStruct {
    CCSynchStruct object_struct CACHE_ALIGN;
    volatile Node * volatile head CACHE_ALIGN;
    PoolStruct pool_node[N_THREADS] CACHE_ALIGN;
} StackCCSynchStruct;


typedef struct CCStackThreadState {
    ThreadState th_state;
} CCStackThreadState;


inline static void stackCCSynchInit(StackCCSynchStruct *stack_object_struct) {
    CCSynchStructInit(&stack_object_struct->object_struct);
    stack_object_struct->head = null;
}

inline static void stackThreadStateInit(StackCCSynchStruct *object_struct, CCStackThreadState *lobject_struct, int pid) {
    threadStateInit(&lobject_struct->th_state, (int)pid);
    init_pool(&object_struct->pool_node[pid], sizeof(Node));
}


inline static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        StackCCSynchStruct *st = (StackCCSynchStruct *)state;
        Node *node;

        node = alloc_obj(&st->pool_node[getThreadId()]);
        node->next = st->head;
        node->val = arg;
        st->head = node;
 
        return 0;
    } else {
        volatile StackCCSynchStruct *st = (StackCCSynchStruct *)state;
        volatile Node *node = st->head;

        if (st->head == null) {
            return -1;
        } else { 
            st->head = st->head->next;
            return node->val;
        }
        return 0;
    }
}


inline static void applyPushPop(StackCCSynchStruct *object_struct, CCStackThreadState *lobject_struct, ArgVal arg, int pid) {
    applyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) arg, pid);
}
#endif
