/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  LibMemcached
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <libmemcached/common.h>
#include <assert.h>
#include <iso646.h>

struct memcached_array_st
{
  struct memcached_st *root;
  size_t size;
  char c_str[];
};


memcached_array_st *memcached_array_clone(struct memcached_st *memc, const memcached_array_st *original)
{
  if (not original)
    return NULL;

  return memcached_strcpy(memc, original->c_str, original->size);
}

memcached_array_st *memcached_strcpy(struct memcached_st *memc, const char *str, size_t str_length)
{
  assert(memc);
  assert(str);
  assert(str_length);

  memcached_array_st *array= (struct memcached_array_st *)libmemcached_malloc(memc, sizeof(struct memcached_array_st) +str_length +1);

  if (not array)
    return NULL;

  array->root= memc;
  array->size= str_length; // We don't count the NULL ending
  memcpy(array->c_str, str, str_length);
  array->c_str[str_length]= 0;

  return array;
}

bool memcached_array_is_null(memcached_array_st *array)
{
  assert(array);
  assert(array->root);

  if (not array)
    return false;

  if (array->size and array->c_str)
    return false;

  assert(not array->size and not array->c_str);

  return true;
}

memcached_string_t memcached_array_to_string(memcached_array_st *array)
{
  assert(array);
  assert(array->c_str);
  assert(array->size);
  memcached_string_t tmp;
  tmp.c_str= array->c_str;
  tmp.size= array->size;

  return tmp;
}

void memcached_array_free(memcached_array_st *array)
{
  if (not array)
    return;

  WATCHPOINT_ASSERT(array->root);
  libmemcached_free(array->root, array);
}

size_t memcached_array_size(memcached_array_st *array)
{
  if (not array)
    return 0;

  return array->size;
}

const char *memcached_array_string(memcached_array_st *array)
{
  if (not array)
    return NULL;

  return array->c_str;
}
