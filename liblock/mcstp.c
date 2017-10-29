/* ########################################################################## */
/* (C) UPMC, 2010-2011                                                        */
/*     Authors:                                                               */
/*       Jean-Pierre Lozi <jean-pierre.lozi@lip6.fr>                          */
/*       Gaël Thomas <gael.thomas@lip6.fr>                                    */
/*       Florian David <florian.david@lip6.fr>                                */
/*       Julia Lawall <julia.lawall@lip6.fr>                                  */
/*       Gilles Muller <gilles.muller@lip6.fr>                                */
/* -------------------------------------------------------------------------- */
/* ########################################################################## */
#include <papi.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "liblock.h"
#include "liblock-fatal.h"

/* ########################################################################## */
/* Based on the pseudo-code from :                                            */
/* 1) www.cs.rochester.edu/research/synchronization/pseudocode/tp_locks.html  */
/* 2) Technical Report URCS-867.                                              */
/* ########################################################################## */

/* Approximate upper bound on the length of a critical section. */
#define MAX_CS_TIME 10000
/* Approximate max number of threads in the system. */
#define MAX_THREADS 1000
/* Approximate length of time it takes a thread to see a timestamp published on
   another thread, including any potential clock skew. */
#define UPDATE_DELAY 10
/* Patience to wait in the queue, in microseconds. 50us is the value used in the
   original paper. */
#define PATIENCE 50

typedef enum { INIT, AVAILABLE, WAITING, TIMED_OUT, FAILED } qnode_status;

struct liblock_impl;
typedef struct mcstp_qnode {
    struct liblock_impl *last_lock;   // Lock from last attempt
    volatile long long time;  // Published timestamp
    struct mcstp_qnode *volatile next;
    volatile /*qnode_status*/ uint64_t status;
    char __pad[pad_to_cache_line(sizeof(struct liblock_impl *) +
                                 sizeof(long long) +
                                 sizeof(uint64_t) +
                                 sizeof(struct mcstp_qnode *))];
} mcstp_qnode;

struct liblock_impl {
    pthread_mutex_t posix_lock;
    mcstp_qnode *volatile tail;
    volatile long long cs_start_time;
    char __pad[pad_to_cache_line(sizeof(pthread_mutex_t) +
                                 sizeof(mcstp_qnode *) +
                                 sizeof(long long))];
};

__thread mcstp_qnode *volatile my_qnode = NULL;

static int trylock_mcstp(struct liblock_impl* impl)
{
    mcstp_qnode *pred;
    long long start_time = PAPI_get_real_usec();

    /* Try to reclaim position in queue */
    if (my_qnode->status != TIMED_OUT || my_qnode->last_lock != impl ||
        !__sync_bool_compare_and_swap(&my_qnode->status, TIMED_OUT, WAITING))
    {
       my_qnode->status = WAITING;
       my_qnode->next = 0;
       pred = __sync_lock_test_and_set(&impl->tail, my_qnode);

       if (!pred)
       { // lock was free
           impl->cs_start_time = PAPI_get_real_usec();
           return 1;
       } else pred->next = my_qnode;
    }

    for (;;)
    {
       if (my_qnode->status == AVAILABLE)
       {
           impl->cs_start_time = PAPI_get_real_usec();
           return 1;
       }
       else if (my_qnode->status == FAILED)
       {
           if (PAPI_get_real_usec() - impl->cs_start_time > MAX_CS_TIME)
              pthread_yield();

           my_qnode->last_lock = impl;
           return 0;
       }

       while (my_qnode->status == WAITING)
       {
           my_qnode->time = PAPI_get_real_usec();

           if (PAPI_get_real_usec() - start_time <= PATIENCE)
               continue;
           
           if (!__sync_bool_compare_and_swap(&my_qnode->status,
                                             WAITING, TIMED_OUT))
           {
// !
//             my_qnode->last_lock = impl;
               break;
           }

           if (PAPI_get_real_usec() - impl->cs_start_time > MAX_CS_TIME)
               pthread_yield();

// !
           my_qnode->last_lock = impl;
           return 0;
        }
    }
}

static void lock_mcstp(struct liblock_impl* impl)
{
    while (!trylock_mcstp(impl))
        ;
}

static void unlock_mcstp(struct liblock_impl* impl)
{
    int scanned_nodes = 0;
    mcstp_qnode *succ, *curr = my_qnode, *last = NULL;
    
    for (;;)
    {
        succ = curr->next;

        if (!succ)
        {
            if (__sync_bool_compare_and_swap(&impl->tail, curr, 0)) {
                curr->status = FAILED;
                return; /* I was last in line. */
            }

            while (!succ)
                succ = curr->next;
        }

        if (++scanned_nodes < MAX_THREADS)
            curr->status = FAILED;
        else if (!last)
            last = curr; /* Handle treadmill case. */

        if (succ->status == WAITING)
        {
            long long succ_time = succ->time;

            if ((PAPI_get_real_usec() - succ_time <= UPDATE_DELAY) &&
                __sync_bool_compare_and_swap(&succ->status, WAITING, AVAILABLE))
            {
                for ( ; last && last != curr; last = last->next)
                    last->status = FAILED;

                return;
            }
        }

        curr = succ;
    }
}

