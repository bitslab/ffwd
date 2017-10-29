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
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "liblock.h"

#define DEFAULT    "rcl"

#define _NB_THREADS (topology->nb_cores - 1)

static unsigned int nb_threads;

void sigterm_handler(int n) {
	fprintf(stderr, "sigterm from: %d (%p)\n", self.id, (void*)pthread_self());
}

void sigsegv_handler(int n) {
	fprintf(stderr, "sigsegv from: %d (%p)\n", self.id, (void*)pthread_self());
	exit(0);
}

#define NBREQS    200000
#define NBLOOPS   10

const char*    liblock_name;
liblock_lock_t lock;
liblock_lock_t other_lock;

int volatile shared=0;

void* critical(void* n) {
	int j;
	for(j=0; j<NBLOOPS; j++) {
		shared += 42;
	}
	return 0;
}

int volatile started=0;

void *thread_routine(void *arg) {

	__sync_sub_and_fetch(&started, 1);

	//printf("start thread routine: %d - %d\n", self.id, started);
	while(started)
		PAUSE();

	int i;
 	for(i=0; i<NBREQS; i++) {
		//		if(!(i % 10000))
		//			printf("===   %d requests executed for %d\n", i, self.running_core->core_id);
		liblock_exec(&lock, critical, arg);
 	}
	//printf("quiiting %d\n", self.id);
	return 0;
}

void test_lock() {
	int i, n=0;

	printf("====== test lock =====\n");
	pthread_t tid[nb_threads];

	shared = 0;

	for(i=0; i<nb_threads; i++) {
		liblock_thread_create_and_bind(&topology->cores[(i % (topology->nb_cores-1))], 0, &tid[i], 0, thread_routine, (void*)(uintptr_t)i);
		n = (n + 1) % nb_threads;
	}

	for(i=0; i<nb_threads; i++) {
		void *res;
		pthread_join(tid[i], &res);
	}

 	printf("result is: %d\n", shared);
 	int j, r=0;
 	for(i=0; i<nb_threads; i++)
 		for(j=0; j<NBREQS*NBLOOPS; j++)
			r += 42;
 	printf("should be: %d\n", r);

	if(r != shared)
		printf("[WARNING]         invalid result %d, should be %d\n", shared, r);
}


void* bad_synchro_cs(void* arg) {
	static int volatile var = 0;

	if(!arg) { // sender
		printf("sleeping 2s, then sending\n");
		sleep(2);
		printf("I'm up\n");
		var = 1;
	} else { // receiver
		printf("waiting\n");
		while(!var)
			PAUSE();
		printf("done!\n");
	}
	return 0;
}

void* bad_synchro(void* _arg) {
	int arg = (int)(uintptr_t)_arg;
	pthread_t tid;
	
	switch(arg) {
		case 0:
			printf("====== evil synchro =====\n");
			liblock_lock_init(liblock_name, &topology->cores[topology->nb_cores-1], &other_lock, 0);
			liblock_thread_create(&tid, 0, bad_synchro, (void*)(uintptr_t)1);
			liblock_exec(&lock, bad_synchro_cs, (void*)1);
			pthread_join(tid, 0);
			liblock_lock_destroy(&other_lock);
			break;
		case 1: 
			printf("!!!   second thread  !!!!\n");
			liblock_exec(&other_lock, bad_synchro_cs, (void*)0);
			break;
	}

	return 0;
}

int volatile var = 0;
liblock_cond_t pcond;

void* sender(void* arg) {
	printf("sender (go to sleep 5s)\n");
	sleep(5);
	printf("sender ACTIVATE var\n");
	var = 1;
	liblock_cond_broadcast(&pcond);
	printf("sender done\n");
	return 0;
}

void* zzz(void* arg) {
	printf("zzz thread (go to sleep 1s)\n");
	sleep(1);
	liblock_exec(&lock, sender, 0);
	printf("zzz thread done\n");
	return 0;
}

void* waiter(void* arg) {
	struct timespec ts;
	struct timeval  tv;

	printf("starting waiter\n");
	while(!var) {
		gettimeofday(&tv, 0);
		ts.tv_sec =  tv.tv_sec      + 0;
		ts.tv_nsec = tv.tv_usec*1e3 + 5e8;
		if(ts.tv_nsec > 1e9) {
			ts.tv_nsec -= 1e9;
			ts.tv_sec++;
		}

		liblock_cond_timedwait(&pcond, &lock, &ts);
		printf("waiter is up: %d\n", var);
	}

	printf("waiter done\n");
	return 0;
}

void test_wait() {
	pthread_t tid;
	printf("====== test wait =====\n");
	liblock_cond_init(&pcond, 0);
	liblock_thread_create(&tid, 0, zzz, 0);
	liblock_exec(&lock, waiter, 0);
	printf("waiting zzz\n");
	pthread_join(tid, 0);
	printf("------------     quiting test        -----------------\n");
}

extern void test_mini_thread();

void* two_lock_cs(void* arg) {
	liblock_lock_t* lock2 = arg;
	if(lock2 == 0) {
		printf("second cs\n");
	} else {
		printf("first cs\n");
		liblock_exec(lock2, two_lock_cs, 0);
	}
	return 0;
}

void two_locks() {
	printf("---- two locks ----\n");
	liblock_lock_t lock2;
	liblock_lock_init(liblock_name, &topology->cores[topology->nb_cores-1], &lock2, 0);
	liblock_exec(&lock, two_lock_cs, &lock2);
	liblock_lock_destroy(&lock2);
}

int main(int argc, char** argv) {
	struct sigaction sa;
	struct timeval start, end;
	liblock_name = DEFAULT;

	nb_threads = _NB_THREADS;

	if(argc > 1) {
		liblock_name = argv[1];

		if(argc > 2)
			nb_threads = atoi(argv[2]);
	}

	//liblock_monitored_lib = liblock_name;
	//liblock_count_cycles = 1;

	sigaction(SIGSEGV, 0, &sa);
	sa.sa_handler = sigsegv_handler;
	sigaction(SIGSEGV, &sa, 0);

	//sigaction(SIGTERM, 0, &sa);
	//sa.sa_handler = sigterm_handler;
	//sigaction(SIGTERM, &sa, 0);

	liblock_printlibs();
	printf("testing '%s' library with %d threads\n", liblock_name, nb_threads);

	liblock_servers_always_up = 0;

	liblock_define_core(&topology->cores[0]);
	liblock_bind_thread(pthread_self(), &topology->cores[0], 0);

	liblock_reserve_core_for(&topology->cores[topology->nb_cores-1], liblock_name);
	liblock_lock_init(liblock_name, &topology->cores[topology->nb_cores-1], &lock, 0);

	started = nb_threads;

	gettimeofday(&start, 0);
	//bad_synchro(0);
	test_lock();
	//test_wait();
	//two_locks();
	gettimeofday(&end, 0);

	printf("execution time: %3lf secondes\n",
				 (double)end.tv_sec + 1e-6*(double)end.tv_usec
				 - (double)start.tv_sec - 1e-6*(double)start.tv_usec);

	liblock_lock_destroy(&lock);

	printf("--- quitting ---\n");

	return 0;
}

