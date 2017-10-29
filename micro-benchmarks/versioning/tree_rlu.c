#include "benchmark_list.h"
#include "rlu.h"
#include <stdio.h>

#define TEST_RLU_MAX_WS 1

typedef struct node {
	int value;
	struct node *child[2];
} node_t;

typedef struct rlu_tree {
	node_t *root;
} rlu_tree_t;

static node_t *rlu_new_node(int key)
{
	node_t *node = RLU_ALLOC(sizeof(node_t));

	node->value = key;
	node->child[0] = NULL;
	node->child[1] = NULL;

	return node;
}

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
	rlu_tree_t *tree;
	node_t *prev, *cur, *new_node;
	int i, key, val, direction;

	tree = (rlu_tree_t *)malloc(sizeof(rlu_tree_t));
	if (tree == NULL)
		return NULL;
	tree->root = rlu_new_node(INT_MAX);

	i = 0;
	while (i < init_size) {
		key = rand() % value_range;

		prev = tree->root;
		cur = prev->child[0];
		direction = 0;
		while (cur != NULL) {
			prev = cur;
			val = cur->value;
			if (val > key) {
				direction = 0;
				cur = cur->child[0];
			} else if (val < key) {
				direction = 1;
				cur = cur->child[1];
			} else
				break;
		}
		if (cur != NULL)
			continue;
		new_node = rlu_new_node(key);
		if (new_node == NULL)
			return NULL;
		prev->child[direction] = new_node;
		i++;
	}

	RLU_INIT(RLU_TYPE_FINE_GRAINED, TEST_RLU_MAX_WS);

	return tree;
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
	rlu_tree_t *tree = (rlu_tree_t *)data->list;
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;
	node_t *prev, *cur, *new_node;
	int direction, ret, val;

restart:
	RLU_READER_LOCK(rlu_data);

	prev = (node_t *)RLU_DEREF(rlu_data, (tree->root));
	cur = (node_t *)RLU_DEREF(rlu_data, (prev->child[0]));
	direction = 0;
	while (cur != NULL) {
		val = cur->value;
		if (val > key) {
			direction = 0;
			prev = cur;
			cur = (node_t *)RLU_DEREF(rlu_data, (cur->child[0]));
		} else if (val < key) {
			direction = 1;
			prev = cur;
			cur = (node_t *)RLU_DEREF(rlu_data, (cur->child[1]));
		} else
			break;
	}
	ret = (cur == NULL);
	if (ret) {
		if (!RLU_TRY_LOCK(rlu_data, &prev)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		new_node = rlu_new_node(key);
		RLU_ASSIGN_PTR(rlu_data, &(prev->child[direction]), new_node);
	}

	RLU_READER_UNLOCK(rlu_data);

	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	rlu_tree_t *tree = (rlu_tree_t *)data->list;
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;
	node_t *prev, *cur, *prev_succ, *succ, *next;
	node_t *cur_child_l, *cur_child_r;
	int direction, ret, val;

restart:
	RLU_READER_LOCK(rlu_data);

	prev = (node_t *)RLU_DEREF(rlu_data, (tree->root));
	cur = (node_t *)RLU_DEREF(rlu_data, (prev->child[0]));
	direction = 0;
	while (cur != NULL) {
		val = cur->value;
		if (val > key) {
			direction = 0;
			prev = cur;
			cur = (node_t *)RLU_DEREF(rlu_data, (cur->child[0]));
		} else if (val < key) {
			direction = 1;
			prev = cur;
			cur = (node_t *)RLU_DEREF(rlu_data, (cur->child[1]));
		} else
			break;
	}

	ret = (cur != NULL);
	if (!ret)
		goto out;

	cur_child_l = (node_t *)RLU_DEREF(rlu_data, (cur->child[0]));
	cur_child_r = (node_t *)RLU_DEREF(rlu_data, (cur->child[1]));
	if (cur_child_l == NULL) {
		if (!RLU_TRY_LOCK(rlu_data, &prev) ||
		    !RLU_TRY_LOCK(rlu_data, &cur)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		RLU_ASSIGN_PTR(rlu_data, &(prev->child[direction]), cur_child_r);
		goto out;
	}
	if (cur_child_r == NULL) {
		if (!RLU_TRY_LOCK(rlu_data, &prev) ||
		    !RLU_TRY_LOCK(rlu_data, &cur)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		RLU_ASSIGN_PTR(rlu_data, &(prev->child[direction]), cur_child_l);
		goto out;
	}
	prev_succ = cur;
	succ = cur_child_r;
	next = (node_t *)RLU_DEREF(rlu_data, (succ->child[0]));
	while (next != NULL) {
		prev_succ = succ;
		succ = next;
		next = (node_t *)RLU_DEREF(rlu_data, next->child[0]);
	}

	if (prev_succ == cur) {
		if (!RLU_TRY_LOCK(rlu_data, &prev) ||
		    !RLU_TRY_LOCK(rlu_data, &cur) ||
		    !RLU_TRY_LOCK(rlu_data, &succ)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		RLU_ASSIGN_PTR(rlu_data, &(prev->child[direction]), succ);
		RLU_ASSIGN_PTR(rlu_data, &(succ->child[0]), cur_child_l);
	} else {
		if (!RLU_TRY_LOCK(rlu_data, &prev) ||
		    !RLU_TRY_LOCK(rlu_data, &cur) ||
		    !RLU_TRY_LOCK(rlu_data, &prev_succ) ||
		    !RLU_TRY_LOCK(rlu_data, &succ)) {
			RLU_ABORT(rlu_data);
			goto restart;
		}
		RLU_ASSIGN_PTR(rlu_data, &(prev->child[direction]), succ);
		RLU_ASSIGN_PTR(rlu_data, &(prev_succ->child[0]),
		               RLU_DEREF(rlu_data, (succ->child[1])));
		RLU_ASSIGN_PTR(rlu_data, &(succ->child[0]), cur_child_l);
		RLU_ASSIGN_PTR(rlu_data, &(succ->child[1]), cur_child_r);
	}

out:
	if (ret)
		RLU_FREE(rlu_data, cur);
	RLU_READER_UNLOCK(rlu_data);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{
	rlu_tree_t *tree = (rlu_tree_t *)data->list;
	rlu_thread_data_t *rlu_data = (rlu_thread_data_t *)data->ds_data;
	node_t *cur;
	int ret, val;

	RLU_READER_LOCK(rlu_data);

	cur = (node_t *)RLU_DEREF(rlu_data, (tree->root));
	cur = (node_t *)RLU_DEREF(rlu_data, (cur->child[0]));
	while (cur != NULL) {
		val = cur->value;
		if (val > key) {
			cur = (node_t *)RLU_DEREF(rlu_data, (cur->child[0]));
		} else if (val < key) {
			cur = (node_t *)RLU_DEREF(rlu_data, (cur->child[1]));
		} else
			break;
	}
	ret = (cur != NULL);

	RLU_READER_UNLOCK(rlu_data);

	return ret;
}
