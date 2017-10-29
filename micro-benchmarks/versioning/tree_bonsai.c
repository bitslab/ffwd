#include <assert.h>

#include "benchmark_list.h"

typedef struct node {
	int value;
	struct node *left;
	struct node *right;
	unsigned int size;
} node_t;

typedef struct bonsai_tree {
	pthread_spinlock_t spinlock;
	node_t *root;
} bonsai_tree_t;

#define URCU_MAX_FREE_PTRS (1000)

#define URCU_FREE_PTRS_SIZE (2000)

typedef struct rcu_pthread_data {
	volatile long time;
	int nr_threads;
	struct rcu_pthread_data **sync_data;
	unsigned long *sync_time;
	int f_size;
	void *free_ptrs[URCU_FREE_PTRS_SIZE];
} rcu_pthread_data_t;

#define smp_wmb() __asm__ __volatile__("": : :"memory")

#define rcu_assign_pointer(p, v) \
	({ \
		if (!__builtin_constant_p(v) || \
		    ((v) != NULL)) \
			smp_wmb(); \
		(p) = (typeof(*v)*)(v); \
	})

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

inline void rcu_writer_lock(bonsai_tree_t *tree)
{
	pthread_spin_lock(&tree->spinlock);
}

inline void rcu_writer_unlock(bonsai_tree_t *tree)
{
	pthread_spin_unlock(&tree->spinlock);
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

void rcu_free(void *ptr, rcu_pthread_data_t *rcu_data)
{
	rcu_data->free_ptrs[rcu_data->f_size] = ptr;
	rcu_data->f_size++;

	assert(rcu_data->f_size <= URCU_FREE_PTRS_SIZE);
}

inline void rcu_maybe_free_ptrs(rcu_pthread_data_t *rcu_data)
{
	int i;

	if (rcu_data->f_size >= URCU_MAX_FREE_PTRS) {
		urcu_synchronize(rcu_data);

		for (i = 0; i < rcu_data->f_size; i++)
			free(rcu_data->free_ptrs[i]);

		rcu_data->f_size = 0;
	}
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

void list_global_exit(void *list)
{
	bonsai_tree_t *l = (bonsai_tree_t *)list;
	// free l->head;

	pthread_spin_destroy(&l->spinlock);
}

#define WEIGHT (4)
#define INPLACE (1)

#define bonsai_get_size(node) ((node != NULL) ? (node->size) : (0))

static node_t *bonsai_mknode(node_t *left, node_t *right, int key)
{
	node_t *node = (node_t *)malloc(sizeof(node_t));

	assert(node != NULL);
	node->left = left;
	node->right = right;
	node->size = bonsai_get_size(left) + bonsai_get_size(right) + 1;
	node->value = key;

	return node;
}

static node_t *bonsai_singleL(node_t *left, node_t *right, int key, rcu_pthread_data_t *rcu_data)
{
	node_t *res;

	res = bonsai_mknode(left, right->left, key);
	res = bonsai_mknode(res, right->right, right->value);
	rcu_free(right, rcu_data);

	return res;
}

static node_t *bonsai_doubleL(node_t *left, node_t *right, int key, rcu_pthread_data_t *rcu_data)
{
	node_t *new_left, *new_right, *res;

	new_left = bonsai_mknode(left, right->left->left, key);
	new_right = bonsai_mknode(right->left->right, right->right, right->value);
	res = bonsai_mknode(new_left, new_right, right->left->value);
	rcu_free(right->left, rcu_data);
	rcu_free(right, rcu_data);

	return res;
}

static node_t *bonsai_singleR(node_t *left, node_t *right, int key, rcu_pthread_data_t *rcu_data)
{
	node_t *res;

	res = bonsai_mknode(left->right, right, key);
	res = bonsai_mknode(left->left, res, left->value);
	rcu_free(left, rcu_data);

	return res;
}

static node_t *bonsai_doubleR(node_t *left, node_t *right, int key, rcu_pthread_data_t *rcu_data)
{
	node_t *new_left, *new_right, *res;

	new_left = bonsai_mknode(left->left, left->right->left, left->value);
	new_right = bonsai_mknode(left->right->right, right, key);
	res = bonsai_mknode(new_left, new_right, left->right->value);
	rcu_free(left->right, rcu_data);
	rcu_free(left, rcu_data);

	return res;
}

static node_t *bonsai_mkbalancedL(node_t *left, node_t *right, int key, rcu_pthread_data_t *rcu_data)
{
	unsigned int rln, rrn;

	rln = bonsai_get_size(right->left);
	rrn = bonsai_get_size(right->right);
	if (rln < rrn)
		return bonsai_singleL(left, right, key, rcu_data);
	return bonsai_doubleL(left, right, key, rcu_data);
}

static node_t *bonsai_mkbalancedR(node_t *left, node_t *right, int key, rcu_pthread_data_t *rcu_data)
{
	unsigned int lln, lrn;

	lln = bonsai_get_size(left->left);
	lrn = bonsai_get_size(left->right);
	if (lrn < lln)
		return bonsai_singleR(left, right, key, rcu_data);
	return bonsai_doubleR(left, right, key, rcu_data);
}

static node_t *
bonsai_mkbalanced(node_t *cur, node_t *left, node_t *right,
                  int replace, int inplace, rcu_pthread_data_t *rcu_data)
{
	unsigned int ln, rn;
	int key;
	node_t *res;

	ln = bonsai_get_size(left);
	rn = bonsai_get_size(right);
	key = cur->value;
	if (ln + rn < 2)
		goto balanced;
	if (rn > WEIGHT * ln)
		res = bonsai_mkbalancedL(left, right, key, rcu_data);
	else if  (ln > WEIGHT * rn)
		res = bonsai_mkbalancedR(left, right, key, rcu_data);
	else
		goto balanced;

	rcu_free(cur, rcu_data);
	return res;

balanced:
	if (inplace) {
		if (replace == 0) {
			assert(cur->right == right);
			smp_wmb();
			cur->left = left;
		} else {
			assert(cur->left == left);
			smp_wmb();
			cur->right = right;
		}
		cur->size = bonsai_get_size(left) + bonsai_get_size(right) + 1;
		return cur;
	} else {
		res = bonsai_mknode(left, right, key);
		rcu_free(cur, rcu_data);
		return res;
	}
}

static node_t *bonsai_insert(node_t *node, int key, int *rv, rcu_pthread_data_t *rcu_data)
{
	node_t *child;

	if (!node) {
		*rv = 1;
		return bonsai_mknode(NULL, NULL, key);
	}

	if (key < node->value) {
		child = bonsai_insert(node->left, key, rv, rcu_data);
		return bonsai_mkbalanced(node, child, node->right, 0, INPLACE, rcu_data);
	}
	if (key > node->value) {
		child = bonsai_insert(node->right, key, rv, rcu_data);
		return bonsai_mkbalanced(node, node->left, child, 1, INPLACE, rcu_data);
	}

	*rv = 0;
	return node;
}

static node_t *bonsai_delete_succ(node_t *node, node_t **succ, rcu_pthread_data_t *rcu_data)
{
	node_t *child, *left, *right;
	left = node->left;
	right = node->right;
	if (left == NULL) {
		*succ = node;
		return right;
	}
	child = bonsai_delete_succ(left, succ, rcu_data);
	return bonsai_mkbalanced(node, child, right, 0, !INPLACE, rcu_data);
}

static node_t *bonsai_delete(node_t *node, int key, node_t **deleted, rcu_pthread_data_t *rcu_data)
{
	node_t *left, *right, *child, *succ;

	if (!node) {
		*deleted = NULL;
		return NULL;
	}

	left = node->left;
	right = node->right;
	if (key < node->value) {
		child = bonsai_delete(left, key, deleted, rcu_data);
		return bonsai_mkbalanced(node, child, right, 0, INPLACE, rcu_data);
	}
	if (key > node->value) {
		child = bonsai_delete(right, key, deleted, rcu_data);
		return bonsai_mkbalanced(node, left, child, 1, INPLACE, rcu_data);
	}

	*deleted = node;
	rcu_free(node, rcu_data);
	if (!left)
		return right;
	if (!right)
		return left;
	right = bonsai_delete_succ(right, &succ, rcu_data);

	return bonsai_mkbalanced(succ, left, right, 1, !INPLACE, rcu_data);
}

int list_ins(int key, pthread_data_t *data)
{
	bonsai_tree_t *tree = (bonsai_tree_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *root;
	int ret = 0;

	rcu_writer_lock(tree);
	root = bonsai_insert(tree->root->left, key, &ret, rcu_data);
	rcu_assign_pointer(tree->root->left, root);
	rcu_writer_unlock(tree);

	rcu_maybe_free_ptrs(rcu_data);

	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	bonsai_tree_t *tree = (bonsai_tree_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *root, *deleted;

	rcu_writer_lock(tree);
	root = bonsai_delete(tree->root->left, key, &deleted, rcu_data);
	rcu_assign_pointer(tree->root->left, root);
	rcu_writer_unlock(tree);

	rcu_maybe_free_ptrs(rcu_data);

	return (deleted != NULL);
}

int list_find(int key, pthread_data_t *data)
{
	bonsai_tree_t *tree = (bonsai_tree_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *cur;
	int ret, val;

	rcu_reader_lock(rcu_data);
	cur = tree->root->left;
	while (cur != NULL) {
		val = cur->value;
		if (val > key)
			cur = cur->left;
		else if (val < key)
			cur = cur->right;
		else
			break;
	}
	ret = (cur != NULL);
	rcu_reader_unlock(rcu_data);

	return ret;
}

void *list_global_init(int init_size, int value_range)
{
	bonsai_tree_t *tree;
	rcu_pthread_data_t *rcu_data;
	node_t *root;
	int i, j, key, ret;

	rcu_data = (rcu_pthread_data_t *)malloc(sizeof(rcu_pthread_data_t));
	if (rcu_data == NULL)
		return NULL;
	rcu_data->f_size = 0;

	tree = (bonsai_tree_t *)malloc(sizeof(bonsai_tree_t));
	if (tree == NULL)
		return NULL;
	pthread_spin_init(&tree->spinlock, PTHREAD_PROCESS_PRIVATE);
	tree->root = bonsai_mknode(NULL, NULL, INT_MAX);
	if (tree->root == NULL)
		return NULL;

	i = 0;
	while (i < init_size) {
		key = rand() % value_range;
		root = bonsai_insert(tree->root->left, key, &ret, rcu_data);
		tree->root->left = root;
		for (j = 0; j < rcu_data->f_size; j++)
			free(rcu_data->free_ptrs[j]);
		rcu_data->f_size = 0;
		if (ret)
			i++;
	}

	return tree;
}

