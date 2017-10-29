/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2009 Brian Aker All rights reserved.
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

#pragma once

/**
  Strings are always under our control so we make some assumptions
  about them.

  1) is_initialized is always valid.
  2) A string once intialized will always be, until free where we
     unset this flag.
  3) A string always has a root.
*/

struct memcached_string_st {
  char *end;
  char *string;
  size_t current_size;
  memcached_st *root;
  struct {
    bool is_allocated:1;
    bool is_initialized:1;
  } options;
};

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_LOCAL
memcached_string_st *memcached_string_create(memcached_st *ptr,
                                             memcached_string_st *string,
                                             size_t initial_size);
LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_check(memcached_string_st *string, size_t need);

LIBMEMCACHED_LOCAL
char *memcached_string_c_copy(memcached_string_st *string);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_append_character(memcached_string_st *string,
                                                     char character);
LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_append(memcached_string_st *string,
                                           const char *value, size_t length);
LIBMEMCACHED_LOCAL
memcached_return_t memcached_string_reset(memcached_string_st *string);

LIBMEMCACHED_LOCAL
void memcached_string_free(memcached_string_st *string);

LIBMEMCACHED_LOCAL
size_t memcached_string_length(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
size_t memcached_string_size(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
const char *memcached_string_value(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
char *memcached_string_take_value(memcached_string_st *self);

LIBMEMCACHED_LOCAL
char *memcached_string_value_mutable(const memcached_string_st *self);

LIBMEMCACHED_LOCAL
void memcached_string_set_length(memcached_string_st *self, size_t length);

#ifdef __cplusplus
}
#endif
