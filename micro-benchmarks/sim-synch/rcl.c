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
#include <stdint.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <numa.h>
#include <assert.h>
#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include "liblock.h"
#include "liblock-fatal.h"
#include "fqueue.h"

#define nop() asm volatile ("nop")

//#define GDB_PROOF_IMPL 1

//#define LOCK_PROFILER_FRIENDLY
//#define BASIC_PROFILING_ON
//#define ADVANCED_PROFILING_ON 0x80000002

//#ifdef ADVANCED_PROFILING_ON

/*
#ifndef BASIC_PROFILING_ON
#define BASIC_PROFILING_ON
#endif

#include <papi.h>

#endif
*/

/*
 *      constants
 */
static const struct timespec manager_timeout = { 0, 50000000 };

#define PRIO_BACKUP    2
#define PRIO_SERVICING 3
#define PRIO_MANAGER   4

#define SERVER_DOWN     0
#define SERVER_STOPPING 1
#define SERVER_STARTING 2
#define SERVER_UP       3

#define STACK_SIZE            r_align(1024*1024, PAGE_SIZE)
#define MINI_STACK_SIZE       r_align(64*1024, PAGE_SIZE)

/*
 *  structures
 */
struct request {
	struct liblock_impl* volatile impl;            /* lock associated with the request */
	void* volatile                val;             /* argument of the pending request */
	void*              (*volatile pending)(void*); /* pending request or null if no pending request */
    void* volatile                servicing_owner;
	char                          pad[pad_to_cache_line(4*sizeof(void*))];
};

struct liblock_impl {
	struct request* volatile   cur_request;        /* current request, used to find it in wait, to update in server_loop to save 150 cycles */
	liblock_lock_t*            liblock_lock;
	int volatile               locked;	           /* state of the lock */

	char                       pad[pad_to_cache_line(sizeof(int) + 2*sizeof(void*))];
};

struct native_thread {
	int volatile                   timestamp;       /* has recently run */
	struct mini_thread* volatile   mini_thread;     /* currently associated mini thread */
	struct server*                 server;          /* server of the thread */
	pthread_t                      tid;             /* thread id */
	int                            is_servicing;    /* interrupted */
	ucontext_t                     initial_context; /* initial context of the thread */
	void*                          stack;           /* pointer to the stack */
	struct fqueue                  ll;              /* pointer to next node */
	struct native_thread*          all_next;        /* next thread */
};

struct mini_thread {
	ucontext_t                   context;       /* context of the mini thread */
	struct server*               server;        /* server of the mini thread, used for broadcast */
	int volatile                 is_timed;      /* true if timed */
	struct timespec              deadline;      /* deadline, only used when the mini-thread is in a timed wait */
	liblock_cond_t* volatile     wait_on;       /* queue of the mini_thread */
	int volatile                 wait_res;      /* result of the wait (timeout or not) */
	struct fqueue                ll_ready;
	struct fqueue                ll_timed;
	struct fqueue                ll_all;
	void*                        stack;
};

struct profiling {
	unsigned long long nb_false;
	unsigned long long nb_tot;
	unsigned long long nb_cs;
	unsigned long long nb_normal_path;
	unsigned long long nb_slow_path;
	unsigned long long nb_event;
};

struct server {
	/* always used by the server in non blocked case, also private in this case */
	int volatile                    state;                  /* state of the server (running, starting...) */
	int volatile                    alive;                  /* true if native thread are able to make progress */
	int volatile                    timestamp;              /* current timestamp */
	int volatile                    nb_ready_and_servicing; /* number of threads and of mini threads that are pendings */ 
	char                            pad0[pad_to_cache_line(4*sizeof(int))];

	/* always shared (in read) in non blocked case */
	struct core*                    core;                   /* core where the server run (read by client) */
	struct request*                 requests;               /* the request array (read by client) */
	char                            pad1[pad_to_cache_line(2*sizeof(void*))];

	/* used in blocked case, private */
	struct fqueue* volatile         mini_thread_all;        /* list of all active mini threads               */
	struct native_thread*           all_threads;            /* list all the threads */
	struct fqueue* volatile         prepared_threads;       /* list of prepared threads */
	int volatile                    nb_free_threads;        /* number of servicing threads that are not executing critical sections */
	int                             wakeup;                 /* futex value */
	struct timespec volatile        next_deadline;          /* next timed wait deadline */
	char                            pad2[pad_to_cache_line(3*sizeof(void*) + 2*sizeof(int) + sizeof(struct timespec))];

	/* not intensive shared accesses */
	void                            (*volatile callback)(); /* callback called when the server is ready to handle request */
	struct fqueue* volatile         mini_thread_timed;      /* sorted list of mini threads that have timeout */
	struct fqueue* volatile         mini_thread_ready;      /* list of active mini threads                   */
	struct fqueue* volatile         mini_thread_prepared;   /* list of sleeping mini threads                 */
	pthread_mutex_t                 lock_state;             /* lock for state transition */
	pthread_cond_t                  cond_state;             /* condition to wait on state transition */
	int volatile                    nb_attached_locks;      /* number of locks attached to this server */

	char                            pad3[pad_to_cache_line(4*sizeof(void*) + sizeof(int) + sizeof(pthread_mutex_t) + sizeof(pthread_cond_t))];
#ifdef BASIC_PROFILING_ON
	struct profiling                profiling;
	char                            pad4[pad_to_cache_line(sizeof(struct profiling))];
#endif
};

