#include <assert.h>

#include "benchmark_list.h"
#include "qsbr.h"
#include "util.h"

typedef struct vtree_slot {
	unsigned long epoch;
	struct node *child[2];
	struct vtree_slot *slot_next;
	struct vtree_record *rec;
	char padding[216];
} vtree_slot_t;

#define VTREE_ENTRIES_PER_TASK 4

typedef struct vtree_record {
	unsigned long epoch;
	struct vtree_record *rec_next;
	int count;
	struct node *nodes[VTREE_ENTRIES_PER_TASK];
	struct vtree_slot *slots[VTREE_ENTRIES_PER_TASK];
} vtree_record_t;

typedef struct node {
	int value;
	vtree_slot_t *slots;
	char padding[240];
} node_t;

typedef struct vtree_tree {
	node_t *root;
} vtree_tree_t;

typedef struct vtree_pthread_data {
	vtree_record_t *rec;
	unsigned long epoch;
	vtree_record_t *new_rec;
	unsigned long count;
	qsbr_pthread_data_t *qsbr_data;
} vtree_pthread_data_t;

#define INDIRECT_EPOCH 0
#define INACTIVE_EPOCH 1
#define STARTING_EPOCH 2

static volatile vtree_record_t *g_committed_rec = NULL;

#define QSBR_PERIOD 1000
static inline void vtree_maybe_quiescent(vtree_pthread_data_t *vtree_data)
{
	vtree_record_t *rec;

	vtree_data->count++;
	if (vtree_data->count % QSBR_PERIOD == 0) {
		do {
			rec = *(vtree_record_t **)&g_committed_rec;
		} while (rec->epoch < vtree_data->epoch &&
		         !CAS(&g_committed_rec, rec, vtree_data->rec));
		qsbr_quiescent_state(vtree_data->qsbr_data);
	}
}

static node_t *vtree_new_node(int key)
{
	node_t *new_node = (node_t *)malloc(sizeof(node_t));
	vtree_slot_t *slot;

	if (new_node == NULL)
		return NULL;

	new_node->slots = slot = (vtree_slot_t *)malloc(sizeof(vtree_slot_t));
	if (slot == NULL)
		return NULL;
	new_node->value = key;
	slot->epoch = STARTING_EPOCH;
	slot->child[0] = NULL;
	slot->child[1] = NULL;
	slot->slot_next = NULL;
	slot->rec = NULL;

	return new_node;
}

static inline unsigned long read_slot_epoch(vtree_slot_t *slot)
{
	if (slot->epoch == INDIRECT_EPOCH)
		return slot->rec->epoch;
	return slot->epoch;
}

static inline void vtree_free_later(void *ptr, vtree_pthread_data_t *vtree_data)
{
	qsbr_free_ptr(ptr, vtree_data->qsbr_data);
}

static inline void vtree_free_slots_later(vtree_slot_t *slot, vtree_pthread_data_t *vtree_data)
{
	struct vtree_slot *it_slot;

	for (it_slot = slot; it_slot != NULL; it_slot = it_slot->slot_next) {
		vtree_free_later(it_slot, vtree_data);
		if (read_slot_epoch(it_slot) >= STARTING_EPOCH)
			break;
	}
}

static inline void vtree_free_node_later(node_t *node, vtree_pthread_data_t *vtree_data)
{
	vtree_free_slots_later(node->slots, vtree_data);
	vtree_free_later(node, vtree_data);
}

static inline void add_slot(node_t *node, node_t *left_child, node_t *right_child,
                            vtree_pthread_data_t *vtree_data)
{
	vtree_slot_t *old_slot, *slot;
	vtree_record_t *rec = vtree_data->new_rec;

	slot = (vtree_slot_t *)malloc(sizeof(vtree_slot_t));
	slot->epoch = INDIRECT_EPOCH;
	slot->rec = rec;
	slot->child[0] = left_child;
	slot->child[1] = right_child;
	rec->nodes[rec->count] = node;
	rec->slots[rec->count] = slot;
	rec->count++;
	do {
		old_slot = node->slots;
		slot->slot_next = old_slot;
		MEMBARSTLD();
	} while (!CAS(&(node->slots), old_slot, slot));
}

