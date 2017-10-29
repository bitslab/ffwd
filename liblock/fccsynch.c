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
/* Based on "Revisiting the Combining Synchronization Technique" (P. Fatourou */
/* and N. D. Kallimanis, PPoPP'12).                                           */
/* ########################################################################## */


/* Maximum number of iterations by the combiner */
#define H 1440000


// SFENCE                                                                        
#define SFENCE()  asm volatile("sfence"::) 

// SWAP
#define SWAP __sync_lock_test_and_set

 
/* Node */
typedef struct fccsynch_node {
	void *(*volatile req)(void *);
	void *volatile ret;
	char volatile wait;
	char volatile completed;
	struct fccsynch_node *volatile next; // 'volatile' probably not needed
    int volatile x;
	char __pad[pad_to_cache_line(sizeof(void *(*)(void *)) +
								 sizeof(void *) +
								 sizeof(char) +
								 sizeof(char) +
								 sizeof(struct fccsynch_node *))];
} fccsynch_node;

/* The lock itself (only contains the global tail) */
struct liblock_impl {
    fccsynch_node *volatile tail;
    char __pad[pad_to_cache_line(sizeof(fccsynch_node *))];
};

volatile int count = 0;
void testcount(void) {
    count++;
}

/* Local node */
static __thread fccsynch_node *my_node;

int event_set;
int thread_count = 0;
__thread int thread_id;

static struct liblock_impl *do_liblock_init_lock(fccsynch)
                                (liblock_lock_t *lock,
                                 struct core *core,
                                 pthread_mutexattr_t *attr)
{
    struct liblock_impl *impl = liblock_allocate(sizeof(struct liblock_impl));

    impl->tail = anon_mmap(r_align(sizeof(fccsynch_node), PAGE_SIZE));
    
    impl->tail->req = NULL;
    impl->tail->ret = NULL;
    impl->tail->wait = 0;
	impl->tail->completed = 0;
    impl->tail->next = NULL;
/*
    if (PAPI_library_init(PAPI_VER_CURRENT) < 0)                                 
        fatal("PAPI_library_init failed"); 

    if(PAPI_create_eventset(&event_set) != PAPI_OK)                          
        warning("PAPI_create_eventset");                                     
    else if(PAPI_add_event(event_set, PAPI_L2_DCM) != PAPI_OK)          
        warning("PAPI_add_events");                                          
  */                                                                                
    return impl;
}

static int do_liblock_destroy_lock(fccsynch)(liblock_lock_t *lock)
{
    /* No need to unmap impl ? */
//	munmap(lock->impl->tail, r_align(sizeof(fccsynch_node), PAGE_SIZE));


    return 0;
}

