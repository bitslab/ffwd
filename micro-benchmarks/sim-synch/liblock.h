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
#ifndef _LOCKLIB_LOCK_H_
#define _LOCKLIB_LOCK_H_

#include <stdint.h>
#include <sys/types.h>

__BEGIN_DECLS

#define PAGE_SIZE       4096
#define CACHE_LINE_SIZE 64

#define r_align(n, r)        (((n) + (r) - 1) & -(r)) 
#define cache_align(n)       r_align(n , CACHE_LINE_SIZE)
#define pad_to_cache_line(n) (cache_align(n) - (n))

struct timespec;

/*
 *  topology description
 */
struct core {
	struct core_node* node;
	int               core_id;
	float             frequency;
	const char*       server_type; /* 0 => free, "client" => client, other => a server */
};

struct core_node {
	struct core** cores;
	int           node_id;
	int           nb_cores;
};

struct topology {
	struct core_node* nodes;
	struct core*      cores;   /* indexed by the virtual core id */
	int               nb_nodes;
	int               nb_cores;
};

/*
 *  thread description
 */
struct thread_descriptor {
	struct core* running_core;
	unsigned int id;
};

/*
 *  thread id management
 */
struct id_manager {
	unsigned int   volatile first;
	unsigned int   volatile first_free;
	unsigned int   volatile fragmented;
	unsigned int   volatile last;
	unsigned char* volatile bitmap; /* be careful, 0 means busy! */
};

/*
 *  exported variables
 */
struct id_manager                         id_manager;
extern struct topology*                   topology;
extern __thread struct thread_descriptor  self;
extern int                                liblock_start_server_threads_by_hand; /* default: 0 */
extern int                                liblock_servers_always_up;            /* default: 1 */

/*
 *  definition of a liblock
 */
struct liblock_impl;

typedef struct {
	struct liblock_lib*  lib;
	void*                r0;
	struct liblock_impl* impl;
} liblock_lock_t;

/*
 *  internal functions to build a liblock instance
 */
extern void* liblock_allocate(size_t n);
extern void* anon_mmap(size_t n);
extern void* anon_mmap_huge(size_t n);
extern void  liblock_bind_mem(void* area, size_t n, struct core_node* node);

typedef struct liblock_cond {
	struct liblock_lib*    lib;
	int                has_attr;
	pthread_condattr_t attr;
	union {
		pthread_cond_t       posix_cond;
		void*                data;
	} impl;
} liblock_cond_t;

struct liblock_lib {
	const char* lib_name;
	void      (*on_thread_start)(struct thread_descriptor*);                        /* private */
	void      (*on_thread_exit)(struct thread_descriptor*);                         /* private */
	void      (*init_library)();                                                    /* private */
	void      (*kill_library)();                                                    /* unimplemented */
	void      (*run)(void (*callback)());                                           /* private */
	void      (*declare_server)(struct core* core);                                 /* private */
	struct liblock_impl* (*init_lock)(liblock_lock_t* lock, struct core* core, pthread_mutexattr_t* arg);          /* public */
	void*     (*_execute_operation)(liblock_lock_t* lock, void* (*pending)(void*), void* val); /* public */
	int       (*_cond_init)(liblock_cond_t* cond);                                  /* public */
	int       (*_cond_wait)(liblock_cond_t* cond, liblock_lock_t* lock);            /* public */
	int       (*_cond_timedwait)(liblock_cond_t* cond, liblock_lock_t* lock, const struct timespec* ts); /* public */
	int       (*_cond_signal)(liblock_cond_t* cond);                                /* public */
	int       (*_cond_broadcast)(liblock_cond_t* cond);                             /* public */
	int       (*_cond_destroy)(liblock_cond_t* cond);                               /* public */
	void      (*_unlock_in_cs)(liblock_lock_t* locl);                               /* public */
	void      (*_relock_in_cs)(liblock_lock_t* lock);                               /* public */
	int       (*_destroy_lock)(liblock_lock_t* lock);                               /* public */
};

int                        liblock_getmutex_type(pthread_mutexattr_t* attr);
extern int                 liblock_register(const char* name, struct liblock_lib* liblock);
extern struct liblock_lib* liblock_lookup(const char* name);
extern void                liblock_on_server_thread_start(const char* lib, unsigned int thread_id);
extern void                liblock_on_server_thread_end(const char* lib, unsigned int thread_id);

