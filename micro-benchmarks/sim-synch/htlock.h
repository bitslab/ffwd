/*
 * File: htlock.h
 * Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *
 * Description: a numa-aware hierarchical teicket lock 
 *    The htlock contains N local ticket locks (N = number of memory
 *    nodes) and 1 global ticket lock. A thread always tries to acquire
 *    the local ticket lock first. If there isn't any (local) available,
 *    it enqueues for acquiring the global ticket lock and at the same
 *    time it "gives" NB_TICKETS_LOCAL tickets to the local ticket lock, 
 *    so that if more threads from the same socket try to acquire the lock,
 *    they will enqueue on the local lock, without even accessing the
 *    global one.      
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Vasileios Trigonakis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _HTICKET_H_
#define _HTICKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#ifndef __sparc__
#  ifndef __tile__
#    include <numa.h>
#    include <emmintrin.h>
#  endif
#endif
#include <pthread.h>
#include <assert.h>
#include <stdint.h>
// #include "utils.h"
//#include "atomic_ops.h"

#define PAUSE __asm__ __volatile__ ("rep;nop" ::: "memory");
#define MEM_BARRIER __sync_synchronize()

typedef uint64_t ticks;
#define NUMBER_OF_SOCKETS 4

#define NB_TICKETS_LOCAL	128 /* max number of local tickets of local tickets
                                   before releasing global*/
#define CACHE_LINE_SIZE 64
#define ALIGNED(N) __attribute__ ((aligned (N)))

typedef struct htlock_global
{
    volatile uint32_t nxt;
    volatile uint32_t cur;
    uint8_t padding[CACHE_LINE_SIZE - 8];
} htlock_global_t;

typedef struct htlock_local
{
    volatile int32_t nxt;
    volatile int32_t cur;
    uint8_t padding[CACHE_LINE_SIZE - 8];
} htlock_local_t;

typedef struct ALIGNED(CACHE_LINE_SIZE) htlock
{
    htlock_global_t* global;
    htlock_local_t* local[NUMBER_OF_SOCKETS];
} htlock_t;

extern int create_htlock(htlock_t* htl);
extern void init_htlock(htlock_t* htl); /* initiliazes an htlock */
extern void init_thread_htlocks(uint32_t thread_num);
extern htlock_t* init_htlocks(uint32_t num_locks);
extern void free_htlocks(htlock_t* locks);


extern uint32_t is_free_hticket(htlock_t* htl);
extern void htlock_lock(htlock_t* l);
extern uint32_t htlock_trylock(htlock_t* l);

extern void htlock_release(htlock_t* l);
extern inline void htlock_release_try(htlock_t* l);	/* trylock rls */

static inline ticks getticks(void)
    {
        unsigned hi, lo;
        __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
        return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
    }

static inline uint8_t tas_uint8(volatile uint8_t *addr) {
    uint8_t oldval;
    __asm__ __volatile__("ldstub %1,%0"
            : "=r"(oldval), "=m"(*addr)
            : "m"(*addr) : "memory");
    return oldval;
}


static inline unsigned long xchg32(volatile unsigned int *m, unsigned int val)
{
    unsigned long tmp1, tmp2;

    __asm__ __volatile__(
            "       mov             %0, %1\n"
            "1:     lduw            [%4], %2\n"
            "       cas             [%4], %2, %0\n"
            "       cmp             %2, %0\n"
            "       bne,a,pn        %%icc, 1b\n"
            "        mov            %1, %0\n"
            : "=&r" (val), "=&r" (tmp1), "=&r" (tmp2)
            : "0" (val), "r" (m)
            : "cc", "memory");
    return val;
}

#  include <xmmintrin.h>


//Swap uint64_t
static inline uint64_t swap_uint64(volatile uint64_t* target,  uint64_t x) {
    __asm__ __volatile__("xchgq %0,%1"
            :"=r" ((uint64_t) x)
            :"m" (*(volatile uint64_t *)target), "0" ((uint64_t) x)
            :"memory");

    return x;
}

