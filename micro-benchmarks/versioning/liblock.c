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
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <numaif.h>
#include "liblock.h"
#include "liblock-fatal.h"

#define MAX_NUMBER_OF_CORES   1024
#define MAX_NUMBER_OF_THREADS 256*1024

#define GET_NODES_CMD																										\
	"NODES=/sys/devices/system/node;"																			\
	"CPUS=/sys/devices/system/cpu;"																				\
	"if [ -d $NODES ]; then"																							\
  "  for F in `ls -d $NODES/node*`; do if [ $(cat $F/cpulist | grep '-') ]; then " \
	"for i in $(cat $F/cpulist | sed \"s/,/ /g\"); do seq -s ' ' $(echo $i | tr -s '-' ' '); done; else cat $F/cpulist | tr -s ',' ' '; fi; done;" \
	"else"																																\
	"  find $CPUS -maxdepth 1 -name \"cpu[0-9]*\" -exec basename {} \\; | sed -e 's/cpu\\(.*\\)/\\1/g' | tr -s '\n' ' ';" \
	"fi"

#define GET_FREQUENCIES_CMD																							\
	"cat /proc/cpuinfo | grep \"cpu MHz\" | sed -e 's/cpu MHz\\t\\t://'"

struct liblock_info {
	struct liblock_info* next;
	const char*          name;
	struct liblock_lib*  liblock;
};

struct start_routine {
	void*        (*start_routine)(void*);
	struct core* core;
	void*        arg;
	const char*  server_type;
};

static struct liblock_info*       liblocks = 0;

__thread struct thread_descriptor self = { 0, 0 };

struct id_manager                 id_manager;
struct topology                   real_topology;
struct topology*                  topology = &real_topology;
static cpu_set_t                  client_cpuset;
int                               liblock_start_server_threads_by_hand = 0;
int                               liblock_servers_always_up = 1;
unsigned int                      do_cycle_count = 1;

__attribute__ ((weak)) void liblock_auto_bind() {}
__attribute__ ((weak)) void liblock_on_server_thread_start(const char* lib, unsigned int thread_id) {}
__attribute__ ((weak)) void liblock_on_server_thread_end(const char* lib, unsigned int thread_id) {}

inline void* liblock_allocate(size_t n) {
	void* res=0;
	if((posix_memalign((void **)&res, CACHE_LINE_SIZE, cache_align(n)) < 0) || !res)
		fatal("posix_memalign(%llu, %llu)", (unsigned long long)n, (unsigned long long)cache_align(n));
	return res;
}

