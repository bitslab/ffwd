#include "benchmark_list.h"
#include "rlu.h"
#include <stdio.h>
#include <stdint.h>
#include <numa.h>
#include "liblock.h"
#include "liblock-config.h"
#include "liblock-fatal.h"

#define TEST_RLU_MAX_WS 1

typedef struct node {
	int value;
	struct node *child[2];
} node_t;

typedef struct rlu_tree {
	node_t *root;
} rlu_tree_t;


rlu_tree_t *tree;

static node_t *rlu_new_node(int key)
{
	node_t *node = malloc(sizeof(node_t));

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
}

void *list_global_init(int init_size, int value_range)
{
	// rlu_tree_t *tree;
	node_t *prev, *cur, *new_node;
	int i, key, val, direction;

	tree = (rlu_tree_t *)numa_alloc_onnode(sizeof(rlu_tree_t), 0);

	if (tree == NULL){
		return NULL;
	}
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
		if (new_node == NULL){
			return NULL;
		}
		prev->child[direction] = new_node;
		i++;
	}

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

uint64_t list_ins_ffwd(int key)
{
	node_t *prev, *cur, *new_node;
	int direction, val;
	uint64_t ret;

	prev = (node_t *)(tree->root);
	cur = (node_t *)(prev->child[0]);
	direction = 0;
	while (cur != NULL) {
		val = cur->value;
		if (val > key) {
			direction = 0;
			prev = cur;
			cur = (node_t *)(cur->child[0]);
		} else if (val < key) {
			direction = 1;
			prev = cur;
			cur = (node_t *)(cur->child[1]);
		} else
			break;
	}
	ret = (cur == NULL);
	if (ret) {
		new_node = rlu_new_node(key);
		prev->child[direction] = new_node;
	}

	return ret;
}


int list_ins(int key, pthread_data_t *data)
{
	uint64_t ret;
	ret = liblock_exec(&(lhead), &list_ins_ffwd, (void *)key);

	return ret;
}

uint64_t list_del_ffwd(int key)
{
	node_t *prev, *cur, *prev_succ, *succ, *next;
	node_t *cur_child_l, *cur_child_r;
	int direction, val;
	uint64_t ret;

	prev = (node_t *)(tree->root);
	cur = (node_t *)(prev->child[0]);
	direction = 0;
	while (cur != NULL) {
		val = cur->value;
		if (val > key) {
			direction = 0;
			prev = cur;
			cur = (node_t *)(cur->child[0]);
		} else if (val < key) {
			direction = 1;
			prev = cur;
			cur = (node_t *)(cur->child[1]);
		} else
			break;
	}

	ret = (cur != NULL);
	if (!ret)
		goto out;

	cur_child_l = (node_t *)(cur->child[0]);
	cur_child_r = (node_t *)(cur->child[1]);
	if (cur_child_l == NULL) {
		prev->child[direction] = cur_child_r;
		goto out;
	}
	if (cur_child_r == NULL) {
		prev->child[direction] = cur_child_l;
		goto out;
	}
	prev_succ = cur;
	succ = cur_child_r;
	next = (node_t *)(succ->child[0]);
	while (next != NULL) {
		prev_succ = succ;
		succ = next;
		next = (node_t *)(next->child[0]);
	}

	if (prev_succ == cur) {
		prev->child[direction] = succ;
		succ->child[0] = cur_child_l;
	} else {
		prev->child[direction] = succ;
		prev_succ->child[0] = (succ->child[1]);
		succ->child[0] = cur_child_l;
		succ->child[1] = cur_child_r;
	}

out:
	free(cur);
	return ret;
}

int list_del(int key, pthread_data_t *data)
{

	uint64_t ret;
	ret = liblock_exec(&(lhead), &list_del_ffwd, (void *)key);

	return ret;
}

uint64_t list_find_ffwd(int key)
{
  	node_t *cur;
	int val;
	uint64_t ret;

	cur = (node_t *)(tree->root);
	cur = (node_t *)(cur->child[0]);

	while (cur != NULL) {
		val = cur->value;
		if (val > key) {
			cur = (node_t *)(cur->child[0]);
		} else if (val < key) {
			cur = (node_t *)(cur->child[1]);
		} else
			break;
	}
	ret = (cur != NULL);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{

	uint64_t ret;
	ret = liblock_exec(&(lhead), &list_find_ffwd, (void *)key);

	return ret;
}
