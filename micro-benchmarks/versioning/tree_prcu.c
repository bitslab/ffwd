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

typedef struct prcu_tree {
	node_t *root;
} prcu_tree_t;

typedef struct predicate_info {
	int min_key;
	int max_key;
	int max;
} predicate_info_t;

static int max_key_in_range;

#ifdef PRCU_EER
#define PRCU_TABLE_SIZE (1)
#elif defined PRCU_D
#define PRCU_TABLE_SIZE (1023)
typedef struct prcu_table {
	int which;
	volatile long count[2];
	pthread_mutex_t lock;
	char padding[128];
} prcu_table_t;

static prcu_table_t *prcu_table;
#elif defined PRCU_DEER
#define PRCU_TABLE_SIZE (16)
typedef struct prcu_table {
	volatile unsigned long time;
	char p[184];
} prcu_table_t;
#endif

#define URCU_MAX_FREE_PTRS (1000)

typedef struct prcu_pthread_data {
#ifdef PRCU_EER
	volatile unsigned long time;
	volatile long key;
	int nr_threads;
	struct prcu_pthread_data **sync_data;
#elif defined PRCU_D
	int which;
#elif defined PRCU_DEER
	prcu_table_t *prcu_table;
	int nr_threads;
	struct prcu_pthread_data **sync_data;
#endif
	int f_size;
	void *free_ptrs[URCU_MAX_FREE_PTRS];
} prcu_pthread_data_t;

static node_t *prcu_new_node(int key)
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

int pred_hash(int key)
{
	int num_buckets = PRCU_TABLE_SIZE;
	int result, num_elements_per_bucket, overflow, threshold;

	if (num_buckets > max_key_in_range)
		return key;
	num_elements_per_bucket = max_key_in_range / num_buckets;
	overflow = max_key_in_range - (num_buckets * num_elements_per_bucket);
	if (overflow == 0)
		result = (key / num_elements_per_bucket);
	else {
		threshold = overflow * (num_elements_per_bucket + 1);
		if (key < threshold)
			result = (key / (num_elements_per_bucket + 1));
		else
			result = overflow + ((key - (threshold)) / num_elements_per_bucket);
	}
	return result;
}

int pred(predicate_info_t *info, int key)
{
	return (info->min_key < key && info->max_key >= key);
}

int pred_next(predicate_info_t *info, int cur_bucket)
{
	int max_bucket = pred_hash(info->max_key);
	if (cur_bucket < max_bucket)
		return cur_bucket + 1;
	else
		return -1;
}

static inline uint64_t read_tsc(void)
{
	unsigned upper, lower;
	asm volatile("rdtsc"
	             : "=a" (lower), "=d" (upper)
	             :
	             : "memory");
	return ((uint64_t) lower) | (((uint64_t) upper) << 32);
}

static inline void set_bit(int nr, volatile unsigned long *addr)
{
    asm("btsl %1,%0" : "+m" (*addr) : "Ir" (nr));
}

void prcu_enter(int key, prcu_pthread_data_t *prcu_data)
{
#ifdef PRCU_EER
	prcu_data->key = key;
	__sync_lock_test_and_set(&prcu_data->time, read_tsc() << 1);
#elif defined PRCU_D
	int j = pred_hash(key);
	prcu_data->which = prcu_table[j].which;
	__sync_fetch_and_add(&prcu_table[j].count[prcu_data->which], 1);
#elif defined PRCU_DEER
	int j = pred_hash(key);
	__sync_lock_test_and_set(&prcu_data->prcu_table[j].time, read_tsc() << 1);
#else
	assert(0);
#endif
}

void prcu_exit(int key, prcu_pthread_data_t *prcu_data)
{
#ifdef PRCU_EER
	set_bit(0, &prcu_data->time);
#elif defined PRCU_D
	int j = pred_hash(key);
	__sync_fetch_and_add(&prcu_table[j].count[prcu_data->which], -1);
#elif defined PRCU_DEER
	int j = pred_hash(key);
	set_bit(0, &prcu_data->prcu_table[j].time);
#else
	assert(0);
#endif
}

