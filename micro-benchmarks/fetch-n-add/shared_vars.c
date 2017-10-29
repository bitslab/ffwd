#include <pthread.h>
#include <numa.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <numaif.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

#define MAX_VARS 4096

#ifdef SSYNC

#ifdef USE_MUTEX_LOCKS
#include <utils.h>
#endif
#include "lock_if.h"
#include "atomic_ops.h"

#elif FFWD

#include "ffwd.h"
#include "macro.h"

#elif LIBLOCK

#include <liblock.h>
#include <liblock-config.h>
#include <liblock-fatal.h>
volatile int g_server_started = 0;
volatile int g_number_of_finished_clients = 0;

#endif

#define MAX_THREADS 128
#define ITERATION 1000000
#define USE_MEM_BAR
#define MULTIPLIER  5101

/* global data */
//lock_global_data the_lock;
int num_of_threads = 0;
int num_of_vars = 1;
int delay = 0;
int num_of_servers = 1;
int start = 0;

#ifdef SSYNC
struct lock 
{
  lock_global_data lock;
  char padding[128-sizeof(lock_global_data)];
} __attribute__((aligned(128)));
struct lock locks[MAX_VARS] __attribute__((aligned(128)));
#elif LIBLOCK
struct lock 
{
  liblock_lock_t lock;
  char padding[128-sizeof(liblock_lock_t)];
} __attribute__((aligned(128)));
struct lock locks[MAX_VARS] __attribute__((aligned(128)));
#endif

struct variable
{
  uint64_t var;
  char padding[128-sizeof(uint64_t)];
} __attribute__((aligned(128)));
struct variable variables[MAX_VARS] __attribute__((aligned(128)));

#ifdef SSYNC

void* client(void *id)
{
  int cpu = *((int*)id), i, j; 
  int vars = num_of_vars;
  int randno = rand() % vars;
  int local_delay = delay;

  /* local data */
  lock_local_data my_data;
  /*initialize this thread's local data*/
  init_lock_local(cpu, &locks[0].lock, &my_data); // locks param matters only for HCLH & ARRAY Locks
#ifdef USE_MEM_BAR
    MEM_BARRIER;
#else
  while(start != 1)
    PAUSE;
#endif

  for(i = 0; i<ITERATION; i++)
  {
    acquire_lock(&my_data, &locks[randno].lock);
    variables[randno].var++;
    release_lock(&my_data, &locks[randno].lock);
    for (j=0; j<local_delay; j++)
      __asm__ __volatile__ ("rep;nop":::"memory");
    randno = (randno+MULTIPLIER) % vars;
  }

#ifdef USE_MEM_BAR
    MEM_BARRIER;
#endif
    /*free internal memory structures which may have been allocated for the local data*/
    free_lock_local(my_data);

    return NULL;
}

#elif FFWD

void increment(int a){
  variables[a].var++;
}

void* client(void * id)
{
  int i, j;
  int local_delay = delay;
  int vars = num_of_vars;
  int return_value; //this is going to hold the function return value from ffwd_exec
  int index_to_increment;
  int rem = (vars % num_of_servers) ? 1 : 0;
  int vars_per_server = (vars/num_of_servers) + rem;

  GET_CONTEXT();

  int randno = rand() % vars;

  for(i = 0; i < ITERATION; i++)
  {
    FFWD_EXEC(randno/vars_per_server, &increment, return_value, 1, randno)
    randno = (randno+MULTIPLIER) % vars;
    for (j=0; j<local_delay; j++)
      __asm__ __volatile__ ("rep;nop":::"memory");
  }
  return 0;
}

#elif LIBLOCK

void server_started() {
    g_server_started = 1;
}

void* function(void *randno)
{
  int index = (int)randno;
  variables[index].var++;
}

void* client(void *arg)
{
  uint64_t i, j, randno;
  int vars = num_of_vars;
  int local_delay = delay;
  randno = rand() % vars;

  while(!g_server_started)
      PAUSE();

  for(i = 0; i<ITERATION; i++)
  {
    liblock_exec(&(locks[randno].lock), &function, (void *)randno);
    for (j=0; j<local_delay; j++)
      __asm__ __volatile__ ("rep;nop":::"memory");
    randno = (randno+MULTIPLIER) % vars;
  }

  if(__sync_add_and_fetch(&g_number_of_finished_clients, 1) == num_of_threads)
  {
    for (i = 0; i < vars; i++)
    {
      liblock_lock_destroy(&(locks[i].lock));
    }
  }
  return NULL;
}
#endif

