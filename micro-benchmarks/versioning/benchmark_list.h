#ifndef BENCHMARK_LIST_H
#define BENCHMARK_LIST_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <limits.h>

typedef struct barrier {
	pthread_cond_t complete;
	pthread_mutex_t mutex;
	int count;
	int crossing;
} barrier_t;

#define CACHE_ALIGN (192)

#define CACHE_ALIGN_SIZE(size) ((((size - 1) / CACHE_ALIGN) + 1) * CACHE_ALIGN)

#define PTHREAD_PADDING (16)

typedef struct pthread_data {
	long pthread_padding[PTHREAD_PADDING];
	long id;
	unsigned long nr_ins;
	unsigned long nr_del;
	unsigned long nr_find;
	unsigned long nr_txn;
	int range;
	int update_ratio;
	unsigned int seed;
	barrier_t *barrier;
	void *list;
	void *ds_data; // data structure specific data
} pthread_data_t;

pthread_data_t *alloc_pthread_data(void);

void free_pthread_data(pthread_data_t *d);

void *list_global_init(int init_size, int value_range);

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads);

void list_global_exit(void *list);

int list_ins(int key, pthread_data_t *data);

int list_del(int key, pthread_data_t *data);

int list_find(int key, pthread_data_t *data);

uint64_t list_ins_ffwd(int);
uint64_t list_del_ffwd(int);
uint64_t list_find_ffwd(int);

#endif
