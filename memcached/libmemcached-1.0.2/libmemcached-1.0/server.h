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

#ifndef WIN32
#include <netdb.h>
#endif

#ifdef NI_MAXHOST
#define MEMCACHED_NI_MAXHOST NI_MAXHOST
#else
#define MEMCACHED_NI_MAXHOST 1025
#endif

enum memcached_server_state_t {
  MEMCACHED_SERVER_STATE_NEW, // fd == -1, no address lookup has been done
  MEMCACHED_SERVER_STATE_ADDRINFO, // ADDRRESS information has been gathered
  MEMCACHED_SERVER_STATE_IN_PROGRESS,
  MEMCACHED_SERVER_STATE_CONNECTED,
  MEMCACHED_SERVER_STATE_IN_TIMEOUT
};

struct memcached_server_st {
  struct {
    bool is_allocated:1;
    bool is_initialized:1;
    bool is_shutting_down:1;
    bool is_dead:1;
  } options;
  uint32_t number_of_hosts;
  uint32_t cursor_active;
  in_port_t port;
  memcached_socket_t fd;
  uint32_t io_bytes_sent; /* # bytes sent since last read */
  uint32_t server_failure_counter;
  uint64_t server_failure_counter_query_id;
  uint32_t weight;
  uint32_t version;
  enum memcached_server_state_t state;
  struct {
    uint32_t read;
    uint32_t write;
  } io_wait_count;
  uint8_t major_version; // Default definition of UINT8_MAX means that it has not been set.
  uint8_t micro_version; // ditto
  uint8_t minor_version; // ditto
  memcached_connection_t type;
  char *read_ptr;
  size_t read_buffer_length;
  size_t read_data_length;
  size_t write_buffer_offset;
  struct addrinfo *address_info;
  struct addrinfo *address_info_next;
  time_t next_retry;
  memcached_st *root;
  uint64_t limit_maxbytes;
  struct memcached_error_t *error_messages;
  char read_buffer[MEMCACHED_MAX_BUFFER];
  char write_buffer[MEMCACHED_MAX_BUFFER];
  char hostname[MEMCACHED_NI_MAXHOST];
};


#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_server_cursor(const memcached_st *ptr,
                                           const memcached_server_fn *callback,
                                           void *context,
                                           uint32_t number_of_callbacks);

LIBMEMCACHED_API
  memcached_server_instance_st memcached_server_by_key(memcached_st *ptr,
                                                       const char *key,
                                                       size_t key_length,
                                                       memcached_return_t *error);

LIBMEMCACHED_API
void memcached_server_error_reset(memcached_server_st *ptr);

LIBMEMCACHED_API
void memcached_server_free(memcached_server_st *ptr);

LIBMEMCACHED_LOCAL
memcached_server_st *memcached_server_clone(memcached_server_st *destination,
                                            memcached_server_st *source);

LIBMEMCACHED_API
memcached_server_instance_st memcached_server_get_last_disconnect(const memcached_st *ptr);


LIBMEMCACHED_API
memcached_return_t memcached_server_add_udp(memcached_st *ptr,
                                            const char *hostname,
                                            in_port_t port);
LIBMEMCACHED_API
memcached_return_t memcached_server_add_unix_socket(memcached_st *ptr,
                                                    const char *filename);
LIBMEMCACHED_API
memcached_return_t memcached_server_add(memcached_st *ptr,
                                        const char *hostname, in_port_t port);

LIBMEMCACHED_LOCAL
  memcached_return_t memcached_server_add_parsed(memcached_st *ptr,
                                                 const char *hostname,
                                                 size_t hostname_length,
                                                 in_port_t port,
                                                 uint32_t weight);

LIBMEMCACHED_API
memcached_return_t memcached_server_add_udp_with_weight(memcached_st *ptr,
                                                        const char *hostname,
                                                        in_port_t port,
                                                        uint32_t weight);
LIBMEMCACHED_API
memcached_return_t memcached_server_add_unix_socket_with_weight(memcached_st *ptr,
                                                                const char *filename,
                                                                uint32_t weight);
LIBMEMCACHED_API
memcached_return_t memcached_server_add_with_weight(memcached_st *ptr, const char *hostname,
                                                    in_port_t port,
                                                    uint32_t weight);

/**
  Operations on Single Servers.
*/
LIBMEMCACHED_API
uint32_t memcached_server_response_count(const memcached_server_instance_st self);

LIBMEMCACHED_API
const char *memcached_server_name(const memcached_server_instance_st self);

LIBMEMCACHED_API
in_port_t memcached_server_port(const memcached_server_instance_st self);

LIBMEMCACHED_API
const char *memcached_server_type(const memcached_server_instance_st ptr);


LIBMEMCACHED_LOCAL
void __server_free(memcached_server_st *);

#ifdef __cplusplus
} // extern "C"
#endif
