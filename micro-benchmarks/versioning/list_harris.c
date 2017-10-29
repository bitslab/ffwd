#include <assert.h>

#include "benchmark_list.h"
#include "util.h"
#include "qsbr.h"

typedef struct node {
	int value;
	struct node *next;
} node_t;

typedef struct harris_list {
	node_t *head;
} harris_list_t;

typedef struct harris_pthread_data {
	unsigned long count;
	qsbr_pthread_data_t *qsbr_data;
} harris_pthread_data_t;

static inline void harris_free_node(node_t *node, qsbr_pthread_data_t *qsbr_data)
{
	qsbr_free_ptr(node, qsbr_data);
}

#define QSBR_PERIOD 100

static inline void harris_maybe_quiescent(harris_pthread_data_t *harris_data)
{
	harris_data->count++;
	if (harris_data->count % QSBR_PERIOD == 0)
		qsbr_quiescent_state(harris_data->qsbr_data);
}

pthread_data_t *alloc_pthread_data(void)
{
	pthread_data_t *d;
	size_t pthread_size, harris_size, qsbr_size;

	pthread_size = sizeof(pthread_data_t);
	pthread_size = CACHE_ALIGN_SIZE(pthread_size);
	harris_size = sizeof(harris_pthread_data_t);
	harris_size = CACHE_ALIGN_SIZE(harris_size);
	qsbr_size = sizeof(qsbr_pthread_data_t);
	qsbr_size = CACHE_ALIGN_SIZE(qsbr_size);

	d = (pthread_data_t *)malloc(pthread_size + harris_size + qsbr_size);
	if (d != NULL) {
		d->ds_data = ((void *)d) + pthread_size;
		((harris_pthread_data_t *)d->ds_data)->qsbr_data = ((void *)d) + pthread_size + harris_size;
	}

	return d;
}

void free_pthread_data(pthread_data_t *d)
{
	//free qsbr freelist
	free(d);
}

void *list_global_init(int init_size, int value_range)
{
	harris_list_t *list;
	node_t *node;
	int i;

	list = (harris_list_t *)malloc(sizeof(harris_list_t));
	if (list == NULL)
		return NULL;
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

	qsbr_init();

	return list;
}

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads)
{
	harris_pthread_data_t *harris_data = (harris_pthread_data_t *)data->ds_data;

	harris_data->count = 0;
	qsbr_pthread_init(harris_data->qsbr_data);

	return 0;
}

void list_global_exit(void *list)
{
	// free l->head;
}

#define __list_find(prev, cur, next, val, key, harris_data, tag) \
	do { \
		for (cur = *prev; cur != NULL; cur = next) { \
			if (*prev != cur) \
				goto tag; \
			next = cur->next; \
			if ((unsigned long)next & 0x1) { \
				if (!CAS(prev, cur, (node_t *)((unsigned long)next - 1))) \
					goto tag; \
				harris_free_node(cur, harris_data->qsbr_data); \
				next = (node_t *)((unsigned long)next - 1); \
			} else { \
				val = cur->value; \
				if (val >= key) \
					break; \
				prev = &cur->next; \
			} \
		} \
	} while (0)

int list_ins(int key, pthread_data_t *data)
{
	harris_list_t *list = (harris_list_t *)data->list;
	harris_pthread_data_t *harris_data = (harris_pthread_data_t *)data->ds_data;
	node_t **prev, *cur, *next, *new_node;
	int ret, val;

	new_node = (node_t *)malloc(sizeof(node_t));
	assert(new_node != NULL);
	new_node->value = key;

restart:
	prev = &list->head->next;

	__list_find(prev, cur, next, val, key, harris_data, restart);

	ret = (val != key);
	if (ret) {
		new_node->next = cur;
		MEMBARSTLD();

		if (!CAS(prev, cur, new_node))
			goto restart;
	} else
		free(new_node);

	harris_maybe_quiescent(harris_data);

	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	harris_list_t *list = (harris_list_t *)data->list;
	harris_pthread_data_t *harris_data = (harris_pthread_data_t *)data->ds_data;
	node_t **prev, *cur, *next;
	int ret, val;
restart:
	prev = &list->head->next;

	__list_find(prev, cur, next, val, key, harris_data, restart);

	ret = (val == key);
	if (ret) {
		if (!CAS(&cur->next, next, (node_t *)((unsigned long)next + 1)))
			goto restart;
		MEMBARSTLD();

		if (CAS(prev, cur, next))
			harris_free_node(cur, harris_data->qsbr_data);
	}

	harris_maybe_quiescent(harris_data);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{
	harris_list_t *list = (harris_list_t *)data->list;
	harris_pthread_data_t *harris_data = (harris_pthread_data_t *)data->ds_data;
	node_t **prev, *cur, *next;
	int ret, val;

restart:
	prev = &list->head->next;

	__list_find(prev, cur, next, val, key, harris_data, restart);

	ret = (val == key);

	harris_maybe_quiescent(harris_data);

	return ret;
}
