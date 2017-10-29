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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "liblock.h"
#include "liblock-fatal.h"

struct mcs_mon_node {
	struct mcs_mon_node* volatile next;
	int volatile spin;
	char __pad[pad_to_cache_line(sizeof(void*) + sizeof(int))];
};

struct liblock_impl {
	pthread_mutex_t posix_lock;
	struct mcs_mon_node* volatile tail;
	char pad[pad_to_cache_line(sizeof(pthread_mutex_t) + sizeof(void*))];
};

static inline void *xchg(struct mcs_mon_node* volatile* ptr, 
                         struct mcs_mon_node* x)
{
	__asm__ __volatile__ ("xchg %0,%1" : "=r" (x)
									   : "m" (*ptr), "0" (x)
									   : "memory");

	return x;
}

static int thread_counter = 0;

static __thread struct mcs_mon_node* my_node = 0;
static __thread int thread_id;
static __thread long long thread_cycle_counter = -1;
static __thread long long time_before;

static __thread int event_set = PAPI_NULL;
static __thread long long results[1];

static int mcs_mon_used = 0;

/*
static __thread unsigned long long last_count = 0, curr_count = 0;
static unsigned long long count = 0;
*/

#define MON_START (time_before = PAPI_get_real_cyc());
#define MON_STOP  (thread_cycle_counter += PAPI_get_real_cyc() - time_before);

#define MON_THREAD 47

#define MON_START_THREAD(t) {if (thread_id == (t)) \
                                 time_before = PAPI_get_real_cyc();}
#define MON_STOP_THREAD(t)  {if (thread_id == (t)) \
                                thread_cycle_counter += PAPI_get_real_cyc() - time_before;}

#define MON_EVENT_ON 1
#define MON_EVENT PAPI_L2_DCM
//#define MON_EVENT PAPI_TOT_INS


static inline void lock_mcs_mon(struct liblock_impl* impl)
{
	struct mcs_mon_node *tail, *me = my_node;

//MON_START

#if MON_EVENT_ON == 1
if (thread_id == MON_THREAD) 
{
    PAPI_reset(event_set);
}
#endif

	me->next = 0;

//MON_STOP

//MON_START

	me->spin = 0;

//MON_STOP

	tail = __sync_lock_test_and_set(&impl->tail, me); //xchg(&impl->tail, me);

//MON_STOP
//MON_START	
	/* No one there? */
	if (!tail) {
//MON_START
//MON_START_THREAD(MON_THREAD)
		return;
    }
//MON_START

	/* Someone there, need to link in */
	tail->next = me;

//MON_STOP

//MON_START

#if MON_EVENT_ON == 1
    if (thread_id == MON_THREAD) 
    {
        PAPI_accum(event_set, results);
    }
#endif

    /* Spin on my spin variable */
 	while (!me->spin) {
		PAUSE();

//MON_START
//MON_START_THREAD(MON_THREAD)
	}

//MON_START
//MON_STOP

/*
    // Results of this : 46.0 ! (pretty obvious, in hindsight)
    
    curr_count = __sync_fetch_and_add(&count, 1);
    thread_cycle_counter += curr_count - last_count;
    last_count = curr_count;
*/
    return;
}

static inline void unlock_mcs_mon(struct liblock_impl* impl)
{
//MON_START

	struct mcs_mon_node* me = my_node;
	
    /* No successor yet? */
	if (!me->next) {
		/* Try to atomically unlock */
		if (__sync_val_compare_and_swap(&impl->tail, me, 0) == me) {
//MON_STOP
//MON_STOP_THREAD(MON_THREAD)
			return;
        }
	
		/* Wait for successor to appear */
		while(!me->next) 
			PAUSE();
	}

//MON_START

//MON_STOP

	/* Unlock next one */
	me->next->spin = 1;

//__sync_synchronize();

//MON_STOP

//MON_STOP_THREAD(MON_THREAD)
}

static struct liblock_impl* do_liblock_init_lock(mcs_mon)(liblock_lock_t* lock, 
        struct core* core, pthread_mutexattr_t* attr)
{
	struct liblock_impl* impl = liblock_allocate(sizeof(struct liblock_impl));

	impl->tail = 0;
	pthread_mutex_init(&impl->posix_lock, 0);

    mcs_mon_used = 1;
	
	return impl;
}

static int do_liblock_destroy_lock(mcs_mon)(liblock_lock_t* lock)
{
	pthread_mutex_destroy(&lock->impl->posix_lock);
	return 0;
}

static void* do_liblock_execute_operation(mcs_mon)(liblock_lock_t* lock, void* (*pending)(void*), void* val)
{
	struct liblock_impl* impl = lock->impl;
	void* res;

//MON_START

	lock_mcs_mon(impl);

//MON_STOP
//MON_START

	res = pending(val);

//MON_STOP
//MON_START

	unlock_mcs_mon(impl);

//MON_STOP

	return res;
}