static struct server**                servers = 0; /* array of server (one per core) */
static struct liblock_impl            fake_impl;   /* fake lock always taken, used in wait to avoid a second call to the request */
static __thread struct native_thread* volatile me; /* (local) pointer to the the native thread */

#ifdef BASIC_PROFILING_ON
static unsigned int nb_client_threads = 0;
#endif

/*
 *    functions pre-declarations
 */
static void servicing_loop();

/* 
 * debug 
 */
void rclprintf(struct server* server, const char* msg, ...) {
	va_list va;
	char  buf[65536];
	char* tmp = buf;

	if(self.running_core)
		tmp += sprintf(tmp, "[%2d", self.running_core->core_id);
	else
		tmp += sprintf(tmp, "[NA");
	tmp += sprintf(tmp, "/%2d/%-2d - ", server->core->core_id, self.id);

	switch(server->state) {
		case SERVER_DOWN:     tmp += printf(tmp, "    down"); break;
		case SERVER_STOPPING: tmp += printf(tmp, "stopping"); break;
		case SERVER_STARTING: tmp += printf(tmp, "starting"); break;
		case SERVER_UP:       tmp += printf(tmp, "      up"); break;
	}
	tmp += sprintf(tmp, " - %d/%d] - ", server->nb_free_threads, server->nb_ready_and_servicing);
	va_start(va, msg);
	tmp += vsprintf(tmp, msg, va);
	tmp += sprintf(tmp, "\n");

	printf("%s", buf);
	if(server->nb_free_threads < 0)
		fatal("should not happen");
}

static long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3) {
	return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

static inline void wakeup_manager(struct server* server) {
	server->wakeup = 1;
	sys_futex(&server->wakeup, FUTEX_WAKE_PRIVATE, 1, 0, 0, 0);
}

/*
 *      time spec shortcuts
 */
#define ts_lt(ts1, ts2)																									\
	(((ts1)->tv_sec < (ts2)->tv_sec) ||																		\
	 (((ts1)->tv_sec == (ts2)->tv_sec) && ((ts1)->tv_nsec < (ts2)->tv_nsec)))

#define ts_le(ts1, ts2)																									\
	(((ts1)->tv_sec < (ts2)->tv_sec) ||																		\
	 (((ts1)->tv_sec == (ts2)->tv_sec) && ((ts1)->tv_nsec <= (ts2)->tv_nsec)))

#define ts_add(res, ts1, ts2)																						\
	({																																		\
		(res)->tv_sec = (ts1)->tv_sec + (ts2)->tv_sec;											\
		(res)->tv_nsec = (ts1)->tv_nsec + (ts2)->tv_nsec;										\
		if((res)->tv_nsec > 1e9) {																					\
			(res)->tv_nsec -= 1e9;																						\
			(res)->tv_sec++;																									\
		}																																		\
	})

#define ts_sub(res, ts1, ts2)																						\
	({																																		\
		(res)->tv_sec = (ts1)->tv_sec - (ts2)->tv_sec;											\
		if((ts1)->tv_nsec < (ts2)->tv_nsec) {																\
			(res)->tv_sec--;																									\
			(res)->tv_nsec = 1e9 + (ts1)->tv_nsec - (ts2)->tv_nsec;						\
		}	else																															\
			(res)->tv_nsec = (ts1)->tv_nsec - (ts2)->tv_nsec;									\
	})

#define ts_gettimeofday(ts, tz)																					\
	({ struct timeval tv; int r = gettimeofday(&tv, tz); (ts)->tv_sec = tv.tv_sec; (ts)->tv_nsec = tv.tv_usec*1e3; r; })

#define ts_print(ts) printf("%ld.%9.0ld", (ts)->tv_sec, (ts)->tv_nsec)

#if 0
#define lock_print(server, msg) rclprintf(server, msg)
#else
#define lock_print(server, msg)
#endif

#define lock_state(server)   ({									 \
			lock_print(server, "state locking");		 \
			pthread_mutex_lock(&(server)->lock_state); \
			lock_print(server, "state locked");			 \
		})
#define unlock_state(server) ({ lock_print(server, "state unlock"); pthread_mutex_unlock(&(server)->lock_state); })

/*
 * scheduling
 */
static __attribute__((always_inline)) inline void setprio(pthread_t thread_id, unsigned int prio) {
	//printf("%d goes to prio %d\n", self.id, prio);
	if(pthread_setschedprio(thread_id, prio))
		fatal("unable to set priority: %s for %d", strerror(errno), self.id);
}

/*
 * atomic operations on a local core
 */
/* CAS but without any global synchronization */
#define local_val_compare_and_swap(type, ptr, old_val, new_val) ({			\
			type tmp = old_val;																								\
			asm volatile( "\tcmpxchg %3,%1;" /* no lock! */										\
										: "=a" (tmp), "=m" (*(ptr))													\
										: "0" (tmp), "r" (new_val)													\
										: "cc"); /* remove "memory" to avoid reload of other adresses */ \
																																				\
			tmp;																															\
		})

/* atomic add but without any global synchronization */
static inline __attribute__((always_inline)) int local_fetch_and_add(int volatile* ptr, int val) {
	asm volatile( "\txadd %2, %1;" /* no lock! */
								: "=r" (val), "=m" (*(ptr))
								: "0" (val)
								: "cc"); /* just tremove "memory" to avoid reload of other adresses */
	return val;
}

