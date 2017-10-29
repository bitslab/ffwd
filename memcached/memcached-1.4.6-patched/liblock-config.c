# define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <liblock.h>
#include <liblock-fatal.h>
#include "liblock-memcached.h"

const  char* liblock_lock_name;
struct core* liblock_server_core_1;

static int volatile go = 0;
static int volatile current_nb_threads = 0;
static int volatile wait_nb_threads = 0;
static int* client_cores;

static void do_go(); 
static void liblock_splash();
 
static void do_go() {
	go = 1;
}

extern struct core* get_liblock_server_core_1() {
	return liblock_server_core_1;
}

__attribute__ ((constructor (103))) static void liblock_splash() {
	char get_cmd[128];
	int is_rcl;
//	char* nthreads = getenv("NPROCS");

	liblock_lock_name = getenv("LIBLOCK_LOCK_NAME");
	if(!liblock_lock_name)
		liblock_lock_name = "rcl";

	liblock_server_core_1 = topology->nodes[0].cores[0];

	is_rcl = !strcmp(liblock_lock_name, "rcl") || !strcmp(liblock_lock_name, "multircl");

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

	printf("**** testing %s with lock %s placed on core %d\n", buf, liblock_lock_name, 
				 liblock_server_core_1->core_id);

	if(is_rcl) {
		go = 0;

		liblock_reserve_core_for(liblock_server_core_1, liblock_lock_name);

		liblock_lookup(liblock_lock_name)->run(do_go); /* launch the liblock threads */

		while(!go)
			PAUSE();
	}

	client_cores = malloc(sizeof(int)*topology->nb_cores);

	int i, j, z;
	for(i=0, z=0; i<topology->nb_nodes; i++)
		for(j=0; j<topology->nodes[i].nb_cores; j++)
			if(topology->nodes[i].cores[j] != liblock_server_core_1)
				client_cores[z++] = topology->nodes[i].cores[j]->core_id;

	client_cores[z++] = liblock_server_core_1->core_id;

//	if(nthreads) {
		liblock_auto_bind();
//		wait_nb_threads = atoi(nthreads);
//	}
}

void liblock_auto_bind() {
	if(!self.running_core) {
		static int cur_core = 0;

		struct core*      core;
		
		do {
			int n = __sync_fetch_and_add(&cur_core, 1) % topology->nb_cores;
			core = &topology->cores[client_cores[n % topology->nb_cores]];
		} while(core->server_type);


		self.running_core = core;

		cpu_set_t    cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(core->core_id, &cpuset);
		if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset))
			fatal("pthread_setaffinity_np");

		__sync_fetch_and_add(&current_nb_threads, 1);

		printf("autobind thread %d to core %d/%d (%d threads out of %d)\n", self.id, sched_getcpu(), self.running_core->core_id, current_nb_threads, wait_nb_threads);
	
//		while(current_nb_threads < wait_nb_threads)
//			PAUSE();
	}
}

