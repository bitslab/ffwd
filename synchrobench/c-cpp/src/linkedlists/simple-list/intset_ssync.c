/*
 *  intset.c
 *  
 *  Integer set operations (contain, insert, delete) 
 *  that call stm-based / lock-free counterparts.
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "intset_ssync.h"

int set_contains(intset_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2)
{
	int result;
	
#ifdef DEBUG
	printf("++> set_contains(%d)\n", (int)val);
	IO_FLUSH;
#endif
	
	node_t *prev, *next;
	
	LOCK(data1, set->lock)
	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val == val);
	UNLOCK(data1, set->lock)
	return result;
}

inline int set_seq_add(intset_t *set, val_t val)
{
	int result;
	node_t *prev, *next;
	
	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val != val);
	if (result) {
		prev->next = new_node(val, next, 0);
	}
	return result;
}	
		

int set_add(intset_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2)
{
	int result;
	
#ifdef DEBUG
	printf("++> set_add(%d)\n", (int)val);
	IO_FLUSH;
#endif
	LOCK(data1, set->lock)
	result = set_seq_add(set, val);
	UNLOCK(data1, set->lock)
	
	return result;
}

int set_add_2(intset_t *set, val_t val)
{
	int result;
	result = set_seq_add(set, val);
	return result;
}

int set_remove(intset_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2)
{
	int result = 0;
	
#ifdef DEBUG
	printf("++> set_remove(%d)\n", (int)val);
	IO_FLUSH;
#endif
	LOCK(data1, set->lock)
	node_t *prev, *next;

	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val == val);
	if (result) {
		prev->next = next->next;
		free(next);
	}
	UNLOCK(data1, set->lock)
			
	return result;
}


