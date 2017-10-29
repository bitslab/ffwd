#include <assert.h>

#include "benchmark_list.h"
#include "qsbr.h"
#include "util.h"

typedef struct vlist_slot {
	unsigned long epoch;
	struct node *next;
	struct vlist_slot *slot_next;
	struct vlist_record *rec;
} vlist_slot_t;

#define VLIST_ENTRIES_PER_TASK 2

typedef struct vlist_record {
	unsigned long epoch;
	struct vlist_record *rec_next;
	int count;
	struct node *nodes[VLIST_ENTRIES_PER_TASK];
	struct vlist_slot *slots[VLIST_ENTRIES_PER_TASK];
} vlist_record_t;

typedef struct node {
	int value;
	vlist_slot_t *slots;
} node_t;

typedef struct vlist_list {
	node_t *head;
} vlist_list_t;

typedef struct vlist_pthread_data {
	vlist_record_t *rec;
	unsigned long epoch;
	vlist_record_t *new_rec;
	unsigned long count;
	qsbr_pthread_data_t *qsbr_data;
} vlist_pthread_data_t;

#define INDIRECT_EPOCH 0
#define INACTIVE_EPOCH 1
#define STARTING_EPOCH 2

static volatile vlist_record_t *g_committed_rec = NULL;

#define QSBR_PERIOD 100
static inline void vlist_maybe_quiescent(vlist_pthread_data_t *vlist_data)
{
	vlist_record_t *rec;

	vlist_data->count++;
	if (vlist_data->count % QSBR_PERIOD == 0) {
		do {
			rec = *(vlist_record_t **)&g_committed_rec;
		} while (rec->epoch < vlist_data->epoch &&
		         !CAS(&g_committed_rec, rec, vlist_data->rec));
		qsbr_quiescent_state(vlist_data->qsbr_data);
	}
}

static inline node_t *vlist_new_node()
{
	node_t *new_node = (node_t *)malloc(sizeof(node_t));
	vlist_slot_t *slot;

	if (new_node == NULL)
		return NULL;

	new_node->slots = slot = (vlist_slot_t *)malloc(sizeof(vlist_slot_t));
	if (slot == NULL)
		return NULL;
	slot->epoch = STARTING_EPOCH;
	slot->next = new_node;
	slot->slot_next = NULL;
	slot->rec = NULL;

	return new_node;
}

static inline unsigned long read_slot_epoch(vlist_slot_t *slot)
{
	if (slot->epoch == INDIRECT_EPOCH)
		return slot->rec->epoch;
	return slot->epoch;
}

static inline void vlist_free_later(void *ptr, vlist_pthread_data_t *vlist_data)
{
	qsbr_free_ptr(ptr, vlist_data->qsbr_data);
}

static inline void vlist_free_slots_later(vlist_slot_t *slot, vlist_pthread_data_t *vlist_data)
{
	struct vlist_slot *it_slot;

	for (it_slot = slot; it_slot != NULL; it_slot = it_slot->slot_next) {
		vlist_free_later(it_slot, vlist_data);
		if (read_slot_epoch(it_slot) >= STARTING_EPOCH)
			break;
	}
}

static inline void vlist_free_node_later(node_t *node, vlist_pthread_data_t *vlist_data)
{
	vlist_free_slots_later(node->slots, vlist_data);
	vlist_free_later(node, vlist_data);
}

static inline void add_slot(node_t *node, node_t *next, vlist_pthread_data_t *vlist_data)
{
	vlist_slot_t *old_slot, *slot;
	vlist_record_t *rec = vlist_data->new_rec;

	slot = (vlist_slot_t *)malloc(sizeof(vlist_slot_t));
	slot->epoch = INDIRECT_EPOCH;
	slot->rec = rec;
	slot->next = next;
	rec->nodes[rec->count] = node;
	rec->slots[rec->count] = slot;
	rec->count++;
	do {
		old_slot = node->slots;
		slot->slot_next = old_slot;
		MEMBARSTLD();
	} while (!CAS(&(node->slots), old_slot, slot));
}

static inline vlist_slot_t *read_slot(node_t *node, vlist_pthread_data_t *vlist_data)
{
	vlist_slot_t *it_slot;
	unsigned long slot_epoch, epoch;

	epoch = vlist_data->epoch;
	for (it_slot = node->slots; it_slot != NULL; it_slot = it_slot->slot_next) {
		slot_epoch = read_slot_epoch(it_slot);
		if (slot_epoch >= STARTING_EPOCH && slot_epoch <= epoch)
			break;
	}

	assert(it_slot->next != node);

	return it_slot;
}

static inline node_t *vlist_get_next(node_t *node, vlist_pthread_data_t *vlist_data)
{
	return read_slot(node, vlist_data)->next;
}

