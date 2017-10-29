/*
 * File: htlock.c
 * Author:  Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *
 * Description: an numa-aware hierarchical ticket lock
 *  The htlock contains N local ticket locks (N = number of memory
 *  nodes) and 1 global ticket lock. A thread always tries to acquire
 *  the local ticket lock first. If there isn't any (local) available,
 *  it enqueues for acquiring the global ticket lock and at the same
 *  time it "gives" NB_TICKETS_LOCAL tickets to the local ticket lock, 
 *  so that if more threads from the same socket try to acquire the lock,
 *  they will enqueue on the local lock, without even accessing the
 *  global one.
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

#include "htlock.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <sched.h>
#include <inttypes.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

__thread uint32_t htlock_node_mine, htlock_id_mine;
#  define PREFETCHW(x)  
 static inline void
        nop_rep(uint32_t num_reps)
        {
            uint32_t i;
            for (i = 0; i < num_reps; i++)
            {
                asm volatile ("NOP");
            }
        }

#  define NUMBER_OF_SOCKETS 4
#  define CORES_PER_SOCKET 16
#  define CACHE_LINE_SIZE 64
# define NOP_DURATION 1
    static uint8_t   __attribute__ ((unused)) the_cores[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 
        8, 9, 10, 11, 12, 13, 14, 15, 
        16, 17, 18, 19, 20, 21, 22, 23, 
        24, 25, 26, 27, 28, 29, 30, 31, 
        32, 33, 34, 35, 36, 37, 38, 39, 
        40, 41, 42, 43, 44, 45, 46, 47, 
        48, 49, 50, 51, 52, 53, 54, 55, 
        56, 57, 58, 59, 60, 61, 62, 63
    };
    static uint8_t the_sockets[] = 
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3,
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3,
    };

static inline int get_cluster(int thread_id) {
     if (thread_id>=80){
            perror("Thread id too high");
            return 0;
        }
        return the_sockets[thread_id];
}

static inline void set_cpu(int cpu) {
    cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(cpu, &mask);
        numa_set_preferred(get_cluster(cpu));
        pthread_t thread = pthread_self();
        if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &mask) != 0) {
            fprintf(stderr, "Error setting thread affinity\n");
        }
}


int create_htlock(htlock_t* htl)
{
//    htlock_t* htl;
//    htl = memalign(CACHE_LINE_SIZE, sizeof(htlock_t));
//    if (htl == NULL) 
//    {
//        fprintf(stderr,"Error @ memalign : create htlock\n");
//    }
    assert(htl != NULL);

    htl->global = memalign(CACHE_LINE_SIZE, sizeof(htlock_global_t));
    if (htl == NULL) 
    {
        fprintf(stderr,"Error @ memalign : create htlock\n");
     }
    assert(htl->global != NULL);

    uint32_t s;
    for (s = 0; s < NUMBER_OF_SOCKETS; s++)
    {
#if defined(PLATFORM_NUMA)
        numa_set_preferred(s);
        htl->local[s] = (htlock_local_t*) numa_alloc_onnode(sizeof(htlock_local_t), s);
#else
        htl->local[s] = (htlock_local_t*) malloc(sizeof(htlock_local_t));
#endif
        htl->local[s]->cur = NB_TICKETS_LOCAL;
        htl->local[s]->nxt = 0;
        assert(htl->local != NULL);
    }

#if defined(PLATFORM_NUMA)
    numa_set_preferred(htlock_node_mine);
#endif

    htl->global->cur = 0;
    htl->global->nxt = 0;

    MEM_BARRIER;
    return 0;
}


    void
init_htlock(htlock_t* htl)
{
    assert(htl != NULL);
    htl->global->cur = 0;
    htl->global->nxt = 0;
    uint32_t n;
    for (n = 0; n < NUMBER_OF_SOCKETS; n++)
    {
        htl->local[n]->cur = NB_TICKETS_LOCAL;
        htl->local[n]->nxt = 0;
    }
    MEM_BARRIER;
}

    void
init_thread_htlocks(uint32_t phys_core)
{
    set_cpu(phys_core);

#if defined(XEON)
    uint32_t real_core_num = 0;
    uint32_t i;
    for (i = 0; i < (NUMBER_OF_SOCKETS * CORES_PER_SOCKET); i++) 
    {
        if (the_cores[i]==phys_core) 
        {
            real_core_num = i;
            break;
        }
    }
    htlock_id_mine = real_core_num;
    htlock_node_mine = get_cluster(phys_core);
#else
    htlock_id_mine = phys_core;
    htlock_node_mine = get_cluster(phys_core);
#endif
    /* printf("core %02d / node %3d\n", phys_core, htlock_node_mine); */
    MEM_BARRIER;
}

