#include "benchmark_list_move.h"
#include "rlu.h"

#define TEST_RLU_MAX_WS 1

typedef struct node{
	int value;
	struct node *next;
} node_t;

typedef struct rlu_list {
	node_t *head[2];
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
	node_t *node[2];
	int i;

	list = (rlu_list_t *)malloc(sizeof(rlu_list_t));
	if (list == NULL)
		return NULL;
	list->head[0] = (node_t *)RLU_ALLOC(sizeof(node_t));
	list->head[1] = (node_t *)RLU_ALLOC(sizeof(node_t));
	node[0] = list->head[0];
	node[1] = list->head[1];
	if (node[0] == NULL || node[1] == NULL)
		return NULL;
	node[0]->value = INT_MIN;
	node[1]->value = INT_MIN;

	for (i = 0; i < value_range; i+= value_range / init_size) {
		node[0]->next = (node_t *)RLU_ALLOC(sizeof(node_t));
		node[1]->next = (node_t *)RLU_ALLOC(sizeof(node_t));
		if (node[0]->next == NULL || node[1]->next == NULL)
			return NULL;
		node[0] = node[0]->next;
		node[1] = node[1]->next;
		node[0]->value = i;
		node[1]->value = i + 1;
	}
	node[0]->next = (node_t *)RLU_ALLOC(sizeof(node_t));
	node[1]->next = (node_t *)RLU_ALLOC(sizeof(node_t));
	node[0] = node[0]->next;
	node[1] = node[1]->next;
	if (node[0] == NULL || node[1] == NULL)
		return NULL;
	node[0]->value = INT_MAX;
	node[1]->value = INT_MAX;
	node[0]->next = NULL;
	node[1]->next = NULL;

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

int list_move(int key, pthread_data_t *data, int from)
{
	rlu_list_t *list = (rlu_list_t *)data->list;
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;
	node_t *cur, *prev_src, *next_src, *prev_dst, *next_dst;
	int ret, val;

restart:
	RLU_READER_LOCK(rlu_data);

	prev_src = (node_t *)RLU_DEREF(rlu_data, (list->head[from]));
	cur = (node_t *)RLU_DEREF(rlu_data, (prev_src->next));
	while (1) {
		val = cur->value;
		if (val >= key)
			break;
		prev_src = cur;
		cur = (node_t *)RLU_DEREF(rlu_data, (prev_src->next));
	}
	ret = (val == key);
	if (!ret)
		goto out;
	prev_dst = (node_t *)RLU_DEREF(rlu_data, (list->head[1 - from]));
	next_dst = (node_t *)RLU_DEREF(rlu_data, (prev_dst->next));
	while (1) {
		val = next_dst->value;
		if (val >= key)
			break;
		prev_dst = next_dst;
		next_dst = (node_t *)RLU_DEREF(rlu_data, (prev_dst->next));
	}
	ret = (val != key);
	if (!ret)
		goto out;
	next_src = (node_t *)RLU_DEREF(rlu_data, (cur->next));
	/*
	 * Do we want to lock here, or right after first search?
	 * locking after first search might avoid unnecessary second search
	 * locking here might grants more concurrency
	 */
	if (!RLU_TRY_LOCK(rlu_data, &prev_src)) {
		RLU_ABORT(rlu_data);
		goto restart;
	}
	if (!RLU_TRY_LOCK(rlu_data, &cur)) {
		RLU_ABORT(rlu_data);
		goto restart;
	}
	if (!RLU_TRY_LOCK(rlu_data, &prev_dst)) {
		RLU_ABORT(rlu_data);
		goto restart;
	}
	RLU_ASSIGN_PTR(rlu_data, &(prev_src->next), next_src);
	RLU_ASSIGN_PTR(rlu_data, &(cur->next), next_dst);
	RLU_ASSIGN_PTR(rlu_data, &(prev_dst->next), cur);

out:
	RLU_READER_UNLOCK(rlu_data);

	return ret;
}