#define do_liblock_on_thread_start(name)   liblock_ ## name ## _on_thread_start
#define do_liblock_on_thread_exit(name)    liblock_ ## name ## _on_thread_exit
#define do_liblock_init_library(name)      liblock_ ## name ## _init_library
#define do_liblock_kill_library(name)      liblock_ ## name ## _kill_library
#define do_liblock_init_lock(name)         liblock_ ## name ## _init_lock
#define do_liblock_destroy_lock(name)      liblock_ ## name ## _destroy_lock
#define do_liblock_execute_operation(name) liblock_ ## name ## _execute_operation
#define do_liblock_run(name)               liblock_ ## name ## _run
#define do_liblock_declare_server(name)    liblock_ ## name ## _declare_server
#define do_liblock_cond_init(name)         liblock_ ## name ## _cond_init
#define do_liblock_cond_wait(name)         liblock_ ## name ## _cond_wait
#define do_liblock_cond_timedwait(name)    liblock_ ## name ## _cond_timedwait
#define do_liblock_cond_signal(name)       liblock_ ## name ## _cond_signal
#define do_liblock_cond_broadcast(name)    liblock_ ## name ## _cond_broadcast
#define do_liblock_cond_destroy(name)      liblock_ ## name ## _cond_destroy
#define do_liblock_unlock_in_cs(name)      liblock_ ## name ## _unlock_in_cs
#define do_liblock_relock_in_cs(name)      liblock_ ## name ## _relock_in_cs

#define liblock_declare(name, ...)																			\
	__attribute__ ((constructor (102))) static void name ## _constructor_222() { \
		static struct liblock_lib lll = {																		\
			#name,																														\
			do_liblock_on_thread_start(name),																	\
			do_liblock_on_thread_exit(name),																	\
			do_liblock_init_library(name),																		\
		  do_liblock_kill_library(name),																		\
			do_liblock_run(name),																							\
			do_liblock_declare_server(name),																	\
			do_liblock_init_lock(name),																				\
			do_liblock_execute_operation(name),																\
			do_liblock_cond_init(name),																				\
			do_liblock_cond_wait(name),																				\
			do_liblock_cond_timedwait(name),																	\
			do_liblock_cond_signal(name),																			\
			do_liblock_cond_broadcast(name),																	\
			do_liblock_cond_destroy(name),																		\
			do_liblock_unlock_in_cs(name),																		\
			do_liblock_relock_in_cs(name),																		\
			do_liblock_destroy_lock(name),																		\
		};																																	\
		liblock_register(#name, &lll);																			\
	}
	
#define PAUSE()  asm volatile("pause"::)
#define MFENCE()  asm volatile("mfence"::)

/*
 *  external API - liblock specific functions
 */
extern void  liblock_printlibs();

/*
 *  external API - thread and core functions
 */
extern void  print_topology();
extern void  liblock_reserve_core_for(struct core* core, const char* server_type);
extern void  liblock_define_core(struct core* core);
extern void  liblock_bind_thread(pthread_t tid, struct core* core, const char* server_type);
extern int   liblock_thread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
extern int   liblock_thread_create_and_bind(struct core* core, const char* server_type, pthread_t *thread, 
																						const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

/*
 *  external API - lock functions
 */
extern void* liblock_exec(liblock_lock_t* lock, void* (*pending)(void*), void* val);

extern int liblock_lock_init(const char* type, struct core* core, liblock_lock_t* lock, void* arg);
extern int liblock_lock_destroy(liblock_lock_t* lock);
#define liblock_unlock_in_cs(lock)               (lock)->lib->_unlock_in_cs(lock)
#define liblock_relock_in_cs(lock)               (lock)->lib->_relock_in_cs(lock)

extern int liblock_cond_init(liblock_cond_t* cond, const pthread_condattr_t* attr);
extern int liblock_cond_signal(liblock_cond_t* cond);
extern int liblock_cond_broadcast(liblock_cond_t* cond);
extern int liblock_cond_wait(liblock_cond_t* cond, liblock_lock_t* lock);
extern int liblock_cond_timedwait(liblock_cond_t* cond, liblock_lock_t* lock, struct timespec* ts);
extern int liblock_cond_destroy(liblock_cond_t* cond);

extern void  liblock_auto_bind();         /* defines this function to automatically bind a thread */

__END_DECLS

#endif
