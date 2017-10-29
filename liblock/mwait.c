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

static struct liblock_impl* do_liblock_init_lock(mwait)(liblock_lock_t* lock, struct core* server, pthread_mutexattr_t* attr) {
	struct liblock_impl* impl = liblock_allocate(sizeof(struct liblock_impl));

	impl->lock = 0;
	pthread_mutex_init(&impl->posix_lock, 0);

	return impl;
}

static int do_liblock_destroy_lock(mwait)(liblock_lock_t* lock) {
	pthread_mutex_destroy(&lock->impl->posix_lock);
	return 0;
}

static inline void __monitor(const void volatile *eax, unsigned long ecx, unsigned long edx) {
	asm volatile("monitor"
							 //asm volatile(".byte 0x0f, 0x01, 0xc8;"
							 :: "a" (eax), "c" (ecx), "d"(edx));
}

static inline void __mwait(void volatile *eax, unsigned long ecx) {
	/* "mwait %eax, %ecx;" */
	asm volatile("mwait"
							 //asm volatile(".byte 0x0f, 0x01, 0xc9;"
							 :: "a" (eax), "c" (ecx));
}

static inline void acquire(struct liblock_impl* impl) {
    fprintf(stderr, "acquiring\n");
	while(__sync_val_compare_and_swap(&impl->lock, 0, 1)) {
		__monitor(&impl->lock, 0, 0);
		if(!impl->lock)
			__mwait(&impl->lock, 0);
	}
}

static inline void release(struct liblock_impl* impl) {
    fprintf(stderr, "releasing\n");
	impl->lock = 0;
}

static void* do_liblock_execute_operation(mwait)(liblock_lock_t* lock, void* (*pending)(void*), void* val) {
	struct liblock_impl* impl = lock->impl;
	void* res;

	acquire(impl);
	
	res = pending(val);

	release(impl);

	return res;
}

static void do_liblock_init_library(mwait)() {
}

static void do_liblock_kill_library(mwait)() {
}

static void do_liblock_run(mwait)(void (*callback)()) {
	if(__sync_val_compare_and_swap(&liblock_start_server_threads_by_hand, 1, 0) != 1)
		fatal("servers are not managed by hand");
	if(callback)
		callback();
}

static int do_liblock_cond_init(mwait)(liblock_cond_t* cond) { 
	return cond->has_attr ?
		pthread_cond_init(&cond->impl.posix_cond, &cond->attr) :
		pthread_cond_init(&cond->impl.posix_cond, 0);
}

static int cond_timedwait(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) {
	fatal("implement me");
	struct liblock_impl* impl = lock->impl;
	int res;

	pthread_mutex_lock(&impl->posix_lock);
	release(impl);

	if(ts)
		res = pthread_cond_timedwait(&cond->impl.posix_cond, &impl->posix_lock, ts);
	else
		res = pthread_cond_wait(&cond->impl.posix_cond, &impl->posix_lock);
	pthread_mutex_unlock(&impl->posix_lock);

	acquire(impl);

	return res;
}

static int do_liblock_cond_timedwait(mwait)(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) { 
	return cond_timedwait(cond, lock, ts);
}

static int do_liblock_cond_wait(mwait)(liblock_cond_t* cond, liblock_lock_t* lock) { 
	return cond_timedwait(cond, lock, 0);
}

static int do_liblock_cond_signal(mwait)(liblock_cond_t* cond) { 
	return pthread_cond_signal(&cond->impl.posix_cond);
}

static int do_liblock_cond_broadcast(mwait)(liblock_cond_t* cond) { 
	return pthread_cond_broadcast(&cond->impl.posix_cond);
}

static int do_liblock_cond_destroy(mwait)(liblock_cond_t* cond) { 
	return pthread_cond_destroy(&cond->impl.posix_cond);
}

static void do_liblock_on_thread_exit(mwait)(struct thread_descriptor* desc) {
}

static void do_liblock_on_thread_start(mwait)(struct thread_descriptor* desc) {
}

static void do_liblock_unlock_in_cs(mwait)(liblock_lock_t* lock) {
	struct liblock_impl* impl = lock->impl;
	release(impl);
}

static void do_liblock_relock_in_cs(mwait)(liblock_lock_t* lock) {
	struct liblock_impl* impl = lock->impl;
	acquire(impl);
}

static void do_liblock_declare_server(mwait)(struct core* core) {
}

liblock_declare(mwait);