static void *do_liblock_execute_operation(fccsynch)(liblock_lock_t *lock,
                                                   void *(*pending)(void*),
                                                   void *val)
{
	struct liblock_impl *impl = lock->impl;
    fccsynch_node *next_node, *cur_node, *tmp_node, *tmp_node_next;
	int counter = 0;
	
    next_node = my_node;
	next_node->next = NULL;
	next_node->wait = 1;
	next_node->completed = 0;
	
	cur_node = SWAP(&impl->tail, next_node);
	cur_node->req = pending;
	cur_node->next = next_node;
	
	my_node = cur_node;
	
    int flag = 0;
/*    
    int c = 0;

    while (cur_node->wait && c < 100) {
       c++;
       PAUSE();
    }
 */

    counter = 0;
    tmp_node = cur_node;
    while (cur_node->wait) {
    	if (!flag) {
            while ((tmp_node_next = tmp_node->next) && counter < H)
            {
//              tmp_node_next->x = 1;
                counter++;
                tmp_node = tmp_node_next;
                if (!cur_node->wait) break;
            }
            if (counter == H) flag = 1;
        }

        PAUSE();
    }

/* 
    while (cur_node->wait)
        PAUSE();
*/

	if (cur_node->completed)
		return cur_node->ret;

	tmp_node = cur_node;

    counter = 0;
/*
    if(PAPI_start(event_set) != PAPI_OK)                                 
        warning("PAPI_start");                                           
*/
//pre_combiner_loop:
    while (tmp_node->next && counter < H)
	{
    
        //tmp_node->x++;
        //tmp_node->wait = 1;
        tmp_node->next->x = 1;
		counter++;

		tmp_node_next = tmp_node->next;
        // Au vu du résultat de objdump, quel que soit le degré d'optimisation,
        // il n'y a pas d'instructions de préfetch qui sont ajoutées 
        // automatiquement.
//        __builtin_prefetch(tmp_node_next, 1, 3);

        // bizarre, supprimer l'appel à la fonction améliore beaucoup les perfs
        // (21000->17000), même quand H est très grand... alors qu'enlever 
        // l'appel à la fonction dans rcl ne coûte rien. Autre chose de bizarre,
        // quand on passe d'un accès à une ligne de cache à zéro dans la section
        // critique, on ne gagne que mille cycles...
		tmp_node->ret = (tmp_node->req)(val);
        // Faire ça fait chuter à 17000 aussi, donc c'est bien l'appel à la 
        // fonction qui fait ramer.
		//tmp_node->ret = (void *)tmp_node->req;
		// Bizarrement on rest à 17000 avec ça, alors qu'elle aussi ne fait
        // qu'un cache miss...
        //testcount();
        tmp_node->completed = 1;
		tmp_node->wait = 0;

		tmp_node = tmp_node_next;
	}
//post_combiner_loop:
/*
     if (PAPI_stop(event_set, values) != PAPI_OK)                         
         warning("servicing-loop::PAPI_stop");
  */ 
	tmp_node->wait = 0;

	//SFENCE();
	return cur_node->ret;
}

static void do_liblock_init_library(fccsynch)()
{}

static void do_liblock_kill_library(fccsynch)()
{/* printf("%d\n", count);
printf("", my_node->x);*/
 }

static void do_liblock_run(fccsynch)(void (*callback)())
{
    if(__sync_bool_compare_and_swap(&liblock_start_server_threads_by_hand,
                                    1, 0) != 1)
        fatal("servers are not managed manually");
    if(callback)
        callback();
}

static int do_liblock_cond_init(fccsynch)(liblock_cond_t *cond)
{
 	fatal("not implemented");
}

static int do_liblock_cond_timedwait(fccsynch)(liblock_cond_t *cond,
                                              liblock_lock_t *lock,
                                              const struct timespec *ts)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_wait(fccsynch)(liblock_cond_t *cond,
									     liblock_lock_t *lock)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_signal(fccsynch)(liblock_cond_t *cond)
{ 
    fatal("not implemented");
}

static int do_liblock_cond_broadcast(fccsynch)(liblock_cond_t *cond)
{
    fatal("not implemented");
}

static int do_liblock_cond_destroy(fccsynch)(liblock_cond_t *cond)
{
    fatal("not implemented"); 
}

static void do_liblock_on_thread_start(fccsynch)(struct thread_descriptor *desc)
{
    my_node = anon_mmap(r_align(sizeof(fccsynch_node), PAGE_SIZE));
    
    my_node->req = NULL;
    my_node->ret = NULL;
    my_node->wait = 0;
	my_node->completed = 0;
    my_node->next = NULL;

    thread_id = __sync_fetch_and_add(&thread_count, 1) + 1;
}

static void do_liblock_on_thread_exit(fccsynch)(struct thread_descriptor *desc)
{
//    munmap(my_node, r_align(sizeof(fccsynch_node), PAGE_SIZE));
}

static void do_liblock_unlock_in_cs(fccsynch)(liblock_lock_t *lock)
{
    fatal("not implemented");
}

static void do_liblock_relock_in_cs(fccsynch)(liblock_lock_t *lock)
{
    fatal("not implemented");
}

static void do_liblock_declare_server(fccsynch)(struct core *core)
{}

liblock_declare(fccsynch);

