#include <assert.h>

#include "benchmark_list.h"

typedef struct node {
	int value;
	struct node *next;
} node_t;

typedef struct rcu_list {
	pthread_spinlock_t spinlock;
	node_t *head;
} rcu_list_t;

#define URCU_MAX_FREE_PTRS (1000)

typedef struct rcu_pthread_data {
	volatile long time;
	int nr_threads;
	struct rcu_pthread_data **sync_data;
	unsigned long *sync_time;
	int f_size;
	void *free_ptrs[URCU_MAX_FREE_PTRS];
} rcu_pthread_data_t;

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

inline void rcu_writer_lock(rcu_list_t *list)
{
	pthread_spin_lock(&list->spinlock);
}

inline void rcu_writer_unlock(rcu_list_t *list)
{
	pthread_spin_unlock(&list->spinlock);
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
	rcu_list_t *list;
	node_t *node;
	int i;

	list = (rcu_list_t *)malloc(sizeof(rcu_list_t));
	if (list == NULL)
		return NULL;
	pthread_spin_init(&list->spinlock, PTHREAD_PROCESS_PRIVATE);
	list->head = (node_t *)malloc(sizeof(node_t));
	node = list->head;
	if (node == NULL)
		return NULL;
	node->value = INT_MIN;

	for (i = 0; i < value_range; i+= value_range / init_size) {
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
	rcu_list_t *l = (rcu_list_t *)list;
	// free l->head;

	pthread_spin_destroy(&l->spinlock);
}

int list_ins(int key, pthread_data_t *data)
{
	rcu_list_t *list = (rcu_list_t *)data->list;
	node_t *prev, *cur, *new_node;
	int ret, val;

	rcu_writer_lock(list);
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
	rcu_writer_unlock(list);

	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	rcu_list_t *list = (rcu_list_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *prev, *cur;
	int ret, val;

	rcu_writer_lock(list);
	for (prev = list->head, cur = prev->next; cur != NULL; prev = cur, cur = cur->next)
		if ((val = cur->value) >= key)
			break;
	ret = (val == key);
	if (ret)
		prev->next = cur->next;
	rcu_writer_unlock(list);

	if (ret)
		rcu_free(cur, rcu_data);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{
	rcu_list_t *list = (rcu_list_t *)data->list;
	rcu_pthread_data_t *rcu_data = (rcu_pthread_data_t *)data->ds_data;
	node_t *cur;
	int ret, val;

	rcu_reader_lock(rcu_data);
	for (cur = list->head->next; cur != NULL; cur = cur->next)
		if ((val = cur->value) >= key)
			break;
	ret = (val == key);
	rcu_reader_unlock(rcu_data);

	return ret;
}