/* atomic add but without any global synchronization */
static inline __attribute__((always_inline)) int local_add_and_fetch(int volatile* ptr, int val) {
	return local_fetch_and_add(ptr, val) + val;
}

/*
 * mini-threads
 */
/* commut from in to out */

static int mini_thread_lt(struct fqueue* ln, struct fqueue* rn) {
	struct mini_thread* l = ln->content, *r = rn->content;
	return ts_lt(&l->deadline, &r->deadline);
}

static inline __attribute__((always_inline)) void swap_mini_thread(struct mini_thread* in, struct mini_thread* out) {
	//rclprintf(in->server, "switching from %p to %p", in, out);
	me->mini_thread = out;
	swapcontext(&in->context, &out->context);
}

static struct mini_thread* allocate_mini_thread(struct server* server) {
	struct mini_thread* res;

	res = liblock_allocate(sizeof(struct mini_thread));

	res->stack = anon_mmap(STACK_SIZE);
	mprotect(res->stack, PAGE_SIZE, PROT_NONE);

	//rclprintf(server, "CREATE context %p with stack at %p and size %d", res, res->stack, STACK_SIZE);

	getcontext(&res->context);
	res->server = server;
	res->context.uc_link = 0;
	res->context.uc_stack.ss_sp = res->stack;
	res->context.uc_stack.ss_size = STACK_SIZE;
	res->ll_ready.content = res;
	res->ll_timed.content = res;
	res->ll_all.content   = res;

	makecontext(&res->context, (void(*)())servicing_loop, 0);

	fqueue_enqueue(&server->mini_thread_all, &res->ll_all);

	return res;
}

static struct mini_thread* get_ready_mini_thread(struct server* server) {
	struct fqueue* res;

	res = fqueue_dequeue(&server->mini_thread_ready);
	
	if(res) {
		__sync_fetch_and_sub(&server->nb_ready_and_servicing, 1);
		return res->content;
	} else
		return 0;
}

static struct mini_thread* get_or_allocate_mini_thread(struct server* server) {
	struct mini_thread* res;
	struct fqueue*      node;

	res = get_ready_mini_thread(server);

	//rclprintf(server, "*** get or allocate mini thread: ready is %p", res);
	if(res) 
		return res;

	node = fqueue_dequeue(&server->mini_thread_prepared);

	if(node)
		return node->content;
	else
		return allocate_mini_thread(server);
}

static void insert_in_ready_and_remove_from_timed(struct fqueue* node) {
	struct mini_thread* mini_thread = node->content;
	struct server* server = mini_thread->server;
	//rclprintf(server, "++++      reinjecting mini thread: %p", mini_thread);
	//ll_print("ready", &server->mini_thread_ready);
	fqueue_remove(&server->mini_thread_timed, &mini_thread->ll_timed, 0);
	fqueue_enqueue(&server->mini_thread_ready, &mini_thread->ll_ready);
	__sync_fetch_and_add(&server->nb_ready_and_servicing, 1);
	//rclprintf(server, "++++      reinjecting mini thread: %p done", mini_thread);
	//ll_print("ready", &server->mini_thread_ready);
}

/*
 *   liblock API
 */
/* execute operation (client side) */
static void* do_liblock_execute_operation(rcl)(liblock_lock_t* lock, void* (*pending)(void*), void* val) {
	struct liblock_impl* impl = lock->impl;
	struct server* server = lock->r0;

	//rclprintf(server, "*** sending operation %p::%p for client %d - %p", pending, val, self.id, (void*)pthread_self());
	if(me && self.running_core == server->core) {
		void* res;

		while(local_val_compare_and_swap(int, &impl->locked, 0, 1)) { /* one of my thread own the lock */
			me->timestamp = server->timestamp;
			pthread_yield();                          /* give a chance to one of our thread to release the lock */
		}

		res = pending(val);
		impl->locked = 0;                           /* I release the lock */

		return res;
	}

	struct request* req = &server->requests[self.id];

	req->impl = impl;
	req->val = val;
	req->pending = pending;

#if 0
	int i=0;

	for(i=0; i<500; i++) {
		PAUSE();
		
		if(!req->pending)
			return req->val;
	}

	while(req->pending) {
		pthread_yield();
	}
#else
	while(req->pending) {
		//pthread_yield();
        PAUSE();
	}
#endif

	return req->val;
}

__attribute__ ((noinline)) static int servicing_loop_slow_path(struct server* server, int time) {
	struct mini_thread *next = get_ready_mini_thread(server), *cur;

	if(next) {
		cur = me->mini_thread;
		/* more than one ready mini threads, activate the next one and put the running one in the prepared list */
		//rclprintf(server, "servicing-loop::elect mini-thread: %p (and %p goes to prepared)", next, cur);
		fqueue_enqueue(&server->mini_thread_prepared, &cur->ll_ready);
		//ll_print("prepared", &server->mini_thread_prepared);
		swap_mini_thread(cur, next);
		//rclprintf(server, "servicing-loop::mini-thread: %p is up", me->mini_thread);
		time = 0;
	} else if(server->nb_free_threads > 0) {
		/* more than one free thread, put this thread in the sleeping state (not servicing anymore) */
		
		if(local_val_compare_and_swap(int, &me->is_servicing, 1, 0)) {
			//rclprintf(server, "servicing::gona sleep %p %p %p", me, &me->ll, me->ll.content);
			
			__sync_fetch_and_sub(&server->nb_ready_and_servicing, 1);
			
			fqueue_enqueue(&server->prepared_threads, &me->ll);
			
			//rclprintf(server, "servicing-loop::unactivating");
			sys_futex(&me->is_servicing, FUTEX_WAIT_PRIVATE, 0, 0, 0, 0);
			//rclprintf(server, "servicing-loop::activating");
			
			if(!me->is_servicing)
				fatal("inconsistent futex");
			
			local_fetch_and_add(&server->nb_free_threads, -1);
		}
		
		time = 0;
	} else {
		/* ok, I have strictly more than one servicing thread and no free threads, this one is free, activate the next */
		//static int z=0; if(!(++z % 200000)) rclprintf(server, "servicing-loop::yield processor");
		local_fetch_and_add(&server->nb_free_threads, 1);
		if(time++ > 1000) {
			pthread_yield(); /* all the threads are busy */
			time = 0;
		}
		local_fetch_and_add(&server->nb_free_threads, -1);
	}
	
	return time;
}

