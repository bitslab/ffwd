#include <assert.h>

#include "benchmark_list.h"
#include "qsbr.h"
#include "util.h"

typedef struct vtree_slot {
	unsigned long epoch;
	struct node *child[2];
	// LSB for red (0)/black (1)
	struct node *parent;
	struct vtree_slot *slot_next;
	struct vtree_record *rec;
	char padding[208];
} vtree_slot_t;

#define VTREE_ENTRIES_PER_TASK 62

typedef struct vtree_record {
	unsigned long epoch;
	struct vtree_record *rec_next;
	int count;
	struct node *nodes[VTREE_ENTRIES_PER_TASK];
	struct vtree_slot *slots[VTREE_ENTRIES_PER_TASK];
	char padding[8];
} vtree_record_t;

typedef struct node {
	int value;
	vtree_slot_t *slots;
	char padding[240];
} node_t;

#define RB_RED		0
#define RB_BLACK	1

#define get_node_color(parent) \
		((unsigned long long)(parent) & 0x1)
#define get_node_parent(parent) \
		((node_t *)((unsigned long long)(parent) & (~0x1)))
#define set_node_black(parent) \
		((node_t *)((unsigned long long)(parent) | (0x1)))
#define set_node_red(parent) \
		((node_t *)((unsigned long long)(parent) & (~0x1)))
#define change_node_parent(old_parent, new_parent) \
		((node_t *)((unsigned long long)(new_parent) | get_node_color(old_parent)))

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
	slot->parent = NULL;
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

static struct vtree_slot *
add_slot(node_t *node, node_t *left, node_t *right, node_t *parent,
         vtree_pthread_data_t *vtree_data)
{
	vtree_slot_t *old_slot, *slot;
	vtree_record_t *rec = vtree_data->new_rec;

	slot = (vtree_slot_t *)malloc(sizeof(vtree_slot_t));
	assert(slot != NULL);
	slot->epoch = INDIRECT_EPOCH;
	slot->rec = rec;
	slot->child[0] = left;
	slot->child[1] = right;
	slot->parent = parent;
	assert(rec->count < VTREE_ENTRIES_PER_TASK);
	rec->nodes[rec->count] = node;
	rec->slots[rec->count] = slot;
	rec->count++;
	do {
		old_slot = node->slots;
		slot->slot_next = old_slot;
		MEMBARSTLD();
	} while (!CAS(&(node->slots), old_slot, slot));

	return slot;
}

