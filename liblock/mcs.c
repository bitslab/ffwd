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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "liblock.h"
#include "liblock-fatal.h"

struct mcs_node {
	struct mcs_node* volatile next;
	int              volatile spin;
	char             __pad[pad_to_cache_line(sizeof(void*) + sizeof(int))];
};

struct liblock_impl {
	pthread_mutex_t           posix_lock;
	struct mcs_node* volatile tail;
	char                      pad[pad_to_cache_line(sizeof(pthread_mutex_t) + sizeof(void*))];
};

static inline void *xchg(struct mcs_node* volatile* ptr, struct mcs_node* x) {
	__asm__ __volatile__(  "xchg %0,%1"
											 : "=r" (x)
											 : "m" (*ptr), "0" (x)
											 : "memory");

	return x;
}

__thread struct mcs_node* my_node = 0;

static void lock_mcs(struct liblock_impl* impl) {
	struct mcs_node *tail, *me = my_node;
	
	me->next = 0;
	me->spin = 0;

	tail = __sync_lock_test_and_set(&impl->tail, me); //xchg(&impl->tail, me);
	
	/* No one there? */
	if (!tail) 
		return;

	/* Someone there, need to link in */
	tail->next = me;

	/* Spin on my spin variable */
 	while (!me->spin) {
		PAUSE();
	}

	return;
}

static void unlock_mcs(struct liblock_impl* impl) {
	struct mcs_node* me = my_node;
	/* No successor yet? */
	if (!me->next) {
		/* Try to atomically unlock */
		if (__sync_val_compare_and_swap(&impl->tail, me, 0) == me) 
			return;
	
		/* Wait for successor to appear */
		while(!me->next) 
			PAUSE();
	}

	/* Unlock next one */
	me->next->spin = 1;	
}

static struct liblock_impl* do_liblock_init_lock(mcs)(liblock_lock_t* lock, struct core* core, pthread_mutexattr_t* attr) {
	struct liblock_impl* impl = liblock_allocate(sizeof(struct liblock_impl));

	impl->tail = 0;
	pthread_mutex_init(&impl->posix_lock, 0);
	
	return impl;
}

static int do_liblock_destroy_lock(mcs)(liblock_lock_t* lock) {
	pthread_mutex_destroy(&lock->impl->posix_lock);
	return 0;
}

static void* do_liblock_execute_operation(mcs)(liblock_lock_t* lock, void* (*pending)(void*), void* val) {
	struct liblock_impl* impl = lock->impl;
	void* res;
	//	static int n = 0; if(!(__sync_fetch_and_add(&n, 1) % 100)) printf("execute mcs %d\n", n);
	lock_mcs(impl);

	res = pending(val);

	unlock_mcs(impl);

	return res;
}

static void do_liblock_init_library(mcs)() {
}

static void do_liblock_kill_library(mcs)() {
}

static void do_liblock_run(mcs)(void (*callback)()) {
	if(__sync_val_compare_and_swap(&liblock_start_server_threads_by_hand, 1, 0) != 1)
		fatal("servers are not managed by hand");
	if(callback)
		callback();
}

static int do_liblock_cond_init(mcs)(liblock_cond_t* cond) { 
	return cond->has_attr ?
		pthread_cond_init(&cond->impl.posix_cond, &cond->attr) :
		pthread_cond_init(&cond->impl.posix_cond, 0);
}

static int cond_timedwait(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) {
	struct liblock_impl* impl = lock->impl;
	int res;

	pthread_mutex_lock(&impl->posix_lock);
	
	unlock_mcs(impl);
	if(ts)
		res = pthread_cond_timedwait(&cond->impl.posix_cond, &impl->posix_lock, ts);
	else
		res = pthread_cond_wait(&cond->impl.posix_cond, &impl->posix_lock);
	pthread_mutex_unlock(&impl->posix_lock);

	lock_mcs(impl);

	return res;
}

static int do_liblock_cond_timedwait(mcs)(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) { 
	return cond_timedwait(cond, lock, ts);
}

static int do_liblock_cond_wait(mcs)(liblock_cond_t* cond, liblock_lock_t* lock) { 
	return cond_timedwait(cond, lock, 0);
}

static int do_liblock_cond_signal(mcs)(liblock_cond_t* cond) { 
	return pthread_cond_signal(&cond->impl.posix_cond);
}

static int do_liblock_cond_broadcast(mcs)(liblock_cond_t* cond) { 
	return pthread_cond_broadcast(&cond->impl.posix_cond);
}

static int do_liblock_cond_destroy(mcs)(liblock_cond_t* cond) { 
	return pthread_cond_destroy(&cond->impl.posix_cond);
}

static void do_liblock_on_thread_start(mcs)(struct thread_descriptor* desc) {
	my_node = anon_mmap(r_align(sizeof(struct mcs_node), PAGE_SIZE));
}

static void do_liblock_on_thread_exit(mcs)(struct thread_descriptor* desc) {
	munmap(my_node, r_align(sizeof(struct mcs_node), PAGE_SIZE));
}

static void do_liblock_unlock_in_cs(mcs)(liblock_lock_t* lock) {
	unlock_mcs(lock->impl);
}

static void do_liblock_relock_in_cs(mcs)(liblock_lock_t* lock) {
	lock_mcs(lock->impl);
}

static void do_liblock_declare_server(mcs)(struct core* core) {
}

liblock_declare(mcs);