//     uint32_t
// is_free_hticket(htlock_t* htl)
// {
//     htlock_global_t* glb = htl->global;
// #if defined(OPTERON_OPTIMIZE)
//     PREFETCHW(glb);
// #endif
//     if (glb->cur == glb->nxt) 
//     {
//         return 1;
//     }
//     return 0;
// }

    static htlock_t* 
create_htlock_no_alloc(htlock_t* htl, htlock_local_t* locals[NUMBER_OF_SOCKETS], size_t offset)
{
    htl->global = memalign(CACHE_LINE_SIZE, sizeof(htlock_global_t));
    if (htl == NULL) 
    {
        fprintf(stderr,"Error @ memalign : create htlock\n");
    }
    assert(htl->global != NULL);

    uint32_t s;
    for (s = 0; s < NUMBER_OF_SOCKETS; s++)
    {
        htl->local[s] = locals[s] + offset;
    }

    htl->global->cur = 0;
    htl->global->nxt = 0;
    uint32_t n;
    for (n = 0; n < NUMBER_OF_SOCKETS; n++)
    {
        htl->local[n]->cur = NB_TICKETS_LOCAL;
        htl->local[n]->nxt = 0;
    }

    MEM_BARRIER;
    return htl;
}

    htlock_t*
init_htlocks(uint32_t num_locks)
{
    htlock_t* htls;
    htls = memalign(CACHE_LINE_SIZE, num_locks * sizeof(htlock_t));
    if (htls == NULL) 
    {
        fprintf(stderr, "Error @ memalign : init_htlocks\n");
    }
    assert(htls != NULL);


    size_t alloc_locks = (num_locks < 64) ? 64 : num_locks;

    htlock_local_t* locals[NUMBER_OF_SOCKETS];
    uint32_t n;
    for (n = 0; n < NUMBER_OF_SOCKETS; n++)
    {
#if defined(PLATFORM_NUMA)
        numa_set_preferred(n);
#endif
        locals[n] = (htlock_local_t*) calloc(alloc_locks, sizeof(htlock_local_t));
        *((volatile int*) locals[n]) = 33;
        assert(locals[n] != NULL);
    }

#if defined(OPTERON) || defined(XEON)
    numa_set_preferred(htlock_node_mine);
#endif

    uint32_t i;
    for (i = 0; i < num_locks; i++)
    {
        create_htlock_no_alloc(htls + i, locals, i);
    }

    MEM_BARRIER;
    return htls;
}


    void 
free_htlocks(htlock_t* locks)
{
    free(locks);
}

    static inline uint32_t
sub_abs(const uint32_t a, const uint32_t b)
{
    if (a > b)
    {
        return a - b;
    }
    else
    {
        return b - a;
    }
}


#define TICKET_BASE_WAIT 512
#define TICKET_MAX_WAIT  4095
#define TICKET_WAIT_NEXT 64


    static inline void
