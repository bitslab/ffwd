#include <assert.h>
#include <stdint.h>

#include "benchmark_list.h"

typedef struct node {
	int value;
	struct node *child[2] __attribute__((aligned(16)));
	pthread_mutex_t lock;
	int marked;
	int tag[2];
} node_t;

typedef struct rcu_tree {
	node_t *root;
} rcu_tree_t;

#define URCU_MAX_FREE_PTRS (1000)

typedef struct rcu_pthread_data {
	volatile long time;
	int nr_threads;
	struct rcu_pthread_data **sync_data;
	unsigned long *sync_time;
	int f_size;
	void *free_ptrs[URCU_MAX_FREE_PTRS];
} rcu_pthread_data_t;

static node_t *rcu_new_node(int key)
{
	node_t *ret = (node_t *)malloc(sizeof(node_t));
	assert(ret != NULL);
	ret->value = key;
	ret->marked = 0;
	ret->child[0] = NULL;
	ret->child[1] = NULL;
	ret->tag[0] = 0;
	ret->tag[1] = 0;
	if (pthread_mutex_init(&(ret->lock), NULL) != 0)
		assert(0);
	return ret;
}

static inline void set_bit(int nr, volatile unsigned long *addr)
{
    asm("btsl %1,%0" : "+m" (*addr) : "Ir" (nr));
}

inline void rcu_reader_lock(rcu_pthread_data_t *rcu_data)
{
	__sync_add_and_fetch(&rcu_data->time, 1);
}

inline void rcu_reader_unlock(rcu_pthread_data_t *rcu_data)
{
	set_bit(0, (unsigned long *)&rcu_data->time);
}

void urcu_synchronize(rcu_pthread_data_t *rcu_data)
{
	int i;
	unsigned long t;

	for (i = 0; i < rcu_data->nr_threads; i++)
		rcu_data->sync_time[i] = rcu_data->sync_data[i]->time;

	for (i = 0; i < rcu_data->nr_threads; i++) {
		if (rcu_data->sync_time[i] & 1)
			continue;
		while (1) {
			t = rcu_data->sync_data[i]->time;
			if (t & 1 || t > rcu_data->sync_time[i])
				break;
		}
	}
}

inline void rcu_free(void *ptr, rcu_pthread_data_t *rcu_data)
{
	int i;

	rcu_data->free_ptrs[rcu_data->f_size] = ptr;
	rcu_data->f_size++;

	if (rcu_data->f_size == URCU_MAX_FREE_PTRS) {
		urcu_synchronize(rcu_data);

		for (i = 0; i < URCU_MAX_FREE_PTRS; i++)
			free(rcu_data->free_ptrs[i]);

		rcu_data->f_size = 0;
	}
}

int validate(node_t *prev, int tag, node_t *cur, int direction)
{
	int ret;

	if (cur == NULL)
		ret = (!(prev->marked) && (prev->child[direction] == cur) &&
		      (prev->tag[direction] == tag));
	else
		ret = (!(prev->marked) && !(cur->marked) &&
		      prev->child[direction] == cur);
	return ret;
}

pthread_data_t *alloc_pthread_data(void)
{
	pthread_data_t *d;
	size_t pthread_size, rcu_size;

	pthread_size = sizeof(pthread_data_t);
	pthread_size = CACHE_ALIGN_SIZE(pthread_size);
	rcu_size = sizeof(rcu_pthread_data_t);
	rcu_size = CACHE_ALIGN_SIZE(rcu_size);

	d = (pthread_data_t *)malloc(pthread_size + rcu_size);
	if (d != NULL)
		d->ds_data = ((void *)d) + pthread_size;

	return d;
}

void free_pthread_data(pthread_data_t *d)
{
	int i;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)d->ds_data;

	free(rcu_data->sync_data);
	free(rcu_data->sync_time);

	for (i = 0; i < rcu_data->f_size; i++)
		free(rcu_data->free_ptrs[i]);

	free(d);
}

void *list_global_init(int init_size, int value_range)
{
	rcu_tree_t *tree;
	node_t *prev, *cur, *new_node;
	int i, key, val, direction;

	tree = (rcu_tree_t *)malloc(sizeof(rcu_tree_t));
	if (tree == NULL)
		return NULL;
	tree->root = rcu_new_node(INT_MAX);

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
		new_node = rcu_new_node(key);
		if (new_node == NULL)
			return NULL;
		prev->child[direction] = new_node;
		i++;
	}

	return tree;
}

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads)
{
	int i;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;

	rcu_data->time = 1;
	rcu_data->nr_threads = nr_threads;
	if ((rcu_data->sync_data = malloc(nr_threads * sizeof(struct rcu_pthread_data *))) == NULL)
		return -1;
	if ((rcu_data->sync_time = malloc(nr_threads * sizeof(unsigned long))) == NULL)
		return -1;
	for (i = 0; i < nr_threads; i++) {
		rcu_data->sync_data[i] = sync_data[i]->ds_data;
		rcu_data->sync_time[i] = 1;
	}
	rcu_data->f_size = 0;

	return 0;
}

void list_global_exit(void *tree)
{

}

