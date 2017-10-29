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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_LOCAL
memcached_array_st *memcached_array_clone(struct memcached_st *memc, const memcached_array_st *original);

LIBMEMCACHED_LOCAL
memcached_array_st *memcached_strcpy(struct memcached_st *memc, const char *str, size_t str_length);

LIBMEMCACHED_LOCAL
void memcached_array_free(memcached_array_st *array);

LIBMEMCACHED_LOCAL
size_t memcached_array_size(memcached_array_st *array);

LIBMEMCACHED_LOCAL
const char *memcached_array_string(memcached_array_st *array);

LIBMEMCACHED_LOCAL
memcached_string_t memcached_array_to_string(memcached_array_st *array);

LIBMEMCACHED_LOCAL
bool memcached_array_is_null(memcached_array_st *array);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
#define memcached_print_array(X) static_cast<int>(memcached_array_size(X)), memcached_array_string(X)
#define memcached_param_array(X) memcached_array_string(X), memcached_array_size(X)
#else
#define memcached_print_array(X) (int)memcached_array_size((X)), memcached_array_string((X))
#define memcached_param_array(X) memcached_array_string(X), memcached_array_size(X)
#endif