int main(int argc, char *argv[])
{
  pthread_t threads[MAX_THREADS];
  struct timespec tt_start, tt_end;
  int t;
  int ids[MAX_THREADS];
  int i, j, c;
  delay = 0;

  srand(time(NULL));

  if(numa_available() < 0){
    printf("System does not support NUMA API!\n");
  }

  for(j=0;j<MAX_THREADS;j++)
  {
    ids[j]=j;
  }

  while ((c = getopt(argc, argv, "t:g:d:s:")) != -1)
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
          num_of_vars = atoi(optarg);
          break;
      }
      case 'd':
      {
          delay = atoi(optarg);
          break;
      }
      case 's':
      {
          num_of_servers = atoi(optarg);
          break;
      }
    }
  }

  for (i = 0; i < num_of_vars; i++)
  {
    variables[i].var = 0;
  }

#ifdef SSYNC
  /*initialize the global data*/
  #ifndef USE_ARRAY_LOCKS 
  for(i = 0; i < num_of_vars; i++)
    init_lock_global(&locks[i].lock); 
  #else
    init_lock_global_nt(num_of_threads, &locks[0].lock);
  #endif
  start = 0;

  for(t=0;t<num_of_threads;t++){
    if (pthread_create(&threads[t], NULL, &client, &ids[t])!=0){
      fprintf(stderr,"Error creating thread\n");
      exit(-1);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &tt_start);
  #ifdef USE_MEM_BAR
  MEM_BARRIER;
  #else
  start = 1;
  #endif

  for (t = 0; t < num_of_threads; t++) {
      if (pthread_join(threads[t], NULL) != 0) {
          fprintf(stderr, "Error waiting for thread completion\n");
          exit(1);
      }
  }

#elif FFWD

  ffwd_init();
  launch_servers(num_of_servers);
  for (t = 0; t < num_of_threads; t++){
    ffwd_thread_create(&threads[t],NULL,client,0);
  }

  clock_gettime(CLOCK_MONOTONIC, &tt_start);

  for (t = 0; t < num_of_threads; t++) {
      if (pthread_join(threads[t], NULL) != 0) {
          fprintf(stderr, "Error waiting for thread completion\n");
          exit(1);
      }
  }

#elif LIBLOCK

  liblock_start_server_threads_by_hand = 1;
  liblock_servers_always_up = 0;
  g_server_started = 0;

  struct core cores[MAX_THREADS];
  for (j=0; j<MAX_THREADS; j++){
      cores[j].core_id = j;
      cores[j].server_type = 0;
      cores[j].frequency = 2200;
  }

  struct core* liblock_server_core_1;
  liblock_server_core_1 = &topology->cores[0];

  liblock_bind_thread(pthread_self(), liblock_server_core_1, TYPE_NOINFO);
  liblock_define_core(liblock_server_core_1);

  for (i = 0; i < num_of_vars; i++){
    if(liblock_lock_init(TYPE_NOINFO, liblock_server_core_1, &(locks[i].lock), NULL) != 0)
    {
        printf("\nlock initialization failed\n");
        return 1;
    }
  }

  for (t = 1; t <= num_of_threads; t++)
  {
    if(liblock_thread_create_and_bind(&topology->cores[t], 0, &(threads[t]), NULL, &client, NULL) != 0)
      fprintf(stderr,"Error creating thread\n");
  }

  clock_gettime(CLOCK_MONOTONIC, &tt_start);
  liblock_lookup(TYPE_NOINFO)->run(server_started);

  for (t = 1; t <= num_of_threads; t++) {
      if (pthread_join(threads[t], NULL) != 0) {
          fprintf(stderr, "Error waiting for thread completion\n");
          exit(1);
      }
  }
#endif

  clock_gettime(CLOCK_MONOTONIC, &tt_end);

#ifdef SSYNC
  /*free internal memory strucutres which may have been allocated for this lock */
  #if 0
  #if !defined USE_HTICKET_LOCKS && !defined USE_MCS_LOCKS
  for(i = 0; i < num_of_vars; i++)
    free_lock_global(locks[i].lock);
  #endif
  #endif
#elif FFWD
  ffwd_shutdown();
#endif

#if 0
  int sum = 0;
  for(i = 0; i < num_of_vars; i++)
  {
    sum += variables[i].var;
    //printf("var[%d]=%lu, sum=%d\n", i, variables[i].var, sum);
  }
  printf("counter: %d\n", sum);
#endif

  uint64_t start_time = (tt_start.tv_sec * 1000000000LL) + tt_start.tv_nsec;
  uint64_t finish_time = (tt_end.tv_sec * 1000000000LL) + tt_end.tv_nsec;
  double duration = (double)(finish_time - start_time)/1000000000LL;
  double ops_per_sec = (double)(num_of_threads*ITERATION)/duration;
  double mops_per_sec = ops_per_sec / 1000000LL;
  //printf("%.3f secs\n", (double)(duration));
  //printf("Mops/sec %.3f\n", (double)mops_per_sec);
  //printf("counter:%d\n", counter);
  printf("%.3f\n", (double)mops_per_sec);

  pthread_exit(NULL);
  return 0;
}
