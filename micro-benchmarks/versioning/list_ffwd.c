#include <assert.h>
#include <stdint.h>
#include <numa.h>
#include "macro.h"
#include "ffwd.h"

#include "benchmark_list.h"

typedef struct node {
	int value;
	struct node *next;
} node_t;

typedef struct spinlock_list {
	pthread_spinlock_t spinlock;
	node_t *head;
} spinlock_list_t;

spinlock_list_t *list;

pthread_data_t *alloc_pthread_data(void)
{
	pthread_data_t *d;
	size_t size = sizeof(pthread_data_t);

	size = CACHE_ALIGN_SIZE(size);

	d = (pthread_data_t *)malloc(size);
	if (d != NULL)
		d->ds_data = NULL;

	return d;
}

void free_pthread_data(pthread_data_t *d)
{
	#ifndef FFWD
		free(d);
	#endif
}

void *list_global_init(int init_size, int value_range)
{
	node_t *node;
	int i;

	list = (spinlock_list_t *)malloc(sizeof(spinlock_list_t));
	if (list == NULL)
		return NULL;
	pthread_spin_init(&list->spinlock, PTHREAD_PROCESS_PRIVATE);
	 list->head = (node_t *)malloc(sizeof(node_t));

	node = list->head;
	if (node == NULL)
		return NULL;
	node->value = INT_MIN;

	for (i = 0; i < value_range; i += value_range / init_size) {
		node->next = (node_t *)malloc(sizeof(node_t));
		if (node->next == NULL)
			return NULL;
		node = node->next;
		node->value = i;
	}
	
	node->next = (node_t *)malloc(sizeof(node_t));
	if (node->next == NULL)
		return NULL;
	node = node->next;
	node->value = INT_MAX;
	node->next = NULL;

	return list;
}

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads)
{
	return 0;
}

void list_global_exit(void *list)
{
	spinlock_list_t *l = (spinlock_list_t *)list;
	// free l->head

	pthread_spin_destroy(&l->spinlock);
}

uint64_t ffwd_list_ins(int key)
{
	node_t *prev, *cur, *new_node;
	int val;
	uint64_t ret;

	for (prev = list->head, cur = prev->next; cur != NULL; prev = cur, cur = cur->next)
		if ((val = cur->value) >= key)
			break;
	ret = (val != key);
	if (ret) {
		new_node = (node_t *)malloc(sizeof(node_t));
		assert(new_node != NULL);
		new_node->value = key;
		new_node->next = cur;
		prev->next = new_node;
	}
	
	return ret;
}

int list_ins(int key, pthread_data_t *data)
{
	int ret;

	GET_CONTEXT()
	FFWD_EXEC(0, ffwd_list_ins, ret, 1, key)

	return ret;
}

uint64_t ffwd_list_del(int key)
{
	node_t *prev, *cur;
	int val;
	uint64_t ret;

	for (prev = list->head, cur = prev->next; cur != NULL; prev = cur, cur = cur->next)
		if ((val = cur->value) >= key)
			break;
	ret = (val == key);
	if (ret)
		prev->next = cur->next;

	if (ret)
		free(cur);
	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	int ret;

	GET_CONTEXT()
	FFWD_EXEC(0, ffwd_list_del, ret, 1, key)

	return ret;
}

uint64_t ffwd_list_find(int key)
{
	node_t *cur;
	int val;
	uint64_t ret;

	for (cur = list->head->next; cur != NULL; cur = cur->next)
		if ((val = cur->value) >= key)
			break;
	ret = (val == key);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{

	int ret;

	GET_CONTEXT()
	FFWD_EXEC(0, ffwd_list_find, ret, 1, key)

	return ret;
}
