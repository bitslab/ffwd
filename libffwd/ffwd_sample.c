#define _GNU_SOURCE
#include <pthread.h>
#include <numa.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include "macro.h"
#include "ffwd.h"

#define ITERATION 1000000
#define MULTIPLIER 15485863
#define MAX 373587883

typedef struct data
{
	unsigned long ops;
	
}data;

struct timespec t_start, t_end;

int num_of_servers;
int num_of_threads;
int duration;
static volatile int stop;
volatile char dummy[128] __attribute__((aligned(128)));
volatile int variable[1024*32] __attribute__((aligned(128))) = {0};

uint64_t increment (int a){
	variable[a]++;
	return variable[a];
}

void* client(void *input_data){
	
	data* mydata = (data *) input_data;
	mydata->ops = 0;

	GET_CONTEXT()

	int i, j;
	uint64_t return_value; //this is going to hold the function return value from ffwd_exec

	unsigned long randno = rand();
	int index_to_increment;
	int servers = num_of_servers;

	while (stop == 0){
		
		index_to_increment = (randno % servers);
		FFWD_EXEC(index_to_increment%servers, &increment, return_value, 1, (index_to_increment*128))
		randno = ((randno + MULTIPLIER) % MAX);
		
		mydata->ops++;

		for(j=0; j<25; j++)
			__asm__ __volatile__ ("rep;nop;":::"memory");
	}

	return 0;
}

int main (int argc, char ** argv){
	if(numa_available() < 0){
        printf("System does not support NUMA API!\n");
    }

	int c, i;
	while((c = getopt(argc, argv, "t:s:d:")) != -1){
		switch (c)
		{
			case 't':
			{
				num_of_threads = atoi(optarg);
				break;
			}
			case 's':
			{
				num_of_servers = atoi(optarg);
				break;
			}
			case 'd':
			{
				duration = atoi(optarg);
				break;
			}
		}
	}

	data **th_data;
	th_data = (data **) malloc(num_of_threads * sizeof(data *));

	for (i = 0; i < num_of_threads; i++) {
		th_data[i] = (data *) malloc(128);
		th_data[i]->ops = 0;
	}

	struct timespec timeout;
	timeout.tv_sec = duration / 1000;
	timeout.tv_nsec = (duration % 1000) * 1000000;
	stop = 0;
	
	ffwd_init();
	launch_servers(4);

	pthread_t t[MAX_THREADS];

    for (i = 0; i < num_of_threads; i++){
		ffwd_thread_create(&t[i], 0, client, (void *)(th_data[i]) );
	}

	clock_gettime(CLOCK_MONOTONIC, &t_start);
	nanosleep(&timeout, NULL);
	stop = 1;
	clock_gettime(CLOCK_MONOTONIC, &t_end);

	for (i = 0; i < num_of_threads; i++){
		pthread_join(t[i], 0);
	}

	unsigned long nr_ops = 0;
	unsigned long nr_ops1 = 0;


    for (i = 0;  i < num_of_threads; i++) {
		nr_ops += (th_data[i]->ops);
	}

	uint64_t start = (t_start.tv_sec * 1000000000LL) + t_start.tv_nsec;
    uint64_t finish = (t_end.tv_sec * 1000000000LL) + t_end.tv_nsec;
    uint64_t duration = finish - start;
    double duration_sec = (double)(duration) / 1000000000LL;

   	printf("%d %.3f %.3f\n", num_of_threads, duration_sec, (nr_ops)/((double)(duration_sec*1000000LL)));

	ffwd_shutdown();

	for (i = 0; i < num_of_threads; i++) {
		free(th_data[i]);
	}


	return 0;
}