#ifdef LOCK_PROFILER_FRIENDLY
void liblock_rcl_execute_op_for(liblock_lock_t* lock, size_t id) {}
#endif

static void servicing_loop() {
	struct server* server = me->server;
	void (*callback)() = server->callback;

	if(callback && __sync_val_compare_and_swap(&server->callback, callback, 0) == callback) {
		callback();
	}

#ifdef BASIC_PROFILING_ON
	struct liblock_impl* curr_impl;
	struct profiling     profiling = { 0, 0, 0, 0, 0 };
	int has_false;
	int has_with;
#endif

	int time = 0;

	//rclprintf(server, "::: start servicing loop %p", me->mini_thread);

    do { 
        me->timestamp = server->timestamp;
        server->alive = 1; 

        struct request* request, *last;
        void* (*pending)(void*);

#ifdef BASIC_PROFILING_ON
        curr_impl = 0; 
        has_false = 0; 
        has_with = 0; 
        profiling.nb_normal_path++;
#endif
     
        last = &server->requests[id_manager.first_free];
     
        for(request=&server->requests[id_manager.first];
            request<last;
            request++)
        {    
            __builtin_prefetch (request + 1, 1, 0);

#ifdef GDB_PROOF_IMPL
            if(request->pending &&
               !local_val_compare_and_swap(unsigned int, 
                                           &request->servicing_owner,
                                           0,   
                                           self.id))
            {    

                pending = request->pending;

                struct liblock_impl* impl = request->impl;

#ifdef LOCK_PROFILER_FRIENDLY
                liblock_rcl_execute_op_for(impl->liblock_lock,
                    ((uintptr_t)request - (uintptr_t)server->requests) / 
                    sizeof(struct request));
#endif

#ifdef BASIC_PROFILING_ON
                profiling.nb_cs++;

                if(!has_with)
                {    
                    has_with = 1; 
                    profiling.nb_tot++;
                }    

                if(!curr_impl)
                    curr_impl = impl;
                else if(!has_false && curr_impl != impl)
                {
                    profiling.nb_false++;
                    has_false = 1;
                }
#endif

                if(pending &&
                   !local_val_compare_and_swap(int, &impl->locked, 0, 1))
                {
                    impl->cur_request = request;

                    request->val = pending(request->val);

                    impl->locked = 0;
                    request->pending = 0;
                }
                request->servicing_owner = 0;
            }

#else // !GDB_FRIENDLY_IMPL

            pending = request->pending;

            if(pending) {
                struct liblock_impl* impl = request->impl;

#ifdef LOCK_PROFILER_FRIENDLY
                liblock_rcl_execute_op_for(impl->liblock_lock,
                  ((uintptr_t)request - (uintptr_t)server->requests) /
                  sizeof(struct request));
#endif

#ifdef BASIC_PROFILING_ON
                profiling.nb_cs++;

                if(!has_with)
                {
                    has_with = 1;
                    profiling.nb_tot++;
                }

                if(!curr_impl)
                    curr_impl = impl;
                else if(!has_false && curr_impl != impl)
                {
                    profiling.nb_false++;
                    has_false = 1;
                }
#endif

               if(!local_val_compare_and_swap(int, &impl->locked, 0, 1))
                {
                    impl->cur_request = request;

                    /*
                    rclprintf(server, "executing request %p::%p::%p %p<%p<%p",
                              pending, request->val, request,
                              &server->requests[id_manager.first], request, last);
                    */

                    request->val = pending(request->val);

                    /*                  
                    rclprintf(server, "executing request %p::%p done",
                                      pending, request->val);
                    */

                    request->pending = 0;

                    impl->locked = 0;
                }
            }

#endif
        }

		//{ static int n=0; if(!(++n % 500000)) rclprintf(server, "servicing loop is running"); }		

		if(server->nb_ready_and_servicing > 1) {
			time = servicing_loop_slow_path(server, time);
#ifdef BASIC_PROFILING_ON
			profiling.nb_slow_path++;
#endif
		}

	} while(server->state >= SERVER_STARTING);

#ifdef BASIC_PROFILING_ON
	__sync_fetch_and_add(&server->profiling.nb_false, profiling.nb_false);
	__sync_fetch_and_add(&server->profiling.nb_tot, profiling.nb_tot);
	__sync_fetch_and_add(&server->profiling.nb_cs, profiling.nb_cs);
	__sync_fetch_and_add(&server->profiling.nb_normal_path, profiling.nb_normal_path);
	__sync_fetch_and_add(&server->profiling.nb_slow_path, profiling.nb_slow_path);
#endif

	setcontext(&me->initial_context);
}