void* anon_mmap(size_t n) {
	void* res = mmap(0, n, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if(res == MAP_FAILED)
		fatal("mmap(%d): %s", (int)n, strerror(errno));
	return res;
}

void* anon_mmap_huge(size_t n) {
	//void* res = mmap(0, n, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
	void* res = mmap(0, n, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if(res == MAP_FAILED)
		fatal("mmap(huge)(%d): %s", (int)n, strerror(errno));
	return res;
}

void liblock_bind_mem(void* area, size_t n, struct core_node* node) {
	unsigned long mask = 1 << node->node_id;

	if(topology->nb_nodes > 1)
		if(mbind(area, n, MPOL_BIND, &mask, 1 + topology->nb_nodes, MPOL_MF_MOVE) < 0)
			fatal("mbind: %s", strerror(errno));
}

static void extract_topology(const char* cmd_nodes, const char* cmd_frequencies) {
	FILE *file;
	char  text[1024], *p, *saveptr;
	int   cores_per_nodes[1024];
	int   i;
	int   nb_nodes = 0;
	int   nb_cores = 0;
	const char* ld = getenv("LD_PRELOAD");

	unsetenv("LD_PRELOAD");

	/* We use UNIX commands to find the nodes. */
	if(!(file = popen(cmd_nodes, "r")))
		fatal("popen");

	/* For each node... */
	while(fgets(text, 1024, file)) {
		for(i=0, p = strtok_r(text, " ", &saveptr); p; p = strtok_r(0, " ", &saveptr))
			i++;
		cores_per_nodes[nb_nodes] = i;
		nb_nodes++;
		nb_cores += i;
	}

	if (pclose(file) < 0)
		fatal("pclose");

	topology->nodes = liblock_allocate(nb_nodes*sizeof(struct core_node));
	topology->cores = liblock_allocate(nb_cores*sizeof(struct core));
	topology->nb_nodes = nb_nodes;
	topology->nb_cores = nb_cores;

	for(i=0; i<nb_cores; i++) {
		CPU_SET(i, &client_cpuset);
		topology->cores[i].core_id = i;
		topology->cores[i].server_type = 0;
	}

	/* We use UNIX commands to find the nodes. */
	if(!(file = popen(cmd_nodes, "r")))
		fatal("popen");

	nb_nodes = 0;
	/* For each node... */
	while(fgets(text, 1024, file)) {
		struct core_node* node = &topology->nodes[nb_nodes];

		node->nb_cores          = cores_per_nodes[nb_nodes];
		node->cores             = liblock_allocate(node->nb_cores*sizeof(struct core_node*));
		node->node_id           = nb_nodes++;

		for(i=0, p = strtok_r(text, " ", &saveptr); p; p = strtok_r(0, " ", &saveptr), i++) {
			struct core* core = &topology->cores[atoi(p)];
			node->cores[i] = core;
			core->node = node;
		}
	}

	if (pclose(file) < 0)
		fatal("pclose");

	file = popen(cmd_frequencies, "r");

	if (file == NULL)
		fatal("popen");

	nb_cores = 0;
	while (fgets(text, 1024, file) != NULL)
		topology->cores[nb_cores++].frequency = atof(text);

	if (pclose(file) < 0)
		fatal("pclose");

	if(ld)
		setenv("LD_PRELOAD", ld, 1);

	//print_topology();
}

void print_topology() {
	int i, j;

	printf("-------------- Frequencies --------------\n");
	for(i=0; i<topology->nb_cores; i++) {
		printf("  * Core %d: %f MHz\n", topology->cores[i].core_id, topology->cores[i].frequency);
	}

	printf("------------------ Nodes ----------------\n");
	for(i=0; i<topology->nb_nodes; i++) {
		printf("  * Node %d:", topology->nodes[i].node_id);
		for(j=0; j<topology->nodes[i].nb_cores; j++)
			printf(" %d", topology->nodes[i].cores[j]->core_id);
		printf("\n");
	}
}

void  liblock_define_core(struct core* core) {
	self.running_core = core;
}

void liblock_reserve_core_for(struct core* core, const char* server_type) {
	if(__sync_val_compare_and_swap(&core->server_type, 0, server_type) != 0) {
		//printf("binding %p to core %d with server_type %s from %p\n", server_type, core->core_id, server_type, (void*)pthread_self());
		if(!server_type || strcmp(core->server_type, server_type))
			fatal("try to bind a '%s (%p)' on the '%s (%p)' %d core", server_type, server_type, core->server_type, core->server_type, core->core_id);

		//printf("Binding a thread to core %d\n", core->core_id);
	} else if(server_type) {
		cpu_set_t baseset;

		CPU_CLR(core->core_id, &client_cpuset);

		pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &baseset);
		sched_setaffinity(getpid(), sizeof(cpu_set_t), &client_cpuset);
		if(!self.running_core) {
			CPU_CLR(core->core_id, &baseset);
		}

		pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &baseset);

		liblock_lookup(server_type)->declare_server(core);

		//printf("Binding a server %s to core %d\n", server_type, core->core_id);
	}
}

void liblock_bind_thread_intern(pthread_t tid, struct core* core, const char* server_type) {
	cpu_set_t    _cpuset;
	cpu_set_t*   cpuset = &_cpuset;

	//printf("== bind core %d with thread %p (from %p)\n", core ? core->core_id : -1, (void*)tid, (void*)pthread_self());
	if(core) {
		/* We pin this thread to the right core. */
		liblock_reserve_core_for(core, server_type);

		CPU_ZERO(cpuset);
		CPU_SET(core->core_id, cpuset);
	} else if(server_type)
		fatal("should not happen");
	else
		cpuset = &client_cpuset;

	if(pthread_setaffinity_np(tid, sizeof(cpu_set_t), cpuset))
		fatal("pthread_setaffinity_np");
}

void liblock_bind_thread(pthread_t tid, struct core* core, const char* server_type) {
	liblock_bind_thread_intern(tid, core, server_type);
}

unsigned int liblock_find_id(struct id_manager* id_manager) {
	unsigned int cur;

 	if(id_manager->fragmented) {
 		for(cur=id_manager->first; cur<id_manager->first_free; cur++)
 			if(__sync_val_compare_and_swap(&id_manager->bitmap[cur], 1, 0) == 1)
 				return cur;
		id_manager->fragmented = 0;
	}

	while((cur = id_manager->first) > 0)
		if(__sync_val_compare_and_swap(&id_manager->first, cur, cur-1) == cur)
			return cur-1;

	while((cur = id_manager->first_free) < id_manager->last) {
		if(__sync_val_compare_and_swap(&id_manager->first_free, cur, cur+1) == cur)
			return cur;
	}

	fatal("exhausted client ids...");
	return 0;
}

