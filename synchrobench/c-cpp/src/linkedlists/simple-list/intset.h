/*
 *  linkedlist.h
 *  
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

//#include "harris.h"
//#include "slab.h"
#include "linkedlist.h"

extern pthread_mutex_t glock;

struct data 
{
	intset_t *set;
	val_t val;
};

int set_contains(intset_t *set, val_t val, int transactional);
int set_add(intset_t *set, val_t val, int transactional);
int set_remove(intset_t *set, val_t val, int transactional);

