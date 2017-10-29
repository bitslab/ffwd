/*
 * File:
 *   intset.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Linked list integer set operations
 *
 * Copyright (c) 2009-2010.
 *
 * intset.c is part of Synchrobench
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

#include "lazy.h"

#ifdef SSYNC
int set_contains_l(intset_l_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2)
{
	return parse_find(set, val);
}
#else
int set_contains_l(intset_l_t *set, val_t val, int transactional)
{
	return parse_find(set, val);
}
#endif

/* Non-locking version required for FFWD main thread */
int set_add_l_2(intset_l_t *set, val_t val, int transactional)
{  
	return parse_insert_2(set, val);
}

#ifdef SSYNC
int set_add_l(intset_l_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2)
{  
	return parse_insert(set, val, data1, data2);
}
#else
int set_add_l(intset_l_t *set, val_t val, int transactional)
{  
	return parse_insert(set, val);
}
#endif

#ifdef SSYNC
int set_remove_l(intset_l_t *set, val_t val, int transactional, lock_local_data *data1, lock_local_data *data2)
{
	return parse_delete(set, val, data1, data2);
}
#else
int set_remove_l(intset_l_t *set, val_t val, int transactional)
{
	return parse_delete(set, val);
}
#endif
