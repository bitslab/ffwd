/*
 *  linkedlist.h
 *  
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include "linkedlist_ssync.h"
int set_contains(intset_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2);
int set_add(intset_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2);
int set_remove(intset_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2);
int set_add_2(intset_l_t *set, val_t val); 