void liblock_release_id(struct id_manager* id_manager, unsigned int id) {
	unsigned int cur;

	while(1) {
		if((cur = id_manager->first) == id) {
			if(__sync_val_compare_and_swap(&id_manager->first, cur, cur+1) == cur)
				return;
		} else if((cur = id_manager->first_free) == (id-1)) {
			if(__sync_val_compare_and_swap(&id_manager->first_free, cur, cur+1) == cur)
				return;
		} else {
			id_manager->bitmap[id] = 1;
			id_manager->fragmented = 1;
			return;
		}
	}
}

void liblock_init_id_manager(struct id_manager* id_manager) {
	id_manager->first      = 0;
	id_manager->first_free = 0;
	id_manager->fragmented = 0;
	id_manager->last       = MAX_NUMBER_OF_THREADS;
	id_manager->bitmap     = anon_mmap(MAX_NUMBER_OF_THREADS + sizeof(unsigned char));
}

void* liblock_exec(liblock_lock_t* lock, void* (*pending)(void*), void* val) {
	return lock->lib->_execute_operation(lock, pending, val);
}

static void cleanup_thread(void* arg) {
	struct liblock_info* cur;

	for(cur=liblocks; cur!=0; cur=cur->next)
		    cur->liblock->on_thread_exit(&self);

	liblock_release_id(&id_manager, self.id);
}

static void* my_start_routine(void* arg) {
	struct start_routine* r = arg;
	struct liblock_info*  cur;
	void* res;

	self.id = liblock_find_id(&id_manager);
	liblock_define_core(r->core);

	if(!r->core)
		liblock_auto_bind();

    if (r->server_type) {
        for(cur=liblocks; cur!=0; cur=cur->next) {
            if (!strcmp(cur->name, r->server_type)) {
                cur->liblock->on_thread_start(&self);
                break;
            }
        }
    }
    else {
        for(cur=liblocks; cur!=0; cur=cur->next) {
            cur->liblock->on_thread_start(&self);
        }
    }

	pthread_cleanup_push(cleanup_thread, 0);
	//printf("+++ starting routine: %d with core %d - %d\n", self.id, self.running_core ? self.running_core->core_id : -1, sched_getcpu());
	res = r->start_routine(r->arg);

	//printf("^^^^^ cleanup %d\n", self.id);
	pthread_cleanup_pop(1);
	free(r);

	//printf("finishing %d\n", self.id);

	return res;
}

int liblock_thread_create_and_bind(struct core* core, const char* server_type, pthread_t *thread,
																	 const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	cpu_set_t cpuset;
	struct start_routine* r = liblock_allocate(sizeof(struct start_routine));
	pthread_attr_t other_attr;
	int res;

	if(attr)
		other_attr = *attr;
	else
		pthread_attr_init(&other_attr);

	if(core) {
		//printf("****   create thread on core %d\n", core->core_id);
		CPU_ZERO(&cpuset);
		CPU_SET(core->core_id, &cpuset);
		pthread_attr_setaffinity_np(&other_attr, sizeof(cpu_set_t), &cpuset);
	} else
		pthread_attr_setaffinity_np(&other_attr, sizeof(cpu_set_t), &client_cpuset);

	r->start_routine = start_routine;
	r->core = core;
	r->arg  = arg;
	r->server_type = server_type;

	//printf("build thread on %d\n", core ? core->core_id : -1);
	res = pthread_create(thread, core ? &other_attr : attr, my_start_routine, r);

	//printf("build thread %p on %d done\n", (void*)*thread, core ? core->core_id : -1);

	if(res)
		fatal("pthread_create: %s", strerror(res));

	return 0;
}

int liblock_thread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	liblock_thread_create_and_bind(0, 0, thread, attr, start_routine, arg);
	return 0;
}

