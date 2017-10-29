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

#include "intset.h"
#include "ffwd.h"
#include "macro.h"

int set_contains_intern(intset_t *set, val_t val)
{
	int result;
	
#ifdef DEBUG
	printf("++> set_contains(%d)\n", (int)val);
	IO_FLUSH;
#endif
	
	node_t *prev, *next;
	
	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val == val);
	
	return result;
}

int set_contains(intset_t *set, val_t val, int server)
{
	int return_value; //this is going to hold the function return value from ffwd_exec
	GET_CONTEXT();
#ifdef MUTEX
	pthread_mutex_lock(&glock);
	return_value = set_contains_intern(set, val);
	pthread_mutex_unlock(&glock);
#else
	switch(server)
	{
		case 0: FFWD_EXEC(0, &set_contains_intern, return_value, 2, set, val);
				break;
		case 1: FFWD_EXEC(1, &set_contains_intern, return_value, 2, set, val);
				break;
		case 2: FFWD_EXEC(2, &set_contains_intern, return_value, 2, set, val);
				break;
		case 3: FFWD_EXEC(3, &set_contains_intern, return_value, 2, set, val);
				break;
		default:
				printf("control should never come here\n");
				break;
	}
#endif
	return return_value;
}

int set_seq_add_intern_2(intset_t *set, val_t val)
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
	if (result)
	{
		prev->next = new_node(val, next, 0);
	}
	return result;
}	

int set_seq_add_intern(intset_t *set, val_t val)
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
	if (result)
	{
		prev->next = new_node(val, next, 0);
	}
	return result;
}	
		
int set_seq_add(intset_t *set, val_t val, int server)
{
	int result;
	int return_value; //this is going to hold the function return value from ffwd_exec
	GET_CONTEXT();
#ifdef MUTEX
	pthread_mutex_lock(&glock);
	return_value = set_seq_add_intern(set, val);
	pthread_mutex_unlock(&glock);
#else
	switch(server)
	{
		case 0: FFWD_EXEC(0, &set_seq_add_intern, return_value, 2, set, val);
				break;
		case 1: FFWD_EXEC(1, &set_seq_add_intern, return_value, 2, set, val);
				break;
		case 2: FFWD_EXEC(2, &set_seq_add_intern, return_value, 2, set, val);
				break;
		case 3: FFWD_EXEC(3, &set_seq_add_intern, return_value, 2, set, val);
				break;
		default:
				printf("control should never come here\n");
				break;
	}
#endif
	return return_value;
}
		
int set_seq_add_2(intset_t *set, val_t val)
{
	int result;
	result = set_seq_add_intern_2(set, val);
	return result;
}

int set_add(intset_t *set, val_t val, int server)
{
	int result;
	
#ifdef DEBUG
	printf("++> set_add(%d)\n", (int)val);
	IO_FLUSH;
#endif
	
	result = set_seq_add(set, val, server);
		
	return result;
}

int set_remove_intern(intset_t *set, val_t val)
{
	int result = 0;

#ifdef DEBUG
	printf("++> set_remove(%d)\n", (int)val);
	IO_FLUSH;
#endif
	
	node_t *prev, *next;

	prev = set->head;
	next = prev->next;
	while (next->val < val) {
		prev = next;
		next = prev->next;
	}
	result = (next->val == val);
#if 1
	if (result) {
		prev->next = next->next;
		//slab_free(&s, next);
		free(next);
	}
#else
	result = 0;
#endif
	
	return result;
}

int set_remove(intset_t *set, val_t val, int server)
{
	GET_CONTEXT();
	int return_value; //this is going to hold the function return value from ffwd_exec
#ifdef MUTEX
	pthread_mutex_lock(&glock);
	return_value = set_remove_intern(set, val);
	pthread_mutex_unlock(&glock);
#else
	switch(server)
	{
		case 0: FFWD_EXEC(0, &set_remove_intern, return_value, 2, set, val);
				break;
		case 1: FFWD_EXEC(1, &set_remove_intern, return_value, 2, set, val);
				break;
		case 2: FFWD_EXEC(2, &set_remove_intern, return_value, 2, set, val);
				break;
		case 3: FFWD_EXEC(3, &set_remove_intern, return_value, 2, set, val);
				break;
		default:
				printf("control should never come here\n");
				break;
	}
#endif
	return return_value;
}
