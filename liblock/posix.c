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
#include <string.h>
#include <errno.h>
#include "liblock.h"
#include "liblock-fatal.h"

struct liblock_impl {
	pthread_mutex_t posix_lock;
};

static struct liblock_impl* do_liblock_init_lock(posix)(liblock_lock_t* lock, struct core* server, pthread_mutexattr_t* attr) {
	struct liblock_impl* impl = liblock_allocate(sizeof(struct liblock_impl));
	pthread_mutex_init(&impl->posix_lock, attr);
	return impl;
}

static int do_liblock_destroy_lock(posix)(liblock_lock_t* lock) {
	return pthread_mutex_destroy(&lock->impl->posix_lock);
}

static void* do_liblock_execute_operation(posix)(liblock_lock_t* lock, void* (*pending)(void*), void* val) {
	struct liblock_impl* impl = lock->impl;
	void* res;

	//printf("posix locking %p\n", lock);
	pthread_mutex_lock(&impl->posix_lock);

	res = pending(val);

	pthread_mutex_unlock(&impl->posix_lock);
	
	return res;
}

static void do_liblock_init_library(posix)() {
}

static void do_liblock_kill_library(posix)() {
}

static void do_liblock_run(posix)(void (*callback)()) {
	if(__sync_val_compare_and_swap(&liblock_start_server_threads_by_hand, 1, 0) != 1)
		fatal("servers are not managed by hand");
	if(callback)
		callback();
}

static int do_liblock_cond_init(posix)(liblock_cond_t* cond) { 
	return cond->has_attr ?
		pthread_cond_init(&cond->impl.posix_cond, &cond->attr) :
		pthread_cond_init(&cond->impl.posix_cond, 0);
}

static int do_liblock_cond_wait(posix)(liblock_cond_t* cond, liblock_lock_t* lock) { 
	return pthread_cond_wait(&cond->impl.posix_cond, &lock->impl->posix_lock);
}

static int do_liblock_cond_timedwait(posix)(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) { 
	return pthread_cond_timedwait(&cond->impl.posix_cond, &lock->impl->posix_lock, ts);
}

static int do_liblock_cond_signal(posix)(liblock_cond_t* cond) { 
	return pthread_cond_signal(&cond->impl.posix_cond);
}

static int do_liblock_cond_broadcast(posix)(liblock_cond_t* cond) { 
	return pthread_cond_broadcast(&cond->impl.posix_cond);
}

static int do_liblock_cond_destroy(posix)(liblock_cond_t* cond) { 
	return pthread_cond_destroy(&cond->impl.posix_cond);
}

static void do_liblock_on_thread_start(posix)(struct thread_descriptor* desc) {
}

static void do_liblock_on_thread_exit(posix)(struct thread_descriptor* desc) {
}

static void do_liblock_unlock_in_cs(posix)(liblock_lock_t* lock) {
	pthread_mutex_unlock(&lock->impl->posix_lock);
}

static void do_liblock_relock_in_cs(posix)(liblock_lock_t* lock) {
	pthread_mutex_lock(&lock->impl->posix_lock);
}

static void do_liblock_declare_server(posix)(struct core* core) {
}

liblock_declare(posix);
