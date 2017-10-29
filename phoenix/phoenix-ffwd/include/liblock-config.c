#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <liblock.h>
#include <liblock-fatal.h>
#include <liblock-phoenix.h>

const  char*  liblock_lock_name;
struct core* server_core_1;
struct core* server_core_2;

static int volatile go = 0;

static void do_go() {
	go = 1;
}

extern struct core* get_server_core_1() {
	//static int zzz=0;
// 	struct core* res;
// 	if(__sync_fetch_and_add(&zzz, 1) % 2)
// 	 	res = topology->nodes[0].cores[1];
// 	else
// 	 	res = topology->nodes[0].cores[2];
// // //printf("---> core %d (%d)\n", res->core_id, servers[res->core_id].state);

// 	return res;
	return server_core_1;
}

extern struct core* get_server_core_2() {
	return server_core_2;
}

static int cur_core = 0;

void liblock_auto_bind() {
	int n, nn;
	struct core*      core;
	struct core_node* node;

	do {
		nn = n = __sync_fetch_and_add(&cur_core, 1) % topology->nb_cores;
		for(node=topology->nodes, core=0; !core; node++)
			if(n < node->nb_cores)
				core = node->cores[n];
			else
				n -= node->nb_cores;
	} while(core->server_type);

	self.running_core = core;

	cpu_set_t    cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core->core_id, &cpuset);
	if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset))
	 	fatal("pthread_setaffinity_np");

	//printf("thread %d binded to %d (with %d)\n", self.id, sched_getcpu(), nn);
}

/* ++ EDIT */
int liblock_find_free_cpu() {
  int n, nn;
  struct core*      core;
  struct core_node* node;

  do {
    nn = n = __sync_fetch_and_add(&cur_core, 1) % topology->nb_cores;
    for(node=topology->nodes, core=0; !core; node++)
      if(n < node->nb_cores)
	core = node->cores[n];
      else
	n -= node->nb_cores;
  } while(core->server_type);

  //self.running_core = core;
  // printf("Return %d top %d server %d\n", core->core_id, topology->nb_cores, core->server_type);
  return core->core_id;
}
/* -- EDIT */

void start_core(struct core* core, const char* name) {

	liblock_start_server_threads_by_hand = 1;
	liblock_reserve_core_for(core, name);
	liblock_lookup(name)->run(do_go); /* launch the liblock threads */

	//	if(servers[core->core_id].state != 3)
	while(!go)
		PAUSE();

	usleep(50000);
}

__attribute__ ((constructor (103))) static void liblock_phoenix() {
	char get_cmd[128];
	const char* sc = getenv("LIBLOCK_SERVER_CORE");
	int is_rcl;

	server_core_1 = topology->nodes[0].cores[1];
	//server_core_2 = topology->nodes[0].cores[1];
	
	liblock_lock_name = getenv("LIBLOCK_LOCK_NAME");

	if(!liblock_lock_name)
		liblock_lock_name = "posix";

	is_rcl = !strcmp(liblock_lock_name, "rcl") || !strcmp(liblock_lock_name, "multircl");

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

	if(is_rcl) {
		go = 0;
		start_core(server_core_1, liblock_lock_name);
		//start_core(server_core_2, liblock_lock_name);
	}

	liblock_auto_bind();
	
	fprintf(stdout, "**** testing %s with lock %s placed on core %d/%d\n", buf, liblock_lock_name, 
	       server_core_1->core_id,-1
	       /*, server_core_2->core_id*/);
}