static inline void vlist_set_read_epoch(vlist_pthread_data_t *vlist_data)
{
	vlist_record_t *next = vlist_data->rec;
	vlist_record_t *rec = next;
	unsigned long epoch = next->epoch;

	while ((next = next->rec_next)) {
		rec = next;
		epoch++;
		if ((*(volatile unsigned long *)&next->epoch == INACTIVE_EPOCH))
			next->epoch = epoch;
	}
	vlist_data->rec = rec;
	vlist_data->epoch = epoch;
}

static inline void vlist_read_cs_enter(vlist_pthread_data_t *vlist_data)
{
	vlist_set_read_epoch(vlist_data);
}

static inline void vlist_read_cs_exit(vlist_pthread_data_t *vlist_data)
{
}

static void vlist_write_cs_enter(vlist_pthread_data_t *vlist_data)
{
	vlist_record_t *rec;

	vlist_data->new_rec = rec = (vlist_record_t *)malloc(sizeof(vlist_record_t));
	assert(rec != NULL);
	rec->epoch = INACTIVE_EPOCH;
	rec->count = 0;
	rec->rec_next = NULL;
	vlist_set_read_epoch(vlist_data);
}

static int vlist_write_cs_exit(vlist_pthread_data_t *vlist_data)
{
	vlist_record_t *new_rec, *it_rec;
	unsigned long epoch;
	int ret;

	new_rec = vlist_data->new_rec;
	ret = 0;
	if (new_rec->count == 0) {
		free(new_rec);
		goto out;
	}

	it_rec = vlist_data->rec;
	epoch = vlist_data->epoch + 1;
	while (1) {
		while (it_rec->rec_next) {
			it_rec = it_rec->rec_next;
			if (new_rec->nodes[0] == it_rec->nodes[0] ||
			    new_rec->nodes[1] == it_rec->nodes[0] ||
			    new_rec->nodes[0] == it_rec->nodes[1] ||
			    new_rec->nodes[1] == it_rec->nodes[1]) {
				ret = 1;
				goto out;
			}
			if ((*(volatile unsigned long *)&it_rec->epoch == INACTIVE_EPOCH))
				it_rec->epoch = epoch;
			epoch++;
		}

		if (CAS(&it_rec->rec_next, NULL, new_rec)) {
			new_rec->epoch = epoch;
			new_rec->slots[0]->epoch = epoch;
			new_rec->slots[1]->epoch = epoch;
			vlist_free_slots_later(new_rec->slots[0]->slot_next, vlist_data);
			vlist_free_slots_later(new_rec->slots[1]->slot_next, vlist_data);
			vlist_free_later(it_rec, vlist_data);
			vlist_data->rec = new_rec;
			vlist_data->epoch = epoch;
			break;
		}
	}

out:
	if (ret) {
		new_rec->slots[0]->epoch = INACTIVE_EPOCH;
		new_rec->slots[1]->epoch = INACTIVE_EPOCH;
		vlist_free_later(new_rec, vlist_data);
		vlist_data->new_rec = new_rec = (vlist_record_t *)malloc(sizeof(vlist_record_t));
		assert(new_rec != NULL);
		new_rec->epoch = INACTIVE_EPOCH;
		new_rec->count = 0;
		new_rec->rec_next = NULL;
		vlist_set_read_epoch(vlist_data);
	} else {
		vlist_data->new_rec = NULL;
	}

	return ret;
}

