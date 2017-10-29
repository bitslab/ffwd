/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2006-2010 Brian Aker All rights reserved.
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

memcached_server_list_st 
memcached_server_list_append_with_weight(memcached_server_list_st ptr,
                                         const char *hostname, in_port_t port,
                                         uint32_t weight,
                                         memcached_return_t *error)
{
  uint32_t count;
  memcached_server_list_st new_host_list;

  memcached_return_t unused;
  if (error == NULL)
    error= &unused;

  if (hostname == NULL)
  {
    hostname= "localhost";
  }

  if (hostname[0] == '/')
  {
    port = 0;
  }
  else if (not port)
  {
    port= MEMCACHED_DEFAULT_PORT;
  }


  /* Increment count for hosts */
  count= 1;
  if (ptr != NULL)
  {
    count+= memcached_server_list_count(ptr);
  }

  new_host_list= (memcached_server_write_instance_st)realloc(ptr, sizeof(memcached_server_st) * count);
  if (not new_host_list)
  {
    *error= memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT);
    return NULL;
  }

  memcached_string_t _hostname= { memcached_string_make_from_cstr(hostname) };
  /* @todo Check return type */
  if (not __server_create_with(NULL, &new_host_list[count-1], _hostname, port, weight, port ? MEMCACHED_CONNECTION_TCP : MEMCACHED_CONNECTION_UNIX_SOCKET))
  {
    *error= memcached_set_errno(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT);
    return NULL;
  }

#if 0
  // Handset allocated since 
  new_host_list->options.is_allocated= true;
#endif

  /* Backwards compatibility hack */
  memcached_servers_set_count(new_host_list, count);

  *error= MEMCACHED_SUCCESS;
  return new_host_list;
}

memcached_server_list_st
memcached_server_list_append(memcached_server_list_st ptr,
                             const char *hostname, in_port_t port,
                             memcached_return_t *error)
{
  return memcached_server_list_append_with_weight(ptr, hostname, port, 0, error);
}

uint32_t memcached_server_list_count(const memcached_server_list_st self)
{
  return (self == NULL)
    ? 0
    : self->number_of_hosts;
}

memcached_server_st *memcached_server_list(const memcached_st *self)
{
  if (self)
  {
    return self->servers;
  }

  return NULL;
}

void memcached_server_list_set(memcached_st *self, memcached_server_st *list)
{
  self->servers= list;
}

void memcached_server_list_free(memcached_server_list_st self)
{
  if (not self)
    return;

  for (uint32_t x= 0; x < memcached_server_list_count(self); x++)
  {
    assert_msg(not memcached_is_allocated(&self[x]), "You have called memcached_server_list_free(), but you did not pass it a valid memcached_server_list_st");
    __server_free(&self[x]);
  }

  libmemcached_free(self->root, self);
}