/*
static unsigned long get_thread_id() {
	return (unsigned long)me->tid;
}
*/

static void* servicing_thread(void* arg) {
	struct native_thread* native_thread = arg;
	struct server*        server = native_thread->server;

#ifdef BASIC_PROFILING_ON
	__sync_fetch_and_sub(&nb_client_threads, 1);
#endif
	//rclprintf(server, "start: servicing thread %d", self.id);

	me = native_thread;
	me->mini_thread = get_or_allocate_mini_thread(server);

	local_fetch_and_add(&server->nb_free_threads, -1);

	if(server->state == SERVER_UP) {
#ifdef ADVANCED_PROFILING_ON
		PAPI_thread_init((unsigned long (*)(void))get_thread_id);

	int event_set = PAPI_NULL;
	long long values[1];
	long long g_monitored_event_id = ADVANCED_PROFILING_ON;

	if (g_monitored_event_id != 0) {
		if(PAPI_create_eventset(&event_set) != PAPI_OK)
			warning("PAPI_create_eventset");
		else if(PAPI_add_event(event_set, g_monitored_event_id) != PAPI_OK)
			warning("PAPI_add_events");
		
		/* This seemingly helps increasing PAPI's accuracy. */
		else {
			if(PAPI_start(event_set) != PAPI_OK)
				warning("PAPI_start");
			if (PAPI_stop(event_set, values) != PAPI_OK)
				warning("servicing-loop::PAPI_stop");
			if (PAPI_start(event_set) != PAPI_OK)
				warning("PAPI_start");
		}
	}

#endif
	liblock_on_server_thread_start("rcl", self.id);
	getcontext(&me->initial_context);
	if(server->state == SERVER_UP)
		setcontext(&me->mini_thread->context);


#ifdef ADVANCED_PROFILING_ON
	if (g_monitored_event_id != 0) {
		if(PAPI_stop(event_set, values) != PAPI_OK)
			warning("servicing-loop::PAPI_stop");
		else
			__sync_fetch_and_add(&server->profiling.nb_event, values[0]);
	}
#endif

		liblock_on_server_thread_end("rcl", self.id);
	}

	__sync_fetch_and_sub(&server->nb_ready_and_servicing, 1);

	//rclprintf(server, "::: quitting serviving-loop %p", pthread_self());

	return 0;
}


static void ensure_at_least_one_free_thread(struct server* server) {
	//rclprintf(server, "ensure at least");
	if(server->nb_free_threads < 1) {
		/* ouch, no more free thread, creates or activates a new one */

		if(server->state >= SERVER_STARTING) {
			//rclprintf(server, "no more free threads %d", server->nb_free_threads);
			struct fqueue*        node = fqueue_dequeue(&server->prepared_threads);
			struct native_thread* elected;

			if(node) {
				elected = node->content;
				//rclprintf(server, "REACTIVATING servicing thread %p", elected);
			} else {
				elected = liblock_allocate(sizeof(struct native_thread));
				elected->ll.content = elected;
				
				elected->stack = anon_mmap(MINI_STACK_SIZE);
				mprotect(elected->stack, PAGE_SIZE, PROT_NONE);
				
				elected->server = server;

				elected->all_next = server->all_threads;
				server->all_threads = elected;
				
				//rclprintf(server, "CREATE a new servicing thread %p with stack at %p (%p %p)", elected, elected->stack, &elected->ll, elected->ll.content);
			}

			elected->timestamp = server->timestamp - 1;
			local_fetch_and_add(&server->nb_free_threads, 1);
			__sync_fetch_and_add(&server->nb_ready_and_servicing, 1);
			elected->is_servicing = 1;

			if(node)
				sys_futex(&elected->is_servicing, FUTEX_WAKE_PRIVATE, 1, 0, 0, 0);
			else {
				struct sched_param    param;
				pthread_attr_t        attr;
				
				param.sched_priority = PRIO_SERVICING;
				pthread_attr_init(&attr);
				
				pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
				pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
				pthread_attr_setschedparam(&attr, &param);
				
				//rclprintf(server, "launching the new servicing thread %p", elected);
				liblock_thread_create_and_bind(server->core, "rcl", &elected->tid, &attr, servicing_thread, elected);
				//rclprintf(server, "launching of the servicing thread %p done", elected);
			}
		}
	}
	//rclprintf(server, "ensure done");
}

static void* backup_thread(void* arg) {
	struct server* server = arg;

	//rclprintf(server, "start: backup");
	while(server->state >= SERVER_STARTING) {
		//rclprintf(server, "+++ backup thread is running");
		server->alive = 0;
		wakeup_manager(server);
	}
	//rclprintf(server, "+++ quitting backup thread %p", pthread_self());

	return 0;
}