static struct liblock_impl* do_liblock_init_lock(mcstp)
                                (liblock_lock_t* lock,
                                 struct core* core,
                                 pthread_mutexattr_t* attr)
{
    struct liblock_impl* impl = liblock_allocate(sizeof(struct liblock_impl));

    impl->tail = NULL;
    impl->cs_start_time = 0;
    pthread_mutex_init(&impl->posix_lock, 0);
  
    return impl;
}

static int do_liblock_destroy_lock(mcstp)(liblock_lock_t* lock)
{
    pthread_mutex_destroy(&lock->impl->posix_lock);
    return 0;
}

static void* do_liblock_execute_operation(mcstp)(liblock_lock_t* lock,
                                                 void* (*pending)(void*),
                                                 void* val)
{
    struct liblock_impl* impl = lock->impl;
    void* res;
    
    lock_mcstp(impl);
    
    res = pending(val);
    
    unlock_mcstp(impl);
    
    return res;
}

static void do_liblock_init_library(mcstp)()
{
    if (PAPI_is_initialized() == PAPI_NOT_INITED &&
        PAPI_library_init(PAPI_VER_CURRENT) < 0)
        fatal("PAPI_library_init failed");
}

static void do_liblock_kill_library(mcstp)()
{}

static void do_liblock_run(mcstp)(void (*callback)())
{
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
                                   1, 0) != 1)
        fatal("servers are not managed by hand");
    if(callback)
        callback();
}

static int do_liblock_cond_init(mcstp)(liblock_cond_t* cond)
{ 
    return cond->has_attr ?
        pthread_cond_init(&cond->impl.posix_cond, &cond->attr) :
        pthread_cond_init(&cond->impl.posix_cond, 0);
}

static int cond_timedwait(liblock_cond_t* cond,
                          liblock_lock_t* lock,
                          const struct timespec* ts) {
    struct liblock_impl* impl = lock->impl;
    int res;

    pthread_mutex_lock(&impl->posix_lock);
    unlock_mcstp(impl);
    
    if(ts)
        res = pthread_cond_timedwait(&cond->impl.posix_cond,
                                     &impl->posix_lock, ts);
    else
        res = pthread_cond_wait(&cond->impl.posix_cond, &impl->posix_lock);
    
    pthread_mutex_unlock(&impl->posix_lock);
    lock_mcstp(impl);

    return res;
}

static int do_liblock_cond_timedwait(mcstp)(liblock_cond_t* cond,
                                            liblock_lock_t* lock,
                                            const struct timespec* ts)
{ 
    return cond_timedwait(cond, lock, ts);
}

static int do_liblock_cond_wait(mcstp)(liblock_cond_t* cond,
                                        liblock_lock_t* lock)
{ 
    return cond_timedwait(cond, lock, 0);
}

static int do_liblock_cond_signal(mcstp)(liblock_cond_t* cond)
{ 
    return pthread_cond_signal(&cond->impl.posix_cond);
}

static int do_liblock_cond_broadcast(mcstp)(liblock_cond_t* cond)
{
    return pthread_cond_broadcast(&cond->impl.posix_cond);
}

static int do_liblock_cond_destroy(mcstp)(liblock_cond_t* cond)
{ 
    return pthread_cond_destroy(&cond->impl.posix_cond);
}

static void do_liblock_on_thread_start(mcstp)(struct thread_descriptor* desc)
{
    my_qnode = anon_mmap(r_align(sizeof(mcstp_qnode), PAGE_SIZE));
    
    my_qnode->last_lock = NULL;
    my_qnode->time = 0;
    my_qnode->status = INIT;
    my_qnode->next = NULL;

    if (PAPI_thread_init(pthread_self) != PAPI_OK)
        fatal("PAPI_thread_init failed");
}

static void do_liblock_on_thread_exit(mcstp)(struct thread_descriptor* desc)
{
    munmap(my_qnode, r_align(sizeof(mcstp_qnode), PAGE_SIZE));
}

static void do_liblock_unlock_in_cs(mcstp)(liblock_lock_t* lock)
{
    unlock_mcstp(lock->impl);
}

static void do_liblock_relock_in_cs(mcstp)(liblock_lock_t* lock)
{
    lock_mcstp(lock->impl);
}

static void do_liblock_declare_server(mcstp)(struct core* core)
{}

liblock_declare(mcstp);

