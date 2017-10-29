#ifndef _HSTACK_H_
#define _HSTACK_H_

#include "hsynch.h"
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

typedef struct StackHSynchStruct {
    HSynchStruct object_struct CACHE_ALIGN;
    volatile Node *head CACHE_ALIGN;
    PoolStruct pool_node[N_THREADS] CACHE_ALIGN;
} StackHSynchStruct;


typedef struct HStackThreadState {
    HSynchThreadState th_state;
} HStackThreadState;


inline static void stackHSynchInit(StackHSynchStruct *stack_object_struct) {
    HSynchStructInit(&stack_object_struct->object_struct);
    stack_object_struct->head = null;
}

inline static void stackThreadStateInit(StackHSynchStruct *object_struct, HStackThreadState *lobject_struct, int pid) {
    HSynchThreadStateInit(&lobject_struct->th_state, (int)pid);
    init_pool(&object_struct->pool_node[pid], sizeof(Node));
}


inline static RetVal serialPushPop(void *state, ArgVal arg, int pid) {
    if (arg == POP_OP) {
        StackHSynchStruct *st = (StackHSynchStruct *)state;
        Node *node;

        node = alloc_obj(&st->pool_node[getThreadId()]);
        node->next = st->head;
        node->val = arg;
        st->head = node;
 
        return 0;
    } else {
        volatile StackHSynchStruct *st = (StackHSynchStruct *)state;
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


inline static void applyPushPop(StackHSynchStruct *object_struct, HStackThreadState *lobject_struct, ArgVal arg, int pid) {
    applyOp(&object_struct->object_struct, &lobject_struct->th_state, serialPushPop, object_struct, (ArgVal) arg, pid);
}
#endif
