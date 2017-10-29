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
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "liblock.h"
#include "liblock-fatal.h"



/* ########################################################################## */
/* Based on "Revisiting the Combining Synchronization Technique" (P. Fatourou */
/* and N. D. Kallimanis, PPoPP'12).                                           */
/* ########################################################################## */


/* Maximum number of iterations by the combiner */
#define H 144


// SFENCE                                                                        
#define SFENCE()  asm volatile("sfence"::) 

// SWAP
#define SWAP __sync_lock_test_and_set

 
/* Node */
typedef struct ccsynch_node {
	void *(*volatile req)(void *);
	void *volatile ret;
	char volatile wait;
	char volatile completed;
	struct ccsynch_node *volatile next; // 'volatile' probably not needed
	char __pad[pad_to_cache_line(sizeof(void *(*)(void *)) +
								 sizeof(void *) +
								 sizeof(char) +
								 sizeof(char) +
								 sizeof(struct ccsynch_node *))];
} ccsynch_node;

/* The lock itself (only contains the global tail) */
struct liblock_impl {
    ccsynch_node *volatile tail;
    char __pad[pad_to_cache_line(sizeof(ccsynch_node *))];
};


/* Local node */
static __thread ccsynch_node *my_node;


static struct liblock_impl *do_liblock_init_lock(ccsynch)
                                (liblock_lock_t *lock,
                                 struct core *core,
                                 pthread_mutexattr_t *attr)
{
    struct liblock_impl *impl = liblock_allocate(sizeof(struct liblock_impl));

    impl->tail = anon_mmap(r_align(sizeof(ccsynch_node), PAGE_SIZE));
    
    impl->tail->req = NULL;
    impl->tail->ret = NULL;
    impl->tail->wait = 0;
	impl->tail->completed = 0;
    impl->tail->next = NULL;

    return impl;
}

static int do_liblock_destroy_lock(ccsynch)(liblock_lock_t *lock)
{
    /* No need to unmap impl ? */
	munmap(lock->impl->tail, r_align(sizeof(ccsynch_node), PAGE_SIZE));

    return 0;
}

static void *do_liblock_execute_operation(ccsynch)(liblock_lock_t *lock,
                                                   void *(*pending)(void*),
                                                   void *val)
{
	struct liblock_impl *impl = lock->impl;
    ccsynch_node *next_node, *cur_node, *tmp_node, *tmp_node_next;
	int counter = 0;
	
    next_node = my_node;
	next_node->next = NULL;
	next_node->wait = 1;
	next_node->completed = 0;
	
	cur_node = SWAP(&impl->tail, next_node);
	cur_node->req = pending;
	cur_node->next = next_node;
	
	my_node = cur_node;
	
	while (cur_node->wait)
		PAUSE();

	if (cur_node->completed)
		return cur_node->ret;

	tmp_node = cur_node;

    while (tmp_node->next && counter < H)
	{
		counter++;

		tmp_node_next = tmp_node->next;

		tmp_node->ret = (*tmp_node->req)(val);
		tmp_node->completed = 1;
		tmp_node->wait = 0;

		tmp_node = tmp_node_next;
	}

	tmp_node->wait = 0;

	SFENCE();

	return cur_node->ret;
}

static void do_liblock_init_library(ccsynch)()
{}

static void do_liblock_kill_library(ccsynch)()
{}

static void do_liblock_run(ccsynch)(void (*callback)())
{
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
                                    1, 0) != 1)
        fatal("servers are not managed manually");
    if(callback)
        callback();
}

static int do_liblock_cond_init(ccsynch)(liblock_cond_t *cond)
{
 	fatal("not implemented");
}

static int do_liblock_cond_timedwait(ccsynch)(liblock_cond_t *cond,
                                              liblock_lock_t *lock,
                                              const struct timespec *ts)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_wait(ccsynch)(liblock_cond_t *cond,
									     liblock_lock_t *lock)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_signal(ccsynch)(liblock_cond_t *cond)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_broadcast(ccsynch)(liblock_cond_t *cond)
{
    fatal("not implemented");
}

static int do_liblock_cond_destroy(ccsynch)(liblock_cond_t *cond)
{
    fatal("not implemented"); 
}

static void do_liblock_on_thread_start(ccsynch)(struct thread_descriptor *desc)
{
    my_node = anon_mmap(r_align(sizeof(ccsynch_node), PAGE_SIZE));
    
    my_node->req = NULL;
    my_node->ret = NULL;
    my_node->wait = 0;
	my_node->completed = 0;
    my_node->next = NULL;
}

static void do_liblock_on_thread_exit(ccsynch)(struct thread_descriptor *desc)
{
    munmap(my_node, r_align(sizeof(ccsynch_node), PAGE_SIZE));
}

static void do_liblock_unlock_in_cs(ccsynch)(liblock_lock_t *lock)
{
    fatal("not implemented");
}

static void do_liblock_relock_in_cs(ccsynch)(liblock_lock_t *lock)
{
    fatal("not implemented");
}

static void do_liblock_declare_server(ccsynch)(struct core *core)
{}

liblock_declare(ccsynch);

