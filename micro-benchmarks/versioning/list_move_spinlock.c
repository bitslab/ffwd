#include <assert.h>

#include "benchmark_list_move.h"

typedef struct node {
	int value;
	struct node *next;
} node_t;

typedef struct spinlock_list {
	pthread_spinlock_t spinlock;
	node_t *head[2];
} spinlock_list_t;

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
	free(d);
}

void *list_global_init(int init_size, int value_range)
{
	spinlock_list_t *list;
	node_t *node[2];
	int i;

	list = (spinlock_list_t *)malloc(sizeof(spinlock_list_t));
	if (list == NULL)
		return NULL;
	pthread_spin_init(&list->spinlock, PTHREAD_PROCESS_PRIVATE);
	list->head[0] = (node_t *)malloc(sizeof(node_t));
	list->head[1] = (node_t *)malloc(sizeof(node_t));
	node[0] = list->head[0];
	node[1] = list->head[1];
	if (node[0] == NULL || node[1] == NULL)
		return NULL;
	node[0]->value = INT_MIN;
	node[1]->value = INT_MIN;

	for (i = 0; i < value_range; i += value_range / init_size) {
		node[0]->next = (node_t *)malloc(sizeof(node_t));
		node[1]->next = (node_t *)malloc(sizeof(node_t));
		if (node[0]->next == NULL || node[1]->next == NULL)
			return NULL;
		node[0] = node[0]->next;
		node[1] = node[1]->next;
		node[0]->value = i;
		node[1]->value = i + 1;
	}
	node[0]->next = (node_t *)malloc(sizeof(node_t));
	node[1]->next = (node_t *)malloc(sizeof(node_t));
	if (node[0]->next == NULL || node[1]->next == NULL)
		return NULL;
	node[0]->value = INT_MAX;
	node[1]->value = INT_MAX;

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

int list_move(int key, pthread_data_t *data, int from)
{
	spinlock_list_t *list = (spinlock_list_t *)data->list;
	node_t *prev_src, *cur, *prev_dst, *next_dst;
	int ret, val;

	pthread_spin_lock(&list->spinlock);
	for (prev_src = list->head[from], cur = prev_src->next; cur != NULL;
	     prev_src = cur, cur = cur->next)
		if ((val = cur->value) >= key)
			break;
	ret = (val == key);
	if (!ret)
		goto out;
	for (prev_dst = list->head[1 - from], next_dst = prev_dst->next; next_dst != NULL;
	     prev_dst = next_dst, next_dst = next_dst->next)
		if ((val = next_dst->value) >= key)
			break;
	ret = (val != key);
	if (!ret)
		goto out;
	prev_src->next = cur->next;
	prev_dst->next = cur;
	cur->next = next_dst;
out:
	pthread_spin_unlock(&list->spinlock);

	return ret;
}