int list_ins(int key, pthread_data_t *data)
{
	rcu_tree_t *tree = (rcu_tree_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *new_node;
	int direction, tag, val;

	while (1) {
		rcu_reader_lock(rcu_data);
		prev = tree->root;
		cur = prev->child[0];
		direction = 0;
		while (cur != NULL) {
			val = cur->value;
			if (val > key) {
				direction = 0;
				prev = cur;
				cur = cur->child[0];
			} else if (val < key) {
				direction = 1;
				prev = cur;
				cur = cur->child[1];
			} else
				break;
		}
		tag = prev->tag[direction];
		rcu_reader_unlock(rcu_data);
		if (cur != NULL)
			return 0;
		pthread_mutex_lock(&(prev->lock));
		if (validate(prev, tag, cur, direction)) {
			new_node = rcu_new_node(key);
			prev->child[direction] = new_node;
			pthread_mutex_unlock(&(prev->lock));
			return 1;
		}
		pthread_mutex_unlock(&(prev->lock));
	}
}

int list_del(int key, pthread_data_t *data)
{
	rcu_tree_t *tree = (rcu_tree_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *prev_succ, *succ, *next, *new_node;
	int direction, val, direction_succ;

	while (1) {
		rcu_reader_lock(rcu_data);
		prev = tree->root;
		cur = prev->child[0];
		direction = 0;
		while (cur != NULL) {
			val = cur->value;
			if (val > key) {
				direction = 0;
				prev = cur;
				cur = cur->child[0];
			} else if (val < key) {
				direction = 1;
				prev = cur;
				cur = cur->child[1];
			} else
				break;
		}
		rcu_reader_unlock(rcu_data);
		if (cur == NULL)
			return 0;
		pthread_mutex_lock(&(prev->lock));
		pthread_mutex_lock(&(cur->lock));
		if (!validate(prev, 0, cur, direction)) {
			pthread_mutex_unlock(&(prev->lock));
			pthread_mutex_unlock(&(cur->lock));
			continue;
		}
		if (cur->child[0] == NULL) {
			cur->marked = 1;
			prev->child[direction] = cur->child[1];
			if (prev->child[direction] == NULL)
				prev->tag[direction]++;
			pthread_mutex_unlock(&(prev->lock));
			pthread_mutex_unlock(&(cur->lock));
			rcu_free(cur, rcu_data);
			return 1;
		}
		if (cur->child[1] == NULL) {
			cur->marked = 1;
			prev->child[direction] = cur->child[0];
			if (prev->child[direction] == NULL)
				prev->tag[direction]++;
			pthread_mutex_unlock(&(prev->lock));
			pthread_mutex_unlock(&(cur->lock));
			rcu_free(cur, rcu_data);
			return 1;
		}
		prev_succ = cur;
		succ = cur->child[1];
		next = succ->child[0];
		while (next != NULL) {
			prev_succ = succ;
			succ = next;
			next = next->child[0];
		}
		if (prev_succ != cur) {
			pthread_mutex_lock(&(prev_succ->lock));
			direction_succ = 0;
		} else
			direction_succ = 1;
		pthread_mutex_lock(&(succ->lock));
		if (validate(prev_succ, 0, succ, direction_succ) &&
		    validate(succ, succ->tag[0], NULL, 0)) {
			cur->marked = 1;
			new_node = rcu_new_node(succ->value);
			new_node->child[0] = cur->child[0];
			new_node->child[1] = cur->child[1];
			pthread_mutex_lock(&(new_node->lock));
			prev->child[direction] = new_node;
			urcu_synchronize(rcu_data);

			succ->marked = 1;
			if (prev_succ == cur) {
				new_node->child[1] = succ->child[1];
				if (new_node->child[1] == NULL)
					new_node->tag[1]++;
			} else {
				prev_succ->child[0] = succ->child[1];
				if (prev_succ->child[0] == NULL)
					prev_succ->tag[0]++;
			}
			pthread_mutex_unlock(&(prev->lock));
			pthread_mutex_unlock(&(new_node->lock));
			pthread_mutex_unlock(&(cur->lock));
			if (prev_succ != cur)
				pthread_mutex_unlock(&(prev_succ->lock));
			pthread_mutex_unlock(&(succ->lock));
			rcu_free(cur, rcu_data);
			rcu_free(succ, rcu_data);
			return 1;
		}
		pthread_mutex_unlock(&(prev->lock));
		pthread_mutex_unlock(&(cur->lock));
		if (prev_succ != cur)
			pthread_mutex_unlock(&(prev_succ->lock));
		pthread_mutex_unlock(&(succ->lock));
	}
}

int list_find(int key, pthread_data_t *data)
{
	rcu_tree_t *tree = (rcu_tree_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *cur;
	int val;

	rcu_reader_lock(rcu_data);
	cur = tree->root->child[0];
	while (cur != NULL) {
		val = cur->value;
		if (val > key)
			cur = cur->child[0];
		else if (val < key)
			cur = cur->child[0];
		else
			break;
	}
	rcu_reader_unlock(rcu_data);

	return (cur != NULL);
}
