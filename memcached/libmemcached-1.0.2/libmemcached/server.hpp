/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <cassert>

static inline bool memcached_is_valid_servername(const memcached_string_t& arg)
{
  return arg.size > 0 or arg.size < NI_MAXHOST;
}

static inline void memcached_mark_server_as_clean(memcached_server_write_instance_st server)
{
  server->server_failure_counter= 0;
  server->next_retry= 0;
}


static inline void set_last_disconnected_host(memcached_server_write_instance_st self)
{
  assert(self->root);
  if (self->root == NULL)
  {
    return;
  }

  if (self->root->last_disconnected_server and self->root->last_disconnected_server->version == self->version)
  {
    return;
  }

  // const_cast
  memcached_st *root= (memcached_st *)self->root;

  memcached_server_free(root->last_disconnected_server);
  root->last_disconnected_server= memcached_server_clone(NULL, self);
  root->last_disconnected_server->version= self->version;
}

static inline void memcached_mark_server_for_timeout(memcached_server_write_instance_st server)
{
  if (server->state != MEMCACHED_SERVER_STATE_IN_TIMEOUT)
  {
    struct timeval next_time;
    if (gettimeofday(&next_time, NULL) == 0)
    {
      server->next_retry= next_time.tv_sec +server->root->retry_timeout;
    }
    else
    {
      server->next_retry= 1; // Setting the value to 1 causes the timeout to occur immediatly
    }

    server->state= MEMCACHED_SERVER_STATE_IN_TIMEOUT;
    if (server->server_failure_counter_query_id != server->root->query_id)
    {
      server->server_failure_counter++;
      server->server_failure_counter_query_id= server->root->query_id;
    }
    set_last_disconnected_host(server);
  }
}

LIBMEMCACHED_LOCAL
  memcached_server_st *__server_create_with(memcached_st *memc,
                                            memcached_server_write_instance_st host,
                                            const memcached_string_t& hostname,
                                            const in_port_t port,
                                            uint32_t weight,
                                            const memcached_connection_t type);
