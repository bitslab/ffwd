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

/*
  This is a partial implementation for fetching/creating memcached_server_st objects.
*/
#include <libmemcached/common.h>

static inline void _server_init(memcached_server_st *self, memcached_st *root,
                                const memcached_string_t& hostname,
                                in_port_t port,
                                uint32_t weight, memcached_connection_t type)
{
  self->options.is_shutting_down= false;
  self->options.is_dead= false;
  self->number_of_hosts= 0;
  self->cursor_active= 0;
  self->port= port;
  self->fd= -1;
  self->io_bytes_sent= 0;
  self->server_failure_counter= 0;
  self->server_failure_counter_query_id= 0;
  self->weight= weight ? weight : 1; // 1 is the default weight value
  WATCHPOINT_SET(self->io_wait_count.read= 0);
  WATCHPOINT_SET(self->io_wait_count.write= 0);
  self->major_version= UINT8_MAX;
  self->micro_version= UINT8_MAX;
  self->minor_version= UINT8_MAX;
  self->type= type;
  self->error_messages= NULL;
  self->read_ptr= self->read_buffer;
  self->read_buffer_length= 0;
  self->read_data_length= 0;
  self->write_buffer_offset= 0;
  self->address_info= NULL;
  self->address_info_next= NULL;

  self->state= MEMCACHED_SERVER_STATE_NEW;
  self->next_retry= 0;

  self->root= root;
  if (root)
  {
    self->version= ++root->server_info.version;
  }
  else
  {
    self->version= UINT_MAX;
  }
  self->limit_maxbytes= 0;
  memcpy(self->hostname, hostname.c_str, hostname.size);
  self->hostname[hostname.size]= 0;
}

static memcached_server_st *_server_create(memcached_server_st *self, const memcached_st *memc)
{
  if (not self)
  {
   self= (memcached_server_st *)libmemcached_malloc(memc, sizeof(memcached_server_st));

    if (not self)
    {
      return NULL; /*  MEMCACHED_MEMORY_ALLOCATION_FAILURE */
    }

    self->options.is_allocated= true;
  }
  else
  {
    self->options.is_allocated= false;
  }

  self->options.is_initialized= true;

  return self;
}

memcached_server_st *__server_create_with(memcached_st *memc,
                                          memcached_server_write_instance_st self,
                                          const memcached_string_t& hostname,
                                          const in_port_t port,
                                          uint32_t weight, 
                                          const memcached_connection_t type)
{
  if (memcached_is_valid_servername(hostname) == false)
  {
    memcached_set_error(*memc, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT, memcached_literal_param("Invalid hostname provided"));
    return NULL;
  }

  self= _server_create(self, memc);

  if (self == NULL)
  {
    return NULL;
  }

  _server_init(self, const_cast<memcached_st *>(memc), hostname, port, weight, type);


  if (memc and memcached_is_udp(memc))
  {
    self->write_buffer_offset= UDP_DATAGRAM_HEADER_LENGTH;
    memcached_io_init_udp_header(self, 0);
  }

  return self;
}

void __server_free(memcached_server_st *self)
{
  memcached_quit_server(self, false);

  if (self->address_info)
  {
    freeaddrinfo(self->address_info);
    self->address_info= NULL;
    self->address_info_next= NULL;
  }

  memcached_error_free(*self);

  if (memcached_is_allocated(self))
  {
    libmemcached_free(self->root, self);
  }
  else
  {
    self->options.is_initialized= false;
  }
}

void memcached_server_free(memcached_server_st *self)
{
  if (self == NULL)
  {
    return;
  }

  if (memcached_server_list_count(self))
  {
    memcached_server_list_free(self);
    return;
  }

  __server_free(self);
}

/*
  If we do not have a valid object to clone from, we toss an error.
*/
memcached_server_st *memcached_server_clone(memcached_server_st *destination,
                                            memcached_server_st *source)
{
  /* We just do a normal create if source is missing */
  if (source == NULL)
  {
    return NULL;
  }

  memcached_string_t hostname= { memcached_string_make_from_cstr(source->hostname) };
  destination= __server_create_with(source->root, destination,
                                    hostname,
                                    source->port, source->weight,
                                    source->type);
  if (destination)
  {
    if (source->error_messages)
    {
      destination->error_messages= memcached_error_copy(*source);
    }
  }

  return destination;

}