pthread_data_t *alloc_pthread_data(void)
{
	pthread_data_t *d;
	size_t pthread_size, vlist_size, qsbr_size;

	pthread_size = sizeof(pthread_data_t);
	pthread_size = CACHE_ALIGN_SIZE(pthread_size);
	vlist_size = sizeof(vlist_pthread_data_t);
	vlist_size = CACHE_ALIGN_SIZE(vlist_size);
	qsbr_size = sizeof(qsbr_pthread_data_t);
	qsbr_size = CACHE_ALIGN_SIZE(qsbr_size);

	d = (pthread_data_t *)malloc(pthread_size + vlist_size + qsbr_size);
	if (d != NULL) {
		d->ds_data = ((void *)d) + pthread_size;
		((vlist_pthread_data_t *)d->ds_data)->qsbr_data = ((void *)d) + pthread_size + vlist_size;
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
	vlist_list_t *list;
	node_t *node;
	vlist_record_t *rec;
	vlist_slot_t *slot;
	int i;

	rec = (vlist_record_t *)malloc(sizeof(vlist_record_t));
	if (rec == NULL)
		return NULL;
	rec->epoch = STARTING_EPOCH;
	rec->rec_next = NULL;
	rec->count = 0;
	g_committed_rec = rec;

	list = (vlist_list_t *)malloc(sizeof(vlist_list_t));
	if (list == NULL)
		return NULL;
	list->head = (node_t *)malloc(sizeof(node_t));
	node = list->head;
	if (node == NULL)
		return NULL;
	node->value = INT_MIN;
	node->slots = (vlist_slot_t *)malloc(sizeof(vlist_slot_t));
	slot = node->slots;
	if (slot == NULL)
		return NULL;
	slot->epoch = STARTING_EPOCH;
	slot->slot_next = NULL;
	slot->rec = NULL;

	for (i = 0; i < value_range; i += value_range / init_size) {
		slot->next = (node_t *)malloc(sizeof(node_t));
		if (slot->next == NULL)
			return NULL;
		node = slot->next;
		node->value = i;
		node->slots = (vlist_slot_t *)malloc(sizeof(vlist_slot_t));
		slot = node->slots;
		if (slot == NULL)
			return NULL;
		slot->epoch = STARTING_EPOCH;
		slot->slot_next = NULL;
		slot->rec = NULL;
	}

	slot->next = (node_t *)malloc(sizeof(node_t));
	if (slot->next == NULL)
		return NULL;
	node = slot->next;
	node->value = INT_MAX;
	node->slots = (vlist_slot_t *)malloc(sizeof(vlist_slot_t));
	slot = node->slots;
	if (slot == NULL)
		return NULL;
	slot->epoch = STARTING_EPOCH;
	slot->slot_next = NULL;
	slot->rec = NULL;
	slot->next = NULL;

	qsbr_init();

	return list;
}

int list_thread_init(pthread_data_t *data, pthread_data_t **sync_data, int nr_threads)
{
	vlist_pthread_data_t *vlist_data = (vlist_pthread_data_t *)data->ds_data;

	vlist_data->rec = *(struct vlist_record **)&g_committed_rec;
	vlist_data->epoch = vlist_data->rec->epoch;
	vlist_data->new_rec = NULL;

	vlist_data->count = 0;
	qsbr_pthread_init(vlist_data->qsbr_data);

	return 0;
}

void list_global_exit(void *list)
{
	// free l->head
}

int list_ins(int key, pthread_data_t *data)
{
	vlist_list_t *list = (vlist_list_t *)data->list;
	vlist_pthread_data_t *vlist_data = (vlist_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *new_node;
	int ret, val;

	new_node = vlist_new_node();
	assert(new_node != NULL);
	new_node->value = key;

	vlist_write_cs_enter(vlist_data);
	do {
		data->nr_txn++;
		prev = list->head;
		while (1) {
			cur = vlist_get_next(prev, vlist_data);
			val = cur->value;
			if (val >= key)
				break;
			prev = cur;
		}
		ret = (val != key);
		if (ret) {
			add_slot(prev, new_node, vlist_data);
			add_slot(new_node, cur, vlist_data);
		}
	} while (vlist_write_cs_exit(vlist_data));

	if (ret == 0)
		vlist_free_node_later(new_node, vlist_data);

	vlist_maybe_quiescent(vlist_data);

	return ret;
}

int list_del(int key, pthread_data_t *data)
{
	vlist_list_t *list = (vlist_list_t *)data->list;
	vlist_pthread_data_t *vlist_data = (vlist_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *next;
	int ret, val;

	vlist_write_cs_enter(vlist_data);
	do {
		data->nr_txn++;
		prev = list->head;
		while (1) {
			cur = vlist_get_next(prev, vlist_data);
			val = cur->value;
			if (val >= key)
				break;
			prev = cur;
		}
		ret = (val == key);
		if (ret) {
			next = vlist_get_next(cur, vlist_data);
			add_slot(prev, next, vlist_data);
			add_slot(cur, cur, vlist_data);
		}
	} while (vlist_write_cs_exit(vlist_data));

	if (ret == 1)
		vlist_free_node_later(cur, vlist_data);

	vlist_maybe_quiescent(vlist_data);

	return ret;
}

int list_find(int key, pthread_data_t *data)
{
	vlist_list_t *list = (vlist_list_t *)data->list;
	vlist_pthread_data_t *vlist_data = (vlist_pthread_data_t *)data->ds_data;
	node_t *cur;
	int ret, val;

	vlist_read_cs_enter(vlist_data);
	data->nr_txn++;

	cur = list->head;
	while (1) {
		cur = vlist_get_next(cur, vlist_data);
		val = cur->value;
		if (val >= key)
			break;
	}

	vlist_read_cs_exit(vlist_data);

	ret = (val == key);

	vlist_maybe_quiescent(vlist_data);

	return ret;
}
