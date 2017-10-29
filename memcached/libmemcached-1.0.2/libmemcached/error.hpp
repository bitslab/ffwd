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

#pragma once

#ifdef __cplusplus

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define MEMCACHED_AT __FILE__ ":" TOSTRING(__LINE__)

LIBMEMCACHED_LOCAL
memcached_return_t memcached_set_parser_error(memcached_st& memc,
                                              const char *at,
                                              const char *format, ...);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_error(memcached_st&, memcached_return_t rc, const char *at);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_error(memcached_server_st&, memcached_return_t rc, const char *at);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_error(memcached_st&, memcached_return_t rc, const char *at, const char *str, size_t length);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_error(memcached_server_st&, memcached_return_t rc, const char *at, const char *str, size_t length);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_error(memcached_st& memc, memcached_return_t rc, const char *at, memcached_string_t& str);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_error(memcached_server_st&, memcached_return_t rc, const char *at, memcached_string_t& str);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_errno(memcached_st& memc, int local_errno, const char *at, memcached_string_t& str);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_errno(memcached_server_st&, int local_errno, const char *at, memcached_string_t& str);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_errno(memcached_st& memc, int local_errno, const char *at, const char *str, size_t length);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_errno(memcached_server_st&, int local_errno, const char *at, const char *str, size_t length);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_errno(memcached_st& memc, int local_errno, const char *at);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_set_errno(memcached_server_st&, int local_errno, const char *at);

LIBMEMCACHED_LOCAL
bool memcached_has_current_error(memcached_st&);

LIBMEMCACHED_LOCAL
bool memcached_has_current_error(memcached_server_st&);

LIBMEMCACHED_LOCAL
void memcached_error_free(memcached_st&);

LIBMEMCACHED_LOCAL
void memcached_error_free(memcached_server_st&);

LIBMEMCACHED_LOCAL
memcached_error_t *memcached_error_copy(const memcached_server_st&);

#endif