static void* manager_thread(void* arg) {
	pthread_t backup_tid;
	struct server* server = arg;
	struct native_thread* native_thread, *next;
	struct sched_param param;
	pthread_attr_t attr;
	struct timespec now, deadline;
	int done;

#ifdef BASIC_PROFILING_ON
	server->profiling.nb_false = 0;
	server->profiling.nb_tot = 0;
	server->profiling.nb_cs = 0;
	server->profiling.nb_normal_path = 0;
	server->profiling.nb_slow_path = 0;
	server->profiling.nb_event = 0;
	int nb_wakeup = 0;
	int nb_not_alive = 0;
#endif
	//rclprintf(server, "start: manager");

	// 

	server->alive  = 1;

	lock_state(server);

	/* make sure that we own the lock when going to FIFO scheduling */
	param.sched_priority = PRIO_MANAGER;

	if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) == -1)
		fatal("pthread_setschedparam: %s", strerror(errno));

	param.sched_priority = PRIO_BACKUP;
	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	pthread_attr_setschedparam(&attr, &param);

	liblock_thread_create_and_bind(server->core, "rcl", &backup_tid, &attr, backup_thread, server);

	ensure_at_least_one_free_thread(server);

	server->state = SERVER_UP;

	pthread_cond_broadcast(&server->cond_state);

	unlock_state(server);
	
	while(server->state == SERVER_UP) {
		//rclprintf(server, "manager is working (%d)", server->alive);
		server->wakeup = 0;

#ifdef BASIC_PROFILING_ON
		nb_wakeup++;
#endif

		if(!server->alive) {
			//rclprintf(server, "no more alive servicing threads");

#ifdef BASIC_PROFILING_ON
			nb_not_alive++;
#endif
			ensure_at_least_one_free_thread(server);

			server->alive = 1;
			done = 0;

			while(!done) {
				struct native_thread* cur = server->all_threads;
				while(cur) {
					if(cur->is_servicing) {
						if(done || (cur->timestamp == cur->server->timestamp)) {
							setprio(cur->tid, PRIO_BACKUP);
							setprio(cur->tid, PRIO_SERVICING);
						} else {
							cur->timestamp = server->timestamp; /* set it here because could be in I/O */
							done = 1;
						}
					}
					cur = cur->all_next;
				}

				if(!done)
					server->timestamp++;
			}
		} else
			server->alive = 0;

		ts_gettimeofday(&now, 0);
		ts_add(&deadline, &now, &manager_timeout);

		done = 0;

		//printf("+++++++++++++++++++++++++++++++++++++\n");
		//printf("++++      manager: current time: "); ts_print(&now); printf("\n");
		//printf("++++      manager: initial deadline: "); ts_print(&deadline); printf("\n");
		while(!done) {
			struct fqueue* node = server->mini_thread_timed;
 			if(node) {
				struct mini_thread* cur = node->content;
				//printf("++++      manager: find waiter: %p\n", cur);
				if(ts_le(&cur->deadline, &now)) {
					//printf("++++      manager: reinject expired deadline\n");
					fqueue_remove((struct fqueue**)&cur->wait_on->impl.data, &cur->ll_ready, insert_in_ready_and_remove_from_timed);
				} else {
					struct timespec ddd = cur->deadline;
					if(ts_lt(&ddd, &deadline)) {
						//printf("++++      manager: change deadline to: "); ts_print(&deadline); printf("\n");
						deadline = ddd;
					}
					done = 1;
				}
			} else
				done = 1;
		}
		//printf("++++      manager: next deadline: "); ts_print(&deadline); printf("\n");

		server->next_deadline = deadline;

		ts_sub(&deadline, &deadline, &now);
		//printf("++++      manager: next deadline: "); ts_print(&deadline); printf("\n");

		//rclprintf(server, "manager::sleeping");
		sys_futex(&server->wakeup, FUTEX_WAIT_PRIVATE, 0, &deadline, 0, 0);
		//rclprintf(server, "manager::sleeping done");
	}

	//rclprintf(server, "unblocking all the servicing threads");

	for(native_thread=server->all_threads; native_thread; native_thread=native_thread->all_next) {
		int state = __sync_lock_test_and_set(&native_thread->is_servicing, 2);
		
		if(!state) {
			local_fetch_and_add(&server->nb_free_threads, 1);
			__sync_fetch_and_add(&server->nb_ready_and_servicing, 1);
			sys_futex(&native_thread->is_servicing, FUTEX_WAKE_PRIVATE, 1, 0, 0, 0);
		}
	}
	
	//rclprintf(server, "waiting backup");

	if(pthread_join(backup_tid, 0) != 0)
		fatal("pthread_join");

	//rclprintf(server, "waiting servicing threads");
	
	for(native_thread=server->all_threads; native_thread; native_thread=next) {
		next = native_thread->all_next;
		pthread_join(native_thread->tid, 0);
		munmap(native_thread->stack, MINI_STACK_SIZE);
		free(native_thread);
	}

	//rclprintf(server, "freeing mini threads");
	
	{
		struct fqueue* ll_cur;
		while((ll_cur = fqueue_dequeue(&server->mini_thread_all))) {
			struct mini_thread* cur = ll_cur->content;
			munmap(cur->stack, STACK_SIZE);
			free(cur);
		}
	}

	//rclprintf(server, "quitting");

	server->mini_thread_timed = 0;
	server->mini_thread_ready = 0;
	server->mini_thread_prepared = 0;

	server->prepared_threads = 0;

#ifdef BASIC_PROFILING_ON
	fprintf(stdout, "--- manager of core %d\n", server->core->core_id);
	fprintf(stdout, "    nb wakeup: %d\n", nb_wakeup);
	fprintf(stdout, "    nb not alive: %d\n", nb_not_alive);
	fprintf(stdout, "    false: %llu, total: %llu, cs: %llu, nb-normal %llu, nb-slow %llu\n", server->profiling.nb_false, server->profiling.nb_tot, server->profiling.nb_cs, server->profiling.nb_normal_path, server->profiling.nb_slow_path);
	fprintf(stdout, "    false rate: %lf\n", (double)server->profiling.nb_false / (double)server->profiling.nb_tot);
	fprintf(stdout, "    use rate: %lf\n", (double)server->profiling.nb_cs/(double)(server->profiling.nb_tot*nb_client_threads));
	fprintf(stdout, "    slow path rate: %lf\n", (double)server->profiling.nb_slow_path/(double)server->profiling.nb_normal_path);
