/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  LibMemcached
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2009 Brian Aker
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

/*
  Common include file for libmemached
*/

#pragma once

#include <config.h>

#ifdef __cplusplus
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <ctype.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <libmemcached-1.0/memcached.h>
#include <libmemcached/watchpoint.h>
#include <libmemcached/is.h>

#include <libmemcached/server_instance.h>

#ifdef HAVE_POLL_H
#include <poll.h>
#else
#include "poll/poll.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef memcached_return_t (*memcached_server_execute_fn)(memcached_st *ptr, memcached_server_write_instance_st server, void *context);

LIBMEMCACHED_LOCAL
memcached_server_write_instance_st memcached_server_instance_fetch(memcached_st *ptr, uint32_t server_key);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_server_execute(memcached_st *ptr,
                                            memcached_server_execute_fn callback,
                                            void *context);
#ifdef __cplusplus
} // extern "C"
#endif


/* These are private not to be installed headers */
#include <libmemcached/error.hpp>
#include <libmemcached/memory.h>
#include <libmemcached/io.h>
#ifdef __cplusplus
#include <libmemcached/string.hpp>
#include <libmemcached/io.hpp>
#include <libmemcached/do.hpp>
#endif
#include <libmemcached/internal.h>
#include <libmemcached/array.h>
#include <libmemcached/libmemcached_probes.h>
#include <libmemcached/memcached/protocol_binary.h>
#include <libmemcached/byteorder.h>
#include <libmemcached/initialize_query.h>
#include <libmemcached/response.h>
#include <libmemcached/namespace.h>
#include <libmemcached/virtual_bucket.h>

#ifdef __cplusplus
#include <libmemcached/backtrace.hpp>
#include <libmemcached/assert.hpp>
#include <libmemcached/server.hpp>
#include <libmemcached/key.hpp>
#endif

#include <libmemcached/continuum.hpp>

#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)

#define likely(x)       if((x))
#define unlikely(x)     if((x))

#else

#define likely(x)       if(__builtin_expect((x) != 0, 1))
#define unlikely(x)     if(__builtin_expect((x) != 0, 0))
#endif

#define MEMCACHED_BLOCK_SIZE 1024
#define MEMCACHED_DEFAULT_COMMAND_SIZE 350
#define SMALL_STRING_LEN 1024
#define HUGE_STRING_LEN 8196

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_LOCAL
memcached_return_t memcached_connect(memcached_server_write_instance_st ptr);

LIBMEMCACHED_LOCAL
memcached_return_t run_distribution(memcached_st *ptr);

#define memcached_server_response_increment(A) (A)->cursor_active++
#define memcached_server_response_decrement(A) (A)->cursor_active--
#define memcached_server_response_reset(A) (A)->cursor_active=0

LIBMEMCACHED_LOCAL
memcached_return_t memcached_purge(memcached_server_write_instance_st ptr);

#ifdef __cplusplus
}
#endif
