#include "benchmark_list.h"
#include "rlu.h"

#define TEST_RLU_MAX_WS 1

typedef struct node {
	int value;
	struct node *next;
} node_t;

typedef struct rlu_list {
	node_t *head;
} rlu_list_t;

pthread_data_t *alloc_pthread_data(void)
{
	pthread_data_t *d;
	size_t pthread_size, rlu_size;

	pthread_size = sizeof(pthread_data_t);
	pthread_size = CACHE_ALIGN_SIZE(pthread_size);
	rlu_size = sizeof(rlu_thread_data_t);
	rlu_size = CACHE_ALIGN_SIZE(rlu_size);

	d = (pthread_data_t *)malloc(pthread_size + rlu_size);
	if (d != NULL)
		d->ds_data = ((void *)d) + pthread_size;

	return d;
}

void free_pthread_data(pthread_data_t *d)
{
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)d->ds_data;

	RLU_THREAD_FINISH(rlu_data);

	free(d);
}

void *list_global_init(int init_size, int value_range)
{
	rlu_list_t *list;
	node_t *node;
	int i;

	list = (rlu_list_t *)malloc(sizeof(rlu_list_t));
	if (list == NULL)
		return NULL;
	list->head = (node_t *)RLU_ALLOC(sizeof(node_t));
	node = list->head;
	if (node == NULL)
		return NULL;
	node->value = INT_MIN;

	for (i = 0; i < value_range; i+= value_range / init_size) {
		node->next = (node_t *)RLU_ALLOC(sizeof(node_t));
		if (node->next == NULL)
			return NULL;
		node = node->next;
		node->value = i;
	}
	node->next = (node_t *)RLU_ALLOC(sizeof(node_t));
	if (node->next == NULL)
		return NULL;
	node = node->next;
	node->value = INT_MAX;
	node->next = NULL;

	RLU_INIT(RLU_TYPE_FINE_GRAINED, TEST_RLU_MAX_WS);

	return list;
}

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads)
{
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;

	RLU_THREAD_INIT(rlu_data);

	return 0;
}

void list_global_exit(void *list)
{
	//free l->head;
}

int list_ins(int key, pthread_data_t *data)
{
	rlu_list_t *list = (rlu_list_t *)data->list;
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;
	node_t *prev, *next, *new_node;
	int ret, val;

restart:
	RLU_READER_LOCK(rlu_data);

	prev = (node_t *)RLU_DEREF(rlu_data, (list->head));
	next = (node_t *)RLU_DEREF(rlu_data, (prev->next));
	while (1) {
		val = next->value;
		if (val >= key)
			break;
		prev = next;
		next = (node_t *)RLU_DEREF(rlu_data, (prev->next));
	}
	ret = (val != key);

	if (ret) {
		if (!RLU_TRY_LOCK(rlu_data, &prev)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		if (!RLU_TRY_LOCK(rlu_data, &next)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}

		new_node = (node_t *)RLU_ALLOC(sizeof(node_t));
		new_node->value = key;
		RLU_ASSIGN_PTR(rlu_data, &(new_node->next), next);
		RLU_ASSIGN_PTR(rlu_data, &(prev->next), new_node);
	}

	RLU_READER_UNLOCK(rlu_data);

	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	rlu_list_t *list = (rlu_list_t *)data->list;
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;
	node_t *prev, *next, *n;
	int ret, val;

restart:
	RLU_READER_LOCK(rlu_data);

	prev = (node_t *)RLU_DEREF(rlu_data, (list->head));
	next = (node_t *)RLU_DEREF(rlu_data, (prev->next));
	while (1) {
		val = next->value;
		if (val >= key)
			break;
		prev = next;
		next = (node_t *)RLU_DEREF(rlu_data, (prev->next));
	}

	ret = (val == key);

	if (ret) {
		n = (node_t *)RLU_DEREF(rlu_data, (next->next));
		if (!RLU_TRY_LOCK(rlu_data, &prev)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		if (!RLU_TRY_LOCK(rlu_data, &next)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		RLU_ASSIGN_PTR(rlu_data, &(prev->next), n);
		RLU_FREE(rlu_data, next);
	}

	RLU_READER_UNLOCK(rlu_data);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{
	rlu_list_t *list = (rlu_list_t *)data->list;
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;
	node_t *next;
	int ret, val;

	RLU_READER_LOCK(rlu_data);

	next = (node_t *)RLU_DEREF(rlu_data, (list->head));
	next = (node_t *)RLU_DEREF(rlu_data, (next->next));
	while (1) {
		val = next->value;
		if (val >= key)
			break;
		next = (node_t *)RLU_DEREF(rlu_data, (next->next));
	}

	ret = (val == key);

	RLU_READER_UNLOCK(rlu_data);

	return ret;
}