int liblock_getmutex_type(pthread_mutexattr_t* attr) {
	int res;
	// if(attr) {
		pthread_mutexattr_gettype(attr, &res);
	// 	switch(res) {
	// 		case PTHREAD_MUTEX_FAST_NP:       res = PTHREAD_MUTEX_NORMAL; break;
	// 		case PTHREAD_MUTEX_RECURSIVE_NP:  res = PTHREAD_MUTEX_RECURSIVE; break;
	// 		case PTHREAD_MUTEX_ERRORCHECK_NP: res = PTHREAD_MUTEX_ERRORCHECK; break;
	// 	}
	// } else
		res = PTHREAD_MUTEX_NORMAL;

	return res;
}

void liblock_printlibs() {
	struct liblock_info** cur;
	int k=0;

	for(cur=&liblocks; *cur!=0; cur=&(*cur)->next) {
		if(k++ > 0)
			printf(", ");
		printf("%s", (*cur)->name);
	}
	printf("\n");
}

static struct liblock_info** lookup_info(const char* name) {
	struct liblock_info** cur;

	for(cur=&liblocks; *cur!=0; cur=&(*cur)->next)
		if(!strcmp((*cur)->name, name))
			return cur;

	return 0;
}

struct liblock_lib* liblock_lookup(const char* name) {
	struct liblock_info** pnode = lookup_info(name);
	struct liblock_lib* res = 0;

	//printf("lookup for the '%s' locking library", name);
	if(pnode) {
		res = (*pnode)->liblock;
	} else
		fatal("unable to find locking library '%s'", name);
	return res;
}

int liblock_register(const char* name, struct liblock_lib* liblock) {
	struct liblock_info *node, *supposed;

	node = liblock_allocate(sizeof(struct liblock_info));
	node->name = name;
	node->liblock = liblock;

	do {
		supposed = liblocks;
		node->next = liblocks;
	} while(__sync_val_compare_and_swap(&liblocks, supposed, node) != supposed);

	liblock->init_library();

	return 0;
}

int liblock_lock_init(const char* type, struct core* core, liblock_lock_t* lock, void* arg) {
	struct liblock_lib* lib = liblock_lookup(type);

	if(!lib)
		fatal("unable to find lock: %s", type);

//    printf("DEBUG: liblock_lock_init: lock: %p\n", (void*)lock);
	lock->lib  = lib;
	lock->impl = lib->init_lock(lock, core, arg);

	return lock->impl ? 0 : -1;
}

int liblock_lock_destroy(liblock_lock_t* lock) {
	return lock->lib->_destroy_lock(lock);
}

int liblock_cond_init(liblock_cond_t* cond, const pthread_condattr_t* attr) {
	cond->lib = 0;
	if(attr) {
		cond->has_attr = 1;
		cond->attr = *attr;
	} else
		cond->has_attr = 0;
	return 0;
}

int liblock_cond_signal(liblock_cond_t* cond) {
	if(cond->lib)
		return cond->lib->_cond_signal(cond);
	else
		return 0;
}

int liblock_cond_broadcast(liblock_cond_t* cond) {
	if(cond->lib)
		return cond->lib->_cond_broadcast(cond);
	else
		return 0;
}

int liblock_cond_wait(liblock_cond_t* cond, liblock_lock_t* lock) {
	struct liblock_lib* lib = lock->lib;

	if(__sync_val_compare_and_swap(&cond->lib, 0, lib)) {
		if(cond->lib != lib)
			fatal("try to dynamically change the type of a cond");
	} else
		lib->_cond_init(cond);

	return lib->_cond_wait(cond, lock);
}

int liblock_cond_timedwait(liblock_cond_t* cond, liblock_lock_t* lock, struct timespec* ts) {
	struct liblock_lib* lib = lock->lib;

	if(__sync_val_compare_and_swap(&cond->lib, 0, lib)) {
		if(cond->lib != lib)
			fatal("try to dynamically change the type of a cond");
	} else
		lib->_cond_init(cond);

	return lib->_cond_timedwait(cond, lock, ts);
}

int liblock_cond_destroy(liblock_cond_t* cond) {
	if(cond->lib)
		return cond->lib->_cond_destroy(cond);
	else
		return 0;
}

__attribute__ ((constructor (101))) static void liblock_init_library() {
	CPU_ZERO(&client_cpuset);
	extract_topology(GET_NODES_CMD, GET_FREQUENCIES_CMD);
	liblock_init_id_manager(&id_manager);
	self.id = liblock_find_id(&id_manager);
}


__attribute__ ((constructor (103))) static void liblock_post_init_library() {
	struct liblock_info*  cur;

	for(cur=liblocks; cur!=0; cur=cur->next) {
		cur->liblock->on_thread_start(&self);
	}
}
