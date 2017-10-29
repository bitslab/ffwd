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

#include <libmemcached/common.h>
#include <libmemcached/memcached/protocol_binary.h>

memcached_return_t memcached_delete(memcached_st *ptr, const char *key, size_t key_length,
                                    time_t expiration)
{
  return memcached_delete_by_key(ptr, key, key_length, key, key_length, expiration);
}

static inline memcached_return_t ascii_delete(memcached_st *ptr,
                                              memcached_server_write_instance_st instance,
                                              uint32_t ,
                                              const char *key,
                                              size_t key_length,
                                              bool& reply,
                                              bool& flush)
{
  char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
  int send_length= snprintf(buffer, MEMCACHED_DEFAULT_COMMAND_SIZE,
                            "delete %.*s%.*s%s\r\n",
                            memcached_print_array(ptr->_namespace),
                            (int)key_length, key, 
                            reply ? "" :  " noreply");

  if (send_length >= MEMCACHED_DEFAULT_COMMAND_SIZE || send_length < 0)
  {
    return memcached_set_error(*ptr, MEMCACHED_MEMORY_ALLOCATION_FAILURE, MEMCACHED_AT, 
                               memcached_literal_param("snprintf(MEMCACHED_DEFAULT_COMMAND_SIZE)"));
  }

  if (ptr->flags.use_udp and flush == false)
  {
    if (send_length > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return MEMCACHED_WRITE_FAILURE;
    }

    if (send_length +instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(instance, NULL, 0, true);
    }
  }

  return memcached_do(instance, buffer, (size_t)send_length, flush);
}

static inline memcached_return_t binary_delete(memcached_st *ptr,
                                               memcached_server_write_instance_st instance,
                                               uint32_t server_key,
                                               const char *key,
                                               size_t key_length,
                                               bool& reply,
                                               bool& flush)
{
  protocol_binary_request_delete request= {};

  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  if (reply)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETE;
  }
  else
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETEQ;
  }
  request.message.header.request.keylen= htons((uint16_t)(key_length + memcached_array_size(ptr->_namespace)));
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;
  request.message.header.request.bodylen= htonl((uint32_t)(key_length + memcached_array_size(ptr->_namespace)));

  if (ptr->flags.use_udp and flush == false)
  {
    size_t cmd_size= sizeof(request.bytes) + key_length;
    if (cmd_size > MAX_UDP_DATAGRAM_LENGTH - UDP_DATAGRAM_HEADER_LENGTH)
    {
      return MEMCACHED_WRITE_FAILURE;
    }

    if (cmd_size +instance->write_buffer_offset > MAX_UDP_DATAGRAM_LENGTH)
    {
      memcached_io_write(instance, NULL, 0, true);
    }
  }

  struct libmemcached_io_vector_st vector[]=
  {
    { request.bytes, sizeof(request.bytes) },
    { memcached_array_string(ptr->_namespace), memcached_array_size(ptr->_namespace) },
    { key, key_length }
  };

  memcached_return_t rc= MEMCACHED_SUCCESS;

  if ((rc= memcached_vdo(instance, vector,  3, flush)) != MEMCACHED_SUCCESS)
  {
    memcached_io_reset(instance);
  }

  if (ptr->number_of_replicas > 0)
  {
    request.message.header.request.opcode= PROTOCOL_BINARY_CMD_DELETEQ;

    for (uint32_t x= 0; x < ptr->number_of_replicas; ++x)
    {
      memcached_server_write_instance_st replica;

      ++server_key;
      if (server_key == memcached_server_count(ptr))
        server_key= 0;

      replica= memcached_server_instance_fetch(ptr, server_key);

      if (memcached_vdo(replica, vector, 3, flush) != MEMCACHED_SUCCESS)
      {
        memcached_io_reset(replica);
      }
      else
      {
        memcached_server_response_decrement(replica);
      }
    }
  }

  return rc;
}

memcached_return_t memcached_delete_by_key(memcached_st *ptr,
                                           const char *group_key, size_t group_key_length,
                                           const char *key, size_t key_length,
                                           time_t expiration)
{
  LIBMEMCACHED_MEMCACHED_DELETE_START();

  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr)))
  {
    return rc;
  }

  rc= memcached_validate_key_length(key_length, ptr->flags.binary_protocol);
  if (memcached_failed(rc))
  {
    return rc;
  }

  if (expiration)
  {
    return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT, 
                               memcached_literal_param("Memcached server version does not allow expiration of deleted items"));
  }
  
  // If a delete trigger exists, we need a response, so no buffering/noreply
  if (ptr->delete_trigger)
  {
    if (ptr->flags.buffer_requests)
    {
      return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT, 
                                 memcached_literal_param("Delete triggers cannot be used if buffering is enabled"));
    }

    if (ptr->flags.no_reply)
    {
      return memcached_set_error(*ptr, MEMCACHED_INVALID_ARGUMENTS, MEMCACHED_AT, 
                                 memcached_literal_param("Delete triggers cannot be used if MEMCACHED_BEHAVIOR_NOREPLY is set"));
    }
  }


  uint32_t server_key= memcached_generate_hash_with_redistribution(ptr, group_key, group_key_length);
  memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, server_key);

  bool to_write= (ptr->flags.buffer_requests) ? false : true;

  // Invert the logic to make it simpler to read the code
  bool reply= (ptr->flags.no_reply) ? false : true;

  if (ptr->flags.binary_protocol)
  {
    rc= binary_delete(ptr, instance, server_key, key, key_length, reply, to_write);
  }
  else
  {
    rc= ascii_delete(ptr, instance, server_key, key, key_length, reply, to_write);
  }

  if (rc == MEMCACHED_SUCCESS)
  {
    if (to_write == false)
    {
      rc= MEMCACHED_BUFFERED;
    }
    else if (reply)
    {
      char buffer[MEMCACHED_DEFAULT_COMMAND_SIZE];
      rc= memcached_response(instance, buffer, MEMCACHED_DEFAULT_COMMAND_SIZE, NULL);
      if (rc == MEMCACHED_DELETED)
      {
        rc= MEMCACHED_SUCCESS;
      }
    }

    if (rc == MEMCACHED_SUCCESS and ptr->delete_trigger)
    {
      ptr->delete_trigger(ptr, key, key_length);
    }
  }

  LIBMEMCACHED_MEMCACHED_DELETE_END();
  return rc;
}