static void do_liblock_init_library(mcs_mon)()
{
    if (PAPI_is_initialized() == PAPI_NOT_INITED &&
        PAPI_library_init(PAPI_VER_CURRENT) < 0)                                 
        fatal("PAPI_library_init failed");
}

static void do_liblock_kill_library(mcs_mon)()
{}

static void do_liblock_run(mcs_mon)(void (*callback)())
{
	if (__sync_val_compare_and_swap(&liblock_start_server_threads_by_hand, 1, 0) != 1)
		fatal("servers are not managed by hand");
	if (callback)
		callback();
}

static int do_liblock_cond_init(mcs_mon)(liblock_cond_t* cond)
{ 
	return cond->has_attr ?
		pthread_cond_init(&cond->impl.posix_cond, &cond->attr) :
		pthread_cond_init(&cond->impl.posix_cond, 0);
}

static int cond_timedwait(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts)
{
	struct liblock_impl* impl = lock->impl;
	int res;

	pthread_mutex_lock(&impl->posix_lock);
	
	unlock_mcs_mon(impl);
	if (ts)
		res = pthread_cond_timedwait(&cond->impl.posix_cond, &impl->posix_lock, ts);
	else
		res = pthread_cond_wait(&cond->impl.posix_cond, &impl->posix_lock);

	pthread_mutex_unlock(&impl->posix_lock);

	lock_mcs_mon(impl);

	return res;
}

static int do_liblock_cond_timedwait(mcs_mon)(liblock_cond_t* cond,
        liblock_lock_t* lock, const struct timespec* ts)
{ 
	return cond_timedwait(cond, lock, ts);
}

static int do_liblock_cond_wait(mcs_mon)(liblock_cond_t* cond, liblock_lock_t* lock)
{ 
	return cond_timedwait(cond, lock, 0);
}

static int do_liblock_cond_signal(mcs_mon)(liblock_cond_t* cond)
{ 
	return pthread_cond_signal(&cond->impl.posix_cond);
}

static int do_liblock_cond_broadcast(mcs_mon)(liblock_cond_t* cond)
{ 
	return pthread_cond_broadcast(&cond->impl.posix_cond);
}

static int do_liblock_cond_destroy(mcs_mon)(liblock_cond_t* cond)
{ 
	return pthread_cond_destroy(&cond->impl.posix_cond);
}

static void do_liblock_on_thread_start(mcs_mon)(struct thread_descriptor* desc)
{
    my_node = anon_mmap(r_align(sizeof(struct mcs_mon_node), PAGE_SIZE));
    
    thread_id = __sync_fetch_and_add(&thread_counter, 1);

	if (PAPI_thread_init((unsigned long (*)(void))pthread_self) != PAPI_OK)
        fatal("PAPI_thread_init failed");

// Events -------------------------------------------------------------------- v
#if MON_EVENT_ON == 1
    if (thread_id == MON_THREAD)
    {
        if (PAPI_create_eventset(&event_set) != PAPI_OK)
            fatal("PAPI_create_eventset failed");

        if (PAPI_add_event(event_set, MON_EVENT) != PAPI_OK)
            fatal("PAPI_add_event failed");

        if (PAPI_start(event_set) != PAPI_OK)
            fatal("PAPI_start failed");
        
        if (PAPI_stop(event_set, results) != PAPI_OK)
            fatal("PAPI_stop failed");
        
        if (PAPI_start(event_set) != PAPI_OK)
            fatal("PAPI_start failed");
        
        results[0] = 0;
    }
    else
    {
        results[0] = -1;
    }
#endif
// --------------------------------------------------------------------------- ^
}

static void do_liblock_on_thread_exit(mcs_mon)(struct thread_descriptor* desc)
{
// Events -------------------------------------------------------------------- v
#if MON_EVENT_ON == 1
    if (thread_id == MON_THREAD)
    {
//        if (PAPI_stop(event_set, results) != PAPI_OK)
//            fatal("PAPI_stop failed");
    }
#endif
// --------------------------------------------------------------------------- ^

    if (mcs_mon_used)
    {
#if MON_EVENT_ON == 0
        printf("Thread %d: %lld\n", thread_id, thread_cycle_counter); 
#else
        printf("Thread %d: %lld\n", thread_id, results[0]); 
#endif
    }

    munmap(my_node, r_align(sizeof(struct mcs_mon_node), PAGE_SIZE));
}

static void do_liblock_unlock_in_cs(mcs_mon)(liblock_lock_t* lock)
{
	unlock_mcs_mon(lock->impl);
}

static void do_liblock_relock_in_cs(mcs_mon)(liblock_lock_t* lock)
{
	lock_mcs_mon(lock->impl);
}

static void do_liblock_declare_server(mcs_mon)(struct core* core)
{}

liblock_declare(mcs_mon);