static inline vtree_slot_t *read_slot(node_t *node, vtree_pthread_data_t *vtree_data)
{
	vtree_slot_t *it_slot;
	unsigned long slot_epoch, epoch;

	epoch = vtree_data->epoch;
	for (it_slot = node->slots; it_slot != NULL; it_slot = it_slot->slot_next) {
		slot_epoch = read_slot_epoch(it_slot);
		if (slot_epoch >= STARTING_EPOCH && slot_epoch <= epoch)
			break;
	}

	return it_slot;
}

static inline void vtree_set_read_epoch(vtree_pthread_data_t *vtree_data)
{
	vtree_record_t *next = vtree_data->rec;
	vtree_record_t *rec = next;
	unsigned long epoch = next->epoch;

	while ((next = next->rec_next)) {
		rec = next;
		epoch++;
		if ((*(volatile unsigned long *)&next->epoch == INACTIVE_EPOCH))
			next->epoch = epoch;
	}
	vtree_data->rec = rec;
	vtree_data->epoch = epoch;
}

static inline void vtree_read_cs_enter(vtree_pthread_data_t *vtree_data)
{
	vtree_set_read_epoch(vtree_data);
}

static inline void vtree_read_cs_exit(vtree_pthread_data_t *vtree_data)
{
}

static void vtree_write_cs_enter(vtree_pthread_data_t *vtree_data)
{
	vtree_record_t *rec;

	vtree_data->new_rec = rec = (vtree_record_t *)malloc(sizeof(vtree_record_t));
	assert(rec != NULL);
	rec->epoch = INACTIVE_EPOCH;
	rec->count = 0;
	rec->rec_next = NULL;
	vtree_set_read_epoch(vtree_data);
}

static int vtree_write_cs_exit(vtree_pthread_data_t *vtree_data)
{
	vtree_record_t *new_rec, *it_rec;
	unsigned long epoch;
	int ret, i, j;

	new_rec = vtree_data->new_rec;
	ret = 0;
	if (new_rec->count == 0) {
		free(new_rec);
		goto out;
	}

	it_rec = vtree_data->rec;
	epoch = vtree_data->epoch + 1;
	while (1) {
		while (it_rec->rec_next) {
			it_rec = it_rec->rec_next;
			for (i = 0; i < new_rec->count; i++)
				for (j = 0; j < it_rec->count; j++)
					if (new_rec->nodes[i] == it_rec->nodes[j]) {
						ret = 1;
						goto out;
					}
			if ((*(volatile unsigned long *)&it_rec->epoch == INACTIVE_EPOCH))
				it_rec->epoch = epoch;
			epoch++;
		}

		if (CAS(&it_rec->rec_next, NULL, new_rec)) {
			new_rec->epoch = epoch;
			for (i = 0; i < new_rec->count; i++) {
				new_rec->slots[i]->epoch = epoch;
				vtree_free_slots_later(new_rec->slots[i]->slot_next, vtree_data);
			}
			vtree_data->rec = new_rec;
			vtree_data->epoch = epoch;
			break;
		}
	}

out:
	if (ret) {
		for (i = 0; i < new_rec->count; i++)
			new_rec->slots[i]->epoch = INACTIVE_EPOCH;
		vtree_free_later(new_rec, vtree_data);
		vtree_data->new_rec = new_rec = (vtree_record_t *)malloc(sizeof(vtree_record_t));
		assert(new_rec != NULL);
		new_rec->epoch = INACTIVE_EPOCH;
		new_rec->count = 0;
		new_rec->rec_next = NULL;
		vtree_set_read_epoch(vtree_data);
	} else {
		vtree_data->new_rec = NULL;
	}

	return ret;
}