#ifdef ADVANCED_PROFILING_ON
	long long g_monitored_event_id = ADVANCED_PROFILING_ON;
	if(g_monitored_event_id) {
		char buf[65536];
		strcpy(buf, "<none>");
		PAPI_event_code_to_name(g_monitored_event_id, buf);
		fprintf(stdout, "    '%s'/cs: %lf\n", buf, (double)server->profiling.nb_event / (double)server->profiling.nb_cs);
	}
#endif
#endif

	lock_state(server);

	server->state = SERVER_DOWN;
	pthread_cond_broadcast(&server->cond_state);

	unlock_state(server);

	return 0;
}

static void launch_server(struct server* server, void (*callback)()) {
	pthread_t tid;

	//rclprintf(server, "launch server 1: %d", server->state);
	while(server->state != SERVER_DOWN && server->state != SERVER_UP) {
		//rclprintf(server, "launch_server::waiting the server");
		pthread_cond_wait(&server->cond_state, &server->lock_state);
		//rclprintf(server, "launch_server::waiting the server done");
	}

	//rclprintf(server, "launch server 2: %d", server->state);

	if(server->state == SERVER_UP)
		return;

	server->callback = callback;

	//rclprintf(server, "launching server");

	server->state = SERVER_STARTING;

	liblock_thread_create_and_bind(server->core, "rcl", &tid, 0, manager_thread, server);

	while(server->state == SERVER_STARTING) {
		//rclprintf(server, "launch_server::waiting the server");
		pthread_cond_wait(&server->cond_state, &server->lock_state);
		//rclprintf(server, "launch_server::waiting the server done");
	}
}

static void destroy_server(struct server* server) {
	lock_state(server);

	if(server->state == SERVER_UP) {
		server->state = SERVER_STOPPING;
		wakeup_manager(server);

		while(server->state == SERVER_STOPPING) {
			//rclprintf(lock->server, "launch_server::waiting the end of the server");
			pthread_cond_wait(&server->cond_state, &server->lock_state);
			//rclprintf(impl->server, "launch_server::waiting the end of server done");
		}
	}

	unlock_state(server);
}

static int cond_signal(liblock_cond_t* cond) {
	struct fqueue*         node;
	struct mini_thread*    mini_thread;
	struct server*         server;

	node = fqueue_dequeue((struct fqueue**)&cond->impl.data);

	if(node) {
		mini_thread = node->content;
		server = mini_thread->server;

		//rclprintf(server, "broadcast::dequeuing: %p", mini_thread);

		if(mini_thread->is_timed)
			fqueue_remove((struct fqueue**)&mini_thread->wait_on->impl.data, &mini_thread->ll_timed, 0);

		fqueue_enqueue(&server->mini_thread_ready, node);
		__sync_fetch_and_add(&server->nb_ready_and_servicing, 1);

		return 1;
	} else
		return 0;
}

static int do_liblock_cond_signal(rcl)(liblock_cond_t* cond) { 
	cond_signal(cond);
	return 0;
}

static int do_liblock_cond_broadcast(rcl)(liblock_cond_t* cond) { 
	while(cond_signal(cond));
	return 0;
}

static int do_liblock_cond_timedwait(rcl)(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts) { 
	struct liblock_impl* impl = lock->impl;
	struct mini_thread*  cur = me->mini_thread;
	struct server* server = me->server;
	//rclprintf(server, "timed wait");
	struct mini_thread*  next = get_or_allocate_mini_thread(server);
	struct request*      request = impl->cur_request;

	/* prepare meta informations */
	cur->is_timed = ts ? 1 : 0;
	cur->wait_on = cond;
	cur->wait_res = 0;

	/* first, don't re-execute the request */ 
	request->impl = &fake_impl;

	/* then, enqueue my request in cond  */
	//rclprintf(cur->server, "cond:enqueuing: %p", cur);
	fqueue_enqueue((struct fqueue**)&cond->impl.data, &cur->ll_ready);
	//ll_print("cond->impl.data", (struct mini_thread_ll**)&cond->impl.data);

	/* release the lock */
	impl->locked = 0;

	if(ts) {
		//rclprintf(cur->server, "cond:timed: %p", cur);
		cur->deadline = *ts;

		fqueue_ordered_insert(&server->mini_thread_timed, &cur->ll_timed, mini_thread_lt);

		if(ts_lt(ts, &server->next_deadline))
			wakeup_manager(server);
	}

	//rclprintf(cur->server, "swapping: me is %p", me);

	/* and finally, jump to the next mini thread */
	swap_mini_thread(cur, next);
	//rclprintf(server, "%p mini-thread is running (%d)", cur, impl->locked);

	while(local_val_compare_and_swap(int, &impl->locked, 0, 1)) { /* one of my thread own the lock */
		me->timestamp = me->server->timestamp;
		pthread_yield();                          /* give a chance to one of our thread to release the lock */
	}

	//rclprintf(cur->server, "relected: me continue %p", me);

	request->impl = impl;

	return cur->wait_res;
}