static inline struct vtree_slot *
get_slot_from_rec_new(node_t *node, vtree_pthread_data_t *vtree_data)
{
	int i;
	struct vtree_record *rec = vtree_data->new_rec;

	for (i = rec->count - 1; i >= 0; i--)
		if (rec->nodes[i] == node)
			return rec->slots[i];

	return NULL;
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

static inline void
add_or_change_slot_parent(node_t *node, node_t *parent, vtree_pthread_data_t *vtree_data)
{
	vtree_slot_t *slot;

	slot = get_slot_from_rec_new(node, vtree_data);

	if (slot)
		slot->parent = change_node_parent(slot->parent, parent);
	else {
		slot = read_slot(node, vtree_data);
		add_slot(node, slot->child[0], slot->child[1],
		         change_node_parent(slot->parent, parent),
		         vtree_data);
	}
}

static inline void
add_or_change_slot_child(node_t *node, node_t *old_child, node_t *new_child,
                         vtree_pthread_data_t *vtree_data)
{
	vtree_slot_t *slot;

	slot = get_slot_from_rec_new(node, vtree_data);

	if (slot) {
		if (slot->child[0] == old_child)
			slot->child[0] = new_child;
		else
			slot->child[1] = new_child;
	}else {
		slot = read_slot(node, vtree_data);
		if (slot->child[0] == old_child)
			add_slot(node, new_child, slot->child[1], slot->parent,
			         vtree_data);
		else
			add_slot(node, slot->child[0], new_child, slot->parent,
			         vtree_data);
	}
}

static inline void
add_or_change_slot(node_t *node, node_t *left, node_t *right, node_t *parent,
                   vtree_pthread_data_t *vtree_data)
{
	vtree_slot_t *slot;

	slot = get_slot_from_rec_new(node, vtree_data);

	if (slot) {
		slot->child[0] = left;
		slot->child[1] = right;
		slot->parent = parent;
	} else {
		add_slot(node, left, right, parent, vtree_data);
	}
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

		if (CAS(&(it_rec->rec_next), NULL, new_rec)) {
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

/*
 * rot_left
 *
 *    A                  C
 *   / \                / \
 *  B   C     ===>     A   E
 *     / \            / \
 *    D   E          B   D
 *
 */
static void
rot_left(node_t *root, node_t *nodeA, vtree_slot_t *slotA, vtree_pthread_data_t *vtree_data)
{
	node_t *nodeC, *nodeD, *parent;
	vtree_slot_t *slotC;
	int slotC_old;

	nodeC = slotA->child[1];
	if ((slotC = get_slot_from_rec_new(nodeC, vtree_data)) == NULL) {
		slotC_old = 1;
		slotC = read_slot(nodeC, vtree_data);
	} else
		slotC_old = 0;
	if ((nodeD = slotC->child[0]) != NULL)
		add_or_change_slot_parent(nodeD, nodeA, vtree_data);
	parent = get_node_parent(slotA->parent);
	if (!parent)
		add_or_change_slot(root, nodeC, NULL, NULL, vtree_data);
	else
		add_or_change_slot_child(parent, nodeA, nodeC, vtree_data);

	if (get_slot_from_rec_new(nodeA, vtree_data) == NULL) {
		add_slot(nodeA, slotA->child[0], nodeD,
		         change_node_parent(slotA->parent, nodeC),
		         vtree_data);
	} else {
		slotA->child[1] = nodeD;
		slotA->parent = change_node_parent(slotA->parent, nodeC);
	}
	if (slotC_old) {
		add_slot(nodeC, nodeA, slotC->child[1],
		         change_node_parent(slotC->parent, parent),
		         vtree_data);
	} else {
		slotC->child[0] = nodeA;
		slotC->parent = change_node_parent(slotC->parent, parent);
	}
}

/*
 * rot_right
 *
 *      A              B
 *     / \            / \
 *    B   C   ===>   D   A
 *   / \                / \
 *  D   E              E   C
 *
 */
static void
rot_right(node_t *root, node_t *nodeA, vtree_slot_t *slotA, vtree_pthread_data_t *vtree_data)
{
	node_t *nodeB, *nodeE, *parent;
	vtree_slot_t *slotB;
	int slotB_old;

	nodeB = slotA->child[0];
	if ((slotB = get_slot_from_rec_new(nodeB, vtree_data)) == NULL) {
		slotB_old = 1;
		slotB = read_slot(nodeB, vtree_data);
	} else
		slotB_old = 0;
	if ((nodeE = slotB->child[1]) != NULL)
		add_or_change_slot_parent(nodeE, nodeA, vtree_data);
	parent = get_node_parent(slotA->parent);
	if (!parent)
		add_or_change_slot(root, nodeB, NULL, NULL, vtree_data);
	else
		add_or_change_slot_child(parent, nodeA, nodeB, vtree_data);

	if (get_slot_from_rec_new(nodeA, vtree_data) == NULL) {
		add_slot(nodeA, nodeE, slotA->child[1],
		         change_node_parent(slotA->parent, nodeB),
		         vtree_data);
	} else {
		slotA->child[0] = nodeE;
		slotA->parent = change_node_parent(slotA->parent, nodeB);
	}
	if (slotB_old) {
		add_slot(nodeB, slotB->child[0], nodeA,
		         change_node_parent(slotB->parent, parent),
		         vtree_data);
	} else {
		slotB->child[1] = nodeA;
		slotB->parent = change_node_parent(slotB->parent, parent);
	}
}

#ifdef RBTREE_CHECK
static int
rbtree_check(node_t *node, node_t *parent, int parent_color,
             vtree_pthread_data_t *vtree_data)
{
	int ln, rn;
	int color;
	vtree_slot_t *slot;

	if (node == NULL)
		return 0;
	slot = read_slot(node, vtree_data);
	assert(parent == get_node_parent(slot->parent));
	color = get_node_color(slot->parent);
	assert(color == RB_BLACK || parent_color == RB_BLACK);

	assert(slot->child[0] == NULL || slot->child[0]->value < node->value);
	assert(slot->child[1] == NULL || slot->child[1]->value > node->value);
	ln = rbtree_check(slot->child[0], node, color, vtree_data);
	rn = rbtree_check(slot->child[1], node, color, vtree_data);
	assert(ln == rn);
	if (color == RB_BLACK)
		return 1 + ln;
	return ln;
}
#else
#define rbtree_check(n, p, pc, vd)
#endif

int list_ins(int key, pthread_data_t *data)
{
	vtree_tree_t *tree = (vtree_tree_t *)data->list;
	vtree_pthread_data_t *vtree_data = (vtree_pthread_data_t *)data->ds_data;
	node_t *prev, *cur, *node, *new_node, *gprev, *prev_sib;
	vtree_slot_t *slot, *prev_slot, *gprev_slot, *prev_sib_slot;
	int prev_direction, direction, ret, val;

	new_node = vtree_new_node(key);
	assert(new_node != NULL);

	vtree_write_cs_enter(vtree_data);
	do {
		data->nr_txn++;
		prev = tree->root;
		prev_slot = read_slot(prev, vtree_data);
		cur = prev_slot->child[0];
		rbtree_check(cur, NULL, RB_RED, vtree_data);
		direction = 0;
		while (cur != NULL) {
			val = cur->value;
			if (val > key) {
				prev_slot = read_slot(cur, vtree_data);
				direction = 0;
				prev = cur;
				cur = prev_slot->child[0];
			} else if (val < key) {
				prev_slot = read_slot(cur, vtree_data);
				direction = 1;
				prev = cur;
				cur = prev_slot->child[1];
			} else
				break;
		}
		ret = (cur == NULL);
		if (!ret)
			goto out;
		node = new_node;
		if (direction == 0)
			prev_slot = add_slot(prev, node, prev_slot->child[1],
			                     prev_slot->parent, vtree_data);
		else
			prev_slot = add_slot(prev, prev_slot->child[0], node,
			                     prev_slot->parent, vtree_data);
		if (prev == tree->root) {
			slot = add_slot(node, NULL, NULL, set_node_black(NULL), vtree_data);
			goto out;
		}
		// newly inserted node is red to maintain invariant
		slot = add_slot(node, NULL, NULL, set_node_red(prev), vtree_data);
loop:
		if (prev == NULL) {
			// we can always set root to black
			slot->parent = set_node_black(slot->parent);
			goto out;
		}
		if (get_node_color(prev_slot->parent) == RB_BLACK) {
			// parent is black, no invariant violated
			goto out;
		}

		// now both node and parent are red, check grandparent,
		// since parent is red, there must be grandparent and it is black
		gprev = get_node_parent(prev_slot->parent);
		gprev_slot = read_slot(gprev, vtree_data);
		prev_direction = (gprev_slot->child[1] == prev);
		prev_sib = gprev_slot->child[!prev_direction];
		if (prev_sib) {
			prev_sib_slot = read_slot(prev_sib, vtree_data);
			if (get_node_color(prev_sib_slot->parent) == RB_RED) {
				// if sib is red, change parent and sib to black
				// and move upwards
				prev_slot->parent = set_node_black(prev_slot->parent);
				add_slot(prev_sib,
				         prev_sib_slot->child[0],
				         prev_sib_slot->child[1],
				         set_node_black(prev_sib_slot->parent),
				         vtree_data);
				prev = get_node_parent(gprev_slot->parent);
				slot = add_slot(gprev,
				                gprev_slot->child[0],
				                gprev_slot->child[1],
				                set_node_red(prev),
				                vtree_data);
				node = gprev;
				if (prev) {
					prev_slot = read_slot(prev, vtree_data);
					direction = (prev_slot->child[1] == node);
					prev_slot = add_slot(prev,
					                     prev_slot->child[0],
					                     prev_slot->child[1],
					                     prev_slot->parent,
					                     vtree_data);
				}
				goto loop;
			}
		}
		// now, either there is no sib of parent or it is black
		if (prev_direction == 0) {
			if (direction == 1) {
				rot_left(tree->root, prev, prev_slot, vtree_data);
				gprev_slot = get_slot_from_rec_new(gprev, vtree_data);
				prev = node;
				prev_slot = get_slot_from_rec_new(node, vtree_data);
			}
			rot_right(tree->root, gprev, gprev_slot, vtree_data);
			gprev_slot = get_slot_from_rec_new(gprev, vtree_data);
			gprev_slot->parent = set_node_red(gprev_slot->parent);
			prev_slot->parent = set_node_black(prev_slot->parent);
		} else {
			if (direction == 0) {
				rot_right(tree->root, prev, prev_slot, vtree_data);
				gprev_slot = get_slot_from_rec_new(gprev, vtree_data);
				prev = node;
				prev_slot = get_slot_from_rec_new(node, vtree_data);
			}
			rot_left(tree->root, gprev, gprev_slot, vtree_data);
			gprev_slot = get_slot_from_rec_new(gprev, vtree_data);
			gprev_slot->parent = set_node_red(gprev_slot->parent);
			prev_slot->parent = set_node_black(prev_slot->parent);
		}
out:
		;
	} while (vtree_write_cs_exit(vtree_data));

	if (ret == 0)
		vtree_free_node_later(new_node, vtree_data);

	vtree_maybe_quiescent(vtree_data);

	return ret;
}

static void
vtree_rebalance(node_t *root, node_t *prev, vtree_slot_t *prev_slot, int direction, vtree_pthread_data_t *vtree_data)
{
	node_t *node, *sib_node, *tmp1_node, *tmp2_node;
	vtree_slot_t *sib_slot, *tmp1_slot, *tmp2_slot;

	node = NULL;
loop:
	/*
	 * loop invariants:
	 * - node is black (or NULL on 1st iteration)
	 * - node is not roiot (parent is not NULL)
	 * -ALL leaf paths going through parent and node have
	 * black node count is 1 lower than other leaf path
	 */
	sib_node = prev_slot->child[!direction];
	if ((sib_slot = get_slot_from_rec_new(sib_node, vtree_data)) == NULL)
		sib_slot = read_slot(sib_node, vtree_data);
	if (get_node_color(sib_slot->parent) == RB_RED) {
		if (direction == 0) {
			tmp1_node = sib_slot->child[0];
			rot_left(root, prev, prev_slot, vtree_data);
		} else {
			tmp1_node = sib_slot->child[1];
			rot_right(root, prev, prev_slot, vtree_data);
		}
		tmp1_slot = get_slot_from_rec_new(tmp1_node, vtree_data);
		prev_slot->parent = set_node_red(prev_slot->parent);
		sib_slot = get_slot_from_rec_new(sib_node, vtree_data);
		sib_slot->parent = set_node_black(sib_slot->parent);
		sib_node = tmp1_node;
		sib_slot = tmp1_slot;
	}
	// now node and sib are both black
	tmp1_node = sib_slot->child[!direction];
	if (tmp1_node != NULL)
		if ((tmp1_slot = get_slot_from_rec_new(tmp1_node, vtree_data)) == NULL)
			tmp1_slot = read_slot(tmp1_node, vtree_data);
	if (tmp1_node == NULL || get_node_color(tmp1_slot->parent) == RB_BLACK) {
		tmp2_node = sib_slot->child[direction];
		if (tmp2_node != NULL)
			if ((tmp2_slot = get_slot_from_rec_new(tmp2_node, vtree_data)) == NULL)
				tmp2_slot = read_slot(tmp2_node, vtree_data);
		if (tmp2_node == NULL || get_node_color(tmp2_slot->parent) == RB_BLACK) {
			// sibling color flip to red
			// this violate rbtree variant, flip parent to black if it was red, or recurse
			add_or_change_slot(sib_node, sib_slot->child[0], sib_slot->child[1],
			                   set_node_red(sib_slot->parent), vtree_data);
			if (get_node_color(prev_slot->parent) == RB_RED)
				prev_slot->parent = set_node_black(prev_slot->parent);
			else {
				node = prev;
				prev = get_node_parent(prev_slot->parent);
				if (prev) {
					if ((prev_slot = get_slot_from_rec_new(prev, vtree_data)) == NULL) {
						prev_slot = read_slot(prev, vtree_data);
						prev_slot = add_slot(prev,
						                     prev_slot->child[0],
						                     prev_slot->child[1],
						                     prev_slot->parent,
						                     vtree_data);
					}
					direction = (prev_slot->child[1] == node);
					goto loop;
				}
			}
			goto out;
		}
		if (direction == 0) {
			rot_right(root, sib_node, sib_slot, vtree_data);
		} else {
			rot_left(root, sib_node, sib_slot, vtree_data);
		}
		sib_slot = get_slot_from_rec_new(sib_node, vtree_data);
		sib_slot->parent = set_node_red(sib_slot->parent);
		tmp2_slot = get_slot_from_rec_new(tmp2_node, vtree_data);
		tmp2_slot->parent = set_node_black(tmp2_slot->parent);
		tmp1_node = sib_node;
		tmp1_slot = sib_slot;
		sib_node = tmp2_node;
	}
	if (direction == 0) {
		rot_left(root, prev, prev_slot, vtree_data);
	} else {
		rot_right(root, prev, prev_slot, vtree_data);
	}
	sib_slot = get_slot_from_rec_new(sib_node, vtree_data);
	sib_slot->parent = change_node_parent(prev_slot->parent, get_node_parent(sib_slot->parent));
	prev_slot->parent = set_node_black(prev_slot->parent);
	add_or_change_slot(tmp1_node, tmp1_slot->child[0], tmp1_slot->child[1],
	                   set_node_black(tmp1_slot->parent), vtree_data);
out:
	return;
}

int list_del(int key, pthread_data_t *data)
{
	vtree_tree_t *tree = (vtree_tree_t *)data->list;
	vtree_pthread_data_t *vtree_data = (vtree_pthread_data_t *)data->ds_data;
	node_t *prev, *node, *succ, *succ_prev, *succ_right, *node_child, *rebalance_node;
	vtree_slot_t *slot, *prev_slot, *succ_slot, *succ_prev_slot, *node_child_slot, *rebalance_slot, *tmp_slot;
	int direction, rebalance_direction, ret, val;

	vtree_write_cs_enter(vtree_data);
	do {
		data->nr_txn++;
		prev = tree->root;
		slot = read_slot(prev, vtree_data);
		node = slot->child[0];
		rbtree_check(node, NULL, RB_RED, vtree_data);
		direction = 0;
		while (node != NULL) {
			val = node->value;
			if (val > key) {
				slot = read_slot(node, vtree_data);
				direction = 0;
				prev = node;
				node = slot->child[0];
			} else if (val < key) {
				slot = read_slot(node, vtree_data);
				direction = 1;
				prev = node;
				node = slot->child[1];
			} else
				break;
		}

		ret = (node != NULL);
		if (!ret)
			goto out;

		prev_slot = slot;
		slot = read_slot(node, vtree_data);
		rebalance_node = NULL;
		if (slot->child[0] == NULL) {
			if ((succ = slot->child[1]) != NULL) {
				// only right, right is red and node is black, no rebalance required
				rebalance_node = NULL;
				succ_slot = read_slot(succ, vtree_data);
				add_slot(succ, succ_slot->child[0], succ_slot->child[1],
				         slot->parent, vtree_data);
			} else {
				if (get_node_color(slot->parent) == RB_BLACK && prev != tree->root)
					rebalance_node = prev;
				else
					rebalance_node = NULL;
			}
		} else {
			if ((succ = slot->child[1]) != NULL) {
				// both left and right
				succ_prev = NULL;
				succ_prev_slot = NULL;
				succ_slot = read_slot(succ, vtree_data);
				node_child = succ;
				node_child_slot = succ_slot;
				while (succ_slot->child[0]) {
					succ_prev = succ;
					succ_prev_slot = succ_slot;
					succ = succ_slot->child[0];
					succ_slot = read_slot(succ, vtree_data);
				}
				if (succ_prev) {
					// move succ to node
					add_slot(succ, slot->child[0], slot->child[1], slot->parent, vtree_data);
					succ_right = succ_slot->child[1];
					// succ has no left child, if succ is red, no rebalance is required
					if (get_node_color(succ_slot->parent) == RB_BLACK) {
						rebalance_node = succ_prev;
						rebalance_direction = 0;
					}
					if (succ_prev != node_child) {
						add_slot(node_child,
						         node_child_slot->child[0],
						         node_child_slot->child[1],
						         change_node_parent(node_child_slot->parent, succ),
						         vtree_data);
						rebalance_slot = add_slot(succ_prev,
						                          succ_right,
						                          succ_prev_slot->child[1],
						                          succ_prev_slot->parent,
						                          vtree_data);
					} else {
						rebalance_slot = add_slot(succ_prev,
						                          succ_right,
						                          succ_prev_slot->child[1],
						                          change_node_parent(succ_prev_slot->parent,
						                                             succ),
						                          vtree_data);
					}
					if (succ_right != NULL) {
						// if succ has right, it must be a red node with no child
						add_slot(succ_right, NULL, NULL, set_node_red(succ_prev), vtree_data);
					}
				} else {
					// succ is right child of node
					if (get_node_color(succ_slot->parent) == RB_BLACK) {
						rebalance_node = succ;
						rebalance_direction = 1;
					}
					rebalance_slot = add_slot(succ,
					                          slot->child[0],
					                          succ_slot->child[1],
					                          slot->parent,
					                          vtree_data);
				}
				node_child = slot->child[0];
				node_child_slot = read_slot(node_child, vtree_data);
				add_slot(node_child,
				         node_child_slot->child[0],
				         node_child_slot->child[1],
			                 change_node_parent(node_child_slot->parent, succ),
				         vtree_data);
			} else {
				// only left, left is red and node is black, no rebalance required
				succ = slot->child[0];
				rebalance_node = NULL;
				succ_slot = read_slot(succ, vtree_data);
				add_slot(succ, succ_slot->child[0], succ_slot->child[1],
				         slot->parent, vtree_data);
			}
		}

		if (prev == tree->root) {
			add_slot(tree->root, succ, NULL, NULL, vtree_data);
		} else {
			if (direction == 0) {
				tmp_slot = add_slot(prev, succ, prev_slot->child[1],
				                    prev_slot->parent,
				                    vtree_data);
			} else {
				tmp_slot = add_slot(prev, prev_slot->child[0], succ,
				                    prev_slot->parent,
				                    vtree_data);
			}
			if (rebalance_node == prev) {
				rebalance_slot = tmp_slot;
				rebalance_direction = direction;
			}
		}
		add_slot(node, node, node, node, vtree_data);

		if (rebalance_node)
			vtree_rebalance(tree->root, rebalance_node, rebalance_slot, rebalance_direction,
			                vtree_data);
out:
		;
	} while (vtree_write_cs_exit(vtree_data));

	if (ret == 1)
		vtree_free_node_later(node, vtree_data);

	vtree_maybe_quiescent(vtree_data);

	return ret;
}

static void
rot_left_init(node_t *root, node_t *nodeA, vtree_slot_t *slotA)
{
	node_t *nodeC, *nodeD, *parent;
	vtree_slot_t *slotC;

	nodeC = slotA->child[1];
	slotC = nodeC->slots;
	if ((nodeD = slotC->child[0]) != NULL)
		nodeD->slots->parent = change_node_parent(nodeD->slots->parent, nodeA);
	parent = get_node_parent(slotA->parent);
	if (!parent)
		root->slots->child[0] = nodeC;
	else {
		if (parent->slots->child[0] == nodeA)
			parent->slots->child[0] = nodeC;
		else {
			parent->slots->child[1] = nodeC;
		}
	}

	slotA->child[1] = nodeD;
	slotA->parent = change_node_parent(slotA->parent, nodeC);
	slotC->child[0] = nodeA;
	slotC->parent = change_node_parent(slotC->parent, parent);
}

static void
rot_right_init(node_t *root, node_t *nodeA, vtree_slot_t *slotA)
{
	node_t *nodeB, *nodeE, *parent;
	vtree_slot_t *slotB;

	nodeB = slotA->child[0];
	slotB = nodeB->slots;
	if ((nodeE = slotB->child[1]) != NULL)
		nodeE->slots->parent = change_node_parent(nodeE->slots->parent, nodeA);
	parent = get_node_parent(slotA->parent);
	if (!parent)
		root->slots->child[0] = nodeB;
	else {
		if (parent->slots->child[0] == nodeA)
			parent->slots->child[0] = nodeB;
		else {
			parent->slots->child[1] = nodeB;
		}
	}

	slotA->child[0] = nodeE;
	slotA->parent = change_node_parent(slotA->parent, nodeB);
	slotB->child[1] = nodeA;
	slotB->parent = change_node_parent(slotB->parent, parent);
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
	rbtree_check(cur, NULL, RB_RED, vtree_data);
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

void *list_global_init(int init_size, int value_range)
{
	vtree_tree_t *tree;
	node_t *prev, *cur, *node, *gprev, *prev_sib;
	vtree_slot_t *slot, *prev_slot, *gprev_slot, *prev_sib_slot;
	vtree_record_t *rec;
	int i, key, val, direction, prev_direction;

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
		node = vtree_new_node(key);
		if (node == NULL)
			return NULL;
		prev->slots->child[direction] = node;
		if (prev == tree->root) {
			node->slots->parent = set_node_black(NULL);
			goto cont;
		}
		slot = node->slots;
		slot->parent = set_node_red(prev);
loop:
		if (prev == NULL) {
			slot->parent = set_node_black(slot->parent);
			goto cont;
		}
		prev_slot = prev->slots;
		if (get_node_color(prev_slot->parent) == RB_BLACK)
			goto cont;

		gprev = get_node_parent(prev_slot->parent);
		gprev_slot = gprev->slots;
		prev_direction = (gprev_slot->child[1] == prev);
		prev_sib = gprev_slot->child[!prev_direction];
		if (prev_sib) {
			prev_sib_slot = prev_sib->slots;
			if (get_node_color(prev_sib_slot->parent) == RB_RED) {
				prev_slot->parent = set_node_black(prev_slot->parent);
				prev_sib_slot->parent = set_node_black(prev_sib_slot->parent);
				prev = get_node_parent(gprev_slot->parent);
				gprev_slot->parent = set_node_red(prev);
				node = gprev;
				slot = gprev_slot;
				if (prev) {
					prev_slot = prev->slots;
					direction = (prev_slot->child[1] == node);
				}
				goto loop;
			}
		}
		if (prev_direction == 0) {
			if (direction == 1) {
				rot_left_init(tree->root, prev, prev_slot);
				gprev_slot = gprev->slots;
				prev = node;
				prev_slot = prev->slots;
			}
			rot_right_init(tree->root, gprev, gprev_slot);
			gprev_slot = gprev->slots;
			gprev_slot->parent = set_node_red(gprev_slot->parent);
			prev_slot->parent = set_node_black(prev_slot->parent);
		} else {
			if (direction == 0) {
				rot_right_init(tree->root, prev, prev_slot);
				gprev_slot = gprev->slots;
				prev = node;
				prev_slot = prev->slots;
			}
			rot_left_init(tree->root, gprev, gprev_slot);
			gprev_slot = gprev->slots;
			gprev_slot->parent = set_node_red(gprev_slot->parent);
			prev_slot->parent = set_node_black(prev_slot->parent);
		}
cont:
		i++;
	}

	qsbr_init();

	return tree;
}