#ifdef PRCU_D
static inline void prcu_wait(prcu_table_t *node)
{
	int i, which;

	for (i = 0; i < 10000; i++)
		if (node->count[0] == 0 && node->count[1] == 0)
			return;

	pthread_mutex_lock(&node->lock);
	which = node->which;
	node->which = !which;
	asm volatile("":::"memory");
	while (node->count[which] != 0) ;

	node->which = which;
	asm volatile("":::"memory");
	while (node->count[!which] != 0) ;

	pthread_mutex_unlock(&node->lock);
}
#elif defined PRCU_DEER
static inline void prcu_wait(int j, uint64_t now, prcu_pthread_data_t *prcu_data)
{
	int i;
	unsigned long t;

	for (i = 0; i < prcu_data->nr_threads; i++) {
		while (1) {
			t = prcu_data->sync_data[i]->prcu_table[j].time;
			if (t & 1 || t > now)
				break;
		}
	}
}
#endif

void prcu_wait_for_readers(int min, predicate_info_t *pred_info, prcu_pthread_data_t *prcu_data)
{
#ifdef PRCU_EER
	uint64_t now;
	unsigned long t;
	int i;

	asm volatile("mfence" ::: "memory");

	now = read_tsc() << 1;
	for (i = 0; i < prcu_data->nr_threads; i++) {
		if (!pred(pred_info, prcu_data->sync_data[i]->key))
			continue;
		while (1) {
			t = prcu_data->sync_data[i]->time;
			if (t & 1 || t > now)
				break;
		}
	}
#elif defined PRCU_D
	int bucket;

	asm volatile("mfence" ::: "memory");

	bucket = min;
	while (bucket >= 0) {
		prcu_wait(&(prcu_table[bucket]));
		bucket = pred_next(pred_info, bucket);
	}
#elif defined PRCU_DEER
	uint64_t now;
	int bucket;

	asm volatile("mfence" ::: "memory");

	now = read_tsc() << 1;
	bucket = min;
	while (bucket >= 0) {
		prcu_wait(bucket, now, prcu_data);
		bucket = pred_next(pred_info, bucket);
	}
#else
	assert(0);
#endif
}

inline void rcu_free(void *ptr, prcu_pthread_data_t *prcu_data)
{
	int i;
	predicate_info_t pred_info;

	prcu_data->free_ptrs[prcu_data->f_size] = ptr;
	prcu_data->f_size++;

	if (prcu_data->f_size == URCU_MAX_FREE_PTRS) {
		pred_info.min_key = 0;
		pred_info.max_key = max_key_in_range - 1;
		pred_info.max = max_key_in_range;
		prcu_wait_for_readers(0, &pred_info, prcu_data);

		for (i = 0; i < URCU_MAX_FREE_PTRS; i++)
			free(prcu_data->free_ptrs[i]);

		prcu_data->f_size = 0;
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
	size_t pthread_size, prcu_size;

	pthread_size = sizeof(pthread_data_t);
	pthread_size = CACHE_ALIGN_SIZE(pthread_size);
	prcu_size = sizeof(prcu_pthread_data_t);
	prcu_size = CACHE_ALIGN_SIZE(prcu_size);

	d = (pthread_data_t *)malloc(pthread_size + prcu_size);
	if (d != NULL)
		d->ds_data = ((void *)d) + pthread_size;

	return d;
}

void free_pthread_data(pthread_data_t *d)
{
#if (defined PRCU_EER || defined PRCU_DEER)
	prcu_pthread_data_t *prcu_data = (prcu_pthread_data_t *)d->ds_data;

	free(prcu_data->sync_data);
#endif

	free(d);
}


void *list_global_init(int init_size, int value_range)
{
	prcu_tree_t *tree;
	node_t *prev, *cur, *new_node;
	int i, key, val, direction;

	tree = (prcu_tree_t *)malloc(sizeof(prcu_tree_t));
	if (tree == NULL)
		return NULL;
	tree->root = prcu_new_node(INT_MAX);

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
		new_node = prcu_new_node(key);
		if (new_node == NULL)
			return NULL;
		prev->child[direction] = new_node;
		i++;
	}

	max_key_in_range = value_range;

#ifdef PRCU_D
	prcu_table = malloc(sizeof(prcu_table_t) * PRCU_TABLE_SIZE);
	if (prcu_table == NULL)
		return NULL;
	for (i = 0; i < PRCU_TABLE_SIZE; i++) {
		prcu_table[i].count[0] = 0;
		prcu_table[i].count[1] = 0;
		prcu_table[i].which = 0;
		pthread_mutex_init(&(prcu_table[i].lock), NULL);
	}
#endif

	return tree;
}

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads)
{
#ifdef PRCU_EER
	int i;
	prcu_pthread_data_t *prcu_data = (prcu_pthread_data_t *)data->ds_data;

	prcu_data->time = 1;
	prcu_data->key = 0;
	prcu_data->nr_threads = nr_threads;
	if ((prcu_data->sync_data = malloc(nr_threads * sizeof(prcu_pthread_data_t *))) == NULL)
		return -1;
	for (i = 0; i < nr_threads; i++)
		prcu_data->sync_data[i] = sync_data[i]->ds_data;
#elif defined PRCU_D
	prcu_pthread_data_t *prcu_data = (prcu_pthread_data_t *)data->ds_data;

	prcu_data->which = 0;
#elif defined PRCU_DEER
	int i;
	prcu_pthread_data_t *prcu_data = (prcu_pthread_data_t *)data->ds_data;

	prcu_data->prcu_table = (prcu_table_t *)malloc(sizeof(prcu_table_t) * PRCU_TABLE_SIZE);
	if (prcu_data->prcu_table == NULL)
		return -1;
	for (i = 0; i < PRCU_TABLE_SIZE; i++)
		prcu_data->prcu_table[i].time = 1;

	prcu_data->nr_threads = nr_threads;
	if ((prcu_data->sync_data = malloc(nr_threads * sizeof(prcu_pthread_data_t *))) == NULL)
		return -1;
	for (i = 0; i < nr_threads; i++)
		prcu_data->sync_data[i] = sync_data[i]->ds_data;
#else
	assert(0);
#endif

	prcu_data->f_size = 0;

	return 0;
}