pthread_data_t *alloc_pthread_data(void)
{
	pthread_data_t *d;
	size_t pthread_size, vtree_size, qsbr_size;

	pthread_size = sizeof(pthread_data_t);
	pthread_size = CACHE_ALIGN_SIZE(pthread_size);
	vtree_size = sizeof(vtree_pthread_data_t);
	vtree_size = CACHE_ALIGN_SIZE(vtree_size);
	qsbr_size = sizeof(qsbr_pthread_data_t);
	qsbr_size = CACHE_ALIGN_SIZE(qsbr_size);

	d = (pthread_data_t *)malloc(pthread_size + vtree_size + qsbr_size);
	if (d != NULL) {
		d->ds_data = ((void *)d) + pthread_size;
		((vtree_pthread_data_t *)d->ds_data)->qsbr_data = ((void *)d) + pthread_size + vtree_size;
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
	vtree_tree_t *tree;
	node_t *prev, *cur, *new_node;
	vtree_record_t *rec;
	int i, key, val, direction;

	rec = (vtree_record_t *)malloc(sizeof(vtree_record_t));
	if (rec == NULL)
		return NULL;
	rec->epoch = STARTING_EPOCH;
	rec->rec_next = NULL;
	rec->count = 0;
	g_committed_rec = rec;

	tree = (vtree_tree_t *)malloc(sizeof(vtree_tree_t));
	if (tree == NULL)
		return NULL;
	tree->root = vtree_new_node(INT_MAX);
	if (tree->root == NULL)
		return NULL;

	i = 0;
	while (i < init_size) {
		key = rand() % value_range;

		prev = tree->root;
		cur = prev->slots->child[0];
		direction = 0;
		while (cur != NULL) {
			prev = cur;
			val = cur->value;
			if (val > key) {
				direction = 0;
				cur = cur->slots->child[0];
			} else if (val < key) {
				direction = 1;
				cur = cur->slots->child[1];
			} else
				break;
		}
		if (cur != NULL)
			continue;
		new_node = vtree_new_node(key);
		if (new_node == NULL)
			return NULL;
		prev->slots->child[direction] = new_node;
		i++;
	}

	qsbr_init();

	return tree;
}

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads)
{
	vtree_pthread_data_t *vtree_data = (vtree_pthread_data_t *)data->ds_data;

	vtree_data->rec = *(struct vtree_record **)&g_committed_rec;
	vtree_data->epoch = vtree_data->rec->epoch;
	vtree_data->new_rec = NULL;

	vtree_data->count = 0;
	qsbr_pthread_init(vtree_data->qsbr_data);

	return 0;
}

void list_global_exit(void *list)
{
	// free l->head
}