static int do_liblock_cond_wait(rcl)(liblock_cond_t* cond, liblock_lock_t* lock) { 
	return do_liblock_cond_timedwait(rcl)(cond, lock, 0);
}

static int do_liblock_cond_init(rcl)(liblock_cond_t* cond) { 
	cond->impl.data = 0;
	return 0;
}

static int do_liblock_cond_destroy(rcl)(liblock_cond_t* cond) { 
	return 0;
}

static void do_liblock_unlock_in_cs(rcl)(liblock_lock_t* lock) {
	fatal("implement me"); 
}

static void do_liblock_relock_in_cs(rcl)(liblock_lock_t* lock) {
	fatal("implement me"); 
}

static struct liblock_impl* do_liblock_init_lock(rcl)(liblock_lock_t* lock, struct core* core, pthread_mutexattr_t* attr) {
	struct server* server = servers[core->core_id];
	struct liblock_impl* impl = liblock_allocate(sizeof(struct liblock_impl));

	lock->r0 = server;

	impl->locked = 0;
	impl->liblock_lock = lock;

	__sync_fetch_and_add(&server->nb_attached_locks, 1);

	liblock_reserve_core_for(core, "rcl");

	return impl;
}

static int do_liblock_destroy_lock(rcl)(liblock_lock_t* lock) {
	struct server* server = lock->r0;
	//rclprintf(impl->server, "destroying lock %p", lock);

	int n = __sync_sub_and_fetch(&server->nb_attached_locks, 1);

	if(!liblock_servers_always_up && !n)
		destroy_server(server);

	return 0;
}

static void do_liblock_run(rcl)(void (*callback)()) {
	int i, n=0;

	if(__sync_val_compare_and_swap(&liblock_start_server_threads_by_hand, 1, 0) != 1)
		fatal("servers are not managed by hand");

	for(i=0; i<topology->nb_cores; i++) {
		if(topology->cores[i].server_type && !strcmp(topology->cores[i].server_type, "rcl"))
			n++;
	}

	if(!n)
		callback();
	
	for(i=0; i<topology->nb_cores; i++) {
		lock_state(servers[i]);
		if(topology->cores[i].server_type && !strcmp(topology->cores[i].server_type, "rcl")) {
			//rclprintf(servers[i], "++++ launching: %d from %d", i, sched_getcpu());
			launch_server(servers[i], --n ? 0 : callback);
			//rclprintf(servers[i], "++++ launched: %d from %d", i, sched_getcpu());
		}
		unlock_state(servers[i]);
	}

	//printf("-----------     finishing run    ------------------\n");
}

static void do_liblock_declare_server(rcl)(struct core* core) {
	if(!liblock_start_server_threads_by_hand) {
		lock_state(servers[core->core_id]);
		launch_server(servers[core->core_id], 0);
		unlock_state(servers[core->core_id]);
	}
}

static void do_liblock_on_thread_exit(rcl)(struct thread_descriptor* desc) {
}

static void do_liblock_on_thread_start(rcl)(struct thread_descriptor* desc) {
	int i;

	//printf("rcl::thread on core: %d\n", self.id);

	for(i=0; i<topology->nb_cores; i++) {
		servers[i]->requests[self.id].pending = 0;
	}

#ifdef BASIC_PROFILING_ON
	__sync_fetch_and_add(&nb_client_threads, 1);
#endif
}

static void force_shutdown() {
	int i;
	
	for(i=0; i<topology->nb_cores; i++)
		destroy_server(servers[i]);
}

static void do_liblock_init_library(rcl)() {
	int i;

	servers = liblock_allocate(sizeof(struct server*) * topology->nb_cores);

	fake_impl.locked = 1;

	for(i=0; i<topology->nb_cores; i++) {
		struct core* core = &topology->cores[i];
		int          cid = core->core_id;
		size_t       request_size = r_align(sizeof(struct request)*id_manager.last, PAGE_SIZE);
		size_t       server_size  = r_align(sizeof(struct server), PAGE_SIZE);
		void*        ptr = anon_mmap_huge(request_size + server_size);

        ((uint64_t *)ptr)[0] = 0; // To avoid a page fault later.
		//liblock_bind_mem(ptr, request_size + server_size, core->node);

		servers[cid] = ptr + request_size;
		servers[cid]->core = core;

		servers[cid]->state = SERVER_DOWN;
		servers[cid]->nb_attached_locks = 0;
		servers[cid]->nb_free_threads = 0;
		servers[cid]->nb_ready_and_servicing = 0;
		servers[cid]->requests = ptr;

		servers[cid]->mini_thread_all = 0;
		servers[cid]->mini_thread_timed = 0;
		servers[cid]->mini_thread_ready = 0;
		servers[cid]->mini_thread_prepared = 0;

		servers[cid]->all_threads = 0;
		servers[cid]->prepared_threads = 0;

		servers[cid]->timestamp = 1;

		pthread_mutex_init(&servers[cid]->lock_state, 0);
		pthread_cond_init(&servers[cid]->cond_state, 0);
	}

	atexit(force_shutdown);

#ifdef ADVANCED_PROFILING_ON
	if (PAPI_is_initialized() == PAPI_NOT_INITED &&
        PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
		fatal("PAPI_library_init");

	PAPI_thread_init((unsigned long (*)(void))pthread_self);

#endif
}

static void do_liblock_kill_library(rcl)() {
	fatal("implement me");
}

liblock_declare(rcl);