htlock_wait_ticket(htlock_local_t* lock, const uint32_t ticket)
{

#if defined(OPTERON_OPTIMIZE)
    uint32_t wait = TICKET_BASE_WAIT;
    uint32_t distance_prev = 1;

    while (1)
    {
        PREFETCHW(lock);
        int32_t lock_cur = lock->cur;
        if (lock_cur == ticket)
        {
            break;
        }
        uint32_t distance = sub_abs(lock->cur, ticket);
        if (distance > 1)
        {
            if (distance != distance_prev)
            {
                distance_prev = distance;
                wait = TICKET_BASE_WAIT;
            }

            nop_rep(distance * wait);
            wait = (wait + TICKET_BASE_WAIT) & TICKET_MAX_WAIT;
        }
        else
        {
            nop_rep(TICKET_WAIT_NEXT);
        }
    }  
#else
    while (lock->cur != ticket)
    {
        uint32_t distance = sub_abs(lock->cur, ticket);
        if (distance > 1)
        {
            nop_rep(distance * TICKET_BASE_WAIT);
        }
        else
        {
            PAUSE;
        }
    }
#endif	/* OPTERON_OPTIMIZE */
}

    static inline void
htlock_wait_global(htlock_local_t* lock, const uint32_t ticket)
{
    while (lock->cur != ticket)
    {
        uint32_t distance = sub_abs(lock->cur, ticket);
        if (distance > 1)
        {
            wait_cycles(distance * 256);
        }
        else
        {
            PAUSE;
        }
    }
}

    void
htlock_lock(htlock_t* l)
{
    htlock_local_t* localp = l->local[htlock_node_mine];
    int32_t local_ticket;

again_local:
    local_ticket = DAF_U32(&localp->nxt);
    if (local_ticket < -1)	
    {
        PAUSE;
        wait_cycles(-local_ticket * 120);
        PAUSE;
        goto again_local;
    }

    if (local_ticket >= 0)	/* local grabing successful */
    {
        htlock_wait_ticket((htlock_local_t*) localp, local_ticket);
    }
    else				/* no local ticket available */
    {
        do
        {
#if defined(OPTERON_OPTIMIZE)
            PREFETCHW(localp);
#endif
        } while (localp->cur != NB_TICKETS_LOCAL);
        localp->nxt = NB_TICKETS_LOCAL; /* give tickets to the local neighbors */

        htlock_global_t* globalp = l->global;
        uint32_t global_ticket = FAI_U32(&globalp->nxt);

        htlock_wait_global((htlock_local_t*) globalp, global_ticket);
    }
}

    void
htlock_release(htlock_t* l)
{
    htlock_local_t* localp = l->local[htlock_node_mine];
#if defined(OPTERON_OPTIMIZE)
    PREFETCHW(localp);
#endif
    int32_t local_cur = localp->cur;
    int32_t local_nxt = CAS_U32(&localp->nxt, local_cur, 0);
    if (local_cur == 0 || local_cur == local_nxt) /* global */
    {
#if defined(OPTERON_OPTIMIZE)
        PREFETCHW((l->global));
        PREFETCHW(localp);
#endif
        localp->cur = NB_TICKETS_LOCAL;
        l->global->cur++;
    }
    else				/* local */
    {
#if defined(OPTERON_OPTIMIZE)
        PREFETCHW(localp);
#endif
        localp->cur = local_cur - 1;
    }
}

//     uint32_t 
// htlock_trylock(htlock_t* l)
// {
//     htlock_global_t* globalp = l->global;
//     PREFETCHW(globalp);  
//     uint32_t global_nxt = globalp->nxt;

//     htlock_global_t tmp = 
//     {
//         .nxt = global_nxt, 
//         .cur = global_nxt
//     };
//     htlock_global_t tmp_new = 
//     {
//         .nxt = global_nxt + 1, 
//         .cur = global_nxt
//     };

//     uint64_t tmp64 = *(uint64_t*) &tmp;
//     uint64_t tmp_new64 = *(uint64_t*) &tmp_new;

//     if (CAS_U64((uint64_t*) globalp, tmp64, tmp_new64) == tmp64)
//     {
//         return 1;
//     }

//     return 0;
// }


    inline void
htlock_release_try(htlock_t* l)	/* trylock rls */
{
    PREFETCHW((l->global));
    l->global->cur++;
}