int list_ins(int key, pthread_data_t *data)
{
	vtree_tree_t *tree = (vtree_tree_t *)data->list;
	vtree_pthread_data_t *vtree_data = (vtree_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *new_node;
	vtree_slot_t *slot;
	int direction, ret, val;

	new_node = vtree_new_node(key);
	assert(new_node != NULL);

	vtree_write_cs_enter(vtree_data);
	do {
		data->nr_txn++;
		prev = tree->root;
		slot = read_slot(prev, vtree_data);
		cur = slot->child[0];
		direction = 0;
		while (cur != NULL) {
			val = cur->value;
			if (val > key) {
				slot = read_slot(cur, vtree_data);
				direction = 0;
				prev = cur;
				cur = slot->child[0];
			} else if (val < key) {
				slot = read_slot(cur, vtree_data);
				direction = 1;
				prev = cur;
				cur = slot->child[1];
			} else
				break;
		}
		ret = (cur == NULL);
		if (ret) {
			if (direction == 0)
				add_slot(prev, new_node, slot->child[1], vtree_data);
			else
				add_slot(prev, slot->child[0], new_node, vtree_data);
		}
	} while (vtree_write_cs_exit(vtree_data));

	if (ret == 0)
		vtree_free_node_later(new_node, vtree_data);

	vtree_maybe_quiescent(vtree_data);

	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	vtree_tree_t *tree = (vtree_tree_t *)data->list;
	vtree_pthread_data_t *vtree_data = (vtree_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *prev_succ, *succ, *next;
	vtree_slot_t *slot, *cur_slot, *succ_slot, *prev_succ_slot;
	int direction, ret, val;

	vtree_write_cs_enter(vtree_data);
	do {
		data->nr_txn++;
		prev = tree->root;
		slot = read_slot(prev, vtree_data);
		cur = slot->child[0];
		direction = 0;
		while (cur != NULL) {
			val = cur->value;
			if (val > key) {
				slot = read_slot(cur, vtree_data);
				direction = 0;
				prev = cur;
				cur = slot->child[0];
			} else if (val < key) {
				slot = read_slot(cur, vtree_data);
				direction = 1;
				prev = cur;
				cur = slot->child[1];
			} else
				break;
		}

		ret = (cur != NULL);
		if (!ret)
			goto out;

		cur_slot = read_slot(cur, vtree_data);
		if (cur_slot->child[0] == NULL) {
			if (direction == 0)
				add_slot(prev, cur_slot->child[1], slot->child[1], vtree_data);
			else
				add_slot(prev, slot->child[0], cur_slot->child[1], vtree_data);
			add_slot(cur, NULL, NULL, vtree_data);
			goto out;
		}
		if (cur_slot->child[1] == NULL) {
			if (direction == 0)
				add_slot(prev, cur_slot->child[0], slot->child[1], vtree_data);
			else
				add_slot(prev, slot->child[0], cur_slot->child[0], vtree_data);
			add_slot(cur, NULL, NULL, vtree_data);
			goto out;
		}

		prev_succ = cur;
		prev_succ_slot = cur_slot;
		succ = cur_slot->child[1];
		succ_slot = read_slot(succ, vtree_data);
		next = succ_slot->child[0];
		while (next != NULL) {
			prev_succ = succ;
			succ = next;
			prev_succ_slot = succ_slot;
			succ_slot = read_slot(succ, vtree_data);
			next = succ_slot->child[0];
		}

		if (prev_succ == cur) {
			add_slot(succ, cur_slot->child[0], succ_slot->child[1], vtree_data);
		} else {
			add_slot(prev_succ, succ_slot->child[1], prev_succ_slot->child[1],
			         vtree_data);
			add_slot(succ, cur_slot->child[0], cur_slot->child[1], vtree_data);
		}
		if (direction == 0)
			add_slot(prev, succ, slot->child[1], vtree_data);
		else
			add_slot(prev, slot->child[0], succ, vtree_data);
		add_slot(cur, cur, cur, vtree_data);
out:
		;
	} while (vtree_write_cs_exit(vtree_data));

	if (ret == 1)
		vtree_free_node_later(cur, vtree_data);

	vtree_maybe_quiescent(vtree_data);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{
	vtree_tree_t *tree = (vtree_tree_t *)data->list;
	vtree_pthread_data_t *vtree_data = (vtree_pthread_data_t *)data->ds_data;
	node_t *cur;
	int ret, val;

	vtree_read_cs_enter(vtree_data);
	data->nr_txn++;

	cur = tree->root;
	cur = read_slot(cur, vtree_data)->child[0];
	while (cur != NULL) {
		val = cur->value;
		if (val > key)
			cur = read_slot(cur, vtree_data)->child[0];
		else if (val < key)
			cur = read_slot(cur, vtree_data)->child[1];
		else
			break;
	}

	ret = (cur != NULL);

	vtree_read_cs_exit(vtree_data);

	vtree_maybe_quiescent(vtree_data);

	return ret;
}
