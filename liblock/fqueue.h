#ifndef _FQUEUE_H_
#define _FQUEUE_H_

#include <stdio.h>

struct fqueue {
	uintptr_t volatile next_and_mark;
	void*              content;
};

static inline int fqueue_is_marked(uintptr_t next_and_mark) {
	return next_and_mark & 1;
}

static inline struct fqueue* fqueue_ptr(uintptr_t next_and_mark) {
	return (struct fqueue*)(next_and_mark & -2);
}

static inline uintptr_t fqueue_mark(struct fqueue* node) {
	return (uintptr_t)node | 1;
}

static inline int fqueue_foreach_besteffort(struct fqueue* volatile* root, int (*func)(struct fqueue*, int), int data, int* err) {
	uintptr_t volatile* pred = (uintptr_t*)root;
	uintptr_t cur;
	int res = data;

	while((cur = *pred)) {
		if(fqueue_is_marked(cur)) {
			*err = 1;
			return res;
		} else {
			res = func((struct fqueue*)cur, res);
			pred = &((struct fqueue*)cur)->next_and_mark;
			//if(*pred == cur)
			//fatal("cyclic list");
		}
	}
	*err = 0;
	return res;
}

static int fqueue_remove(struct fqueue* volatile* root, struct fqueue* node, void on_remove(struct fqueue*)) {
	uintptr_t volatile* pred;
	struct fqueue* cur;

	cur = fqueue_ptr(node->next_and_mark);

	if(__sync_val_compare_and_swap(&node->next_and_mark, cur, fqueue_mark(cur)) != (uintptr_t)cur)
		return 0;

	/* find and compress */
 restart:
	pred = (uintptr_t*)root;

	while(1) {
		cur = fqueue_ptr(*pred);

		if(!cur)
			return 0;

		if(fqueue_is_marked(cur->next_and_mark)) {
			/* invariant: if the node is already removed, pred does not contain cur */
			if(__sync_val_compare_and_swap(pred, cur, fqueue_ptr(cur->next_and_mark)) != (uintptr_t)cur)
				goto restart;
			if(on_remove)
				on_remove(cur);
			if(cur == node)
				return 1;
		}
			
		pred = &cur->next_and_mark;
	}
}

static void fqueue_ordered_insert(struct fqueue* volatile* root, struct fqueue* node, int lt(struct fqueue*, struct fqueue*)) {
	uintptr_t volatile* pred;
	struct fqueue* cur;

	/* find and compress */
 restart:
	pred = (uintptr_t*)root;

	while(1) {
		cur = fqueue_ptr(*pred);

		if(!cur) {
			node->next_and_mark = 0;
			if(__sync_val_compare_and_swap(pred, cur, node) != (uintptr_t)cur)
				goto restart;
			else
				return;
		}

		/* compress */
		if(fqueue_is_marked(cur->next_and_mark)) {
			/* invariant: if the node is already removed, pred does not contain cur */
			if(__sync_val_compare_and_swap(pred, cur, fqueue_ptr(cur->next_and_mark)) != (uintptr_t)cur)
				goto restart;
		}

		if(lt(node, cur)) {
			node->next_and_mark = (uintptr_t)cur;
			if(__sync_val_compare_and_swap(pred, cur, node) != (uintptr_t)cur)
				goto restart;
			else
				return;
		}
			
		pred = &cur->next_and_mark;
	}
}

static void fqueue_enqueue(struct fqueue* volatile* root, struct fqueue* node) {
	uintptr_t attempt;
	
	do {
		attempt = (uintptr_t)*root;
		node->next_and_mark = attempt;
		//printf("** enqueue %p with next at %p\n", node, (void*)node->next_and_mark);
	} while(__sync_val_compare_and_swap((uintptr_t*)root, attempt, node) != attempt);
}

static struct fqueue* fqueue_dequeue(struct fqueue* volatile* root) {
	struct fqueue* cur;
	uintptr_t mark;

	while((cur = *root)) {
		mark = cur->next_and_mark;

		if(fqueue_is_marked(mark)) {
			/* second, try to remove it safely */
			if(__sync_val_compare_and_swap((uintptr_t*)root, cur, fqueue_ptr(mark)) == (uintptr_t)cur)
				return cur;
		} else
			/* first, mark the node for deletion */
			__sync_val_compare_and_swap(&cur->next_and_mark, mark, fqueue_mark((struct fqueue*)mark));
	}

	return 0;
}

#endif
