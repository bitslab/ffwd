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

// CAS
#define CAS __sync_val_compare_and_swap

// SWAP
#define SWAP __sync_lock_test_and_set


/* Node */
typedef struct dsmsynch_node {
	void *(*volatile req)(void *);
	void *volatile ret;
	char volatile wait;
	char volatile completed;
	struct dsmsynch_node *volatile next; // 'volatile' probably not needed
	char __pad[pad_to_cache_line(sizeof(void *(*)(void *)) +
								 sizeof(void *) +
								 sizeof(char) +
								 sizeof(char) +
								 sizeof(struct dsmsynch_node *))];
} dsmsynch_node;

/* The lock itself (only contains the global tail) */
struct liblock_impl {
    dsmsynch_node *volatile tail;
    char __pad[pad_to_cache_line(sizeof(dsmsynch_node *))];
};


/* Local node */
static __thread dsmsynch_node *my_nodes[2];
static __thread int toggle = 0;

static struct liblock_impl *do_liblock_init_lock(dsmsynch)
                                (liblock_lock_t *lock,
                                 struct core *core,
                                 pthread_mutexattr_t *attr)
{
    struct liblock_impl *impl = liblock_allocate(sizeof(struct liblock_impl));

    impl->tail = NULL;

    return impl;
}

static int do_liblock_destroy_lock(dsmsynch)(liblock_lock_t *lock)
{
    return 0;
}

static void *do_liblock_execute_operation(dsmsynch)(liblock_lock_t *lock,
                                                   void *(*pending)(void *),
                                                   void *val)
{
	struct liblock_impl *impl = lock->impl;
	dsmsynch_node *tmp_node, *my_node, *my_pred_node;
	int counter = 0;

	toggle = 1 - toggle;
	my_node = my_nodes[toggle];
	my_node->wait = 1;
	my_node->completed = 0;
	my_node->next = NULL;
	my_node->req = pending;

	my_pred_node = SWAP(&impl->tail, my_node);

	if (my_pred_node)
	{
		my_pred_node->next = my_node;
		
		while (my_node->wait)
			PAUSE();

		if (my_node->completed)
			return my_node->ret;
	}

	tmp_node = my_node;

	for (;;)
	{
		counter++;

		tmp_node->ret = (*tmp_node->req)(val);
		tmp_node->completed = 1;
		tmp_node->wait = 0;

		if (!tmp_node->next || !tmp_node->next->next || counter >= H)
			break;
		
		tmp_node = tmp_node->next;
	}

	if (!tmp_node->next)
	{
		if (CAS(&impl->tail, tmp_node, NULL) == tmp_node)
			return my_node->ret;
		while (!tmp_node->next)
			PAUSE();
	}

	tmp_node->next->wait = 0;
	tmp_node->next = NULL;

    SFENCE();

	return my_node->ret;
}

static void do_liblock_init_library(dsmsynch)()
{}

static void do_liblock_kill_library(dsmsynch)()
{}

static void do_liblock_run(dsmsynch)(void (*callback)())
{
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
                                    1, 0) != 1)
        fatal("servers are not managed manually");
    if(callback)
        callback();
}

static int do_liblock_cond_init(dsmsynch)(liblock_cond_t *cond)
{
 	fatal("not implemented");
}

static int do_liblock_cond_timedwait(dsmsynch)(liblock_cond_t *cond,
                                              liblock_lock_t *lock,
                                              const struct timespec* ts)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_wait(dsmsynch)(liblock_cond_t *cond,
									     liblock_lock_t *lock)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_signal(dsmsynch)(liblock_cond_t *cond)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_broadcast(dsmsynch)(liblock_cond_t *cond)
{
    fatal("not implemented");
}

static int do_liblock_cond_destroy(dsmsynch)(liblock_cond_t *cond)
{
    fatal("not implemented"); 
}

static void do_liblock_on_thread_start(dsmsynch)(struct thread_descriptor *desc)
{
	int i;

	for (i = 0; i < 2; i++)
	{
    	my_nodes[i] = anon_mmap(r_align(sizeof(dsmsynch_node), PAGE_SIZE));
    	
    	my_nodes[i]->req = NULL;
    	my_nodes[i]->ret = NULL;
    	my_nodes[i]->wait = 0;
		my_nodes[i]->completed = 0;
    	my_nodes[i]->next = NULL;
	}
}

static void do_liblock_on_thread_exit(dsmsynch)(struct thread_descriptor *desc)
{
	int i;
	
	for (i = 0; i < 2; i++)
	{
    	munmap(my_nodes[i], r_align(sizeof(dsmsynch_node), PAGE_SIZE));
	}
}

static void do_liblock_unlock_in_cs(dsmsynch)(liblock_lock_t *lock)
{
    fatal("not implemented");
}

static void do_liblock_relock_in_cs(dsmsynch)(liblock_lock_t *lock)
{
    fatal("not implemented");
}

static void do_liblock_declare_server(dsmsynch)(struct core *core)
{}

liblock_declare(dsmsynch);

