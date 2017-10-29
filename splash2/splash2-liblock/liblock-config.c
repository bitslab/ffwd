# define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <liblock.h>
#include <liblock-fatal.h>
#include <liblock-splash2.h>


struct servers {
	unsigned int nb_cores;
	struct core** cores;
};

const  char* liblock_lock_name;

static struct servers servers;

static int is_rcl = 0;
static int volatile go = 0;
static int volatile current_nb_threads = 0;
static int volatile wait_nb_threads = 0;

static void do_go() {
	go = 1;
}

extern struct core* get_server_core_1() {
	struct core* res = servers.cores[0];
	//printf("get_server_core_1: core %d\n", res->core_id);
	return res;
}

extern struct core* get_server_core_2() {
	struct core* res;
	if(servers.nb_cores == 1)
		res = servers.cores[0];
	else {
		static int n = 0;
		res = servers.cores[1 + __sync_fetch_and_add(&n, 1) % (servers.nb_cores - 1)];
	}
	//printf("get_server_core_2: core %d (%d)\n", res->core_id, servers.nb_cores);
	return res;
}

__attribute__ ((constructor (103))) static void liblock_splash() {
	char get_cmd[128];
	char* nthreads = getenv("NPROCS");
	char* var, *cur;
	unsigned int node, core;
	
	servers.nb_cores = 0;
	servers.cores = 0;

	var = getenv("LIBLOCK_CORES");
	var = var ? var : "0:1";

	for(cur=var-1; cur; cur=strchr(cur, ',')) {
		sscanf(++cur, "%d:%d,", &node, &core);
		if(node >= topology->nb_nodes || core >= topology->nodes[node].nb_cores)
			fatal("no such core: %d - %d", node, core);
		servers.cores = realloc(servers.cores, ++servers.nb_cores*sizeof(struct core*));
		servers.cores[servers.nb_cores-1] = topology->nodes[node].cores[core];
	}

	if(!servers.nb_cores)
		fatal("no server specified: use: LIBLOCK_CORES=node:core,node:core...");

	liblock_lock_name = getenv("LIBLOCK_LOCK_NAME");
	if(!liblock_lock_name)
		liblock_lock_name = "rcl";

	is_rcl = !strcmp(liblock_lock_name, "rcl");

	liblock_start_server_threads_by_hand = 1;
	liblock_servers_always_up = 1;

	sprintf(get_cmd, "/proc/%d/cmdline", getpid());
	FILE* f=fopen(get_cmd, "r");
	if(!f) {
		printf("!!! warning: unable to find command line\n");
	}
	char buf[1024];
	buf[0] = 0;
	if(!fgets(buf, 1024, f))
		printf("fgets\n");

	printf("**** testing %s with lock %s\n", buf, liblock_lock_name);

	liblock_auto_bind();

	if(is_rcl) {
		int i;

		go = 0;

		for(i=0; i<servers.nb_cores; i++) {
			printf("  server[%d]: core %-2d (on node %-2d)\n", i, servers.cores[i]->core_id, servers.cores[i]->node->node_id);
			liblock_reserve_core_for(servers.cores[i], liblock_lock_name);
		}

		liblock_lookup(liblock_lock_name)->run(do_go); /* launch the liblock threads */

		while(!go)
			PAUSE();
	}

#if 1
	if(nthreads) {
		wait_nb_threads = atoi(nthreads);
	}
#endif
}

void liblock_auto_bind() {
	if(!self.running_core) {
		static int cores_per_node;
		static int cur_core = 0;
		struct core*      core;
		uintptr_t cont;
		
		do {
			int k = __sync_fetch_and_add(&cur_core, 1) % topology->nb_cores;
			int c = k % topology->nodes[0].nb_cores;
			int n = (k / topology->nodes[0].nb_cores) % topology->nb_nodes;
			int i;

			core = topology->nodes[n].cores[c];
			cont = (uintptr_t)core->server_type;

			if(is_rcl) {
				for(i=0; i<servers.nb_cores; i++)
					cont |= core == servers.cores[i];
			}
		} while(cont);

		self.running_core = core;

		cpu_set_t    cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(core->core_id, &cpuset);
		if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset))
			fatal("pthread_setaffinity_np");

		//printf("autobind thread %d to core %d/%d (%d threads out of %d)\n", self.id, sched_getcpu(), self.running_core->core_id, current_nb_threads, wait_nb_threads);
	
		__sync_fetch_and_add(&current_nb_threads, 1);

		while(current_nb_threads < wait_nb_threads)
			PAUSE();
	}
}

int splash2_thread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	return liblock_thread_create(thread, attr, start_routine, arg);
}