void list_global_exit(void *tree)
{

}

int list_ins(int key, pthread_data_t *data)
{
	prcu_tree_t *tree = (prcu_tree_t *)data->list;
	prcu_pthread_data_t *prcu_data = (prcu_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *new_node;
	int direction, tag, val;

	while (1) {
		prcu_enter(key, prcu_data);
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
		prcu_exit(key, prcu_data);
		if (cur != NULL)
			return 0;
		pthread_mutex_lock(&(prev->lock));
		if (validate(prev, tag, cur, direction)) {
			new_node = prcu_new_node(key);
			prev->child[direction] = new_node;
			pthread_mutex_unlock(&(prev->lock));
			return 1;
		}
		pthread_mutex_unlock(&(prev->lock));
	}
}

int list_del(int key, pthread_data_t *data)
{
	prcu_tree_t *tree = (prcu_tree_t *)data->list;
	prcu_pthread_data_t *prcu_data = (prcu_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *prev_succ, *succ, *next, *new_node;
	int direction, val, direction_succ;
	predicate_info_t pred_info;
	int min_bucket;

	while (1) {
		prcu_enter(key, prcu_data);
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
		prcu_exit(key, prcu_data);
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
			rcu_free(cur, prcu_data);
			return 1;
		}
		if (cur->child[1] == NULL) {
			cur->marked = 1;
			prev->child[direction] = cur->child[0];
			if (prev->child[direction] == NULL)
				prev->tag[direction]++;
			pthread_mutex_unlock(&(prev->lock));
			pthread_mutex_unlock(&(cur->lock));
			rcu_free(cur, prcu_data);
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
			new_node = prcu_new_node(succ->value);
			new_node->child[0] = cur->child[0];
			new_node->child[1] = cur->child[1];
			pthread_mutex_lock(&(new_node->lock));
			prev->child[direction] = new_node;
			pred_info.min_key = key;
			pred_info.max_key = succ->value;
			pred_info.max = max_key_in_range;
			min_bucket = pred_hash(key);
			prcu_wait_for_readers(min_bucket, &pred_info, prcu_data);

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
			rcu_free(cur, prcu_data);
			rcu_free(succ, prcu_data);
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
	prcu_tree_t *tree = (prcu_tree_t *)data->list;
	prcu_pthread_data_t *prcu_data = (prcu_pthread_data_t *)data->ds_data;
	node_t *cur;
	int val;

	prcu_enter(key, prcu_data);
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
	prcu_exit(key, prcu_data);

	return (cur != NULL);
}
