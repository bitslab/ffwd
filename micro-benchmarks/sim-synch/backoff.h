#ifndef _BACKOFF_H_

#define _BACKOFF_H_
#include <sys/select.h>
#include "rand.h"

typedef struct BackoffStruct {
    unsigned backoff;
    unsigned backoff_base_bits;
    unsigned backoff_cap_bits;
    unsigned backoff_shift_bits;
    unsigned backoff_base;
    unsigned backoff_cap;
    unsigned backoff_addend;
} BackoffStruct;

inline static void init_backoff(BackoffStruct *b, unsigned base_bits, unsigned cap_bits, unsigned shift_bits) {
    b->backoff_base_bits = base_bits;
    b->backoff_cap_bits = cap_bits;
    b->backoff_shift_bits = shift_bits;
    b->backoff_base = (1 << b->backoff_base_bits) - 1;
    b->backoff_cap = (1 << b->backoff_cap_bits) - 1;
    b->backoff_addend = (1 << b->backoff_shift_bits) - 1;
    b->backoff = b->backoff_base;
}

inline static void reset_backoff(BackoffStruct *b) {
    b->backoff = b->backoff_base;
}


inline static void backoff_delay(BackoffStruct *b) {
#if N_THREADS > USE_CPUS
#   ifdef sparc
    sched_yield();
    sched_yield();
    sched_yield();
    sched_yield();
    sched_yield();
#   else
    sched_yield();
#   endif
#elif defined(DISABLE_BACKOFF)
    ;
#else
    volatile unsigned i;

    for (i = 0; i < b->backoff; i++)
        ;

    b->backoff <<= b->backoff_shift_bits;
    b->backoff += b->backoff_addend;
    b->backoff &= b->backoff_cap;
#endif
}



inline static void backoff_reduce(BackoffStruct *b) {
    b->backoff >>= b->backoff_shift_bits;
    if (b->backoff < b->backoff_base)
        b->backoff = b->backoff_base;
}

inline static void backoff_increase(BackoffStruct *b) {
    b->backoff <<= b->backoff_shift_bits;
    b->backoff += b->backoff_addend;
    if (b->backoff > b->backoff_cap)
        b->backoff = b->backoff_cap;
}

#endif
