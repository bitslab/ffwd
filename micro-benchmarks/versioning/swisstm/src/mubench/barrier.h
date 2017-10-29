/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef WLPDSTM_BARRIER_H_
#define WLPDSTM_BARRIER_H_

#include <stdint.h>

#include "../common/atomic.h"
#include "../common/constants.h"

union barrier_t {
    volatile uintptr_t counter;
    char padding[CACHE_LINE_SIZE_BYTES];
};

inline void init_barrier(barrier_t *b, uintptr_t v) {
    b->counter = v;
}

inline void enter_barrier(barrier_t *b) {
    uintptr_t old_v, new_v;

    do {
        old_v = b->counter;
        new_v = old_v - 1;
    } while(!atomic_cas_full(&b->counter, old_v, new_v));

    while(b->counter > 0) {
        // spin
    }
}

#endif /* WLPDSTM_BARRIER_H_ */