//Swap uint32_t
static inline uint32_t swap_uint32(volatile uint32_t* target,  uint32_t x) {
    __asm__ __volatile__("xchgl %0,%1"
            :"=r" ((uint32_t) x)
            :"m" (*(volatile uint32_t *)target), "0" ((uint32_t) x)
            :"memory");

    return x;
}

//Swap uint16_t
static inline uint16_t swap_uint16(volatile uint16_t* target,  uint16_t x) {
    __asm__ __volatile__("xchgw %0,%1"
            :"=r" ((uint16_t) x)
            :"m" (*(volatile uint16_t *)target), "0" ((uint16_t) x)
            :"memory");

    return x;
}

//Swap uint8_t
static inline uint8_t swap_uint8(volatile uint8_t* target,  uint8_t x) {
    __asm__ __volatile__("xchgb %0,%1"
            :"=r" ((uint8_t) x)
            :"m" (*(volatile uint8_t *)target), "0" ((uint8_t) x)
            :"memory");

    return x;
}


//atomic operations interface
//Compare-and-swap
#define CAS_PTR(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U8(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U16(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U32(a,b,c) __sync_val_compare_and_swap(a,b,c)
#define CAS_U64(a,b,c) __sync_val_compare_and_swap(a,b,c)
//Swap
#define SWAP_PTR(a,b) swap_pointer(a,b)
#define SWAP_U8(a,b) swap_uint8(a,b)
#define SWAP_U16(a,b) swap_uint16(a,b)
#define SWAP_U32(a,b) swap_uint32(a,b)
#define SWAP_U64(a,b) swap_uint64(a,b)
//Fetch-and-increment
#define FAI_U8(a) __sync_fetch_and_add(a,1)
#define FAI_U16(a) __sync_fetch_and_add(a,1)
#define FAI_U32(a) __sync_fetch_and_add(a,1)
#define FAI_U64(a) __sync_fetch_and_add(a,1)
//Fetch-and-decrement
#define FAD_U8(a) __sync_fetch_and_sub(a,1)
#define FAD_U16(a) __sync_fetch_and_sub(a,1)
#define FAD_U32(a) __sync_fetch_and_sub(a,1)
#define FAD_U64(a) __sync_fetch_and_sub(a,1)
//Increment-and-fetch
#define IAF_U8(a) __sync_add_and_fetch(a,1)
#define IAF_U16(a) __sync_add_and_fetch(a,1)
#define IAF_U32(a) __sync_add_and_fetch(a,1)
#define IAF_U64(a) __sync_add_and_fetch(a,1)
//Decrement-and-fetch
#define DAF_U8(a) __sync_sub_and_fetch(a,1)
#define DAF_U16(a) __sync_sub_and_fetch(a,1)
#define DAF_U32(a) __sync_sub_and_fetch(a,1)
#define DAF_U64(a) __sync_sub_and_fetch(a,1)
//Test-and-set
#define TAS_U8(a) tas_uint8(a)

static inline unsigned long xchg64(volatile unsigned long *m, unsigned long val)                                                                                                             
{
    unsigned long tmp1, tmp2;

    __asm__ __volatile__(
            "       mov             %0, %1\n"
            "1:     ldx             [%4], %2\n"
            "       casx            [%4], %2, %0\n"
            "       cmp             %2, %0\n"
            "       bne,a,pn        %%xcc, 1b\n"
            "        mov            %1, %0\n"
            : "=&r" (val), "=&r" (tmp1), "=&r" (tmp2)
            : "0" (val), "r" (m)
            : "cc", "memory");
    return val;
}


    static inline void 
wait_cycles(uint64_t cycles)
{
    if (cycles < 256)
    {
        cycles /= 6;
        while (cycles--)
        {
            PAUSE;
        }
    }
    else
    {
        ticks _start_ticks = getticks();
        ticks _end_ticks = _start_ticks + cycles - 130;
        while (getticks() < _end_ticks);
    }
}

#endif	/* _HTICKET_H_ */


