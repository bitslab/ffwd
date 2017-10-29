#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <numaif.h>
#include <numa.h>
#include <time.h>

#define ITERATION 1000000
#define MULTIPLIER	5101
#define MAX_THREADS 128

pthread_barrier_t start;
pthread_t thread_id[MAX_THREADS];
int num_of_threads;
int delay;
uint64_t g_num_of_vars;
volatile int g_server_started = 0;
volatile int g_number_of_finished_clients = 0;

struct counter
{
	uint64_t i;
	char padding[128-sizeof(uint64_t)];
}  __attribute__((aligned(128)));

struct counter counters[65536] __attribute__((aligned(128)));

#if 0
void * function(void *randno)
{
	int index = *(int*)randno;
	counters[index].i++;
}
#endif

void pin_thread(int core_id){                                                                                                     
   int num_cpu = numa_num_configured_cpus();                                                                                      
   struct bitmask * cpumask = numa_bitmask_alloc(num_cpu);                                                                        
   numa_bitmask_setbit(cpumask, core_id);                                                                                         
   numa_sched_setaffinity(0, cpumask);                                                                                            
}        

void* doSomeThing(void *id)
{
	uint64_t i, j, randno;
	int num_of_vars = g_num_of_vars;
	int local_delay = delay;
	randno = rand() % num_of_vars;

   const int _id = (int)id;                                                                                                
   pin_thread(_id);                                                                                                               
   pthread_barrier_wait(&start);      

    for(i = 0; i<ITERATION; i++)
    {
		__sync_fetch_and_add(&counters[randno].i,1);
	   for (j=0; j<local_delay; j++)
			__asm__ __volatile__ ("rep;nop":::"memory");
	   randno = (randno+MULTIPLIER) % num_of_vars;
    }

#if 0
    if(__sync_add_and_fetch(&g_number_of_finished_clients, 1) == num_of_threads)
    {
	   for (i = 0; i < num_of_vars; i++)
	   {
          liblock_lock_destroy(&(locks[i].lock));
	   }
    }
#endif
    return NULL;
}

int main(int argc, char ** argv)
{
    int j, i = 1;
    int err;
    int c;
	struct timespec tt_start, tt_end;

	srand(time(NULL));

   while ((c = getopt(argc, argv, "t:g:d:")) != -1)
	{
        switch (c)
        {
            case 't':
            {
                num_of_threads = atoi(optarg);
                break;
            }
            case 'g':
            {
                g_num_of_vars = atoi(optarg);
                break;
            }
            case 'd':
            {
                delay = atoi(optarg);
                break;
            }
        }
    }
	
	if (g_num_of_vars <= 0)
	{
		g_num_of_vars = 1;
	}

	for (i = 0; i < g_num_of_vars; i++)
	{
		counters[i].i = 0;
	}

	pthread_barrier_init(&start, 0, num_of_threads+1);

    for (i=0; i<num_of_threads; i++)
    {
      err = pthread_create(&(thread_id[i]), NULL, &doSomeThing, (void*)i);

      if (err != 0)
        printf("\ncannot create thread :[%s]", strerror(err));
    }
    pthread_barrier_wait(&start);

    clock_gettime(CLOCK_MONOTONIC, &tt_start);
    for (i=0; i<num_of_threads; i++){
      err = pthread_join(thread_id[i], 0);
      if( err != 0)
		  {
        printf("\ncannot join thread :[%s]", strerror(err));
        exit(1);
		  }
    }
    clock_gettime(CLOCK_MONOTONIC, &tt_end);

#if 1
	uint64_t sum = 0;
	for (i=0;i<g_num_of_vars;i++)
	{
		sum += counters[i].i;
		//printf("counter[%d]: %lu\n", i, counters[i].i);
	}
	
	if (sum != (num_of_threads*ITERATION))
		printf("Faulty program. sum = %lu, should be %d\n", sum, num_of_threads*ITERATION);
#endif

    uint64_t start = (tt_start.tv_sec * 1000000000LL) + tt_start.tv_nsec;
    uint64_t finish = (tt_end.tv_sec * 1000000000LL) + tt_end.tv_nsec;
    double duration = (double)(finish - start)/1000000000LL;
    double ops_per_sec = (double)(num_of_threads*ITERATION)/duration;
    double mops_per_sec = ops_per_sec / 1000000LL;
    //printf("%.3f secs\n", (double)(duration));
    //printf("Mops/sec %.3f\n", (double)mops_per_sec);
    printf("%.3f\n", (double)mops_per_sec);

    return 0;
}