memcached_return_t memcached_server_cursor(const memcached_st *ptr,
                                           const memcached_server_fn *callback,
                                           void *context,
                                           uint32_t number_of_callbacks)
{
  memcached_return_t rc;
  if (memcached_failed(rc= initialize_const_query(ptr)))
  {
    return rc;
  }

  size_t errors= 0;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_instance_st instance=
      memcached_server_instance_by_position(ptr, x);

    for (uint32_t y= 0; y < number_of_callbacks; y++)
    {
      memcached_return_t ret= (*callback[y])(ptr, instance, context);

      if (memcached_failed(ret))
      {
        errors++;
        continue;
      }
    }
  }

  return errors ? MEMCACHED_SOME_ERRORS : MEMCACHED_SUCCESS;
}

memcached_return_t memcached_server_execute(memcached_st *ptr,
                                            memcached_server_execute_fn callback,
                                            void *context)
{
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, x);

    unsigned int iferror= (*callback)(ptr, instance, context);

    if (iferror)
    {
      continue;
    }
  }

  return MEMCACHED_SUCCESS;
}

memcached_server_instance_st memcached_server_by_key(memcached_st *ptr,
                                                     const char *key,
                                                     size_t key_length,
                                                     memcached_return_t *error)
{
  memcached_return_t unused;
  if (not error)
  {
    error= &unused;
  }


  memcached_return_t rc;
  if (memcached_failed(rc= initialize_const_query(ptr)))
  {
    *error= rc;
    return NULL;
  }

  if (memcached_failed(rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol)))
  {
    *error= rc;
    return NULL;
  }

  if (memcached_failed((memcached_key_test(*ptr, (const char **)&key, &key_length, 1))))
  {
    *error= MEMCACHED_BAD_KEY_PROVIDED;
    return NULL;
  }

  uint32_t server_key= memcached_generate_hash(ptr, key, key_length);
  return memcached_server_instance_by_position(ptr, server_key);

}

void memcached_server_error_reset(memcached_server_st *self)
{
  WATCHPOINT_ASSERT(self);
  if (not self)
  {
    return;
  }

  memcached_error_free(*self);
}

memcached_server_instance_st memcached_server_get_last_disconnect(const memcached_st *self)
{
  WATCHPOINT_ASSERT(self);
  if (not self)
    return 0;

  return self->last_disconnected_server;
}

uint32_t memcached_servers_set_count(memcached_server_st *servers, uint32_t count)
{
  WATCHPOINT_ASSERT(servers);
  if (not servers)
    return 0;

  return servers->number_of_hosts= count;
}

uint32_t memcached_server_count(const memcached_st *self)
{
  WATCHPOINT_ASSERT(self);
  if (not self)
    return 0;

  return self->number_of_hosts;
}

const char *memcached_server_name(const memcached_server_instance_st self)
{
  WATCHPOINT_ASSERT(self);
  if (not self)
    return NULL;

  return self->hostname;
}

in_port_t memcached_server_port(const memcached_server_instance_st self)
{
  WATCHPOINT_ASSERT(self);
  if (not self)
    return 0;

  return self->port;
}

uint32_t memcached_server_response_count(const memcached_server_instance_st self)
{
  WATCHPOINT_ASSERT(self);
  if (not self)
    return 0;

  return self->cursor_active;
}

const char *memcached_server_type(const memcached_server_instance_st ptr)
{
  if (ptr)
  {
    switch (ptr->type)
    {
    case MEMCACHED_CONNECTION_TCP:
      return "TCP";

    case MEMCACHED_CONNECTION_UDP:
      return "UDP";

    case MEMCACHED_CONNECTION_UNIX_SOCKET:
      return "SOCKET";
    }
  }

  return "UNKNOWN";
}
