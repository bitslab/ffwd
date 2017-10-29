/*
 * File:
 *   coupling.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Hand-over-hand lock-based linked list implementation of an integer set
 *
 * Copyright (c) 2009-2010.
 *
 * coupling.c is part of Synchrobench
 * 
 * Synchrobench is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "coupling.h"

/* 
 * Similar algorithm for the delete, find, and insert:
 * Lock the first two elements (locking each before getting the copy of the element)
 * then unlock previous, keep ownership of the current, and lock next in a loop.
 */
#if defined(SSYNC)
int lockc_delete(intset_l_t *set, val_t val, lock_local_data *data1, lock_local_data *data2) 
#else
int lockc_delete(intset_l_t *set, val_t val) 
#endif
{
	node_l_t *curr, *next;
	int found;
	lock_local_data *lru;
	
	LOCK(&set->head->lock,data1);
	curr = set->head;
	LOCK(&curr->next->lock,data2);
	next = curr->next;
	lru = data1;
	
	while (next->val < val) 
	{
		UNLOCK(&curr->lock,lru);
		curr = next;
		LOCK(&next->next->lock,lru);
		next = next->next;
		if (lru == data1)
			lru = data2;
		else
			lru = data1;
	}
	found = (val == next->val);
	if (found) 
	{
	  curr->next = next->next;
	  if(lru == data1)
	  {
	  	UNLOCK(&next->lock, data2);
	  }
	  else
	  {
	  	UNLOCK(&next->lock, data1);
	  }
	  node_delete_l(next);
	  UNLOCK(&curr->lock, lru);
	} 
	else 
	{
	  UNLOCK(&curr->lock, lru);
	  if(lru == data1)
	  {
	  	UNLOCK(&next->lock, data2);
	  }
	  else
	  {
	  	UNLOCK(&next->lock, data1);
	  }
	}
	return found;
}

#if defined(SSYNC)
int lockc_find(intset_l_t *set, val_t val, lock_local_data *data1, lock_local_data *data2) 
#else
int lockc_find(intset_l_t *set, val_t val) 
#endif
{
	node_l_t *curr, *next; 
	int found;
	lock_local_data *lru;
	
	LOCK(&set->head->lock, data1);
	curr = set->head;
	LOCK(&curr->next->lock, data2);
	next = curr->next;
	lru = data1;
	
	while (next->val < val) {
		UNLOCK(&curr->lock, lru);
		curr = next;
		LOCK(&next->next->lock, lru);
		next = curr->next;
		if (lru == data1)
			lru = data2;
		else
			lru = data1;
	}	
	found = (val == next->val);
	UNLOCK(&curr->lock, lru);
	if(lru == data1)
	{
	  UNLOCK(&next->lock, data2);
	}
	else
	{
	  UNLOCK(&next->lock, data1);
	}
	return found;
}

#if defined(SSYNC)
int lockc_insert(intset_l_t *set, val_t val, lock_local_data *data1, lock_local_data *data2) 
#else
int lockc_insert(intset_l_t *set, val_t val) 
#endif
{
	node_l_t *curr, *next, *newnode;
	int found;
	lock_local_data *lru;
	
	LOCK(&set->head->lock, data1);
	curr = set->head;
	LOCK(&curr->next->lock, data2);
	next = curr->next;
	lru = data1;
	
	while (next->val < val) {
		
		UNLOCK(&curr->lock, lru);
		curr = next;
		LOCK(&next->next->lock, lru);
		next = curr->next;
		if (lru == data1)
			lru = data2;
		else
			lru = data1;
		
	}
	found = (val == next->val);
	if (!found) {
		newnode =  new_node_l(val, next, 0);
		curr->next = newnode;
	}
	UNLOCK(&curr->lock, lru);
	if(lru == data1)
	{
	  UNLOCK(&next->lock, data2);
	}
	else
	{
	  UNLOCK(&next->lock, data1);
	}
	return !found;
}

/* Unsynchronised version of insert, to be called from main thread */
int lock_insert(intset_l_t *set, val_t val) 
{
	node_l_t *curr, *next, *newnode;
	int found;
	
	curr = set->head;
	next = curr->next;
	
	while (next->val < val) {
		
		curr = next;
		next = curr->next;
		
	}
	found = (val == next->val);
	if (!found) {
		newnode =  new_node_l(val, next, 0);
		curr->next = newnode;
	}
	return !found;
}
