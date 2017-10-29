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

typedef struct memcached_st memcached_st;
typedef struct memcached_stat_st memcached_stat_st;
typedef struct memcached_analysis_st memcached_analysis_st;
typedef struct memcached_result_st memcached_result_st;
typedef struct memcached_array_st memcached_array_st;
typedef struct memcached_error_t memcached_error_t;

// All of the flavors of memcache_server_st
typedef struct memcached_server_st memcached_server_st;
typedef const struct memcached_server_st *memcached_server_instance_st;
typedef struct memcached_server_st *memcached_server_list_st;

typedef struct memcached_callback_st memcached_callback_st;

// The following two structures are internal, and never exposed to users.
typedef struct memcached_string_st memcached_string_st;
typedef struct memcached_string_t memcached_string_t;
typedef struct memcached_continuum_item_st memcached_continuum_item_st;


#ifdef __cplusplus
extern "C" {
#endif

typedef memcached_return_t (*memcached_clone_fn)(memcached_st *destination, const memcached_st *source);
typedef memcached_return_t (*memcached_cleanup_fn)(const memcached_st *ptr);

/**
  Memory allocation functions.
*/
typedef void (*memcached_free_fn)(const memcached_st *ptr, void *mem, void *context);
typedef void *(*memcached_malloc_fn)(const memcached_st *ptr, const size_t size, void *context);
typedef void *(*memcached_realloc_fn)(const memcached_st *ptr, void *mem, const size_t size, void *context);
typedef void *(*memcached_calloc_fn)(const memcached_st *ptr, size_t nelem, const size_t elsize, void *context);


typedef memcached_return_t (*memcached_execute_fn)(const memcached_st *ptr, memcached_result_st *result, void *context);
typedef memcached_return_t (*memcached_server_fn)(const memcached_st *ptr, memcached_server_instance_st server, void *context);
typedef memcached_return_t (*memcached_stat_fn)(memcached_server_instance_st server,
                                                const char *key, size_t key_length,
                                                const char *value, size_t value_length,
                                                void *context);

/**
  Trigger functions.
*/
typedef memcached_return_t (*memcached_trigger_key_fn)(const memcached_st *ptr,
                                                       const char *key, size_t key_length,
                                                       memcached_result_st *result);
typedef memcached_return_t (*memcached_trigger_delete_key_fn)(const memcached_st *ptr,
                                                              const char *key, size_t key_length);

typedef memcached_return_t (*memcached_dump_fn)(const memcached_st *ptr,
                                                const char *key,
                                                size_t key_length,
                                                void *context);

#ifdef __cplusplus
}
#endif

/**
  @note The following definitions are just here for backwards compatibility.
*/
typedef memcached_return_t memcached_return;
typedef memcached_server_distribution_t memcached_server_distribution;
typedef memcached_behavior_t memcached_behavior;
typedef memcached_callback_t memcached_callback;
typedef memcached_hash_t memcached_hash;
typedef memcached_connection_t memcached_connection;
typedef memcached_clone_fn memcached_clone_func;
typedef memcached_cleanup_fn memcached_cleanup_func;
typedef memcached_execute_fn memcached_execute_function;
typedef memcached_server_fn memcached_server_function;
typedef memcached_trigger_key_fn memcached_trigger_key;
typedef memcached_trigger_delete_key_fn memcached_trigger_delete_key;
typedef memcached_dump_fn memcached_dump_func;
