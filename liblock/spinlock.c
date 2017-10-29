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
#include <string.h>
#include <errno.h>
#include "liblock.h"
#include "liblock-fatal.h"

struct liblock_impl {
	pthread_mutex_t       posix_lock;
	unsigned int volatile lock;
	char                  pad[pad_to_cache_line(sizeof(pthread_mutex_t) + sizeof(unsigned int))];
};

static struct liblock_impl* do_liblock_init_lock(spinlock)(liblock_lock_t* lock, struct core* server, pthread_mutexattr_t* attr) {
	struct liblock_impl* impl = liblock_allocate(sizeof(struct liblock_impl));

	impl->lock = 0;
	pthread_mutex_init(&impl->posix_lock, 0);

	return impl;
}

static int do_liblock_destroy_lock(spinlock)(liblock_lock_t* lock) {
	pthread_mutex_destroy(&lock->impl->posix_lock);
	return 0;
}

static void* do_liblock_execute_operation(spinlock)(liblock_lock_t* lock, void* (*pending)(void*), void* val) {
	struct liblock_impl* impl = lock->impl;
	void* res;
	while(__sync_val_compare_and_swap(&impl->lock, 0, 1)) {
		PAUSE();
	}
	
	res = pending(val);

	impl->lock = 0;

	return res;
}

static void do_liblock_init_library(spinlock)() {
}

static void do_liblock_kill_library(spinlock)() {
}

static void do_liblock_run(spinlock)(void (*callback)()) {
	if(__sync_val_compare_and_swap(&liblock_start_server_threads_by_hand, 1, 0) != 1)
		fatal("servers are not managed by hand");
	if(callback)
		callback();
}

static int do_liblock_cond_init(spinlock)(liblock_cond_t* cond) { 
	return cond->has_attr ?
		pthread_cond_init(&cond->impl.posix_cond, &cond->attr) :
		pthread_cond_init(&cond->impl.posix_cond, 0);
}

static int cond_timedwait(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) {
	struct liblock_impl* impl = lock->impl;
	int res;

	pthread_mutex_lock(&impl->posix_lock);
	impl->lock = 0;
	if(ts)
		res = pthread_cond_timedwait(&cond->impl.posix_cond, &impl->posix_lock, ts);
	else
		res = pthread_cond_wait(&cond->impl.posix_cond, &impl->posix_lock);
	pthread_mutex_unlock(&impl->posix_lock);

	while(__sync_val_compare_and_swap(&impl->lock, 0, 1))
		PAUSE();

	return res;
}

static int do_liblock_cond_timedwait(spinlock)(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) { 
	return cond_timedwait(cond, lock, ts);
}

static int do_liblock_cond_wait(spinlock)(liblock_cond_t* cond, liblock_lock_t* lock) { 
	return cond_timedwait(cond, lock, 0);
}

static int do_liblock_cond_signal(spinlock)(liblock_cond_t* cond) { 
	return pthread_cond_signal(&cond->impl.posix_cond);
}

static int do_liblock_cond_broadcast(spinlock)(liblock_cond_t* cond) { 
	return pthread_cond_broadcast(&cond->impl.posix_cond);
}

static int do_liblock_cond_destroy(spinlock)(liblock_cond_t* cond) { 
	return pthread_cond_destroy(&cond->impl.posix_cond);
}

static void do_liblock_on_thread_exit(spinlock)(struct thread_descriptor* desc) {
}

static void do_liblock_on_thread_start(spinlock)(struct thread_descriptor* desc) {
}

static void do_liblock_unlock_in_cs(spinlock)(liblock_lock_t* lock) {
	struct liblock_impl* impl = lock->impl;
	impl->lock = 0;
}

static void do_liblock_relock_in_cs(spinlock)(liblock_lock_t* lock) {
	struct liblock_impl* impl = lock->impl;
	while(__sync_val_compare_and_swap(&impl->lock, 0, 1))
		PAUSE();
}

static void do_liblock_declare_server(spinlock)(struct core* core) {
}

liblock_declare(spinlock);
